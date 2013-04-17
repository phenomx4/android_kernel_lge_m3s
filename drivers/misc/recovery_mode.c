#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

extern void remote_set_recovery_mode(int info);
static ssize_t recovery_mode_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
    return 1;
}

static ssize_t recovery_mode_write(struct file *fp, const char __user *buf, size_t count, loff_t *pos)
{
	int ret = -EINVAL;
	int mode;

	if (sscanf(buf, "%d", &mode) != 1) {
		return ret;
	}

	printk(KERN_INFO "[EFS_SYNC] Send LG_FW_RECOVERY_MODE rapi and stop EFS SYNC!!!!\n");
	remote_set_recovery_mode(mode);

	ret = count;
	return ret;
}

static const struct file_operations recovery_mode_fops = {
	.owner = THIS_MODULE,
	.read = recovery_mode_read,
	.write = recovery_mode_write,
};

static struct miscdevice recovery_mode_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "recovery_mode",
	.fops = &recovery_mode_fops,
};

static int __init recovery_mode_init(void)
{
	int rc;

	printk("%s: enter\n", __func__);

	rc = misc_register(&recovery_mode_device);
	if (rc)
	{
		printk(KERN_ERR "recovery mode device failed to initialize\n");
		misc_deregister(&recovery_mode_device);
	}
	return 0;
}

static void __exit recovery_mode_exit(void)
{
	misc_deregister(&recovery_mode_device);
}

module_init(recovery_mode_init);
module_exit(recovery_mode_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("matthew.choi@lge.com");
MODULE_DESCRIPTION("Recovery mode dev file");
MODULE_VERSION("1.0");
MODULE_ALIAS("");

