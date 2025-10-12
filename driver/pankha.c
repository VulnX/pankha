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
#define REG_FAN_SPEED 0x11

// IOCTL HANDLER COMMANDS
#define PANKHA_MAGIC 'P'
#define GET_FAN_SPEED _IOR(PANKHA_MAGIC, 1, int)

static DEFINE_MUTEX(pankha_mutex);

// Helper function definitions
int get_fan_speed(int __user *addr);

int get_fan_speed(int __user *addr) {
  u8 byte;
  int speed, err, ret;
  err = ec_read(REG_FAN_SPEED, &byte);
  if (err) {
    pr_err("[pankha] error reading fan speed\n");
    return err;
  }
  speed = byte * 100; // Convert u8 byte to RPM
  ret = copy_to_user(addr, &speed, sizeof(speed));
  if (ret != 0) {
    pr_err("[pankha] failed to copy fan speed to userspace\n");
    return -EFAULT;
  }
  return 0;
}

static long pankha_ioctl(struct file *fp, unsigned int cmd, unsigned long arg) {
  int res;
  res = 0;
  mutex_lock(&pankha_mutex);
  switch (cmd) {
  case GET_FAN_SPEED:
    res = get_fan_speed((int *)arg);
    if (res < 0)
      goto out;
    break;
  default:
    pr_err("[pankha] Invalid ioctl cmd: %u\n", cmd);
    res = -EINVAL;
    goto out;
  }
out:
  mutex_unlock(&pankha_mutex);
  return res;
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