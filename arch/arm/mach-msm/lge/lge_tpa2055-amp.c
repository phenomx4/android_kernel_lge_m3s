/* arch/arm/mach-msm/qdsp5v2/lge_tpa2055-amp.c
 *
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
 *
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <asm/gpio.h>
#include <asm/system.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <asm/ioctls.h>
#include <mach/debug_mm.h>
#include <linux/slab.h>
#include "mach/qdsp5v2/lge_tpa2055-amp.h"

#define MODULE_NAME	"lge_tpa2055"



//speaker=headset = hph lr
char amp_cal_data[AMP_CAL_MAX] = {IN1GAIN_0DB, HPL_VOL_0DB,\
                                                                                    IN1GAIN_0DB, SPK_VOL_0DB,\
                                                                                    IN1GAIN_0DB, HPL_VOL_0DB,\
                                                                                    IN1GAIN_6DB, HPL_VOL_M3DB,\
                                                                                    IN1GAIN_6DB, HPL_VOL_M3DB, SPK_VOL_0DB,\
                                                                                    IN1GAIN_6DB, SPK_VOL_0DB};

//speaker= line out, headset=hph lr 
char amp_cal_lodata[AMP_CAL_MAX] = {IN1GAIN_0DB, HPL_VOL_0DB,\
                                                                                    IN2GAIN_0DB, SPK_VOL_0DB,\
                                                                                    IN1GAIN_0DB, HPL_VOL_M10DB,\
                                                                                    IN1GAIN_0DB, HPL_VOL_M6DB,\
                                                                                    IN1GAIN_6DB, HPL_VOL_M21DB, SPK_VOL_0DB,\
                                                                                    IN2GAIN_6DB, SPK_VOL_M1DB};


char *pAmpCalData;
char amp_cal_pos;
char amp_i2c_reg; 

/* BEGIN:0009753        ehgrace.kim@lge.com     2010.10.22*/
/* MOD: modifiy to delete the first boot noise */
bool first_boot = 1;
/* END:0009753        ehgrace.kim@lge.com     2010.10.22*/
/* END:0009748        ehgrace.kim@lge.com     2010.10.07*/

static uint32_t msm_snd_debug = 1;
module_param_named(debug_mask, msm_snd_debug, uint, 0664);


struct amp_data {
	struct i2c_client *client;
};

static struct amp_data *_data;
static struct amp_platform_data *amp_pdata;

int ReadI2C(char reg, char *ret)
{

	unsigned int err;
	unsigned char buf = reg;

	struct i2c_msg msg[2] = {
		{ _data->client->addr, 0, 1, &buf },
		{ _data->client->addr, I2C_M_RD, 1, ret}
	};

	err = i2c_transfer(_data->client->adapter, msg, 2);
	if (err < 0)
		dev_err(&_data->client->dev, "i2c read error\n");

	return 0;

}

int WriteI2C(char reg, char val)
{
	int	err;
	unsigned char    buf[2];
	struct i2c_msg	msg = { _data->client->addr, 0, 2, buf };

	buf[0] = reg;
	buf[1] = val;

	err = i2c_transfer(_data->client->adapter, &msg, 1);
	if (err < 0)
		return -EIO;
	else
		return 0;
}


