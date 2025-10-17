## Installing the driver

In order for the driver to be loaded, it must be compiled against your kernel. Ensure that you have the prerequisites

- Debian/Ubuntu:

```
sudo apt install linux-headers-$(uname -r) build-essential
```

- Arch Linux:

```
sudo pacman -S linux-headers build-essential
```

Next, clone the repo and head over to the `driver/` directory

```
$ git clone https://github.com/VulnX/pankha.git
$ cd pankha/driver
```

And then compile against your kernel headers

```
$ make -C /usr/lib/modules/$(uname -r)/build M=$(pwd) modules
```

Next, is to load the module

```
$ sudo cp pankha.ko /lib/modules/$(uname -r)/kernel/drivers
$ sudo depmod
$ sudo modprobe pankha
```

*(Optional)* To verify if the driver is loaded successfully

```
$ lsmod | grep pankha
```

(Recommended) If you want the driver to load automatically on boot, you can add it to the kernel's module list

```
echo "pankha" | sudo tee -a /etc/modules-load.d/pankha.conf
```

---

### Uninstall

In case anything breaks, or you wish to remove the driver:

```
$ sudo rmmod pankha
$ sudo rm /lib/modules/$(uname -r)/extra/pankha.ko
$ sudo depmod
$ sudo rm /etc/modules-load.d/pankha.conf
```

## 2. Installing the GUI

Simply download the latest package from the [release page](https://github.com/VulnX/pankha/releases).

### Option 1: Using the `AppImage`

Mark it as executable

```
$ chmod +x pankha_*_amd64.AppImage
```

And run it

```
$ ./pankha_*_amd64.AppImage
```

### Option 2: Using the `.deb` package

Install the package with dpkg

```
sudo dpkg -i pankha_*_amd64.deb
```

Remove *(if)* any missing dependencies

```
sudo apt-get install -f
```

Once installed, you can run the GUI from your applications menu or by executing `pankha` in the terminal



> Recommended: You can map the OMEN key on your keyboard to start pankha by creating a custom shortcut.
