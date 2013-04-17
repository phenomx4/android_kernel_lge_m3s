/* drivers/video/msm/src/panel/mddi/mddi_r61529_hvga.c
 *
 * Copyright (C) 2008 QUALCOMM Incorporated.
 * Copyright (c) 2008 QUALCOMM USA, INC.
 * 
 * All source code in this file is licensed under the following license
 * except where indicated.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */

#include "msm_fb.h"
#include "mddihost.h"
#include "mddihosti.h"
#include <asm/gpio.h>
#include <mach/vreg.h>
#include <mach/board_lge.h>


#define PANEL_DEBUG 0

#define LCD_CONTROL_BLOCK_BASE	0x110000
#define INTFLG		LCD_CONTROL_BLOCK_BASE|(0x18)
#define INTMSK		LCD_CONTROL_BLOCK_BASE|(0x1c)
#define VPOS		LCD_CONTROL_BLOCK_BASE|(0xc0)

//static uint32 mddi_r61529_curr_vpos;
//static boolean mddi_r61529_monitor_refresh_value = FALSE;
//static boolean mddi_r61529_report_refresh_measurements = FALSE;
static int is_lcd_on = 0;
static int ch_used[3];

/* The comment from AMSS codes:
 * Dot clock (10MHz) / pixels per row (320) = rows_per_second
 * Rows Per second, this number arrived upon empirically 
 * after observing the timing of Vsync pulses
 * XXX: TODO: change this values for INNOTEK PANEL */
static uint32 mddi_r61529_rows_per_second = 31250;
static uint32 mddi_r61529_rows_per_refresh = 480;
//static uint32 mddi_r61529_usecs_per_refresh = 15360; /* rows_per_refresh / rows_per_second */
extern boolean mddi_vsync_detect_enabled;

static msm_fb_vsync_handler_type mddi_r61529_vsync_handler = NULL;
static void *mddi_r61529_vsync_handler_arg;
static uint16 mddi_r61529_vsync_attempts;

#if defined(CONFIG_FB_MSM_MDDI_NOVATEK_HITACHI_HVGA)
//extern int g_mddi_lcd_probe;
#endif


static struct msm_panel_r61529_pdata *mddi_r61529_pdata;

static struct msm_panel_r61529_pdata *mddi_r61529_panel_data;

static int mddi_r61529_lcd_on(struct platform_device *pdev);
static int mddi_r61529_lcd_off(struct platform_device *pdev);

static int mddi_r61529_lcd_init(void);
static void mddi_r61529_lcd_panel_poweron(void);
//static void mddi_r61529_lcd_panel_poweroff(void);
static void mddi_r61529_lcd_panel_store_poweron(void);

#define DEBUG 1
#if DEBUG
#define EPRINTK(fmt, args...) printk(fmt, ##args)
#else
#define EPRINTK(fmt, args...) do { } while (0)
#endif

struct display_table {
    unsigned reg;
    unsigned char count;
//    unsigned char val_list[30];
    unsigned val_list[256];
};

#define REGFLAG_DELAY             0XFFFE
#define REGFLAG_END_OF_TABLE      0xFFFF   // END OF REGISTERS MARKER