void set_amp_PowerDown(void)
{
	int fail=0;

#if 0
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (BYPASS | SWS));
#else


	//fail |= WriteI2C(INPUT_CONTROL, (IN2GAIN_0DB | IN1GAIN_0DB | IN2_SE | IN1_SE));	
	
	fail |= WriteI2C(HP_LEFT_VOLUME, HPL_EN | HP_TRACK | HPL_VOL_M60DB);	//Turn volume down to -60dB
	fail |= WriteI2C(HP_LEFT_VOLUME, HPL_VOL_M60DB);			//Disable HPL
	fail |= WriteI2C(HP_RIGHT_VOLUME, HPR_VOL_M60DB);			//Disable HPR
	fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_MUTE);			//Mute the input stages

	fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | SPK_VOL_M60DB)); 
	
	fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_MUTE);	
	//fail |= WriteI2C(LIMITER_CONTROL, (ATTACK_40P96MS | RELEASE_451MS));

	//usleep(100*1000);
	
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (BYPASS | SWS  ));




	//fail |= WriteI2C(LIMITER_CONTROL, (ATTACK_40P96MS | RELEASE_1271MS));

	//fail |= WriteI2C(HP_LEFT_VOLUME, (HPL_VOL_M60DB | HPL_EN | HP_TRACK));
	//fail |= WriteI2C(HP_RIGHT_VOLUME, (HPR_VOL_M60DB | HPR_EN));
	//fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | SPK_VOL_M60DB)); 
	//fail |= WriteI2C(SPEAKER_VOLUME, ~SPK_EN & SPK_VOL_M60DB);
	//fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_MUTE);
	//fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_MUTE);	
	//fail |= WriteI2C(LIMITER_CONTROL, (ATTACK_40P96MS | RELEASE_1271MS));
	//fail |= WriteI2C(SPEAKER_OUTPUT, (SLIMLVL_4P90V | SPKOUT_MUTE));					//~SPLIM_EN
	//fail |= WriteI2C(HEADPHONE_OUTPUT, (HLIMLVL_1P15V | HPOUT_MUTE));					//~HPLIM_EN

	//fail |= WriteI2C(SUBSYSTEM_CONTROL, (BYPASS | SWS | SSM_EN ));
	//fail |= WriteI2C(SUBSYSTEM_CONTROL, BYPASS);
//	fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_MUTE);
//	fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_MUTE);	
#endif



	printk(KERN_INFO "1 set_amp_PowerDown() %d\n", fail);
}
EXPORT_SYMBOL(set_amp_PowerDown);

void set_amp_earpiece_voice(void)
{
	int fail=0;
#if 0
	if(!amp_pdata)
		return;
	if(!amp_pdata->bypass_mode)
		return;
	if(amp_pdata->external_bypass){
		amp_pdata->external_bypass(1);
	}
	else{
		/*Input3 bypass to speaker*/
		fail |= WriteI2C(SUBSYSTEM_CONTROL, (BYPASS));		
	}
#else
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (BYPASS | SWS));		
#endif
    printk(KERN_INFO "2 set_amp_earpiece_voice()\n");
}

void amp_external_switch_off(void)
{
	if(amp_pdata && amp_pdata->external_bypass){
		amp_pdata->external_bypass(0);
	}				
}

EXPORT_SYMBOL(set_amp_earpiece_voice);

void set_amp_headset_mono_voice(void)
{
	int fail=0;
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (~SWS & ~BYPASS & ~SSM_EN));
	fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[HEADSET_VOICE_INPUT] | IN1_SE)); 				//Modify for desired IN gain  //IN1_DIFF ->IN1_SE
	fail |= WriteI2C(HP_LEFT_VOLUME, (HPL_VOL_M60DB | HPL_EN | HP_TRACK));
	fail |= WriteI2C(HP_RIGHT_VOLUME, (HPR_VOL_M60DB | HPR_EN));
	fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_IN1);
	fail |= WriteI2C(HP_LEFT_VOLUME, (pAmpCalData[HEADSET_VOICE_OUTPUT]  | HPL_EN | HP_TRACK));			//Modify for desired HP gain

	fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_MUTE);
    printk(KERN_INFO "2 set_amp_headset_mono_voice() %d\n", fail);
}
EXPORT_SYMBOL(set_amp_headset_mono_voice);

void set_amp_speaker_stereo_voice(void)
{
	int fail=0;
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (~SWS & ~BYPASS & ~SSM_EN));
	if(amp_pdata->line_out)
	{
		fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[SPEAKER_VOICE_INPUT]	| IN2_SE)); 							//Modify for desired IN gain 
		fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | pAmpCalData[SPEAKER_VOICE_OUTPUT] ));							//Modify for desired SP gain
		fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_IN2);
	}
	else
	{
		fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[SPEAKER_VOICE_INPUT] | IN1_SE));							//Modify for desired IN gain 
		fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | pAmpCalData[SPEAKER_VOICE_OUTPUT] ));							//Modify for desired SP gain
		fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_IN1);
	}
	//amp_external_switch_off();
	
    printk(KERN_INFO "3 set_amp_speaker_stereo_voice() %d\n", fail);
}
EXPORT_SYMBOL(set_amp_speaker_stereo_voice);

