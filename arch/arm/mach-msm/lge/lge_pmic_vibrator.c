/*
 * PMIC Vibrator device 
 *
 */
 
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <../../../../drivers/staging/android/timed_output.h>
#include <linux/sched.h>

#include <mach/msm_rpcrouter.h>
#include <mach/pmic.h>
#include <mach/board_lge.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>

#define VIB_MAX_LEVEL_mV	3100
#define VIB_MIN_LEVEL_mV	1200

struct pmic8058_vib {
	struct hrtimer vib_timer;
	struct timed_output_dev timed_dev;
	spinlock_t lock;
	struct work_struct work;

	struct device *dev;
	struct lge_pmic8058_vibrator_pdata *pdata;
	int state;
	int level_mV;
};

static int config_pmic_vibrator(void)
{
	/* Disable the vibrator motor. */
	pmic_vib_mot_set_volt(0);

#if 1 /* Configures vibrator motor to manual mode control */
	if (pmic_vib_mot_set_mode(PM_VIB_MOT_MODE__MANUAL) < 0) {
		printk(KERN_ERR "Failed to set Vibrator manual mode\n");
		return -EFAULT;
	}
#else /* configures vibrator motor to automatic and use DBUS[1 or 2 or 3] as the control line */
	if (pmic_vib_mot_set_mode(PM_VIB_MOT_MODE__DBUS2) < 0) {
		printk(KERN_ERR "Failed to set Vibrator mode\n");
		return -EFAULT;
	}

	/* configures DBUS polarity */
	if (pmic_vib_mot_set_polarity(PM_VIB_MOT_POL__ACTIVE_HIGH) < 0) {
		printk(KERN_ERR "Failed to set Vibrator polarity \n");
		return -EFAULT;
	}
#endif
	return 0;
}

static int set_pmic_vibrator(int on, int level)
{
	if (on) {
		if (pmic_vib_mot_set_volt(level) < 0 ) {
			printk(KERN_ERR "Failed to set Vibrator Motor Voltage level %d mv\n", level);
			return -EFAULT;
		}
	}
	else {
		pmic_vib_mot_set_volt(0);
	}

	return 0;
}

static int pmic8058_vib_set(struct pmic8058_vib *vib, int on)
{
	int rc;

	if (on) {
		rc = pm_runtime_resume(vib->dev);
		if (rc < 0)
			dev_dbg(vib->dev, "pm_runtime_resume failed\n");
		
		set_pmic_vibrator(1,vib->level_mV);
	} else {
		set_pmic_vibrator(0,vib->level_mV);

		rc = pm_runtime_suspend(vib->dev);
		if (rc < 0)
			dev_dbg(vib->dev, "pm_runtime_suspend failed\n");
	}

	return rc;
}

static void pmic8058_vib_enable(struct timed_output_dev *dev, int value)
{
	struct pmic8058_vib *vib = container_of(dev, struct pmic8058_vib,
					 timed_dev);
	unsigned long flags;

	spin_lock_irqsave(&vib->lock, flags);
	hrtimer_cancel(&vib->vib_timer);

	if (value == 0)
		vib->state = 0;
	else {
		value = (value > vib->pdata->max_timeout_ms ?
				 vib->pdata->max_timeout_ms : value);
		vib->state = 1;
		hrtimer_start(&vib->vib_timer,
			      ktime_set(value / 1000, (value % 1000) * 1000000),
			      HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&vib->lock, flags);
	schedule_work(&vib->work);
}

static void pmic8058_vib_update(struct work_struct *work)
{
	struct pmic8058_vib *vib = container_of(work, struct pmic8058_vib,
					 work);

	pmic8058_vib_set(vib, vib->state);
}

static int pmic8058_vib_get_time(struct timed_output_dev *dev)
{
	struct pmic8058_vib *vib = container_of(dev, struct pmic8058_vib,
					 timed_dev);

	if (hrtimer_active(&vib->vib_timer)) {
		ktime_t r = hrtimer_get_remaining(&vib->vib_timer);
		return r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} else
		return 0;
}

static enum hrtimer_restart pmic8058_vib_timer_func(struct hrtimer *timer)
{
	struct pmic8058_vib *vib = container_of(timer, struct pmic8058_vib,
					 vib_timer);
	vib->state = 0;
	schedule_work(&vib->work);
	return HRTIMER_NORESTART;
}

#ifdef CONFIG_PM
static int pmic8058_vib_suspend(struct device *dev)
{
	struct pmic8058_vib *vib = dev_get_drvdata(dev);

	hrtimer_cancel(&vib->vib_timer);
	cancel_work_sync(&vib->work);
	/* turn-off vibrator */
	pmic8058_vib_set(vib, 0);
	return 0;
}

static struct dev_pm_ops pmic8058_vib_pm_ops = {
	.suspend = pmic8058_vib_suspend,
};
#endif

static ssize_t vibrator_amplitude_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *timed_dev = dev_get_drvdata(dev);
	struct pmic8058_vib *vib = container_of(timed_dev, struct pmic8058_vib,timed_dev);

	int mv = vib->level_mV;
	
	return sprintf(buf, "%d\n", mv);
}

