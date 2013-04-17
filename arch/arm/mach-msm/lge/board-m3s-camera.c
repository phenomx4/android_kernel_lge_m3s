/* arch/arm/mach-msm/board-flip-camera.c
 * Copyright (C) 2010 LGE, Inc.
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

////////////////////////////////////////////////////////////////////////////////////
// Author: JongKwon.Chae (jongkwon.chae@lge.com)
// Date: 2011.08.25
////////////////////////////////////////////////////////////////////////////////////

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/platform_device.h>
#include <asm/setup.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/vreg.h>
#include <mach/camera.h>
#include <mach/board.h>

#include "devices.h"
#include <mach/board_lge.h>

int main_camera_power_off (void);
int main_camera_power_on (void);
//Start tao.jin@lge.com LG_FW_CAMERA_CODE added for camcorder current issue 2011-12-23
void camera_onoff_inform(int);
//End tao.jin@lge.com LG_FW_CAMERA_CODE added for camcorder current issue 2011-12-23


//====================================================================================
//                              RESET/PWDN
//====================================================================================
#ifdef CONFIG_S5K4E1
	#define CAM_MAIN_I2C_SLAVE_ADDR         (0x20 >> 1)     
	#define CAM_AF_I2C_SLAVE_ADDR           (0x18 >> 2)     
#elif defined (CONFIG_MT9P017)
	#define CAM_MAIN_I2C_SLAVE_ADDR         (0x36)     
#endif

#define CAM_MAIN_GPIO_RESET_N               (0)

//====================================================================================
//                              MSM VPE Device
//====================================================================================
#ifdef CONFIG_MSM_VPE
static struct resource msm_vpe_resources[] = {
	{
		.start	= 0xAD200000,
		.end	= 0xAD200000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VPE,
		.end	= INT_VPE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_vpe_device = {
       .name = "msm_vpe",
       .id   = 0,
       .num_resources = ARRAY_SIZE(msm_vpe_resources),
       .resource = msm_vpe_resources,
};
#endif

//====================================================================================
//                              Devices
//====================================================================================
static struct platform_device *m3s_camera_msm_devices[] __initdata = {
#ifdef CONFIG_MSM_VPE
    &msm_vpe_device,
#endif
};


//====================================================================================
//                             LM3559 Flash
//====================================================================================

#ifdef CONFIG_MSM_CAMERA_FLASH_LM3559

/* camera flash gpio */
#define GPIO_FLASH_EN					19

#if (CONFIG_LGE_PCB_REVISION == LGE_REV_A)
#define GPIO_FLASH_I2C_SCL				88
#define GPIO_FLASH_I2C_SDA				89
#elif (CONFIG_LGE_PCB_REVISION == LGE_REV_B)
#define GPIO_FLASH_I2C_SCL				155
#define GPIO_FLASH_I2C_SDA				156
#endif

#define CAM_FLASH_I2C_SLAVE_ADDR		0x53

static struct msm_camera_sensor_flash_src led_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
};

static struct msm_camera_sensor_flash_data led_flash_data = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &led_flash_src,
};
#else
static struct msm_camera_sensor_flash_data led_flash_data = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = NULL,
};
#endif

#ifdef CONFIG_MSM_CAMERA_FLASH_LM3559
/* LM3559 flash led driver */
static struct gpio_i2c_pin flash_i2c_pin[] = {
	{
		.sda_pin	= GPIO_FLASH_I2C_SDA,
		.scl_pin	= GPIO_FLASH_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= 0,
	},
};

static struct i2c_gpio_platform_data flash_i2c_pdata = {
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			    = 2,
};

static struct platform_device flash_i2c_device = {
	.name	= "i2c-gpio",
	.dev.platform_data = &flash_i2c_pdata,
};

static struct led_flash_platform_data lm3559_flash_pdata = {
	.gpio_flen = GPIO_FLASH_EN,
};

static struct i2c_board_info i2c_camera_flash_devices[] = {
	{
		I2C_BOARD_INFO("lm3559", CAM_FLASH_I2C_SLAVE_ADDR),
		.platform_data = &lm3559_flash_pdata,
	},
};
#endif



