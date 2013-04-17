#include <linux/init.h>
#include <linux/platform_device.h>
#include <mach/rpc_server_handset.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio_event.h>
#include <mach/pn544.h> // 2011.08.10 garam.kim@lge.com NFC
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <mach/pmic.h>
#include <linux/delay.h>
#include <mach/board_lge.h>
#include <linux/regulator/consumer.h>

#include <mach/board_lge.h>

// matthew.choi@lge.com 111017 Add side volume keys [START]
static unsigned int m3s_col_gpios[] = {180};		// {KEY_DRV}
static unsigned int m3s_row_gpios[] = {20, 147};	// {KEY_UP, KEY_DOWN}

#define KEYMAP_INDEX(col, row) ((col)*ARRAY_SIZE(m3s_col_gpios) + (row))

static const unsigned short m3s_keymap[] = {
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEUP,
	[KEYMAP_INDEX(0, 1)] = KEY_VOLUMEDOWN,
};

int m3s_matrix_info_wrapper(struct gpio_event_input_devs *input_dev,
						   struct gpio_event_info *info, void **data, int func)
{
	int ret;
	if (func == GPIO_EVENT_FUNC_RESUME) {
		gpio_tlmm_config(
			GPIO_CFG(m3s_row_gpios[0], 0,
					 GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(
			GPIO_CFG(m3s_row_gpios[1], 0,
					 GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}

	ret = gpio_event_matrix_func(input_dev, info, data, func);
	return ret;
}

static int m3s_gpio_keypad_power(
		const struct gpio_event_platform_data *pdata, bool on)
{
	/* this is dummy function to make gpio_event driver register suspend function
	 * 2010-01-29, cleaneye.kim@lge.com
	 */

	return 0;
}

static struct gpio_event_matrix_info m3s_keypad_matrix_info = {
	.info.func	= m3s_matrix_info_wrapper,
	.keymap		= m3s_keymap,
	.output_gpios	= m3s_col_gpios,
	.input_gpios	= m3s_row_gpios,
	.noutputs	= ARRAY_SIZE(m3s_col_gpios),
	.ninputs	= ARRAY_SIZE(m3s_row_gpios),
	.settle_time.tv.nsec = 40 * NSEC_PER_USEC,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_DRIVE_INACTIVE | GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_PRINT_UNMAPPED_KEYS//GPIOKPF_ACTIVE_HIGH
};

static struct gpio_event_info *m3s_keypad_info[] = {
	&m3s_keypad_matrix_info.info,
};

static struct gpio_event_platform_data m3s_keypad_data = {
	.name = "gpio-side-keypad",
	.info = m3s_keypad_info,
	.info_count = ARRAY_SIZE(m3s_keypad_info),
	.power = m3s_gpio_keypad_power,
};

static struct platform_device m3s_gpio_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 1,
	.dev        = {
		.platform_data  = &m3s_keypad_data,
	},
};
// matthew.choi@lge.com 111017 Add side volume keys [END]

// 2011.08.10 garam.kim@lge.com added GPIO values for NFC
/* NFC macros */
#if defined (CONFIG_PN544_NFC)
#define NFC_GPIO_I2C_SDA		109
#define NFC_GPIO_I2C_SCL		108
#define NFC_GPIO_IRQ			123
#define NFC_GPIO_VEN			153
#define NFC_GPIO_FIRM			33
#define NFC_I2C_SLAVE_ADDR		0x28
#endif
// 2011.08.10 garam.kim@lge.com End
#if defined(CONFIG_TOUCHSCREEN_MELFAS_TS)
/* this routine should be checked for nessarry */
static struct gpio_i2c_pin ts_i2c_pin[] = {
	[0] = {
		.sda_pin	= TS_GPIO_I2C_SDA,
		.scl_pin	= TS_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= TS_GPIO_IRQ,
	},
};

static struct i2c_gpio_platform_data ts_i2c_pdata = {
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 1,
};

static struct platform_device ts_i2c_device = {
	.name	= "i2c-gpio",
	.dev.platform_data = &ts_i2c_pdata,
};

static int ts_config_gpio(int config)
{
	if (config)
	{		// for wake state 
//		gpio_tlmm_config(GPIO_CFG(TS_GPIO_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
	else
	{		// for sleep state 
//		gpio_tlmm_config(GPIO_CFG(TS_GPIO_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

//		gpio_tlmm_config(GPIO_CFG(TS_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
//		gpio_tlmm_config(GPIO_CFG(TS_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}

	return 0;
}

int ts_set_vreg(unsigned char onoff)
{
	struct vreg *ts_vreg_3v0touch, *ts_vreg_1v8touch;
	int ts_rc;
	static int init = 0;

	printk(KERN_INFO "[Melfas power] %s() onoff:%d\n", __func__, onoff);
	ts_vreg_3v0touch = vreg_get(NULL, "touch3v0");
		if (ts_vreg_3v0touch == NULL)
			pr_err("%s: ts_vreg_3v0touch failed\n", __func__);
	ts_vreg_1v8touch = vreg_get(NULL, "touch1v8");
		if (ts_vreg_1v8touch == NULL)
			pr_err("%s: ts_vreg_1v8touch failed\n", __func__);

	if(onoff)
	{
		if(init ==0){
		ts_config_gpio(1);
		if(ts_vreg_3v0touch && ts_vreg_1v8touch)
		{
			printk(KERN_INFO "[Melfas power] on \n");
			ts_rc = vreg_set_level(ts_vreg_3v0touch, 3000);
			if (ts_rc) {
				printk("<choi> LG_FW : %s, ts_vreg_3v0touch on error %d\n",__func__, ts_rc);
			}

			ts_rc = vreg_set_level(ts_vreg_1v8touch, 1800);
			if (ts_rc) {
				printk("<choi> LG_FW : %s, ts_vreg_1v8touch on error %d\n",__func__, ts_rc);
			}

			ts_rc = vreg_enable(ts_vreg_3v0touch);
			if (ts_rc) {
				printk("<choi> LG_FW : %s, ts_vreg_3v0touch vreg_enable error %d\n",__func__, ts_rc);
			}

			ts_rc = vreg_enable(ts_vreg_1v8touch);
			if (ts_rc) {
				printk("<choi> LG_FW : %s, ts_vreg_1v8touch vreg_enable error %d\n",__func__, ts_rc);
			}
		}
		else
			printk("<choi> LG_FW : %s, <if> can't get vreg_get\n",__func__);
		init = 1;
		}
	}
	else
	{
		if (init == 1) {
		ts_config_gpio(0);
		if(ts_vreg_3v0touch && ts_vreg_1v8touch)
		{
			printk(KERN_INFO "[Melfas power] off \n");
			ts_rc = vreg_disable(ts_vreg_3v0touch);
			if (ts_rc) {
				printk("<choi> LG_FW : %s, ts_vreg_3v0touch vreg_disable error %d\n",__func__, ts_rc);
			}

			ts_rc = vreg_disable(ts_vreg_1v8touch);
			if (ts_rc) {
				printk("<choi> LG_FW : %s, ts_vreg_1v8touch vreg_disable error %d\n",__func__, ts_rc);
			}
		}
		else
			printk("<choi> LG_FW : %s, <else> can't get vreg_get\n",__func__);
		init = 0;
		}
	}
	mdelay(10);
	return 0;
}

static struct touch_platform_data ts_pdata = {
	.ts_x_min = TS_X_MIN,
	.ts_x_max = TS_X_MAX,
	.ts_y_min = TS_Y_MIN,
	.ts_y_max = TS_Y_MAX,
	.power 	  = ts_set_vreg,
	.irq 	  = TS_GPIO_IRQ,
	.scl      = TS_GPIO_I2C_SCL,
	.sda      = TS_GPIO_I2C_SDA,
};

static struct i2c_board_info ts_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("touch_mcs8000", TS_I2C_SLAVE_ADDR),
		.type = "touch_mcs8000",
		.platform_data = &ts_pdata,
	},
};

/* this routine should be checked for nessarry */
static int init_gpio_i2c_pin_touch(struct i2c_gpio_platform_data *i2c_adap_pdata,
		struct gpio_i2c_pin gpio_i2c_pin, struct i2c_board_info *i2c_board_info_data)
{
	i2c_adap_pdata->sda_pin = gpio_i2c_pin.sda_pin;
	i2c_adap_pdata->scl_pin = gpio_i2c_pin.scl_pin;

	printk("<choi> LG_FW : %s\n",__func__);

	gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.sda_pin, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.scl_pin, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(gpio_i2c_pin.sda_pin, 1);
	gpio_set_value(gpio_i2c_pin.scl_pin, 1);

	if (gpio_i2c_pin.reset_pin) {
		gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.reset_pin, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(gpio_i2c_pin.reset_pin, 1);
	}

	if (gpio_i2c_pin.irq_pin) {
		gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.irq_pin, 0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		i2c_board_info_data->irq =
			MSM_GPIO_TO_INT(gpio_i2c_pin.irq_pin);
	}

	return 0;
}

static void __init m3eu_init_i2c_touch(int bus_num, bool platform)
{
	ts_i2c_device.id = bus_num;

	printk("<choi> LG_FW : %s, bus_num is %d\n",__func__, bus_num);
	
	printk("<choi> LG_FW : %s : ts_i2c_pin[0].irq_pin %d, ts_i2c_pin[0].reset_pin %d, ts_i2c_pin[0].scl_pin %d, ts_i2c_pin[0].sda_pin %d\n",__func__, ts_i2c_pin[0].irq_pin, ts_i2c_pin[0].reset_pin, ts_i2c_pin[0].scl_pin, ts_i2c_pin[0].sda_pin);

	init_gpio_i2c_pin_touch(&ts_i2c_pdata, ts_i2c_pin[0], &ts_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, &ts_i2c_bdinfo[0], 1);
	if(platform)
		platform_device_register(&ts_i2c_device);
}

#endif

#ifdef CONFIG_PN544_NFC
// 2011.08.10 garam.kim@lge.com NFC registration
static struct gpio_i2c_pin nfc_i2c_pin[] = {
	[0] = {
		.sda_pin	= NFC_GPIO_I2C_SDA,
		.scl_pin	= NFC_GPIO_I2C_SCL,
		.reset_pin	= NFC_GPIO_VEN,
		.irq_pin	= NFC_GPIO_IRQ,
	},
};

static struct i2c_gpio_platform_data nfc_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device nfc_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &nfc_i2c_pdata,
};

static struct pn544_i2c_platform_data nfc_pdata = {
	.ven_gpio 		= NFC_GPIO_VEN,
	.irq_gpio 	 	= NFC_GPIO_IRQ,
	.scl_gpio		= NFC_GPIO_I2C_SCL,
	.sda_gpio		= NFC_GPIO_I2C_SDA,
	.firm_gpio		= NFC_GPIO_FIRM,
};

static struct i2c_board_info nfc_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("pn544", NFC_I2C_SLAVE_ADDR),
		.type = "pn544",
		.platform_data = &nfc_pdata,
	},
};