static struct display_table r61529_init_on_cmds[] = {
	{0xB0, 4, {0x04,0x00,0x00,0x00}},
	{0x35, 4, {0x00,0x00,0x00,0x00}},
	{0x44, 4, {0x00,0x00,0x00,0x00}},
	{0x36, 4, {0x08,0x00,0x00,0x00}},
	{0x3A, 4, {0x77,0x00,0x00,0x00}}, //24bit    {0x66}--18bit   {0x55}-16bit
	{0x2A, 4, {0x00,0x00,0x01,0x3F}},
	{0X2B, 4, {0x00,0x00,0x01,0xDF}},
	{0xB3,4 , {0x02,0x00,0x00,0x00}},
	//{0xC0, 8, {0x01,0xDF,0x40,0x13,0x00,0x01,0x00,0x33}},
	{0xC0, 8, {0x01,0xDF,0x40,0x10,0x00,0x01,0x00,0x33}},
	//{0xC1,8 , {0x07,0x27,0x08,0x08,0x20,0x00,0x00}},
   	{0xC1,8 , {0x07,0x27,0x08,0x08,0x10,0x00,0x00,0x00}},
	{0xC4,4 , {0x77,0x00,0x03,0x01}},
	{0xC6, 4, {0x00,0x00,0x00,0x00}},

//	{0xC8,24,{0x00,0x10,0x18,0x24,0x2F,0x48,0x38,0x24,0x18,0x0E,0x06,0x00,0x00,0x10,0x18,0x24,0x2F,0x48,0x38,0x24,0x18,0x0E,0x06,0x00}},
	{0xC8,24,{0x02,0x0B,0x16,0x20,0x2D,0x47,0x39,0x25,0x1C,0x14,0x0B,0x00,0x02,0x0B,0x16,0x20,0x2D,0x47,0x39,0x25,0x1C,0x14,0x0B,0x00}},
//	{0xC8,24,{0x00,0x10,0x16,0x21,0x2E,0x47,0x38,0x24,0x18,0x0E,0x06,0x00,0x00,0x10,0x16,0x21,0x2E,0x47,0x38,0x24,0x18,0x0E,0x06,0x00}},

//	{0xC9,24,{0x00,0x10,0x18,0x24,0x2F,0x48,0x38,0x24,0x18,0x0E,0x06,0x00,0x00,0x10,0x18,0x24,0x2F,0x48,0x38,0x24,0x18,0x0E,0x06,0x00}},
	{0xC9,24,{0x02,0x0B,0x16,0x20,0x2D,0x47,0x39,0x25,0x1C,0x14,0x0B,0x00,0x02,0x0B,0x16,0x20,0x2D,0x47,0x39,0x25,0x1C,0x14,0x0B,0x00}},
//	{0xC9,24,{0x00,0x10,0x16,0x21,0x2E,0x47,0x38,0x24,0x18,0x0E,0x06,0x00,0x00,0x10,0x16,0x21,0x2E,0x47,0x38,0x24,0x18,0x0E,0x06,0x00}},

//	{0xCA,24,{0x00,0x10,0x18,0x24,0x2F,0x48,0x38,0x24,0x18,0x0E,0x06,0x00,0x00,0x10,0x18,0x24,0x2F,0x48,0x38,0x24,0x18,0x0E,0x06,0x00}},
	{0xCA,24,{0x02,0x0B,0x16,0x20,0x2D,0x47,0x39,0x25,0x1C,0x14,0x0B,0x00,0x02,0x0B,0x16,0x20,0x2D,0x47,0x39,0x25,0x1C,0x14,0x0B,0x00}},
//	{0xCA,24,{0x00,0x10,0x16,0x21,0x2E,0x47,0x38,0x24,0x18,0x0E,0x06,0x00,0x00,0x10,0x16,0x21,0x2E,0x47,0x38,0x24,0x18,0x0E,0x06,0x00}},

	{0xD0, 16, {0xA9,0x06,0x08,0x20,0x31,0x04,0x01,0x00,0x08,0x01,0x00,0x06,0x01,0x00,0x00,0x20}},
	{0xD1,4 , {0x02,0x22,0x22,0x33}},
	{0xE0,4 , {0x00,0x00,0x00,0x00}},
	{0xE1,8 , {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0xE2, 4, {0x80,0x00,0x00,0x00}},
	{0x2C, 4, {0x00,0x00,0x00,0x00}}, //write memory start
};

static struct display_table r61529_disp_on_cmds[] = {
	{0x11, 4, {0x00, 0x00, 0x00, 0x00}},
	{REGFLAG_DELAY, 120, {0}},
	{0x29, 4, {0x00,0x00,0x00,0x00}},
	{REGFLAG_DELAY, 40, {0}},
	{REGFLAG_END_OF_TABLE, 0x00, {0}}
};

static struct display_table r61529_disp_off_cmds[] = {
	{0x28, 4, {0x00,0x00,0x00,0x00}},
	{REGFLAG_DELAY, 40, {0}},
	{0x10,4, {0x00,0x00,0x00,0x00}}, //sleep mode IN
	{REGFLAG_DELAY, 120, {0}},
	{REGFLAG_END_OF_TABLE, 0x00, {0}}
};

static struct display_table r61529_position_set_cmds[] = {
	{REGFLAG_END_OF_TABLE, 0x00, {0}}
};

void r61529_display_table(struct display_table *table, unsigned int count)
{
	unsigned int i;

    printk("%s : start\n", __func__);

    for(i = 0; i < count; i++) {
		
        unsigned reg;
        reg = table[i].reg;
		
        switch (reg) {
			
            case REGFLAG_DELAY :
                msleep(table[i].count);
				EPRINTK("%s() : delay %d msec\n", __func__, table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
                mddi_host_register_cmds_write32(reg, table[i].count, table[i].val_list, 0, 0, 0);
  //              mddi_host_register_cmds_write8(reg, table[i].count, table[i].val_list, 0, 0, 0);
		EPRINTK("%s: reg : %x, val : %x.\n", __func__, reg, table[i].val_list[0]);
       	}
    }
	
}
#if 0
static void compare_table(struct display_table *table, unsigned int count)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned reg;
        reg = table[i].reg;
		
        switch (reg) {
			
            case REGFLAG_DELAY :              
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
                mddi_host_register_cmds_write8(reg, table[i].count, table[i].val_list, 0, 0, 0);
		EPRINTK("%s: reg : %x, val : %x.\n", __func__, reg, table[i].val_list[0]);
       	}
    }	
}
#endif

