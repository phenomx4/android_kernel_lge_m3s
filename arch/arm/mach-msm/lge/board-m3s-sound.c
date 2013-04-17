/* linux/arch/arm/mach-msm/lge/board-m3s-sound.c
 *
 * Copyright (C) 2010 LGE.
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

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>

#include <asm/mach-types.h>

#include <mach/board.h>
#include <mach/board_lge.h>

#include <mach/qdsp5v2/msm_lpa.h>

#include <linux/mfd/marimba.h>
#include <mach/qdsp5v2/aux_pcm.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/audio_dev_ctl.h>
#include <mach/rpc_server_handset.h>
#include "mach/qdsp5v2/lge_tpa2055-amp.h"
#include <mach/pmic.h>

#include <mach/rpc_pmapp.h>


#if defined(CONFIG_LGE_PMIC_VIBRATOR)
#define PMIC_VIBRATOR_LEVEL	(3000)

static struct lge_pmic8058_vibrator_pdata m3s_vibrator_data = {
	.max_timeout_ms = 30000, /* max time for vibrator enable 30 sec. */
	.level_mV = PMIC_VIBRATOR_LEVEL,
};

static struct platform_device m3s_vibrator_device = {
	.name   = "msm7x30_pm8058-vib",
	.id = -1,
	.dev = {
		.platform_data = &m3s_vibrator_data,
	},
};

#endif /*CONFIG_LGE_PMIC_VIBRATOR*/

// matthew.choi@lge.com 111003 for M3S Rev.B
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_A)
#define GPIO_AMP_I2C_SDA	89
#define GPIO_AMP_I2C_SCL	88
#else
#define GPIO_AMP_I2C_SDA	92
#define GPIO_AMP_I2C_SCL	93
#endif

static struct gpio_i2c_pin lge_amp_i2c_pin[] = {
	[0] = {
    .sda_pin = GPIO_AMP_I2C_SDA,
    .scl_pin = GPIO_AMP_I2C_SCL,
	},
};

static struct i2c_gpio_platform_data lge_amp_i2c_pdata = {
	.sda_pin = GPIO_AMP_I2C_SDA,
	.scl_pin = GPIO_AMP_I2C_SCL,
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

struct platform_device lge_amp_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &lge_amp_i2c_pdata,
};

#if defined(CONFIG_LGE_RCV_SPK_SWITCH)  && (CONFIG_LGE_PCB_REVISION < LGE_REV_B)
#define GPIO_EAR_SPK_SEL	80 

static void amp_bypass_switch_init(void)
{
	int err;
	
	err = gpio_request(GPIO_EAR_SPK_SEL, "rcv_spk");
	if(!err){
		gpio_direction_output(GPIO_EAR_SPK_SEL, 1);
		printk(KERN_INFO "gpio_spk_rcv to low\n");
	}
}

static void amp_bypass_switch(bool on)
{
	gpio_direction_output(GPIO_EAR_SPK_SEL,on?1: 0);
}
#endif /*CONFIG_LGE_RCV_SPK_SWITCH*/

static struct amp_platform_data lge_amp_data = {
	.bypass_mode = 1,
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_0)		
	.line_out = false,
#else
	.line_out = true,
#endif
#if defined(CONFIG_LGE_RCV_SPK_SWITCH) && (CONFIG_LGE_PCB_REVISION < LGE_REV_B)
	.external_bypass_init = amp_bypass_switch_init,
	.external_bypass = amp_bypass_switch,
#endif
};

static struct i2c_board_info lge_amp_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("lge_tpa2055", 0xE0 >> 1),
		.platform_data = &lge_amp_data,			
	},
};

/* LGE init function for sound*/
static void __init lge_m3s_amp_gpio_i2c_init(int bus_num, bool platform)
{
	lge_amp_i2c_device.id = bus_num;
	pr_info("[LGE] Audio subsystem gpio i2c init for (bus num: %d) \n", bus_num);
	init_gpio_i2c_pin(&lge_amp_i2c_pdata, lge_amp_i2c_pin[0], &lge_amp_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, lge_amp_i2c_bdinfo, ARRAY_SIZE(lge_amp_i2c_bdinfo));
	if(platform)
		platform_device_register(&lge_amp_i2c_device);
}

#if defined(CONFIG_LGE_HEADSET_2WIRE_DETECT)
void lge_m3s_mic_bias_power(int on)
{
	if(on){
		pmic_hsed_enable(PM_HSED_CONTROLLER_1, PM_HSED_ENABLE_PWM_TCXO);
	}
	else{
		pmic_hsed_enable(PM_HSED_CONTROLLER_1, PM_HSED_ENABLE_OFF);
	}
}

static struct gpio_h2w_platform_data m3s_h2w_data = {
	.gpio_detect = 26,
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_0)
	.gpio_button_detect = 101,
#else
	.gpio_button_detect = 40,
#endif	
	.gpio_jpole = 102,
	.gpio_mic_bias_en = 103,
	.mic_bias_power = lge_m3s_mic_bias_power,
};

static struct platform_device m3s_h2w_device = {
	.name = "gpio-h2w",
	.id = -1,
	.dev = {
		.platform_data = &m3s_h2w_data,
	},
};
#endif /*CONFIG_LGE_AUDIO_HEADSET*/


static struct platform_device *m3s_audio_devices[] __initdata = {
#ifdef CONFIG_LGE_HEADSET_2WIRE_DETECT
	&m3s_h2w_device,
#endif
#if defined(CONFIG_LGE_PMIC_VIBRATOR)
	&m3s_vibrator_device,
#endif

};


/* common function */
void __init lge_m3s_audio_init(void)
{
	platform_add_devices(m3s_audio_devices, ARRAY_SIZE(m3s_audio_devices));
	lge_add_gpio_i2c_device(lge_m3s_amp_gpio_i2c_init,&lge_amp_i2c_pin[0]);
}

