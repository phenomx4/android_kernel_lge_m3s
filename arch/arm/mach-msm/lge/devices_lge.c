/*
 * Copyright (C) 2008 Google, Inc.
 * Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/msm_rotator.h>
#include <linux/dma-mapping.h>
#include <linux/bootmem.h>
#include <linux/android_pmem.h>
#include <linux/msm_kgsl.h>

#include <asm/mach-types.h>

#include <mach/dal_axi.h>
#include <mach/irqs.h>
#include <mach/msm_iomap.h>
#include <mach/dma.h>
#include <mach/vreg.h>
#include <mach/board.h>
#include <mach/board_lge.h>

#include <asm/mach/flash.h>

#include <mach/socinfo.h>

#include <asm/mach/mmc.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android_composite.h>
#endif

#include <linux/spi/spi.h>
#include <mach/msm_spi.h>

#ifdef CONFIG_PMIC8058
#include <linux/mfd/pmic8058.h>
#endif

#include <mach/board_lge.h>

#include "../devices.h"
#include "../clock-7x30.h"
#include "../gpio_hw.h"
#include "../pm.h"

/* lge gpio i2c device */
#define MAX_GPIO_I2C_DEV_NUM	20
#define LOWEST_GPIO_I2C_BUS_NUM	8

struct i2c_add_gpio_type{
	int i2c_add_cnt;
	int i2c_dev_num;
	int i2c_bus_num_data[MAX_GPIO_I2C_DEV_NUM];
	int i2c_bus_init_flag[MAX_GPIO_I2C_DEV_NUM];
	struct gpio_i2c_pin  *gpio_i2c_pin_data[MAX_GPIO_I2C_DEV_NUM];
	gpio_i2c_init_func_t *i2c_init_func[MAX_GPIO_I2C_DEV_NUM];
};

static struct i2c_add_gpio_type add_gpio __initdata;

void __init lge_add_gpio_i2c_device(gpio_i2c_init_func_t *init_func,struct gpio_i2c_pin *gpio)
{
	int index;
	int found;
	struct gpio_i2c_pin *pPrev;

	if(!gpio)
		return;

	found = 0;
	for (index = 0; index < add_gpio.i2c_add_cnt; index++) {
		pPrev = add_gpio.gpio_i2c_pin_data[index];
		if(!pPrev)
			break;
		if(pPrev->sda_pin==gpio->sda_pin){
			found =1;
			add_gpio.i2c_bus_num_data[add_gpio.i2c_add_cnt] = add_gpio.i2c_bus_num_data[index];
			break;
		}
	}
	if(found==0){
		add_gpio.i2c_bus_num_data[add_gpio.i2c_add_cnt] = LOWEST_GPIO_I2C_BUS_NUM + add_gpio.i2c_dev_num;
		add_gpio.i2c_bus_init_flag[add_gpio.i2c_add_cnt]=1;
		add_gpio.i2c_dev_num++;
	}
	add_gpio.i2c_init_func[add_gpio.i2c_add_cnt] = init_func;
	add_gpio.gpio_i2c_pin_data[add_gpio.i2c_add_cnt] = gpio;
	add_gpio.i2c_add_cnt++;
}

void __init lge_add_gpio_i2c_devices(void)
{
	int index;
	gpio_i2c_init_func_t *init_func_ptr;

	for (index = 0; index < add_gpio.i2c_add_cnt; index++) {
		init_func_ptr = add_gpio.i2c_init_func[index];
		(*init_func_ptr)(add_gpio.i2c_bus_num_data[index],add_gpio.i2c_bus_init_flag[index]?1:0);
	}
}

int init_gpio_i2c_pin(struct i2c_gpio_platform_data *i2c_adap_pdata,
		struct gpio_i2c_pin gpio_i2c_pin,
		struct i2c_board_info *i2c_board_info_data)
{
	i2c_adap_pdata->sda_pin = gpio_i2c_pin.sda_pin;
	i2c_adap_pdata->scl_pin = gpio_i2c_pin.scl_pin;

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
					GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		i2c_board_info_data->irq =
			MSM_GPIO_TO_INT(gpio_i2c_pin.irq_pin);
	}

	return 0;
}

// matthew.choi@lge.com 111026 add virtual keyboard for at cmd [START]
#if 1 //def CONFIG_ATCMD_VIRTUAL_KBD
/* virtual key */
#define ATCMD_VIRTUAL_KEYPAD_ROW	8
#define ATCMD_VIRTUAL_KEYPAD_COL	8

#define KEY_STAR 227
#define KEY_SHARP 228

static unsigned short atcmd_virtual_keycode[ATCMD_VIRTUAL_KEYPAD_ROW][ATCMD_VIRTUAL_KEYPAD_COL] = {
	{KEY_1,          KEY_8,           KEY_Q,          KEY_I,          KEY_D,          KEY_HOME,       KEY_B,          KEY_UP},
	{KEY_2,          KEY_9,           KEY_W,          KEY_O,          KEY_F,          KEY_RIGHTSHIFT, KEY_N,          KEY_DOWN},
	{KEY_3,          KEY_0,           KEY_E,          KEY_P,          KEY_G,          KEY_Z,          KEY_M,          KEY_UNKNOWN},
	{KEY_4,          KEY_BACK,        KEY_R,          KEY_SEARCH,     KEY_H,          KEY_X,          KEY_LEFTSHIFT,  KEY_UNKNOWN},
	{KEY_5,          KEY_BACKSPACE,   KEY_T,          KEY_LEFTALT,    KEY_J,          KEY_C,          KEY_REPLY,      KEY_CAMERA},
	{KEY_6,          KEY_ENTER,       KEY_Y,          KEY_A,          KEY_K,          KEY_V,          KEY_RIGHT,      KEY_UNKNOWN},
	{KEY_7,          KEY_MENU,        KEY_U,          KEY_S,          KEY_L,          KEY_SPACE,      KEY_LEFT,       KEY_SEND},
	{KEY_STAR,       KEY_SHARP,       KEY_END,        KEY_UNKNOWN,    KEY_UNKNOWN,    KEY_UNKNOWN,    KEY_UNKNOWN,    KEY_UNKNOWN},
};

static struct atcmd_virtual_platform_data atcmd_virtual_pdata = {
	.keypad_row = ATCMD_VIRTUAL_KEYPAD_ROW,
	.keypad_col = ATCMD_VIRTUAL_KEYPAD_COL,
	.keycode = (unsigned char *)atcmd_virtual_keycode,
};

static struct platform_device atcmd_virtual_kbd_device = {
	.name = "atcmd_virtual_kbd",
	.id = -1,
	.dev = {
		.platform_data = &atcmd_virtual_pdata,
	},
};

void __init lge_add_atcmd_virtual_kbd_device(void)
{
	printk("atcmd_virtual_kbd : lge_add_atcmd_virtual_kbd_device\n");

    platform_device_register(&atcmd_virtual_kbd_device);
}
#endif
// matthew.choi@lge.com 111026 add virtual keyboard for at cmd [END]