static void mddi_r61529_vsync_set_handler(msm_fb_vsync_handler_type handler,	/* ISR to be executed */
					 void *arg)
{
	boolean error = FALSE;
	unsigned long flags;

	/* LGE_CHANGE [neo.kang@lge.com] 2009-11-26, change debugging api */
	printk("%s : handler = %x\n", 
			__func__, (unsigned int)handler);

	/* Disable interrupts */
	spin_lock_irqsave(&mddi_host_spin_lock, flags);
	/* INTLOCK(); */

	if (mddi_r61529_vsync_handler != NULL) {
		error = TRUE;
	} else {
		/* Register the handler for this particular GROUP interrupt source */
		mddi_r61529_vsync_handler = handler;
		mddi_r61529_vsync_handler_arg = arg;
	}
	
	/* Restore interrupts */
	spin_unlock_irqrestore(&mddi_host_spin_lock, flags);
	/* MDDI_INTFREE(); */
	if (error) {
		printk("MDDI: Previous Vsync handler never called\n");
	} else {
		/* Enable the vsync wakeup */
		/* mddi_queue_register_write(INTMSK, 0x0000, FALSE, 0); */
		mddi_r61529_vsync_attempts = 1;
		mddi_vsync_detect_enabled = TRUE;
	}
}

static void mddi_r61529_lcd_vsync_detected(boolean detected)
{
   printk("%s : start\n", __func__);
	mddi_vsync_detect_enabled = TRUE;;
}

static int mddi_r61529_lcd_on(struct platform_device *pdev)
{
   static int lcd_init_on = 0;
#if 1
   if( lcd_init_on == 1 )
   {
      lcd_init_on=2;
      return 0;
   }	
#endif
   printk("%s : start\n", __func__);

//jsLEE	if(is_lcd_on != TRUE )
	{
	/*
	 * mddi_host_client_cnt_reset:
	 * reset client_status_cnt to 0 to make sure host does not
	 * send RTD cmd to client right after resume before mddi
	 * client be powered up. this fix "MDDI RTD Failure" problem
	 */
	mddi_host_client_cnt_reset();

	// LCD HW Reset
	mddi_r61529_lcd_panel_poweron();

	r61529_display_table(r61529_init_on_cmds, sizeof(r61529_init_on_cmds)/sizeof(struct display_table));
	r61529_display_table(r61529_disp_on_cmds, sizeof(r61529_disp_on_cmds) / sizeof(struct display_table));
	}
#if 1
   if( lcd_init_on == 0)
      lcd_init_on = 1;
#endif   
	return 0;
}

static int mddi_r61529_lcd_store_on(void)
{
	EPRINTK("%s: started.\n", __func__);

	// LCD HW Reset
	mddi_r61529_lcd_panel_store_poweron();

	r61529_display_table(r61529_init_on_cmds, sizeof(r61529_init_on_cmds)/sizeof(struct display_table));
	r61529_display_table(r61529_disp_on_cmds, sizeof(r61529_disp_on_cmds) / sizeof(struct display_table));

	return 0;
}

