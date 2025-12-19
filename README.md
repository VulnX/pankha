# pankha

A low-level utility for manual fan speed control on the `HP OMEN 16 Gaming Laptop`.

> ⚠️ **DISCLAIMER**: Please follow the support device list strictly. The `EC registers` are almost certainly different on other laptop models. **Use at your own risk.**

## Preview

![Preview](./client/screenshot/preview.png)

## Supported Devices

| Device                   | DMI Board ID | Tested by           |
| ------------------------ | ------------ | ------------------- |
| Omen 16-wf1xxx           | `8C78`       | @VulnX              |
| Omen 16-wf0xxx           | `8BAB`       | @PXG-XPG            |
| Omen 16-xd0xxx           | `8BCD`       | @varad-pisale       |
| Omen 16-ap0097ax         | `8E35`       | @locomotiivo        |
| Omen Transcend 14-fb0xxx | `8C58`       | @DistortedDragon1o4 |

>  NOTE: If your board is not listed above and you want support to be added, feel free to open an issue.

## Installation

See [INSTALL.md](INSTALL.md)

## How It Works

- CPU temperature is detected via hwmon.
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

## TODO

- [ ] Feature: auto-adjust fan speed based on temperature
