/* arch/arm/mach-msm/include/mach/board_lge.h
 * Copyright (C) 2010 LGE Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARCH_MSM_BOARD_M3S_H
#define __ASM_ARCH_MSM_BOARD_M3S_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/rfkill.h>
#include <linux/platform_device.h>
#include <asm/setup.h>

#if __GNUC__
#define __WEAK __attribute__((weak))
#endif

/********************************************************************/
// common data
/********************************************************************/
/* board revision information */
#define LGE_REV_0 0
#define LGE_REV_A 1
#define LGE_REV_B 2
#define LGE_REV_C 3
#define LGE_REV_D 4
#define LGE_REV_E 5
#define LGE_REV_F 6
#define LGE_REV_G 7
#define LGE_REV_H 8
#define LGE_REV_1P0 9
#define LGE_REV_1P1 10
#define LGE_REV_1P2 11
#define LGE_REV_1P3 12
#define LGE_REV_1P4 13
#define LGE_REV_1P5 14
#define LGE_REV_1P6 15
#define LGE_REV_1P7 16

/* define gpio pin number of i2c-gpio */
struct gpio_i2c_pin {
	unsigned int sda_pin;
	unsigned int scl_pin;
	unsigned int reset_pin;
	unsigned int irq_pin;
};


/********************************************************************/
// platform data
/********************************************************************/
/* gpio switch platform data */
struct lge_gpio_switch_platform_data {
	const char *name;
	unsigned *gpios;
	size_t num_gpios;
	unsigned long irqflags;
	unsigned int wakeup_flag;
	int (*work_func)(void);
	char *(*print_state)(int state);
	int (*sysfs_store)(const char *buf, size_t size);
};

/* msm pmic leds platform data */
struct msm_pmic_leds_pdata {
	struct led_classdev *custom_leds;
	int (*register_custom_leds)(struct platform_device *pdev);
	void (*unregister_custom_leds)(void);
	void (*suspend_custom_leds)(void);
	void (*resume_custom_leds)(void);
	int (*msm_keypad_led_set)(unsigned char value);
};

/* LED flash platform data */
struct led_flash_platform_data {
	int gpio_flen;
	int gpio_en_set;
	int gpio_inh;
};

/* android vibrator platform data */
struct android_vibrator_platform_data {
	int enable_status;
	int (*power_set)(int enable); 		/* LDO Power Set Function */
	int (*pwn_set)(int enable, int gain); 		/* PWM Set Function */
	int (*ic_enable_set)(int enable); 	/* Motor IC Set Function */
};

/* backlight platform data */
struct backlight_platform_data {
	void (*platform_init)(void);
	int gpio;
	unsigned int mode;		     /* initial mode */
	int max_current;			 /* led max current(0-7F) */
	int init_on_boot;			 /* flag which initialize on system boot */
};


struct m3s_backlight_platform_data {
	void (*platform_init)(void);
	int gpio;
	unsigned int mode;		     /* initial mode */
	int max_current;			 /* led max current(0-7F) */
	int init_on_boot;			 /* flag which initialize on system boot */
	int version;				 /* Chip version number */
};

struct msm_panel_r61529_pdata {
	int gpio;
	int (*backlight_level)(int level, int max, int min);
	int (*pmic_backlight)(int level);
	int (*panel_num)(void);
	void (*panel_config_gpio)(int);
	int *gpio_num;
	int initialized;
};

/* gpio h2w platform data */
struct gpio_h2w_platform_data {
	int gpio_detect;
	int gpio_button_detect;
	int gpio_jpole;
#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT		
	int gpio_mic_bias_en ;
#endif
	void (*mic_bias_power)(int);
};

/* touch screen platform data */
#if defined(CONFIG_TOUCHSCREEN_MELFAS_TS)
/* touch-screen macros */
#define TS_X_MIN		0
#define TS_X_MAX		320
#define TS_Y_MIN		0
#define TS_Y_MAX		480
#define TS_GPIO_I2C_SDA		71
#define TS_GPIO_I2C_SCL		70
#define TS_GPIO_IRQ		43
#define TS_I2C_SLAVE_ADDR	0x48	/* MELFAS Mcs8000(mms-128) addr is 0x48 */

