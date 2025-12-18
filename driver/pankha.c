#include "asm-generic/errno-base.h"
#include "asm-generic/ioctl.h"
#include "linux/acpi.h"
#include "linux/dmi.h"
#include "linux/fs.h"
#include "linux/gfp_types.h"
#include "linux/init.h"
#include "linux/miscdevice.h"
#include "linux/mod_devicetable.h"
#include "linux/module.h"
#include "linux/mutex.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/stat.h"
#include "linux/uaccess.h"

#define MODULE_NAME "pankha"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("VulnX");
MODULE_DESCRIPTION(
    "A device driver used to control fan speed on the HP OMEN Gaming Laptop");

struct miscdevice *misc;
struct file_operations *fops;

#define MAX_FAN_SPEED 5500

/* EC REGISTER MAPPINGS */
struct ec_data {
  struct {
    u8 get_fan_speed;
    u8 controller;
    u8 set_fan1_speed;
    u8 set_fan2_speed;
  } offsets;
  struct {
    u8 bios_controller;
    u8 user_controller;
  } controllers;
};

static const struct ec_data type1_ec = {
    .offsets.get_fan_speed = 0x11,
    .offsets.controller = 0x15,
    .offsets.set_fan1_speed = 0x19,
    .offsets.set_fan2_speed = 0x19,
    .controllers.bios_controller = 0x00,
    .controllers.user_controller = 0x01,
};

static const struct ec_data type2_ec = {
    .offsets.get_fan_speed = 0x2E,
    .offsets.controller = 0x62,
    .offsets.set_fan1_speed = 0x34,
    .offsets.set_fan2_speed = 0x35,
    .controllers.bios_controller = 0x00,
    .controllers.user_controller = 0x06,
};

/* Currently active board's EC mappings data */
static struct ec_data *ec;

/* Conversion macros */
#define BYTE_TO_RPM(byte) ((byte) * 100)
#define RPM_TO_BYTE(rpm) ((rpm) / 100)

/* IOCTL command handlers */
#define PANKHA_MAGIC 'P'
#define IOCTL_GET_FAN_SPEED _IOR(PANKHA_MAGIC, 1, int)
#define IOCTL_GET_CONTROLLER _IOR(PANKHA_MAGIC, 2, int)
#define IOCTL_SET_CONTROLLER _IOW(PANKHA_MAGIC, 3, int)
#define IOCTL_SET_FAN_SPEED _IOW(PANKHA_MAGIC, 4, int)

static DEFINE_MUTEX(pankha_mutex);

const struct dmi_system_id pankha_whitelist[] = {
    {
        .ident = "HP Omen 16-wf1xxx",
        .matches =
            {
                DMI_MATCH(DMI_BOARD_NAME, "8C78"),
            },
        .driver_data = (void *)&type1_ec,
    },
    {
        .ident = "HP Omen 16-wf0xxx",
        .matches =
            {
                DMI_MATCH(DMI_BOARD_NAME, "8BAB"),
            },
        .driver_data = (void *)&type1_ec,
    },
    {
        .ident = "HP Omen 16-xd0015AX",
        .matches =
            {
                DMI_MATCH(DMI_BOARD_NAME, "8BCD"),
            },
        .driver_data = (void *)&type1_ec,
    },
    {
        .ident = "HP Omen 16-ap0097ax",
        .matches =
            {
                DMI_MATCH(DMI_BOARD_NAME, "8E35"),
            },
        .driver_data = (void *)&type2_ec,
    },
    {
        .ident = "HP Omen 15-en1xxx",
        .matches =
            {
                DMI_MATCH(DMI_BOARD_NAME, "88D2"),
            },
        .driver_data = (void *)&type2_ec,
    },
    {}};

/* Helper function declarations */
int _int_get_fan_speed(void);
int get_fan_speed(int __user *addr);
int get_controller(int __user *addr);
int set_controller(int controller);
int set_fan_speed(int speed);

/* Helper funtion definitions */
int _int_get_fan_speed(void) {
  u8 byte;
  int err, speed;

  err = ec_read(ec->offsets.get_fan_speed, &byte);
  if (err) {
    pr_err("[pankha] error reading fan speed\n");
    return -EIO;
  }
  speed = BYTE_TO_RPM(byte);
  return speed;
}

