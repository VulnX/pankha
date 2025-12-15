use std::{
    cmp::{max, min},
    ffi::c_int,
    fs::{self, File},
    os::fd::AsRawFd,
    sync::Mutex,
    thread::{self, sleep},
    time::Duration,
};

use tauri::{Emitter, Runtime};

static PANKHA_FD: Mutex<Option<File>> = Mutex::new(None);

#[allow(dead_code)]
#[tauri::command]
pub fn ensure_pankha() -> Result<(), String> {
    let result = File::open("/dev/pankha");
    match result {
        Ok(file) => {
            let mut handle = PANKHA_FD.lock().unwrap();
            *handle = Some(file);
            Ok(())
        }
        Err(e) => Err(e.to_string()),
    }
}

#[allow(dead_code)]
#[tauri::command]
pub fn get_cpu_temp() -> Option<f32> {
    let hwmons = fs::read_dir("/sys/class/hwmon").ok()?;
    for hwmon in hwmons.flatten() {
        let name = fs::read_to_string(hwmon.path().join("name")).ok()?;
        if name.contains("coretemp") || name.contains("k10temp") {
            let temp = fs::read_to_string(hwmon.path().join("temp1_input")).ok()?;
            let temp: f32 = temp.trim().parse().ok()?;
            return Some(temp / 1000.0);
        }
    }
    None
}

const DATA_FETCHER_FREQ: Duration = Duration::from_secs(1);

#[allow(dead_code)]
#[tauri::command]
pub fn start_periodic_data_fetcher<R: Runtime>(window: tauri::Window<R>) {
    let cpu_temp_window = window.clone();
    let fan_rpm_window = window.clone();
    thread::spawn(move || loop {
        sleep(DATA_FETCHER_FREQ);
        let cpu_temp = get_cpu_temp();
        cpu_temp_window.emit("cpu_temp", cpu_temp).unwrap();
    });
    thread::spawn(move || loop {
        sleep(DATA_FETCHER_FREQ);
        let fan_rpm = get_fan_rpm().unwrap();
        fan_rpm_window.emit("fan_rpm", fan_rpm).unwrap();
    });
}

const RPM_MAX: u64 = 5500;
const SPEED_CHANGE_DELAY: Duration = Duration::from_secs(1);
const FAN_SPEED_STEP: u64 = 500;
const IOCTL_GET_FAN_SPEED: u64 = 0x80045001;
const IOCTL_GET_CONTROLLER: u64 = 0x80045002;
const IOCTL_SET_CONTROLLER: u64 = 0x40045003;
const IOCTL_SET_FAN_SPEED: u64 = 0x40045004;

#[allow(dead_code)]
#[tauri::command]
pub fn get_controller() -> Result<i32, String> {
    let fd = get_fd()?;
    let mut controller: i32 = 0;
    let res = unsafe { libc::ioctl(fd, IOCTL_GET_CONTROLLER, &mut controller) };
    if res < 0 {
        return Err("Failed to get controller".into());
    }
    Ok(controller)
}

#[allow(dead_code)]
#[tauri::command]
pub fn set_controller(controller: i32) -> Result<(), String> {
    let fd = get_fd()?;
    let res = unsafe { libc::ioctl(fd, IOCTL_SET_CONTROLLER, controller) };
    if res < 0 {
        return Err("Failed to set controller".into());
    }
    Ok(())
}

#[tauri::command]
pub fn get_fan_rpm() -> Result<u64, String> {
    let fd = get_fd()?;
    let mut speed: u64 = 0;
    let res = unsafe { libc::ioctl(fd, IOCTL_GET_FAN_SPEED, &mut speed) };
    if res < 0 {
        return Err("Failed to get fan rpm".into());
    }
    Ok(speed)
}

#[allow(dead_code)]
#[tauri::command]
pub fn set_fan_rpm<R: Runtime>(window: tauri::Window<R>, rpm: u64) -> Result<(), String> {
    if rpm % 500 != 0 {
        return Err("rpm should be multiple of 500".into());
    }

    if RPM_MAX < rpm {
        return Err("rpm cannot exceed RPM_MAX".into());
    }

    let mut current_rpm = get_fan_rpm()?;

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
            inner_set_fan_rpm(current_rpm).unwrap();
        }
        window.emit("release_btn_lock", ()).unwrap();
    });

    Ok(())
}

fn inner_set_fan_rpm(rpm: u64) -> Result<(), String> {
    let fd = get_fd()?;
    let res = unsafe { libc::ioctl(fd, IOCTL_SET_FAN_SPEED, rpm as c_int) };
    if res < 0 {
        return Err("Failed to set speed".into());
    }
    Ok(())
}

fn get_fd() -> Result<i32, String> {
    let guard = PANKHA_FD.lock().unwrap();
    let Some(ref file) = *guard else {
        return Err("Failed to get fd".into());
    };
    Ok(file.as_raw_fd())
}