struct touch_platform_data {
	int ts_x_min;
	int ts_x_max;
	int ts_y_min;
	int ts_y_max;
	int (*power)(unsigned char onoff);
	int irq;
	int scl;
	int sda;
};
#endif

//LGE_CHANGE_S SENSOR FIRMWARE UPDATE (jongkwon.chae@lge.com)
#define ACCEL_ECOM_GPIO_SENSOR_EN_N    97	/* LDO 3V0 */
/* accelerometer */
#define ACCEL_GPIO_INT	 		171
#define ACCEL_GPIO_I2C_SCL  	178
#define ACCEL_GPIO_I2C_SDA  	177
#define ACCEL_I2C_ADDRESS		0x08 /* slave address 7bit - BMA222 */

/* Ecompass */
// matthew.choi@lge.com 111003 for M3S Rev.B
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_A)
#define ECOM_GPIO_I2C_SCL		173
#define ECOM_GPIO_I2C_SDA		172
#else
#define ECOM_GPIO_I2C_SCL		149
#define ECOM_GPIO_I2C_SDA		150
#endif
#define ECOM_GPIO_INT			47   /* DRDY */
#define ECOM_I2C_ADDRESS		0x0C /* slave address 7bit - HSCDTD004A */

/* proximity sensor */
#define PROXI_GPIO_I2C_SCL	173
#define PROXI_GPIO_I2C_SDA 	172
#define PROXI_GPIO_DOUT		50
#define PROXI_I2C_ADDRESS	0x44 /* slave address 7bit - GP2AP002 */
//#define PROXI_LDO_NO_VCC	1

/* acceleration platform data */
struct acceleration_platform_data {
	int irq_num;
	int (*power)(unsigned char onoff);
};

/* ecompass platform data */
struct ecom_platform_data {
	int pin_int;
	int pin_rst;
	int (*power)(unsigned char onoff);
	char accelerator_name[20];
	int fdata_sign_x;
        int fdata_sign_y;
        int fdata_sign_z;
	int fdata_order0;
	int fdata_order1;
	int fdata_order2;
	int sensitivity1g;
	s16 *h_layout;
	s16 *a_layout;
	int drdy;
};

/* proximity platform data */
struct proximity_platform_data {
	int irq_num;
	int (*power)(unsigned char onoff);
	int methods;
	int operation_mode;
	int debounce;
	u8 cycle;
};
//LGE_CHANGE_E SENSOR FIRMWARE UPDATE (jongkwon.chae@lge.com)

// SD Card GPIO layout
#define SYS_GPIO_SD_DET		42

/*pm8058 vibrator platform data*/
struct lge_pmic8058_vibrator_pdata {
	int max_timeout_ms;
	int level_mV;
};

// matthew.choi@lge.com 111026 add virtual keyboard for at cmd [START]
/* atcmd virtual keyboard platform data */
struct atcmd_virtual_platform_data {
	unsigned int keypad_row;
	unsigned int keypad_col;
	unsigned char *keycode;
};

void __init lge_add_atcmd_virtual_kbd_device(void);
// matthew.choi@lge.com 111026 add virtual keyboard for at cmd [END]

void __init msm_init_pmic_vibrator(void);
void __init lge_m3s_audio_init(void);

typedef void (gpio_i2c_init_func_t)(int bus_num, bool platform);
void __init lge_add_gpio_i2c_device(gpio_i2c_init_func_t *init_func,struct gpio_i2c_pin *gpio);
void __init lge_add_gpio_i2c_devices(void);
int init_gpio_i2c_pin(struct i2c_gpio_platform_data *i2c_adap_pdata,
		struct gpio_i2c_pin gpio_i2c_pin,
		struct i2c_board_info *i2c_board_info_data);
void __init lge_add_lcd_devices(void);
/********************************************************************/
//  input 
/********************************************************************/
void __init lge_add_input_devices(void);
//LGE_CHANGE_S CAMERA FIRMWARE UPDATE (jongkwon.chae@lge.com)
void __init lge_add_camera_devices(void);
//LGE_CHANGE_E CAMERA FIRMWARE UPDATE (jongkwon.chae@lge.com)

#endif /*!__ASM_ARCH_MSM_BOARD_M3S_H*/