//====================================================================================
//                              Camera Sensor (S5K4E1)
//====================================================================================
static struct i2c_board_info camera_i2c_devices[] = {
	#ifdef CONFIG_S5K4E1
	{
		I2C_BOARD_INFO("s5k4e1", CAM_MAIN_I2C_SLAVE_ADDR),
	},
	{
		I2C_BOARD_INFO("s5k4e1_af", CAM_AF_I2C_SLAVE_ADDR),
	},
	#elif defined (CONFIG_MT9P017)
	{
		I2C_BOARD_INFO("mt9p017", CAM_MAIN_I2C_SLAVE_ADDR),
	},
	#endif
};

#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_gpio_table[] = {
        GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* MCLK */
};

static uint32_t camera_on_gpio_table[] = {
        GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_6MA), /* MCLK */
};

static void config_gpio_table(uint32_t *table, int len)
{
        int n, rc;
        for (n = 0; n < len; n++) {
                rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
                if (rc) {
                        printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
                                __func__, table[n], rc);
                        break;
                }
        }
}

int config_camera_on_gpios(void)
{
    config_gpio_table(camera_on_gpio_table,
            ARRAY_SIZE(camera_on_gpio_table));

	main_camera_power_on();
    return 0;
}

void config_camera_off_gpios(void)
{
        config_gpio_table(camera_off_gpio_table,
                ARRAY_SIZE(camera_off_gpio_table));

	main_camera_power_off();
}

#ifdef CONFIG_S5K4E1
	static struct platform_device msm_camera_sensor_s5k4e1;
#elif defined(CONFIG_MT9P017)
	static struct platform_device msm_camera_sensor_mt9p017;
#endif