static int mddi_r61529_lcd_off(struct platform_device *pdev)
{
   static int lcd_init_off = 0;
#if 1   
   if( lcd_init_off == 0 )
   {
      lcd_init_off=1;
      return 0;
   }
#endif   
   printk("%s : start\n", __func__);
	{
	   r61529_display_table(r61529_disp_off_cmds, sizeof(r61529_disp_off_cmds)/sizeof(struct display_table));
      //	mddi_r61529_lcd_panel_poweroff();
	}
	return 0;
}

ssize_t mddi_r61529_lcd_show_onoff(struct device *dev, struct device_attribute *attr, char *buf)
{
	EPRINTK("%s : strat\n", __func__);
	return 0;
}

ssize_t mddi_r61529_lcd_store_onoff(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct platform_device dummy_pdev;
	int onoff; // = simple_strtol(buf, NULL, count);
	sscanf(buf, "%d", &onoff);

	EPRINTK("%s: onoff : %d\n", __func__, onoff);
	
	if(onoff) {
		mddi_r61529_lcd_store_on();
	}
	else {
		mddi_r61529_lcd_off(&dummy_pdev);
	}

	return 0;
}

int mddi_r61529_position(void)
{
   printk("%s : start\n", __func__);
	r61529_display_table(r61529_position_set_cmds, ARRAY_SIZE(r61529_position_set_cmds));
	return 0;
}
EXPORT_SYMBOL(mddi_r61529_position);

/* LGE_S   jihye.ahn   10-10-20   added LCD on/off interface for CDMA test mode */
static void display_on(void)
{
	printk("%s : start\n", __func__);
	r61529_display_table(r61529_disp_on_cmds, sizeof(r61529_disp_on_cmds) / sizeof(struct display_table));
	return;
}

static void display_off(void)
{
	printk("%s : start\n", __func__);
	r61529_display_table(r61529_disp_off_cmds, sizeof(r61529_disp_off_cmds)/sizeof(struct display_table));
	return;
}
/* LGE_E   jihye.ahn   10-10-20   added LCD on/off interface for CDMA test mode */

static int lcd_power_on(void)
{
	printk("%s : start\n", __func__);
	r61529_display_table(r61529_disp_on_cmds, sizeof(r61529_disp_on_cmds)/sizeof(struct display_table));	
	return 0;
}

ssize_t onoff_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s : start\n", __func__);
	return 0;
}

ssize_t onoff_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value, ret;

   printk("%s : start\n", __func__);

	sscanf(buf, "%d\n", &value);

	if (value == 0)
		display_off();
	else if (value == 1)
      ret = lcd_power_on();
	else
		printk("0 : off, 1 : on \n");

	return count;
}


DEVICE_ATTR(lcd_onoff, 0666, onoff_show, onoff_store);

