use std::{fs::File, io::stdin, os::fd::AsRawFd, process::exit};

const IOCTL_GET_FAN_SPEED: u64 = 0x80045001;
const IOCTL_GET_CONTROLLER: u64 = 0x80045002;
const IOCTL_SET_CONTROLLER: u64 = 0x40045003;
const IOCTL_SET_FAN_SPEED: u64 = 0x40045004;

fn get_fan_speed() {
    let file = File::open("/dev/pankha").expect("failed to open device");
    let fd = file.as_raw_fd();
    let mut speed: i32 = 0;
    let res = unsafe { libc::ioctl(fd, IOCTL_GET_FAN_SPEED, &mut speed) };
    if res < 0 {
        eprintln!("Failed to get fan speed");
        exit(res);
    }
    println!("Current fan speed -> {speed} rpm");
}

fn get_controller() {
    let file = File::open("/dev/pankha").expect("failed to open device");
    let fd = file.as_raw_fd();
    let mut controller: i32 = 0;
    let res = unsafe { libc::ioctl(fd, IOCTL_GET_CONTROLLER, &mut controller) };
    if res < 0 {
        eprintln!("Failed to get controller");
        exit(res);
    }
    if controller == 0 {
        println!("BIOS controlled");
    } else {
        println!("User controlled");
    }
}

fn set_controller() {
    let file = File::open("/dev/pankha").expect("failed to open device");
    let fd = file.as_raw_fd();
    println!("Controller:");
    let controller = get_int();
    let res = unsafe { libc::ioctl(fd, IOCTL_SET_CONTROLLER, controller) };
    if res < 0 {
        eprintln!("Failed to set controller");
        exit(res);
    }
    println!("Successfully set controller to {controller}");
}

fn set_fan_speed() {
    let file = File::open("/dev/pankha").expect("failed to open device");
    let fd = file.as_raw_fd();
    println!("Speed:");
    let speed = get_int();
    let res = unsafe { libc::ioctl(fd, IOCTL_SET_FAN_SPEED, speed) };
    if res < 0 {
        eprintln!("Failed to set speed");
        exit(res);
    }
    println!("Successfully set speed to {speed}");
}

fn get_int() -> i32 {
    let mut s = String::new();
    stdin().read_line(&mut s).expect("failed to read");
    s.trim().parse::<i32>().expect("not an integer")
}

fn menu() -> i32 {
    println!("OPTIONS");
    println!("[0] Exit");
    println!("[1] Get fan speed");
    println!("[2] Get controller");
    println!("[3] Set controller");
    println!("[4] Set fan speed");
    println!("Choice:");
    get_int()
}

fn main() {
    loop {
        let choice = menu();
        match choice {
            0 => break,
            1 => get_fan_speed(),
            2 => get_controller(),
            3 => set_controller(),
            4 => set_fan_speed(),
            _ => unimplemented!(),
        }
    }
}
