# pankha

A low-level utility for manual fan speed control on the `HP OMEN by HP Gaming Laptop 16-wf1xxx`.

> ⚠️ **DISCLAIMER**: This tool has only been tested on **my** device. The `EC registers` are almost certainly different on other laptop models. **Use at your own risk.**

## Preview

![Preview](./client/screenshot/preview.png)

## Installation

See [INSTALL.md](INSTALL.md)

## How It Works

- CPU temperature is detected using the [`lm-sensors`](https://crates.io/crates/lm-sensors) crate.
- Fan speed is read/controlled via IOCTLs through a custom device driver, providing a safe abstraction over EC registers.
- It auto-updates every second.

### EC Register Mapping

1. Register `0x11`
   
   - Default: `0x00`
   - Function: Represents the real fan speed.

2. Register `0x15`
   
   - Default: `0x00`
   - Function: Controller. If it has the value `0` then it means that fan is controlled by the BIOS, any non zero value like `1` would indicate manual control, using the below register.

3. Register `0x19`
   
   - Default: `0xFF`
   - Function: Represents actual fan speed.
     - Values are in multiples of 100 RPM.
     - Example: `0x1E` = `3000 RPM`.
     - **Safety limit:** The app restricts speed to a max of **5500 RPM** to avoid *(potential)* fan damage, although not needed.

## 📎 Notes

- Tested only on `HP OMEN 16-wf1xxx` series.

## 🛠️ TODO

- [ ] Feature: auto-adjust fan speed based on temperature