static int mddi_r61529_lcd_probe(struct platform_device *pdev)
{
	int ret;
	EPRINTK("%s: started.\n", __func__);

	if (pdev->id == 0) {
		mddi_r61529_panel_data = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

   //joosang.lee testmode IF
	ret = device_create_file(&pdev->dev, &dev_attr_lcd_onoff);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mddi_r61529_lcd_probe,
	.driver = {
		.name   = "mddi_r61529_hvga",
	},
};

struct msm_fb_panel_data r61529_panel_data0 = {
	.on = mddi_r61529_lcd_on,
	.off = mddi_r61529_lcd_off,
	.set_backlight = NULL,
	.set_vsync_notifier = mddi_r61529_vsync_set_handler,
};

static int mddi_r61529_lcd_init(void)
{
   printk("%s : start\n", __func__);
   return platform_driver_register(&this_driver);
}

extern unsigned fb_width;
extern unsigned fb_height;

static void mddi_r61529_lcd_panel_poweron(void)
{
	struct msm_panel_r61529_pdata *pdata = mddi_r61529_pdata;

	EPRINTK("%s: started.\n", __func__);

	fb_width = 320;
	fb_height = 480;

   gpio_tlmm_config(GPIO_CFG(124, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE); // LCD_RESET_N
   gpio_tlmm_config(GPIO_CFG(125, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE); // LCD_RESET_N
   gpio_tlmm_config(GPIO_CFG(94, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE); // LCD_RESET_N

   mdelay(10);
   gpio_set_value(124, 0);
   gpio_set_value(125, 0);

   mdelay(10);

	if(pdata && pdata->gpio) 
   {
		gpio_set_value(pdata->gpio, 1);
		mdelay(10);
		gpio_set_value(pdata->gpio, 0);
		mdelay(10);
		gpio_set_value(pdata->gpio, 1);
		mdelay(2);
	}
}

static void mddi_r61529_lcd_panel_store_poweron(void)
{
	struct msm_panel_r61529_pdata *pdata = mddi_r61529_pdata;

	EPRINTK("%s: started.\n", __func__);

	fb_width = 320;
	fb_height = 480;

	if(pdata && pdata->gpio) {
		gpio_set_value(pdata->gpio, 1);
		mdelay(10);
		gpio_set_value(pdata->gpio, 0);
		mdelay(50);
		gpio_set_value(pdata->gpio, 1);
		mdelay(50);
	}
}

/* LGE_CHANGE
  * Add new function to reduce current comsumption in sleep mode.
  * In sleep mode disable LCD by assertion low on reset pin.
  * 2010-06-07, minjong.gong@lge.com
  */
  #if 0
static void mddi_r61529_lcd_panel_poweroff(void)
{
	struct msm_panel_r61529_pdata *pdata = mddi_r61529_pdata;

	EPRINTK("%s: started.\n", __func__);

	fb_width = 320;
	fb_height = 480;

   if(pdata && pdata->gpio) 
   {
		gpio_set_value(pdata->gpio, 0);
		mdelay(5);
	}
}
  #endif

int mddi_r61529_device_register(struct msm_panel_info *pinfo,u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	printk("%s channel=%d, panel=%d\n", __func__, channel, panel);

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mddi_r61529_hvga", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	r61529_panel_data0.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &r61529_panel_data0, sizeof(r61529_panel_data0));
	if (ret) 
   {
		printk(KERN_ERR "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) 
   {
		printk(KERN_ERR "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
   platform_device_put(pdev);
   return ret;
}

int _fps = 6000;
int v_front_porch= 30;
int v_back_porch= 50;
int v_pulse_width= 2;   
int _clk_rate=380000000;
static int mddi_r61529_wqvga_pt_init(void)
{
   int ret;
   struct msm_panel_info pinfo;

   printk("%s\n", __func__);
   
   pinfo.xres = 320;
   pinfo.yres = 480;
   MSM_FB_SINGLE_MODE_PANEL(&pinfo);
   pinfo.type = MDDI_PANEL;
   pinfo.pdest = DISPLAY_1;
   pinfo.mddi.vdopkt =MDDI_DEFAULT_PRIM_PIX_ATTR;
   pinfo.wait_cycle = 0;
   pinfo.bpp = 24;//jsLEE
   // vsync config
   pinfo.lcd.vsync_enable = TRUE;
   pinfo.lcd.refx100 = _fps;//(mddi_r61529_rows_per_second * 100) /mddi_r61529_rows_per_refresh;

/* LGE_CHANGE.
* Change proch values to resolve LCD Tearing. Before BP:14, FP:6. After BP=FP=6.
* The set values on LCD are both 8, but we use 6 for MDDI in order to secure timing margin.
* 2010-08-21, minjong.gong@lge.com
*/

   //jsLEE
   //lcd tunning point wirh pinfo->bpp
   pinfo.lcd.v_back_porch = v_back_porch;//15;//6;
   pinfo.lcd.v_front_porch = v_front_porch;//6;
   pinfo.lcd.v_pulse_width = v_pulse_width;//0;//4;

   pinfo.lcd.hw_vsync_mode = TRUE;
   pinfo.lcd.vsync_notifier_period = (1 * HZ);

   pinfo.bl_max = 4;
   pinfo.bl_min = 1;

	pinfo.clk_rate = _clk_rate;//192000000;//jsLEE
	pinfo.clk_min = 190000000;
	pinfo.clk_max = _clk_rate;//200000000;

// pinfo->clk_rate = 10000000;
// pinfo->clk_min = 9000000;
// pinfo->clk_max = 11000000;
   pinfo.fb_num = 2;

   ret = mddi_r61529_device_register(&pinfo, 0, 1);

	if (ret)
		printk(KERN_ERR "%s: failed to register device!\n", __func__);

   return ret;
}
module_init(mddi_r61529_wqvga_pt_init);
module_init(mddi_r61529_lcd_init);