static void __init m3s_init_i2c_nfc(int bus_num)
{
	int ret;

	gpio_tlmm_config(GPIO_CFG(NFC_GPIO_FIRM, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(NFC_GPIO_VEN, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(NFC_GPIO_IRQ, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE); 
	gpio_tlmm_config(GPIO_CFG(NFC_GPIO_I2C_SDA, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE); 
	gpio_tlmm_config(GPIO_CFG(NFC_GPIO_I2C_SCL, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE); 
				
	nfc_i2c_device.id = bus_num;

	ret = init_gpio_i2c_pin(&nfc_i2c_pdata, nfc_i2c_pin[0],	&nfc_i2c_bdinfo[0]);
	ret = i2c_register_board_info(bus_num, &nfc_i2c_bdinfo[0], 1);
	
	platform_device_register(&nfc_i2c_device);
}

#else
static void m3s_nfc_gpio_sleep_set(void) {
	gpio_tlmm_config(GPIO_CFG(NFC_GPIO_IRQ, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(NFC_GPIO_VEN, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(NFC_GPIO_FIRM, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(NFC_GPIO_I2C_SDA, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(NFC_GPIO_I2C_SCL, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
}
#endif
// 2011.08.10 garam.kim@lge.com End of NFC registration


//LGE_CHANGE_S SENSOR FIRMWARE UPDATE (jongkwon.chae@lge.com)
/** accelerometer **/

static int accel_com_Flag=0;

static void m3s_sensor_ldo_gpio_config(int onoff)
{
	if(onoff)
	{
		gpio_tlmm_config(GPIO_CFG(ACCEL_ECOM_GPIO_SENSOR_EN_N, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
	else
	{
		gpio_tlmm_config(GPIO_CFG(ACCEL_ECOM_GPIO_SENSOR_EN_N, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
}
static void m3s_accel_sensor_gpio_config(int onoff)
{
	if(onoff)
	{
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SDA, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SCL, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_INT, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
	else
	{
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SDA, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_I2C_SCL, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ACCEL_GPIO_INT, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
}

static int accel_power(unsigned char onoff)
{
	int ret = 0;

	struct vreg *gp7_vreg = vreg_get(0, "gp7"); //1V8_SENSOR
	if (!gp7_vreg ) {
			printk("[smiledice] LG_FW : %s, can't get vreg_get\n",__func__);
	}

	printk("[Accelator] %s() : Power %s\n",__FUNCTION__, onoff ? "On" : "Off");

	if (onoff) 
	{
		printk(KERN_INFO "accel_power_on\n");
		printk("accel_com_flag = %d\n", accel_com_Flag);
		if(accel_com_Flag==0)
		{
			gpio_set_value(ACCEL_ECOM_GPIO_SENSOR_EN_N, 1); //3 // 3V0_SENSOR ON (FROM LDO) 2011-09-29 jeongyong.lee@lge.com
			m3s_sensor_ldo_gpio_config(1);
			accel_com_Flag=1;
			printk("accel_com_flag = %d\n", accel_com_Flag);
		}
						
			ret = vreg_set_level(gp7_vreg, 1800);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
					vreg_set_level(gp7_vreg, 1800) return error %d\n",__func__, ret);
			}


			ret = vreg_enable(gp7_vreg);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
					vreg_enable(gp7_vreg) return error %d\n",__func__, ret);
		}
		m3s_accel_sensor_gpio_config(1);

	} 
	else 
	{
		printk(KERN_INFO "accel_power_off\n");
		printk("accel_com_flag = %d\n", accel_com_Flag);
		if(accel_com_Flag==1)
		{
			gpio_set_value(ACCEL_ECOM_GPIO_SENSOR_EN_N, 0); // 3V0_SENSOR OFF (FROM LDO) 2011-09-29 jeongyong.lee@lge.com
			m3s_sensor_ldo_gpio_config(0);
			accel_com_Flag=0;
			printk("accel_com_flag = %d\n", accel_com_Flag);
			}
			ret = vreg_disable(gp7_vreg);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
					vreg_disable(gp7_vreg) return error %d\n",__func__, ret);
			}
			m3s_accel_sensor_gpio_config(0);

	}

	return ret;
}

struct acceleration_platform_data bma222 = {
	.power = accel_power,
};

static struct gpio_i2c_pin accel_i2c_pin[] = {
	[0] = {
		.sda_pin	= ACCEL_GPIO_I2C_SDA,
		.scl_pin	= ACCEL_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ACCEL_GPIO_INT,
	},
};

static struct i2c_gpio_platform_data accel_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device accel_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &accel_i2c_pdata,
};

static struct i2c_board_info accel_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("bma222", ACCEL_I2C_ADDRESS),
		.type = "bma222",
		.platform_data = &bma222,
	},
};

static void __init m3eu_init_i2c_acceleration(int bus_num, bool platform)
{
	accel_i2c_device.id = bus_num;

	printk("[smiledice] LG_FW : %s, bus_num is %d\n",__func__, bus_num);
	
	printk("[smiledice] LG_FW : %s : \
			accel_i2c_pin[0].irq_pin %d, \
			accel_i2c_pin[0].reset_pin %d, \
			accel_i2c_pin[0].scl_pin %d, \
			accel_i2c_pin[0].sda_pin %d\n",
			__func__, 
			accel_i2c_pin[0].irq_pin, accel_i2c_pin[0].reset_pin, 
			accel_i2c_pin[0].scl_pin, accel_i2c_pin[0].sda_pin);

	init_gpio_i2c_pin(&accel_i2c_pdata, accel_i2c_pin[0], &accel_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &accel_i2c_bdinfo[0], 1);

	if(platform) {
		platform_device_register(&accel_i2c_device);
	}
}

/* ecompass */

static void m3s_ecom_sensor_gpio_config(int onoff)
{
	if(onoff)
	{
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_I2C_SDA, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_I2C_SCL, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_INT, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
	else
	{
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_I2C_SDA, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_I2C_SCL, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(ECOM_GPIO_INT, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
	
}

static int ecom_power_set(unsigned char onoff)
{

	int ret = 0;

	struct vreg *gp7_vreg = vreg_get(0, "gp7"); //1V8_SENSOR
	if (!gp7_vreg ){
			printk("[smiledice] LG_FW : %s, can't get vreg_get\n",__func__);
	}

	printk("[Ecompass] %s() : Power %s\n",__FUNCTION__, onoff ? "On" : "Off");

	if (onoff) 
	{
		
		printk(KERN_INFO "ecom_power_on\n");
		printk("accel_com_flag = %d\n", accel_com_Flag);
		if(accel_com_Flag==0)
		{
			gpio_set_value(ACCEL_ECOM_GPIO_SENSOR_EN_N, 1);	// 3V0_SENSOR ON (FROM LDO) 2011-09-29 jeongyong.lee@lge.com
			m3s_sensor_ldo_gpio_config(1);
			accel_com_Flag=1;
			printk("accel_com_flag = %d\n", accel_com_Flag);
		}
		
			ret = vreg_set_level(gp7_vreg, 1800);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
					vreg_set_level(gp7_vreg, 1800) return error %d\n",__func__, ret);
			}


			ret = vreg_enable(gp7_vreg);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
					vreg_enable(gp7_vreg) return error %d\n",__func__, ret);
			}

			m3s_ecom_sensor_gpio_config(1);
	} 
	else 
	{
		printk(KERN_INFO "ecom_power_off\n");
		printk("accel_com_flag = %d\n", accel_com_Flag);
		if(accel_com_Flag==1)
		{
			gpio_set_value(ACCEL_ECOM_GPIO_SENSOR_EN_N, 0);	// 3V0_SENSOR  OFF (FROM LDO) 2011-09-29 jeongyong.lee@lge.com
			m3s_sensor_ldo_gpio_config(0);
			accel_com_Flag=0;
			printk("accel_com_flag = %d\n", accel_com_Flag);
		}

			ret = vreg_disable(gp7_vreg);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
					vreg_disable(gp7_vreg) return error %d\n",__func__, ret);
			}

			m3s_ecom_sensor_gpio_config(0);
	}
	return ret;
}

static struct ecom_platform_data ecom_pdata = {
	.pin_int        	= ECOM_GPIO_INT,
	.pin_rst		= 0,
	.power          	= ecom_power_set,
	.accelerator_name = "bma222",
/* LGE_CHANGE,
 * add accel tuning data for H/W accerleration sensor direction,
 * based on [hyesung.shin@lge.com] for <Sensor driver structure>
 *
 * 2011-07-05
 */
/*	.fdata_sign_x = -1,
    .fdata_sign_y = 1,
    .fdata_sign_z = -1,
    .fdata_order0 = 0,
    .fdata_order1 = 1,
    .fdata_order2 = 2,
    .sensitivity1g = 64,
*/
};

static struct i2c_board_info ecom_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("hscd_i2c", ECOM_I2C_ADDRESS),
		.type = "hscd_i2c",
		.platform_data = &ecom_pdata,
	},
};

static struct gpio_i2c_pin ecom_i2c_pin[] = {
	[0] = {
		.sda_pin	= ECOM_GPIO_I2C_SDA,
		.scl_pin	= ECOM_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ECOM_GPIO_INT,
	},
};

static struct i2c_gpio_platform_data ecom_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device ecom_i2c_device = {
        .name = "i2c-gpio",
        .dev.platform_data = &ecom_i2c_pdata,
};


static void __init m3eu_init_i2c_ecom(int bus_num, bool platform)
{
	ecom_i2c_device.id = bus_num;

	printk("[smiledice] LG_FW : %s, bus_num is %d\n",__func__, bus_num);
	
	printk("[smiledice] LG_FW : %s : ecom_i2c_pin[0].irq_pin %d, \
			ecom_i2c_pin[0].reset_pin %d, \
			ecom_i2c_pin[0].scl_pin %d, \
			ecom_i2c_pin[0].sda_pin %d\n",
			__func__, 
			ecom_i2c_pin[0].irq_pin, ecom_i2c_pin[0].reset_pin, 
			ecom_i2c_pin[0].scl_pin, ecom_i2c_pin[0].sda_pin);

	init_gpio_i2c_pin(&ecom_i2c_pdata, ecom_i2c_pin[0], &ecom_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &ecom_i2c_bdinfo[0], 1);
	if(platform) {
		platform_device_register(&ecom_i2c_device);
	}
}

/* proximity */
/*
static void m3s_proximity_sensor_gpio_config(int onoff)
{
	if(onoff)
	{
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_I2C_SDA, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_I2C_SCL, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_DOUT, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}else
	{
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_I2C_SDA, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_I2C_SCL, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(PROXI_GPIO_DOUT, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
}
*/
static int prox_power_set(unsigned char onoff)
{
/* just return 0, later I'll fix it */
	static bool init_done = 0;
	
	int ret = 0;

#if 0
/* need to be fixed  - for vreg using SUB PMIC */
	struct regulator* ldo5 = NULL;

	ldo5 = regulator_get(NULL, "RT8053_LDO5");
	if (ldo5 == NULL) {
		pr_err("%s: regulator_get(ldo5) failed\n", __func__);
	}

	printk("[Proximity] %s() : Power %s\n",__FUNCTION__, onoff ? "On" : "Off");
	
	if (init_done == 0 && onoff)
	{
		if (onoff) {
			printk(KERN_INFO "LDO5 vreg set.\n");
			ret = regulator_set_voltage(ldo5, 2800000, 2800000);
			if (ret < 0) {
				pr_err("%s: regulator_set_voltage(ldo5) failed\n", __func__);
			}
			ret = regulator_enable(ldo5);
			if (ret < 0) {
                pr_err("%s: regulator_enable(ldo5) failed\n", __func__);
            }
			
			init_done = 1;
		} else {
			ret = regulator_disable(ldo5);
			if (ret < 0) {
                pr_err("%s: regulator_disable(ldo5) failed\n", __func__);
            }

		}
	}

#else

	struct vreg *gp7_vreg    = vreg_get(0, "gp7");     //1V8_SENSOR
	struct vreg *xo_out_vreg = vreg_get(0, "xo_out");  //2V85_PROXIMITY (from PM, not SubPMIC)
	if (!gp7_vreg || !xo_out_vreg) {
			printk("[smiledice] LG_FW : %s, can't get vreg_get\n",__func__);
	}

	printk("[Proximity] %s() : Power %s\n",__FUNCTION__, onoff ? "On" : "Off");
	
	if (onoff) 
	{
		if (init_done == 0)
		{
			printk(KERN_INFO "prox_power_on\n");
			ret = vreg_set_level(xo_out_vreg, 2850);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
						vreg_set_level(xo_out_vreg, 2850) return error %d\n",__func__, ret);
			}
			ret = vreg_set_level(gp7_vreg, 1800);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
						vreg_set_level(gp7_vreg, 1800) return error %d\n",__func__, ret);
			}

			ret = vreg_enable(xo_out_vreg);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
						vreg_enable(xo_out_vreg) return error %d\n",__func__, ret);
			}

			ret = vreg_enable(gp7_vreg);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
						vreg_enable(gp7_vreg) return error %d\n",__func__, ret);
			}
//			m3s_proximity_sensor_gpio_config(1);
			init_done = 1;
		}
	} 
	else 
	{
		if (init_done == 1)
		{
			printk(KERN_INFO "prox_power_off\n");
			ret = vreg_disable(xo_out_vreg);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
						vreg_disable(xo_out_vreg) return error %d\n",__func__, ret);
			}

			ret = vreg_disable(gp7_vreg);
			if (ret) {
				printk("[smiledice] LG_FW : %s, \
						vreg_disable(gp7_vreg) return error %d\n",__func__, ret);
			}

//			m3s_proximity_sensor_gpio_config(0);
			init_done = 0;
		}
	}

#endif

	return ret;
}

// nb_jeans 120516  LS696 ¢®¨úUA¢®E¡§u¡§u¡§u¢®¨Ï ¡§uoA¡Ë¢¥¢®icC¢®¢¯ ¡Ë¡þOAo detection time(.cycle) 32(2) -> 8ms(0)
static struct proximity_platform_data proxi_pdata = {
	.irq_num	= PROXI_GPIO_DOUT,
	.power		= prox_power_set,
	.methods		= 1,
	.operation_mode		= 1,
	.debounce	 = 0,
	.cycle = 0,
};

static struct i2c_board_info prox_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("proximity_gp2ap", PROXI_I2C_ADDRESS),
		.type = "proximity_gp2ap",
		.platform_data = &proxi_pdata,
	},
};

static struct gpio_i2c_pin proxi_i2c_pin[] = {
	[0] = {
		.sda_pin	= PROXI_GPIO_I2C_SDA,
		.scl_pin	= PROXI_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= PROXI_GPIO_DOUT,
	},
};

static struct i2c_gpio_platform_data proxi_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device proxi_i2c_device = {
        .name = "i2c-gpio",
        .dev.platform_data = &proxi_i2c_pdata,
};

static void __init m3eu_init_i2c_prox(int bus_num, bool platform)
{
	proxi_i2c_device.id = bus_num;

	printk("[smiledice] LG_FW : %s, bus_num is %d\n",__func__, bus_num);
	
	printk("[smiledice] LG_FW : %s : proxi_i2c_pin[0].irq_pin %d, proxi_i2c_pin[0].reset_pin %d, proxi_i2c_pin[0].scl_pin %d, proxi_i2c_pin[0].sda_pin %d\n",
			__func__, 
			proxi_i2c_pin[0].irq_pin, proxi_i2c_pin[0].reset_pin, 
			proxi_i2c_pin[0].scl_pin, proxi_i2c_pin[0].sda_pin);

	init_gpio_i2c_pin(&proxi_i2c_pdata, proxi_i2c_pin[0], &prox_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &prox_i2c_bdinfo[0], 1);
	if(platform) {
		platform_device_register(&proxi_i2c_device);
	}
}
//LGE_CHANGE_E SENSOR FIRMWARE UPDATE (jongkwon.chae@lge.com)

// matthew.choi@lge.com 111017 Add side volume keys [START]
static struct platform_device *m3s_input_devices[] __initdata = {
	&m3s_gpio_keypad_device,
};
// matthew.choi@lge.com 111017 Add side volume keys [END]

/* common function */
void __init lge_add_input_devices(void)
{
	printk(KERN_INFO "[%s: %d] enter\n",__func__,__LINE__);
	// add input devices

	//LGE_CHANGE_S SENSOR FIRMWARE UPDATE (jongkwon.chae@lge.com)
	lge_add_gpio_i2c_device(m3eu_init_i2c_acceleration, &accel_i2c_pin[0]);
	lge_add_gpio_i2c_device(m3eu_init_i2c_ecom, &ecom_i2c_pin[0]);
	lge_add_gpio_i2c_device(m3eu_init_i2c_prox, &proxi_i2c_pin[0]);
	//LGE_CHANGE_E SENSOR FIRMWARE UPDATE (jongkwon.chae@lge.com)

#if defined(CONFIG_TOUCHSCREEN_MELFAS_TS)
	lge_add_gpio_i2c_device(m3eu_init_i2c_touch, &ts_i2c_pin[0]);
#endif
// [2011-09-08] NFC Porting, addy.kim@lge.com [START]
#ifdef CONFIG_PN544_NFC
	lge_add_gpio_i2c_device(m3s_init_i2c_nfc, &nfc_i2c_pin[0]);
#else
	m3s_nfc_gpio_sleep_set();
#endif
// [2011-09-08] NFC Porting, addy.kim@lge.com [END]

// matthew.choi@lge.com 111017 Add side volume keys [START]
	platform_add_devices(m3s_input_devices, ARRAY_SIZE(m3s_input_devices));
// matthew.choi@lge.com 111017 Add side volume keys [END]

	return;
}