void set_amp_tty(void)
{
	int fail=0;
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (~SWS & ~BYPASS & ~SSM_EN));
	fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[TTY_INPUT]  | IN1_DIFF)); 				//Modify for desired IN gain 
	fail |= WriteI2C(HP_LEFT_VOLUME, (HPL_VOL_M60DB | HPL_EN | HP_TRACK));
	fail |= WriteI2C(HP_RIGHT_VOLUME, (HPR_VOL_M60DB | HPR_EN));
	fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_IN1);
	fail |= WriteI2C(HP_LEFT_VOLUME, (pAmpCalData[TTY_OUTPUT]  | HPL_EN | HP_TRACK));			//Modify for desired HP gain

	fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_MUTE);

    printk(KERN_INFO "4 set_amp_tty() %d\n", fail);
}
EXPORT_SYMBOL(set_amp_tty);

void set_amp_headset_stereo_audio(void)
{
	int fail=0;
#if 0
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (~SWS & ~BYPASS & ~SSM_EN));
	fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[HEADSET_AUDIO_INPUT]  | IN1_SE)); 				//Modify for desired IN gain 
	fail |= WriteI2C(HP_LEFT_VOLUME, (HPL_VOL_M60DB | HPL_EN | HP_TRACK));
	fail |= WriteI2C(HP_RIGHT_VOLUME, (HPR_VOL_M60DB | HPR_EN));
	fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_IN1);
	fail |= WriteI2C(HP_LEFT_VOLUME, (pAmpCalData[HEADSET_AUDIO_OUTPUT]  | HPL_EN | HP_TRACK));			//Modify for desired HP gain

	fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_MUTE);
#else
	//fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_MUTE);
	
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (~SWS & ~BYPASS & ~SSM_EN));
	//	usleep(300*1000);	// 50ms
	
	fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[HEADSET_AUDIO_INPUT]  | IN1_SE));				//Modify for desired IN gain 
	fail |= WriteI2C(HP_LEFT_VOLUME, (HPL_VOL_M60DB | HPL_EN | HP_TRACK));
	fail |= WriteI2C(HP_RIGHT_VOLUME, (HPR_VOL_M60DB | HPR_EN));

	//fail |= WriteI2C(SUBSYSTEM_CONTROL, (~SWS & ~BYPASS & ~SSM_EN));
		
	fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_IN1);
	fail |= WriteI2C(HP_LEFT_VOLUME, (pAmpCalData[HEADSET_AUDIO_OUTPUT]  | HPL_EN | HP_TRACK)); 		//Modify for desired HP gain
	fail |= WriteI2C(HP_RIGHT_VOLUME, (pAmpCalData[HEADSET_AUDIO_OUTPUT]  | HPR_EN)); 		//Modify for desired HP gain

	fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_MUTE);	
#endif
    printk(KERN_INFO "5 set_amp_headset_stereo_audio() %d\n", fail);
}
EXPORT_SYMBOL(set_amp_headset_stereo_audio);

void set_amp_headset_speaker_audio(void)
{
	int fail=0;
#if 0
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (~SWS & ~BYPASS & ~SSM_EN));
	fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[HEADSET_SPEAKER_INPUT]  | IN1_SE)); 				//Modify for desired IN gain 
	fail |= WriteI2C(HP_LEFT_VOLUME, (HPL_VOL_M60DB | HPL_EN | HP_TRACK));
	fail |= WriteI2C(HP_RIGHT_VOLUME, (HPR_VOL_M60DB | HPR_EN));
	fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_IN1);
	fail |= WriteI2C(HP_LEFT_VOLUME, (pAmpCalData[HEADSET_SPEAKER_OUTPUT_HEADSET]  | HPL_EN | HP_TRACK));			//Modify for desired HP gain

	fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | pAmpCalData[HEADSET_SPEAKER_OUTPUT_SPEAKER]));							//Modify for desired SP gain
	fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_IN1);
