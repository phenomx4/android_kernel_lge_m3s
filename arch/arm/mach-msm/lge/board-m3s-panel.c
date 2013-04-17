/* arch/arm/mach-msm/board-thunderc-panel.c
 * Copyright (C) 2009 LGE, Inc.
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

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <mach/msm_rpcrouter.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <mach/board.h>
#include <mach/board_lge.h>
#include "devices.h"

//#include "board-m3s.h"
#include <mach/board_lge.h>
#include <mach/vreg.h>
#define MSM_FB_LCDC_VREG_OP(name, op, level)			\
do { \
	vreg = vreg_get(0, name); \
	vreg_set_level(vreg, level); \
	if (vreg_##op(vreg)) \
		printk(KERN_ERR "%s: %s vreg operation failed \n", \
			(vreg_##op == vreg_enable) ? "vreg_enable" \
				: "vreg_disable", name); \
} while (0)


#define LCD_RESET_GPIO		94
#if 0
static char *msm_fb_vreg[] = {
	"gp4",
	"gp2",
};
#endif

static int mddi_power_save_on;
static int msm_fb_mddi_power_save(int on)
{
#if 0
	struct vreg *vreg;
	int flag_on = !!on;

	if (mddi_power_save_on == flag_on)
		return 0;

	mddi_power_save_on = flag_on;

	if (on) {
		MSM_FB_LCDC_VREG_OP(msm_fb_vreg[0], enable, 2600);
		MSM_FB_LCDC_VREG_OP(msm_fb_vreg[1], enable, 2800);
	} 
	else {
		MSM_FB_LCDC_VREG_OP(msm_fb_vreg[0], disable, 0);
		MSM_FB_LCDC_VREG_OP(msm_fb_vreg[1], disable, 0);
	}
#endif
	return 0;
}

static int msm_fb_mddi_sel_clk(u32 *clk_rate)
{
	*clk_rate *= 2;
	return 0;
}

static struct mddi_platform_data mddi_pdata = {

	.mddi_power_save = msm_fb_mddi_power_save,
   .mddi_sel_clk = msm_fb_mddi_sel_clk,		// Camera preview tearing
	.mddi_client_power = NULL,//msm_fb_mddi_client_power,
};
//LGE_UPDATE_E minhobb2.kim@lge.com for RevA LCD power & Camera preview tearing
static int mdp_core_clk_rate_table[] = {
	122880000,
	122880000,
	122880000,
	192000000,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 30,
	.mdp_core_clk_rate = 122880000,
	.mdp_core_clk_table = mdp_core_clk_rate_table,
	.num_mdp_clk = ARRAY_SIZE(mdp_core_clk_rate_table),
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("pmdh", &mddi_pdata);
}

//#if defined(CONFIG_FB_MSM_MDDI_R61529_HVGA)
static int mddi_lm3530_backlight(int level)
{
	/* TODO: Backlight control here */
	return 0;
}

static struct msm_panel_r61529_pdata mddi_r61529_panel_data = {
	.gpio = LCD_RESET_GPIO,				/* lcd reset_n */
	.pmic_backlight = mddi_lm3530_backlight,
	.initialized = 1,
};

static struct platform_device mddi_r61529_panel_device = {
	.name   = "mddi_r61529_hvga",
	.id     = 0,
	.dev    = {
	.platform_data = &mddi_r61529_panel_data,
	}
};
//#endif


/* backlight device */
static struct gpio_i2c_pin bl_i2c_pin[] = {
	[0] = {
		.sda_pin	= 89,
		.scl_pin	= 88,
		.reset_pin	= 126,
		.irq_pin	= 0,
	},
};

static struct i2c_gpio_platform_data bl_i2c_pdata = {
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay				= 2,
};

static struct platform_device bl_i2c_device = {
	.name	= "i2c-gpio",
	.dev.platform_data = &bl_i2c_pdata,
};

static struct m3s_backlight_platform_data lm3530bl_data = {
	.gpio = 126,
};

static struct i2c_board_info bl_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("lm3530bl", 0x38),
		.type = "lm3530bl",
		.platform_data =  NULL, //&lm3530bl_data,
	},
};

struct device* m3s_backlight_dev(void)
{
	return &bl_i2c_device.dev;
}

void __init m3s_init_i2c_backlight(int bus_num, bool platform)
{
	bl_i2c_device.id = bus_num;
	bl_i2c_bdinfo[0].platform_data = &lm3530bl_data;
	
	init_gpio_i2c_pin(&bl_i2c_pdata, bl_i2c_pin[0], &bl_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, &bl_i2c_bdinfo[0], 1);
	if(platform)
		platform_device_register(&bl_i2c_device);
}

/* common functions */
void __init lge_add_lcd_devices(void)
{

	platform_device_register(&mddi_r61529_panel_device);
	lge_add_gpio_i2c_device(m3s_init_i2c_backlight,&bl_i2c_pin[0]);
	msm_fb_add_devices();

}
