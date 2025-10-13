#include "asm-generic/errno-base.h"
#include "asm-generic/ioctl.h"
#include "linux/acpi.h"
#include "linux/fs.h"
#include "linux/gfp_types.h"
#include "linux/init.h"
#include "linux/miscdevice.h"
#include "linux/module.h"
#include "linux/mutex.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/stat.h"
#include "linux/uaccess.h"

#define MODULE_NAME "pankha"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("VulnX");
MODULE_DESCRIPTION("A device driver used to control fan speed on - HP OMEN by "
                   "HP Gaming Laptop 16-wf1xxx");

struct miscdevice *misc;
struct file_operations *fops;

// EC REGISTER MAPPINGS
#define REG_GET_FAN_SPEED 0x11
#define REG_CONTROLLER 0x15
#define REG_SET_FAN_SPEED 0x19

#define MAX_FAN_SPEED 5500
#define BIOS_CONTROLLER 0
#define USER_CONTROLLER 1

// CONVERSION MACROS
#define BYTE_TO_RPM(byte) ((byte) * 100)
#define RPM_TO_BYTE(rpm) ((rpm) / 100)

// IOCTL HANDLER COMMANDS
#define PANKHA_MAGIC 'P'
#define IOCTL_GET_FAN_SPEED _IOR(PANKHA_MAGIC, 1, int)
#define IOCTL_GET_CONTROLLER _IOR(PANKHA_MAGIC, 2, int)
#define IOCTL_SET_CONTROLLER _IOW(PANKHA_MAGIC, 3, int)
#define IOCTL_SET_FAN_SPEED _IOW(PANKHA_MAGIC, 4, int)

static DEFINE_MUTEX(pankha_mutex);

// HELPER FUNCTION DECLARATIONS
int _int_get_fan_speed(void);
int get_fan_speed(int __user *addr);
int get_controller(int __user *addr);
int set_controller(int controller);
int set_fan_speed(int speed);

// HELPER FUNCTION DEFINITIONS
int _int_get_fan_speed(void) {
  u8 byte;
  int err, speed;
  err = ec_read(REG_GET_FAN_SPEED, &byte);
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
  err = ec_read(REG_CONTROLLER, &byte);
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

int set_controller(int controller) {
  int err, res, speed;
  if (controller != BIOS_CONTROLLER && controller != USER_CONTROLLER) {
    pr_err("[pankha] invalid controller\n");
    return -EINVAL;
  }
  /**
   * IMPORTANT: If setting controller to USER then copy the current fan speed to
   * user-controlled fan speed register before changing the controller as the
   * fan speed may be invalid, leading to over/under performing fans.
   */
  if (controller == USER_CONTROLLER) {
    res = _int_get_fan_speed();
    if (res < 0)
      return res;
    speed = res;
    err = set_fan_speed(speed);
    if (err)
      return err;
  }
  err = ec_write(REG_CONTROLLER, controller);
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
  err = ec_write(REG_SET_FAN_SPEED, byte);
  if (err) {
    pr_err("[pankha] failed to set fan speed\n");
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
  int ret;
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