#else
	//fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_MUTE);
	//fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_MUTE);	
	
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (~SWS & ~BYPASS & ~SSM_EN));
	//usleep(300*1000);	// 50ms

	fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[HEADSET_SPEAKER_INPUT]  | IN1_SE));	

				//Modify for desired IN gain 
	fail |= WriteI2C(HP_LEFT_VOLUME, (HPL_VOL_M60DB | HPL_EN | HP_TRACK));
	fail |= WriteI2C(HP_RIGHT_VOLUME, (HPR_VOL_M60DB | HPR_EN));

	fail |= WriteI2C(HP_LEFT_VOLUME, (pAmpCalData[HEADSET_SPEAKER_OUTPUT_HEADSET]  | HPL_EN | HP_TRACK));			//Modify for desired HP gain
	fail |= WriteI2C(HP_RIGHT_VOLUME, (pAmpCalData[HEADSET_SPEAKER_OUTPUT_HEADSET]  | HPR_EN ));
	fail |= WriteI2C(HEADPHONE_OUTPUT, HPOUT_IN1);

	fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | SPK_VOL_M60DB)); 
	fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | pAmpCalData[HEADSET_SPEAKER_OUTPUT_SPEAKER]));							//Modify for desired SP gain
	fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_IN1);

#endif
	
	//amp_external_switch_off();


    printk(KERN_INFO " 6 set_amp_headset_speaker_audio() %d\n", fail);
}
EXPORT_SYMBOL(set_amp_headset_speaker_audio);

void set_amp_speaker_stereo_audio(void)
{
	int fail=0;
	
	//fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_MUTE);
	
	fail |= WriteI2C(SUBSYSTEM_CONTROL, (~SWS & ~BYPASS & ~SSM_EN));
	//usleep(300*1000);	// 50ms
	if(amp_pdata->line_out)
	{
#if 0
		fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[SPEAKER_AUDIO_INPUT] | IN2_SE));							//Modify for desired IN gain 
		fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | pAmpCalData[SPEAKER_AUDIO_OUTPUT])); 						//Modify for desired SP gain
		fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_IN2);
#else
	
		fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[SPEAKER_AUDIO_INPUT] | IN2_DIFF));							//Modify for desired IN gain 
		fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | SPK_VOL_M60DB)); 						//Modify for desired SP gain
		fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_IN2);
		fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | pAmpCalData[SPEAKER_AUDIO_OUTPUT])); 

#endif
	}
	else
	{
		fail |= WriteI2C(INPUT_CONTROL, (pAmpCalData[SPEAKER_AUDIO_INPUT] | IN1_SE));							//Modify for desired IN gain 
		fail |= WriteI2C(SPEAKER_VOLUME, (SPK_EN | pAmpCalData[SPEAKER_AUDIO_OUTPUT] ));							//Modify for desired SP gain
		fail |= WriteI2C(SPEAKER_OUTPUT, SPKOUT_IN1);
	}
	//amp_external_switch_off();
	
    printk(KERN_INFO "7 set_amp_speaker_stereo_audio() %d\n", fail);
}
EXPORT_SYMBOL(set_amp_speaker_stereo_audio);


static ssize_t amp_gain_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(amp_cal_pos > AMP_CAL_MAX)
		amp_cal_pos = 0;

	return sprintf(buf, "%d\n", (int)pAmpCalData[(int)amp_cal_pos]);
}
static ssize_t amp_gain_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{

	int gain;

	if(amp_cal_pos > AMP_CAL_MAX)
		return -EINVAL;
	sscanf(buf, "%d", &gain);
	pAmpCalData[(int)amp_cal_pos] =  (char)gain;

	return size;
}
static DEVICE_ATTR(gain, S_IRUGO | S_IWUSR, amp_gain_show, amp_gain_store);