int get_fan_speed(int __user *addr) {
  int speed, ret;

  ret = _int_get_fan_speed();
  if (ret < 0)
    return ret;
  speed = ret;
  ret = copy_to_user(addr, &speed, sizeof(speed));
  if (ret != 0) {
    pr_err("[pankha] failed to copy fan speed to userspace\n");
    return -EFAULT;
  }
  return 0;
}

int get_controller(int __user *addr) {
  u8 byte;
  int err, ret;

  err = ec_read(ec->offsets.controller, &byte);
  if (err) {
    pr_err("[pankha] error reading controller\n");
    return err;
  }
  ret = copy_to_user(addr, &byte, sizeof(byte));
  if (ret != 0) {
    pr_err("[pankha] failed to copy controller to userspace\n");
    return -EFAULT;
  }
  return 0;
}

int set_controller(int manual) {
  int controller, err, res, speed;

  if (manual)
    controller = ec->controllers.user_controller;
  else
    controller = ec->controllers.bios_controller;

  /**
   * IMPORTANT: If setting controller to USER then copy the current fan speed to
   * user-controlled fan speed register before changing the controller as the
   * fan speed may be invalid, leading to over/under performing fans.
   */
  if (controller == ec->controllers.user_controller) {
    res = _int_get_fan_speed();
    if (res < 0)
      return res;
    speed = res;
    err = set_fan_speed(speed);
    if (err)
      return err;
  }
  err = ec_write(ec->offsets.controller, controller);
  if (err) {
    pr_err("[pankha] failed to change controller\n");
    return err;
  }
  return 0;
}

int set_fan_speed(int speed) {
  u8 byte;
  int err;

  if (speed < 0 || MAX_FAN_SPEED < speed) {
    pr_err("[pankha] invalid fan speed range\n");
    return -EINVAL;
  }
  byte = RPM_TO_BYTE(speed);
  err = ec_write(ec->offsets.set_fan1_speed, byte);
  if (err) {
    pr_err("[pankha] failed to set fan1 speed\n");
    return err;
  }
  err = ec_write(ec->offsets.set_fan2_speed, byte);
  if (err) {
    pr_err("[pankha] failed to set fan2 speed\n");
    return err;
  }
  return 0;
}

static long pankha_ioctl(struct file *fp, unsigned int cmd, unsigned long arg) {
  int err;
  err = 0;

  mutex_lock(&pankha_mutex);
  switch (cmd) {
  case IOCTL_GET_FAN_SPEED:
    err = get_fan_speed((int *)arg);
    if (err)
      goto out;
    break;
  case IOCTL_GET_CONTROLLER:
    err = get_controller((int *)arg);
    if (err)
      goto out;
    break;
  case IOCTL_SET_CONTROLLER:
    err = set_controller(arg);
    if (err)
      goto out;
    break;
  case IOCTL_SET_FAN_SPEED:
    err = set_fan_speed(arg);
    if (err)
      goto out;
    break;
  default:
    pr_err("[pankha] Invalid ioctl cmd: %u\n", cmd);
    err = -EINVAL;
    goto out;
  }
out:
  mutex_unlock(&pankha_mutex);
  return err;
}

static int __init pankha_init(void) {
  const struct dmi_system_id *board;
  int ret;

  board = dmi_first_match(pankha_whitelist);
  if (!board) {
    pr_err("[pankha] unsupported device: %s\n",
           dmi_get_system_info(DMI_BOARD_NAME));
    return -ENODEV;
  }
  ec = board->driver_data;
  misc = kzalloc(sizeof(struct miscdevice), GFP_KERNEL);
  if (misc == NULL) {
    pr_err("[pankha] failed to allocate memory for miscdevice\n");
    return -ENOMEM;
  }
  fops = kzalloc(sizeof(struct file_operations), GFP_KERNEL);
  if (fops == NULL) {
    pr_err("[pankha] failed to allocate memory for fops\n");
    return -ENOMEM;
  }
  misc->name = MODULE_NAME;
  misc->mode = S_IRUGO | S_IWUGO;
  misc->fops = fops;
  fops->owner = THIS_MODULE;
  fops->unlocked_ioctl = pankha_ioctl;
  ret = misc_register(misc);
  if (ret < 0) {
    pr_err("[pankha] failed to register miscdevice\n");
    kfree(misc);
    kfree(fops);
    return ret;
  }
  pr_info("[pankha] driver added\n");
  return 0;
}

static void __exit pankha_exit(void) {
  misc_deregister(misc);
  kfree(misc);
  kfree(fops);
  pr_info("[pankha] driver removed\n");
}

module_init(pankha_init);
module_exit(pankha_exit);