static ssize_t vibrator_amplitude_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct timed_output_dev *timed_dev = dev_get_drvdata(dev);
	struct pmic8058_vib *vib = container_of(timed_dev, struct pmic8058_vib,timed_dev);

	int mv;

	sscanf(buf, "%d", &mv);

	if (mv > VIB_MAX_LEVEL_mV || mv < VIB_MIN_LEVEL_mV) {
		printk(KERN_ERR "%s invalid value: should be %d ~ %d\n", __FUNCTION__,VIB_MIN_LEVEL_mV,VIB_MAX_LEVEL_mV);
		return -EINVAL;
	}
	mv = (mv/100)*100;
	
	vib->level_mV =  mv;

	return size;
}

static DEVICE_ATTR(amplitude, S_IRUGO | S_IWUSR, vibrator_amplitude_show, vibrator_amplitude_store);

static int __devinit pmic8058_vib_probe(struct platform_device *pdev)

{
	struct lge_pmic8058_vibrator_pdata *pdata = pdev->dev.platform_data;
	struct pmic8058_vib *vib;
	int rc;

	if (!pdata)
		return -EINVAL;

	if (pdata->level_mV < VIB_MIN_LEVEL_mV ||
			 pdata->level_mV > VIB_MAX_LEVEL_mV)
		return -EINVAL;

	pdata->level_mV = (pdata->level_mV/100)*100;

	vib = kzalloc(sizeof(*vib), GFP_KERNEL);
	if (!vib)
		return -ENOMEM;

	/* Enable runtime PM ops, start in ACTIVE mode */
	rc = pm_runtime_set_active(&pdev->dev);
	if (rc < 0)
		dev_dbg(&pdev->dev, "unable to set runtime pm state\n");
	
	pm_runtime_enable(&pdev->dev);

	vib->pdata	= pdata;
	vib->level_mV	= pdata->level_mV;
	vib->dev	= &pdev->dev;

	spin_lock_init(&vib->lock);
	INIT_WORK(&vib->work, pmic8058_vib_update);

	hrtimer_init(&vib->vib_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vib->vib_timer.function = pmic8058_vib_timer_func;

	vib->timed_dev.name = "vibrator";
	vib->timed_dev.get_time = pmic8058_vib_get_time;
	vib->timed_dev.enable = pmic8058_vib_enable;

	rc = timed_output_dev_register(&vib->timed_dev);
	if (rc < 0)
		goto err_read_vib;

	rc = device_create_file(vib->timed_dev.dev, &dev_attr_amplitude);
	if (rc < 0) {
		timed_output_dev_unregister(&vib->timed_dev);
		device_remove_file(vib->timed_dev.dev, &dev_attr_amplitude);
		goto err_read_vib;
	}

	config_pmic_vibrator();

	platform_set_drvdata(pdev, vib);

	pm_runtime_set_suspended(&pdev->dev);
	return 0;

err_read_vib:
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	kfree(vib);
	
	return rc;
}

static int __devexit pmic8058_vib_remove(struct platform_device *pdev)
{
	struct pmic8058_vib *vib = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	cancel_work_sync(&vib->work);
	hrtimer_cancel(&vib->vib_timer);
	timed_output_dev_unregister(&vib->timed_dev);
	kfree(vib);

	return 0;
}

static struct platform_driver pmic8058_vib_driver = {
	.probe		= pmic8058_vib_probe,
	.remove		= __devexit_p(pmic8058_vib_remove),
	.driver		= {
		.name	= "msm7x30_pm8058-vib",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &pmic8058_vib_pm_ops,
#endif
	},
};


static int __init pmic8058_vib_init(void)
{
	return platform_driver_register(&pmic8058_vib_driver);
}
module_init(pmic8058_vib_init);

static void __exit pmic8058_vib_exit(void)
{
	platform_driver_unregister(&pmic8058_vib_driver);
}
module_exit(pmic8058_vib_exit);


MODULE_ALIAS("platform:msm7x30 pmic8058_vib");
MODULE_DESCRIPTION("PMIC8058 vibrator driver");
MODULE_LICENSE("GPL v2");