static ssize_t amp_pos_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", amp_cal_pos);
}
static ssize_t amp_pos_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int pos;

	sscanf(buf, "%d", &pos);
	if(pos < 0)pos =0;
	if(pos > AMP_CAL_MAX)pos-=1;
	amp_cal_pos =  pos;

	return size;
}
static DEVICE_ATTR(pos, S_IRUGO | S_IWUSR, amp_pos_show, amp_pos_store);

static ssize_t amp_i2cReg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", amp_i2c_reg);
}
static ssize_t amp_i2cReg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int reg;

	sscanf(buf, "%d", &reg);
	amp_i2c_reg =  reg;

	return size;
}
static DEVICE_ATTR(i2cReg, S_IRUGO | S_IWUSR, amp_i2cReg_show, amp_i2cReg_store);

static ssize_t amp_i2cData_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char ret;
	int err;

	err = ReadI2C(amp_i2c_reg,&ret);
	
	return sprintf(buf, "%d (0x%X)\n", ret,ret);
}
static ssize_t amp_i2cData_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int data;
	int err;

	sscanf(buf, "%d", &data);
	
	err = WriteI2C(amp_i2c_reg,data);

	return size;
}
static DEVICE_ATTR(i2cData, S_IRUGO | S_IWUSR, amp_i2cData_show, amp_i2cData_store);

static int flip_amp_ctl_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct amp_data *data;
	struct i2c_adapter* adapter = client->adapter;
	int err;

	amp_pdata = client->dev.platform_data;
	
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA)){
		err = -EOPNOTSUPP;
		return err;
	}

	if (msm_snd_debug & 1)
		printk(KERN_INFO "%s()\n", __FUNCTION__);
	
	data = kzalloc(sizeof (struct amp_data), GFP_KERNEL);
	if (NULL == data) {
			return -ENOMEM;
	}
	_data = data;
	data->client = client;
	i2c_set_clientdata(client, data);

	if(amp_pdata->line_out)
        pAmpCalData = &amp_cal_lodata[ 0 ];
	else		
		pAmpCalData = &amp_cal_data[ 0 ];
		
	if (msm_snd_debug & 1)
		printk(KERN_INFO "%s chip found\n", client->name);

	if(amp_pdata && amp_pdata->external_bypass_init){
		amp_pdata->external_bypass_init();
	}
	
	err = device_create_file(&client->dev, &dev_attr_pos);
	err = device_create_file(&client->dev, &dev_attr_gain);
	err = device_create_file(&client->dev, &dev_attr_i2cReg);	
	err = device_create_file(&client->dev, &dev_attr_i2cData);

	set_amp_PowerDown();
	return 0;
}

static int flip_amp_ctl_remove(struct i2c_client *client)
{
	struct amp_data *data = i2c_get_clientdata(client);
	kfree (data);
	
	printk(KERN_INFO "%s()\n", __FUNCTION__);
	i2c_set_clientdata(client, NULL);
	return 0;
}


static struct i2c_device_id flip_amp_idtable[] = {
	{ "lge_tpa2055", 1 },
};

static struct i2c_driver flip_amp_ctl_driver = {
	.probe = flip_amp_ctl_probe,
	.remove = flip_amp_ctl_remove,
	.id_table = flip_amp_idtable,
	.driver = {
		.name = MODULE_NAME,
	},
};


static int __init Tpa2055_amp_ctl_init(void)
{
	return i2c_add_driver(&flip_amp_ctl_driver);	
}

static void __exit Tpa2055_amp_ctl_exit(void)
{
	return i2c_del_driver(&flip_amp_ctl_driver);
}

module_init(Tpa2055_amp_ctl_init);
module_exit(Tpa2055_amp_ctl_exit);

MODULE_DESCRIPTION("Bryce Amp Control");
MODULE_AUTHOR("Kim EunHye <ehgrace.kim@lge.com>");
MODULE_LICENSE("GPL");
