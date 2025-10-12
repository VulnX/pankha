use std::{fs::File, io::stdin, os::fd::AsRawFd, process::exit};

const IOCTL_GET_FAN_SPEED: u64 = 0x80045001;

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

fn menu() -> i32 {
    let mut s = String::new();
    println!("OPTIONS");
    println!("[0] Exit");
    println!("[1] Get fan speed");
    println!("Choice:");
    stdin().read_line(&mut s).expect("failed to read");
    s.trim().parse::<i32>().expect("not an integer")
}

fn main() {
    loop {
        let choice = menu();
        match choice {
            0 => break,
            1 => get_fan_speed(),
            _ => unimplemented!(),
        }
    }
}
