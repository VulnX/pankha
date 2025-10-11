#![allow(dead_code)]
use std::{
    cmp::{max, min},
    fs::{self, OpenOptions},
    io::{Read, Seek, SeekFrom, Write},
    process::Command,
    thread::{self, sleep},
    time::Duration,
};

use tauri::{Emitter, Runtime};

extern "C" {
    fn getuid() -> u32;
}

#[tauri::command]
pub fn is_root() -> bool {
    let uid = unsafe { getuid() };
    uid == 0
}

#[tauri::command]
pub fn is_ec_sys_loaded() -> bool {
    fs::read_to_string("/proc/modules")
        .map(|contents| contents.contains("ec_sys"))
        .unwrap_or(false)
}

#[tauri::command]
pub fn load_ec_sys_with_write_support() -> Result<(), String> {
    let status = Command::new("modprobe")
        .arg("ec_sys")
        .arg("write_support=1")
        .status()
        .map_err(|e| e.to_string())?;

    if !status.success() {
        return Err("Failed to load ec_sys".into());
    }

    Ok(())
}

#[tauri::command]
pub fn get_cpu_temp() -> i32 {
    let sensors = lm_sensors::Initializer::default().initialize().unwrap();

        let coretemp_chip = sensors
            .chip_iter(None)
            .find(|chip| chip.to_string() == "coretemp-isa-0000")
            .unwrap();

        let package_feature = coretemp_chip
            .feature_iter()
            .find(|f| f.to_string() == "Package id 0")
            .unwrap();

        let temp_sub_feature = package_feature
            .sub_feature_iter()
            .find(|s| s.to_string() == "temp1_input")
            .unwrap();

        let cpu_temp = temp_sub_feature
            .value()
            .unwrap()
            .to_string()
            .split_whitespace()
            .next()
            .and_then(|s| s.parse::<i32>().ok())
            .unwrap_or(-1);

        cpu_temp
}

const CPU_TEMP_FETCHER_FREQ: Duration = Duration::from_secs(5);

#[tauri::command]
pub fn start_periodic_cpu_temp_fetcher<R: Runtime>(window: tauri::Window<R>) {
    thread::spawn(move || loop {
        let cpu_temp = get_cpu_temp();
        window.emit("cpu_temp", cpu_temp).unwrap();

        sleep(CPU_TEMP_FETCHER_FREQ);
    });
}

const ECIO_PATH: &str = "/sys/kernel/debug/ec/ec0/io";
const OFFSET_FAN_STATUS: u64 = 21;
const OFFSET_FAN_SPEED: u64 = 25;
const FAN_OFF: u8 = 0;
const FAN_ON: u8 = 0xff;
const RPM_MAX: u64 = 5500;
const SPEED_CHANGE_DELAY: Duration = Duration::from_secs(2);
const FAN_SPEED_STEP: u64 = 500;

#[tauri::command]
pub fn get_fan_rpm() -> u64 {
    let mut file = OpenOptions::new().read(true).open(ECIO_PATH).unwrap();
    let mut buffer = [0u8; 1];

    file.seek(SeekFrom::Start(OFFSET_FAN_STATUS)).unwrap();
    file.read_exact(&mut buffer).unwrap();
    if buffer[0] == FAN_OFF {
        return 0;
    }

    file.seek(SeekFrom::Start(OFFSET_FAN_SPEED)).unwrap();
    file.read_exact(&mut buffer).unwrap();
    buffer[0] as u64 * 100
}

#[tauri::command]
pub fn set_fan_rpm<R: Runtime>(window: tauri::Window<R>, rpm: u64) -> Result<(), String> {
    if rpm % 500 != 0 {
        return Err("rpm should be multiple of 500".into());
    }

    if RPM_MAX < rpm {
        return Err("rpm cannot exceed RPM_MAX".into());
    }

    let mut current_rpm = get_fan_rpm();

    // Handle in new thread to ensure early return
    // This allows for dynamic updates of event in react
    thread::spawn(move || {
        while current_rpm != rpm {
            sleep(SPEED_CHANGE_DELAY);
            if current_rpm < rpm {
                current_rpm = min(rpm, current_rpm + FAN_SPEED_STEP);
            } else {
                current_rpm = max(rpm, current_rpm.saturating_sub(FAN_SPEED_STEP));
            }
            inner_set_fan_rpm((current_rpm / 100) as u8);
            window.emit("fan_speed", current_rpm).unwrap();
        }
        window.emit("release_btn_lock", ()).unwrap();
    });

    Ok(())
}

fn inner_set_fan_rpm(rpm: u8) {
    let mut file = OpenOptions::new().write(true).open(ECIO_PATH).unwrap();
    file.seek(SeekFrom::Start(OFFSET_FAN_STATUS)).unwrap();
    file.write_all(&[FAN_ON]).unwrap();
    file.flush().unwrap();
    file.seek(SeekFrom::Start(OFFSET_FAN_SPEED)).unwrap();
    file.write_all(&[rpm]).unwrap();
    file.flush().unwrap();
}
