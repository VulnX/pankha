#include "asm-generic/errno-base.h"
#include "linux/fs.h"
#include "linux/gfp_types.h"
#include "linux/miscdevice.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/stat.h"
#include "linux/types.h"
#include "linux/uaccess.h"
#include <linux/module.h>

#define MODULE_NAME "pankha"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("VulnX");
MODULE_DESCRIPTION("A device driver used to control fan speed on - HP OMEN by "
                   "HP Gaming Laptop 16-wf1xxx");

struct miscdevice *misc;
struct file_operations *fops;

static ssize_t pankha_read(struct file *fp, char __user *buf, size_t size,
                           loff_t __user *off) {
  int ret;
  char *msg;
  size_t len;
  msg = "NOT IMPLEMENTED YET\n";
  len = strlen(msg);
  if (*off >= len) {
    return 0;
  }
  if (*off + size > len) {
    size = len - *off;
  }
  ret = copy_to_user(buf, msg, len);
  if (ret == 0) {
    *off += size;
  }
  return len - ret;
}

static ssize_t pankha_write(struct file *fp, const char __user *buf,
                            size_t size, loff_t __user *off) {
  int ret;
  char *buffer;
  buffer = kzalloc(size, GFP_KERNEL);
  if (buffer == NULL) {
    pr_err("[pankha] failed to allocate memory to read userspace\n");
    return -ENOMEM;
  }
  ret = copy_from_user(buffer, buf, size);
  pr_info("[pankha] received \"%s\" from userspace\n", buffer);
  kfree(buffer);
  buffer = NULL;
  return size - ret;
}

static int __init pankha_init(void) {
  int ret;
  misc = kzalloc(sizeof(struct miscdevice), GFP_KERNEL);
  if (misc == NULL) {
    pr_err("[pankha] failed to allocate memory for miscdevice");
    return -ENOMEM;
  }
  fops = kzalloc(sizeof(struct file_operations), GFP_KERNEL);
  if (fops == NULL) {
    pr_err("[pankha] failed to allocate memory for fops");
    return -ENOMEM;
  }
  misc->name = MODULE_NAME;
  misc->mode = S_IRUGO | S_IWUGO;
  misc->fops = fops;
  fops->read = pankha_read;
  fops->write = pankha_write;
  ret = misc_register(misc);
  if (ret < 0) {
    pr_err("[pankha] failed to register miscdevice");
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