int main_camera_power_off (void)
{
    struct vreg *vreg_cam_dvdd_1_8v;
	struct vreg *vreg_cam_iovdd_1_8v;
	struct vreg *vreg_cam_avdd_2_8v;
	struct vreg *vreg_cam_af_2_8v;

	printk(KERN_ERR "%s: main_camera_power_off \n",__func__);

	//[LGE_UPDATE_S] jeonghoon.cho@lge.com 2011.06.22
	gpio_tlmm_config( 
			GPIO_CFG(
				CAM_MAIN_GPIO_RESET_N,
				0, 
				GPIO_CFG_OUTPUT, 
				GPIO_CFG_NO_PULL, 
				GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	//[LGE_UPDATE_E]

	gpio_set_value(CAM_MAIN_GPIO_RESET_N, 0);
	mdelay(1);

#ifdef CONFIG_MT9P017
	//[LGE_UPDATE_S] jeonghoon.cho@lge.com 2011.06.22
	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
	//  msm_camio_sensor_clk_off(&msm_camera_sensor_mt9p017);
	//[LGE_UPDATE_E]
#endif

	mdelay(1);

	vreg_cam_af_2_8v = vreg_get(NULL, "gp6");
	vreg_disable(vreg_cam_af_2_8v);

	vreg_cam_avdd_2_8v = vreg_get(NULL, "gp11");
	vreg_disable(vreg_cam_avdd_2_8v);

	vreg_cam_dvdd_1_8v = vreg_get(NULL, "gp13");
	vreg_disable(vreg_cam_dvdd_1_8v);

	vreg_cam_iovdd_1_8v = vreg_get(NULL, "gp2");
	vreg_disable(vreg_cam_iovdd_1_8v);

//Start tao.jin@lge.com LG_FW_CAMERA_CODE added for camcorder current issue 2011-12-23
	camera_onoff_inform(0);
//End tao.jin@lge.com LG_FW_CAMERA_CODE added for camcorder current issue 2011-12-23

	return 0;
}

int main_camera_power_on (void)
{
    int rc;

	struct vreg *vreg_cam_dvdd_1_8v;
	struct vreg *vreg_cam_iovdd_1_8v;
	struct vreg *vreg_cam_avdd_2_8v;
	struct vreg *vreg_cam_af_2_8v;

	printk(KERN_ERR "%s: main_camera_power_on \n",__func__);

	//[LGE_UPDATE_S] jeonghoon.cho@lge.com 2011.06.22
	gpio_tlmm_config( 
			GPIO_CFG(CAM_MAIN_GPIO_RESET_N, 
				0, 
				GPIO_CFG_OUTPUT, 
				GPIO_CFG_NO_PULL, 
				GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	//[LGE_UPDATE_E]

	gpio_set_value(CAM_MAIN_GPIO_RESET_N, 0);
	mdelay(10);

	vreg_cam_dvdd_1_8v = vreg_get(NULL, "gp13");
	rc = vreg_set_level(vreg_cam_dvdd_1_8v, 1800);
	vreg_enable(vreg_cam_dvdd_1_8v);  

	vreg_cam_avdd_2_8v = vreg_get(NULL, "gp11");
	rc = vreg_set_level(vreg_cam_avdd_2_8v, 2800);
	vreg_enable(vreg_cam_avdd_2_8v);

	vreg_cam_iovdd_1_8v = vreg_get(NULL, "gp2");
	rc = vreg_set_level(vreg_cam_iovdd_1_8v, 1800);
	vreg_enable(vreg_cam_iovdd_1_8v);  

	vreg_cam_af_2_8v = vreg_get(NULL, "gp6");
	rc = vreg_set_level(vreg_cam_af_2_8v, 2800);
	vreg_enable(vreg_cam_af_2_8v);


	/* Input MCLK = 24MHz */
	//    mdelay(300);
	mdelay(10);

#ifdef CONFIG_MT9P017
	//[LGE_UPDATE_S] jeonghoon.cho@lge.com 2011.06.22
	//  msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);

	// msm_camio_clk_rate_set(24000000);    
	mdelay(10);

	// msm_camio_sensor_clk_on(&msm_camera_sensor_mt9p017);
	msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
	//[LGE_UPDATE_E]
#endif

	mdelay(10);

	//[LGE_UPDATE_S] jeonghoon.cho@lge.com 2011.06.22
	gpio_tlmm_config( 
			GPIO_CFG(CAM_MAIN_GPIO_RESET_N, 
				0, 
				GPIO_CFG_OUTPUT, 
				GPIO_CFG_NO_PULL, 
				GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	//[LGE_UPDATE_E]

	gpio_set_value(CAM_MAIN_GPIO_RESET_N, 1);
	mdelay(10);
//Start tao.jin@lge.com LG_FW_CAMERA_CODE added for camcorder current issue 2011-12-23
	camera_onoff_inform(1);
//End tao.jin@lge.com LG_FW_CAMERA_CODE added for camcorder current issue 2011-12-23
	return 0;
}

struct resource msm_camera_resources[] = {
        {
                .start  = 0xA6000000,
                .end    = 0xA6000000 + SZ_1M - 1,
                .flags  = IORESOURCE_MEM,
        },
        {
                .start  = INT_VFE,
                .end    = INT_VFE,
                .flags  = IORESOURCE_IRQ,
        },
};

static struct msm_camera_device_platform_data msm_main_camera_device_data = {
#ifdef CONFIG_MT9P017
        .camera_power_on   = main_camera_power_on,
        .camera_power_off  = main_camera_power_off,
#endif
        .camera_gpio_on    = config_camera_on_gpios,
        .camera_gpio_off   = config_camera_off_gpios,
        .ioext.camifpadphy = 0xAB000000,
        .ioext.camifpadsz  = 0x00000400,
        .ioext.csiphy      = 0xA6100000,
        .ioext.csisz       = 0x00000400,
        .ioext.csiirq      = INT_CSI,
        .ioclk.mclk_clk_rate = 24000000,
        .ioclk.vfe_clk_rate  = 153600000,       //122880000,            //QCT에서 변경 122880000->153600000  : CONFIG_MT9P017용
};

#ifdef CONFIG_S5K4E1
static struct msm_camera_sensor_platform_info s5k4e1_sensor_info = {
	.mount_angle = 0
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k4e1_data = {
        .sensor_name          = "s5k4e1",
        .sensor_reset         = CAM_MAIN_GPIO_RESET_N,
        .sensor_pwd           = CAM_MAIN_GPIO_RESET_N,
        .vcm_pwd              = 1,
        .vcm_enable           = 1,
        .pdata                = &msm_main_camera_device_data,
        .resource             = msm_camera_resources,
        .num_resources        = ARRAY_SIZE(msm_camera_resources),
        .flash_data           = &led_flash_data,
		.sensor_platform_info = &s5k4e1_sensor_info,
        .csi_if               = 1
};

static struct platform_device msm_camera_sensor_s5k4e1 = {
        .name      = "msm_camera_s5k4e1",
        .dev       = {
                .platform_data = &msm_camera_sensor_s5k4e1_data,
        },
};
#endif

#ifdef CONFIG_MT9P017
static struct msm_camera_sensor_info msm_camera_sensor_mt9p017_data = {
        .sensor_name    = "mt9p017",
        .sensor_reset   = CAM_MAIN_GPIO_RESET_N,
        .sensor_pwd     = CAM_MAIN_GPIO_RESET_N,
        .vcm_pwd        = 1,
        .vcm_enable     = 1,
        .pdata          = &msm_main_camera_device_data,
        .resource       = msm_camera_resources,
        .num_resources  = ARRAY_SIZE(msm_camera_resources),
        .flash_data     = &flash_mt9p017,
        .csi_if         = 1
};

static struct platform_device msm_camera_sensor_mt9p017 = {
        .name      = "msm_camera_mt9p017",
        .dev       = {
                .platform_data = &msm_camera_sensor_mt9p017_data,
        },
};
#endif

#endif // CONFIG_MSM_CAMERA

static struct platform_device *m3s_camera_devices[] __initdata = {
#ifdef CONFIG_S5K4E1
    &msm_camera_sensor_s5k4e1,
#elif defined CONFIG_MT9P017
    &msm_camera_sensor_mt9p017,
#endif
};

#ifdef CONFIG_MSM_CAMERA_FLASH_LM3559
static void __init m3s_init_i2c_camera(int bus_num, bool platform)
{
	flash_i2c_device.id = bus_num;

	init_gpio_i2c_pin(&flash_i2c_pdata, flash_i2c_pin[0],
		&i2c_camera_flash_devices[0]);
	printk(KERN_ERR "init_gpio_i2c_pin [Flash]\n");
	i2c_register_board_info(bus_num, i2c_camera_flash_devices,
		ARRAY_SIZE(i2c_camera_flash_devices));
	printk(KERN_ERR "i2c_register_board_info [Flash]\n");
	if(platform)
		platform_device_register(&flash_i2c_device);
	printk(KERN_ERR "platform_device_register [Flash]\n");
}
#endif

void __init lge_add_camera_devices(void)
{
    int rc;
    // Initialize Main Camera Ctrl GPIO
    rc = gpio_tlmm_config( 
			GPIO_CFG(CAM_MAIN_GPIO_RESET_N, 
				0, 
				GPIO_CFG_OUTPUT, 
				GPIO_CFG_NO_PULL, 
				GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
                            
    printk(KERN_ERR "Set Main Camera GPIO : %d\n", rc);

    i2c_register_board_info(4 /* QUP ID */, camera_i2c_devices, ARRAY_SIZE(camera_i2c_devices));
    printk(KERN_ERR "i2c_register_board_info : %d\n", ARRAY_SIZE(camera_i2c_devices));
    
    platform_add_devices(m3s_camera_msm_devices, ARRAY_SIZE(m3s_camera_msm_devices));
    printk(KERN_ERR "platform_add_devices(camera_msm) : %d\n", ARRAY_SIZE(m3s_camera_msm_devices));

    platform_add_devices(m3s_camera_devices, ARRAY_SIZE(m3s_camera_devices));
    printk(KERN_ERR "platform_add_devices(camera) : %d\n", ARRAY_SIZE(m3s_camera_devices));
#ifdef CONFIG_MSM_CAMERA_FLASH_LM3559
	lge_add_gpio_i2c_device(m3s_init_i2c_camera,&flash_i2c_pin[0]);
#endif

}

