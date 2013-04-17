/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/mfd/msm-adie-codec.h>
#include <linux/uaccess.h>
#include <asm/mach-types.h>
#include <mach/qdsp5v2/aux_pcm.h>
#include <mach/qdsp5v2/snddev_ecodec.h>
#include <mach/board.h>
#include <mach/board_lge.h>
#include <mach/qdsp5v2/snddev_icodec.h>
#include <mach/qdsp5v2/snddev_mi2s.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/audio_acdb_def.h>
#include <mach/qdsp5v2/snddev_virtual.h>
#include "timpani_profile_7x30.h"
#include <mach/qdsp5v2/audio_dev_ctl.h>
#include <mach/qdsp5v2/lge_tpa2055-amp.h>

/* define the value for BT_SCO */
#define BT_SCO_PCM_CTL_VAL (PCM_CTL__RPCM_WIDTH__LINEAR_V |\
		PCM_CTL__TPCM_WIDTH__LINEAR_V)
#define BT_SCO_DATA_FORMAT_PADDING (DATA_FORMAT_PADDING_INFO__RPCM_FORMAT_V |\
		DATA_FORMAT_PADDING_INFO__TPCM_FORMAT_V)
#define BT_SCO_AUX_CODEC_INTF   AUX_CODEC_INTF_CTL__PCMINTF_DATA_EN_V

#define CHANNEL_MODE_MONO 1
#define CHANNEL_MODE_STEREO 2

#if 0

/* handset rx - voice*/
static struct adie_codec_action_unit iearpiece_ffa_48KHz_osr256_actions[] =
	EAR_PRI_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry iearpiece_ffa_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iearpiece_ffa_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(iearpiece_ffa_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile iearpiece_ffa_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = iearpiece_ffa_settings,
	.setting_sz = ARRAY_SIZE(iearpiece_ffa_settings),
};

static struct snddev_icodec_data snddev_iearpiece_ffa_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_SPKR,
	.profile = &iearpiece_ffa_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_earpiece_voice,
	.pamp_off = set_amp_PowerDown,
	.property = SIDE_TONE_MASK,
	#if 0
	.max_voice_rx_vol[VOC_NB_INDEX] = -200,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1700,
	.max_voice_rx_vol[VOC_WB_INDEX] = -200,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1700,
	#else
	.max_voice_rx_vol[VOC_NB_INDEX] = 1000,
	.min_voice_rx_vol[VOC_NB_INDEX] = -500,
	.max_voice_rx_vol[VOC_WB_INDEX] = 1000,
	.min_voice_rx_vol[VOC_WB_INDEX] = -500,
	#endif
};

static struct platform_device msm_iearpiece_ffa_device = {
	.name = "snddev_icodec",
	.id = 19, // Marimba Last ID +1
	.dev = { .platform_data = &snddev_iearpiece_ffa_data },
};

/* handset tx - voice*/
static struct adie_codec_action_unit imic_ffa_48KHz_osr256_actions[] =
	AMIC_PRI_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry imic_ffa_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_ffa_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_ffa_48KHz_osr256_actions),
	}
};

static enum hsed_controller imic_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct adie_codec_dev_profile imic_ffa_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_ffa_settings,
	.setting_sz = ARRAY_SIZE(imic_ffa_settings),
};

static struct snddev_icodec_data snddev_imic_ffa_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_MIC,
	.profile = &imic_ffa_profile,
	.channel_mode = 1,
	.pmctl_id = imic_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(imic_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_imic_ffa_device = {
	.name = "snddev_icodec",
	.id = 20,
	.dev = { .platform_data = &snddev_imic_ffa_data },
};

/* Speaker Stereo Rx - voice*/
static struct adie_codec_action_unit ispkr_stereo_48KHz_osr256_actions[] =
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_0)
	SPEAKER_INCALL_PRI_STEREO_48000_OSR_256;
#else
	SPEAKER_INCALL_LINOUT_PRI_STEREO_48000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry ispkr_stereo_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispkr_stereo_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispkr_stereo_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ispkr_stereo_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ispkr_stereo_settings,
	.setting_sz = ARRAY_SIZE(ispkr_stereo_settings),
};

static struct snddev_icodec_data snddev_ispkr_stereo_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_incall_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_STEREO,
	.profile = &ispkr_stereo_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_speaker_stereo_voice,
	.pamp_off = set_amp_PowerDown,
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_0)
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,	
#endif

#if 0
	.max_voice_rx_vol[VOC_NB_INDEX] = 500,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1500,
	.max_voice_rx_vol[VOC_WB_INDEX] = 500,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1500
#else
	.max_voice_rx_vol[VOC_NB_INDEX] = 800,
	.min_voice_rx_vol[VOC_NB_INDEX] =  -700,
	.max_voice_rx_vol[VOC_WB_INDEX] = 800,
	.min_voice_rx_vol[VOC_WB_INDEX] = -700
#endif
};

static struct platform_device msm_ispkr_stereo_device = {
	.name = "snddev_icodec",
	.id = 8,
	.dev = { .platform_data = &snddev_ispkr_stereo_data },
};

/* speakerphone tx - voice*/
static struct adie_codec_action_unit imic_speakerphone_48KHz_osr256_actions[] =
	AMIC_PRI_MONO_8000_OSR_256; /* 8000 profile also works for 48k */

static struct adie_codec_hwsetting_entry imic_speakerphone_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_speakerphone_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_speakerphone_48KHz_osr256_actions),
	}
};

//static enum hsed_controller imic_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct adie_codec_dev_profile imic_speakerphone_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_speakerphone_settings,
	.setting_sz = ARRAY_SIZE(imic_speakerphone_settings),
};

static struct snddev_icodec_data snddev_imic_speakerphone_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_incall_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC,
	.profile = &imic_speakerphone_profile,
	.channel_mode = 1,
	.pmctl_id = imic_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(imic_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_imic_speakerphone_device = {
	.name = "snddev_icodec",
	.id = 30,
	.dev = { .platform_data = &snddev_imic_speakerphone_data },
};


/* Speaker Stereo Rx - audio*/
static struct adie_codec_action_unit ispkr_audio_stereo_48KHz_osr256_actions[] =
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_0)
	SPEAKER_AUDIO_PRI_STEREO_48000_OSR_256;
#else
	SPEAKER_AUDIO_LINOUT_PRI_STEREO_48000_OSR_256;
#endif
static struct adie_codec_hwsetting_entry ispkr_audio_stereo_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispkr_audio_stereo_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispkr_audio_stereo_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ispkr_audio_stereo_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ispkr_audio_stereo_settings,
	.setting_sz = ARRAY_SIZE(ispkr_audio_stereo_settings),
};

static struct snddev_icodec_data snddev_ispkr_audio_stereo_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_stereo_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_STEREO,
	.profile = &ispkr_audio_stereo_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_speaker_stereo_audio,
	.pamp_off = set_amp_PowerDown,
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_0)
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,	
#endif
	.max_voice_rx_vol[VOC_NB_INDEX] = 500,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1500,
	.max_voice_rx_vol[VOC_WB_INDEX] = 500,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1500
};

static struct platform_device msm_ispkr_audio_stereo_device = {
	.name = "snddev_icodec",
	.id = 9,
	.dev = { .platform_data = &snddev_ispkr_audio_stereo_data },
};
/* Headset mono Tx - voice*/
static struct adie_codec_action_unit iheadset_mic_tx_osr256_actions[] =
	AMIC1_HEADSET_TX_MONO_PRIMARY_OSR256;

static struct adie_codec_hwsetting_entry iheadset_mic_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iheadset_mic_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(iheadset_mic_tx_osr256_actions),
	}
};

static struct adie_codec_dev_profile iheadset_mic_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = iheadset_mic_tx_settings,
	.setting_sz = ARRAY_SIZE(iheadset_mic_tx_settings),
};

static struct snddev_icodec_data snddev_headset_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_mono_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_MIC,
	.profile = &iheadset_mic_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_headset_mic_device = {
	.name = "snddev_icodec",
	.id = 6,
	.dev = { .platform_data = &snddev_headset_mic_data },
};


/* Headset incall Tx - voice*/
static struct adie_codec_action_unit iheadset_mic_incall_tx_osr256_actions[] =
	AMIC1_HEADSET_TX_MONO_PRIMARY_OSR256;

static struct adie_codec_hwsetting_entry iheadset_mic_incall_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iheadset_mic_incall_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(iheadset_mic_incall_tx_osr256_actions),
	}
};

static struct adie_codec_dev_profile iheadset_mic_incall_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = iheadset_mic_incall_tx_settings,
	.setting_sz = ARRAY_SIZE(iheadset_mic_incall_tx_settings),
};

static struct snddev_icodec_data snddev_headset_mic_incall_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_incall_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_MIC,
	.profile = &iheadset_mic_incall_profile,
	.channel_mode = 1,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_headset_mic_incall_device = {
	.name = "snddev_icodec",
	.id = 31,
	.dev = { .platform_data = &snddev_headset_mic_incall_data },
};


/* FM  Tx - audio*/
static struct snddev_mi2s_data snddev_mi2s_fm_tx_data = {
	.capability = SNDDEV_CAP_TX ,
	.name = "fmradio_stereo_tx",
	.copp_id = 2,
	.acdb_id = ACDB_ID_FM_TX,
	.channel_mode = 2,
	.sd_lines = MI2S_SD_3,
	.route = NULL,
	.deroute = NULL,
	.default_sample_rate = 48000,
};

static struct platform_device  msm_snddev_mi2s_fm_tx_device = {
	.name = "snddev_mi2s",
	.id = 1,
	.dev = { .platform_data = &snddev_mi2s_fm_tx_data},
};

static struct snddev_mi2s_data snddev_mi2s_fm_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "fmradio_stereo_rx",
	.copp_id = 3,
	.acdb_id = ACDB_ID_FM_RX,
	.channel_mode = 2,
	.sd_lines = MI2S_SD_3,
	.route = NULL,
	.deroute = NULL,
	.default_sample_rate = 48000,
};

static struct platform_device  msm_snddev_mi2s_fm_rx_device = {
	.name = "snddev_mi2s",
	.id = 2,
	.dev = { .platform_data = &snddev_mi2s_fm_rx_data},
};

/* BT  Rx - voice*/
static struct snddev_ecodec_data snddev_bt_sco_earpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_rx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_SPKR,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
	.max_voice_rx_vol[VOC_NB_INDEX] = 400,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1100,
	.max_voice_rx_vol[VOC_WB_INDEX] = 400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1100,
};

static struct snddev_ecodec_data snddev_bt_sco_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_tx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_MIC,
	.channel_mode = 1,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
};

static struct platform_device msm_bt_sco_earpiece_device = {
	.name = "msm_snddev_ecodec",
	.id = 0,
	.dev = { .platform_data = &snddev_bt_sco_earpiece_data },
};

static struct platform_device msm_bt_sco_mic_device = {
	.name = "msm_snddev_ecodec",
	.id = 1,
	.dev = { .platform_data = &snddev_bt_sco_mic_data },
};

/* Headset  Stereo Rx - voice*/
static struct adie_codec_action_unit headset_ab_cpls_48KHz_osr256_actions[] =
	HEADSET_AB_CPLS_48000_OSR_256;

static struct adie_codec_hwsetting_entry headset_ab_cpls_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_ab_cpls_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(headset_ab_cpls_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile headset_ab_cpls_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_ab_cpls_settings,
	.setting_sz = ARRAY_SIZE(headset_ab_cpls_settings),
};

static struct snddev_icodec_data snddev_ihs_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_incall_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_SPKR_STEREO,
	.profile = &headset_ab_cpls_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_headset_mono_voice,
	.pamp_off = set_amp_PowerDown,
	.property = 0,//SIDE_TONE_MASK,
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
#if 0
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400,
#else
	.max_voice_rx_vol[VOC_NB_INDEX] = 600,
	.min_voice_rx_vol[VOC_NB_INDEX] = -900,
	.max_voice_rx_vol[VOC_WB_INDEX] = 600,
	.min_voice_rx_vol[VOC_WB_INDEX] = -900,
#endif


};

static struct platform_device msm_headset_stereo_device = {
	.name = "snddev_icodec",
	.id = 2,
	.dev = { .platform_data = &snddev_ihs_stereo_rx_data },
};

/* Headset  Stereo Rx - audio*/
static struct adie_codec_action_unit headset_audio_ab_cpls_48KHz_osr256_actions[] =
	HEADSET_AUDIO_AB_CPLS_48000_OSR_256;

static struct adie_codec_hwsetting_entry headset_audio_ab_cpls_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_audio_ab_cpls_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(headset_audio_ab_cpls_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile headset_audio_ab_cpls_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_audio_ab_cpls_settings,
	.setting_sz = ARRAY_SIZE(headset_audio_ab_cpls_settings),
};

static struct snddev_icodec_data snddev_ihs_stereo_rx_audio_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_SPKR_STEREO,
	.profile = &headset_ab_cpls_profile,
	.channel_mode = 2,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_headset_stereo_audio,
	.pamp_off = set_amp_PowerDown,
	.property = 0,
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400,
};

static struct platform_device msm_headset_stereo_audio_device = {
	.name = "snddev_icodec",
	.id = 3,
	.dev = { .platform_data = &snddev_ihs_stereo_rx_audio_data },
};

static enum hsed_controller ispk_pmctl_id[] = {PM_HSED_CONTROLLER_0};

static struct snddev_icodec_data snddev_ispkr_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC,
	.profile = &imic_ffa_profile,
	.channel_mode = 1,
	.pmctl_id = ispk_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(ispk_pmctl_id),
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device msm_ispkr_mic_device = {
	.name = "snddev_icodec",
	.id = 18,
	.dev = { .platform_data = &snddev_ispkr_mic_data },
};

static struct adie_codec_action_unit idual_mic_endfire_8KHz_osr256_actions[] =
	AMIC_DUAL_8000_OSR_256;

static struct adie_codec_hwsetting_entry idual_mic_endfire_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = idual_mic_endfire_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_endfire_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 16KHz */
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = idual_mic_endfire_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_endfire_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 48KHz */
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = idual_mic_endfire_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_endfire_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile idual_mic_endfire_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = idual_mic_endfire_settings,
	.setting_sz = ARRAY_SIZE(idual_mic_endfire_settings),
};

static enum hsed_controller idual_mic_endfire_pmctl_id[] = {
	PM_HSED_CONTROLLER_0, PM_HSED_CONTROLLER_2
};

static struct snddev_icodec_data snddev_idual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_endfire_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_MIC_ENDFIRE,
	.profile = &idual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pmctl_id = idual_mic_endfire_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(idual_mic_endfire_pmctl_id),
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_idual_mic_endfire_device = {
	.name = "snddev_icodec",
	.id = 12,
	.dev = { .platform_data = &snddev_idual_mic_endfire_data },
};

static struct snddev_icodec_data snddev_spk_idual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_endfire_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC_ENDFIRE,
	.profile = &idual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pmctl_id = idual_mic_endfire_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(idual_mic_endfire_pmctl_id),
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_spk_idual_mic_endfire_device = {
	.name = "snddev_icodec",
	.id = 14,
	.dev = { .platform_data = &snddev_spk_idual_mic_endfire_data },
};

static struct adie_codec_action_unit itty_mono_tx_actions[] =
	TTY_HEADSET_MONO_TX_8000_OSR_256;

static struct adie_codec_hwsetting_entry itty_mono_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = itty_mono_tx_actions,
		.action_sz = ARRAY_SIZE(itty_mono_tx_actions),
	},
};

static struct adie_codec_dev_profile itty_mono_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = itty_mono_tx_settings,
	.setting_sz = ARRAY_SIZE(itty_mono_tx_settings),
};

static struct snddev_icodec_data snddev_itty_mono_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_TTY_HEADSET_MIC,
	.profile = &itty_mono_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_itty_mono_tx_device = {
	.name = "snddev_icodec",
	.id = 16,
	.dev = { .platform_data = &snddev_itty_mono_tx_data },
};

static struct adie_codec_action_unit itty_mono_rx_actions[] =
	TTY_HEADSET_STEREO_RX_8000_OSR_256;

static struct adie_codec_hwsetting_entry itty_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = itty_mono_rx_actions,
		.action_sz = ARRAY_SIZE(itty_mono_rx_actions),
	},
};

static struct adie_codec_dev_profile itty_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = itty_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(itty_mono_rx_settings),
};

static struct snddev_icodec_data snddev_itty_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_TTY_HEADSET_SPKR,
	.profile = &itty_mono_rx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_headset_mono_voice,
	.pamp_off = set_amp_PowerDown,
	.max_voice_rx_vol[VOC_NB_INDEX] = 0,
	.min_voice_rx_vol[VOC_NB_INDEX] = 0,
	.max_voice_rx_vol[VOC_WB_INDEX] = 0,
	.min_voice_rx_vol[VOC_WB_INDEX] = 0,
};

static struct platform_device msm_itty_mono_rx_device = {
	.name = "snddev_icodec",
	.id = 17,
	.dev = { .platform_data = &snddev_itty_mono_rx_data },
};

static struct snddev_virtual_data snddev_a2dp_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "a2dp_tx",
	.copp_id = 5,
	.acdb_id = PSEUDO_ACDB_ID,
};

static struct snddev_virtual_data snddev_a2dp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "a2dp_rx",
	.copp_id = 2,
	.acdb_id = PSEUDO_ACDB_ID,
};

static struct platform_device msm_a2dp_rx_device = {
	.name = "snddev_virtual",
	.id = 0,
	.dev = { .platform_data = &snddev_a2dp_rx_data },
};

static struct platform_device msm_a2dp_tx_device = {
	.name = "snddev_virtual",
	.id = 1,
	.dev = { .platform_data = &snddev_a2dp_tx_data },
};

static struct snddev_virtual_data snddev_uplink_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "uplink_rx",
	.copp_id = 5,
	.acdb_id = PSEUDO_ACDB_ID,
};

static struct platform_device msm_uplink_rx_device = {
	.name = "snddev_virtual",
	.id = 2,
	.dev = { .platform_data = &snddev_uplink_rx_data },
};

static struct snddev_icodec_data\
		snddev_idual_mic_endfire_real_stereo_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_endfire_tx_real_stereo",
	.copp_id = 0,
	.acdb_id = PSEUDO_ACDB_ID,
	.profile = &idual_mic_endfire_profile,
	.channel_mode = REAL_STEREO_CHANNEL_MODE,
	.default_sample_rate = 48000,
	.pmctl_id = idual_mic_endfire_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(idual_mic_endfire_pmctl_id),
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_real_stereo_tx_device = {
	.name = "snddev_icodec",
	.id = 26,
	.dev = { .platform_data =
			&snddev_idual_mic_endfire_real_stereo_data },
};

static struct adie_codec_action_unit ihs_ffa_mono_rx_48KHz_osr256_actions[] =
	HEADSET_RX_CAPLESS_48000_OSR_256;

static struct adie_codec_hwsetting_entry ihs_ffa_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_ffa_mono_rx_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_ffa_mono_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ihs_ffa_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_ffa_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(ihs_ffa_mono_rx_settings),
};

static struct snddev_icodec_data snddev_ihs_ffa_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_mono_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_SPKR_MONO,
	.profile = &ihs_ffa_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_hsed_voltage_on,
	.pamp_off = msm_snddev_hsed_voltage_off,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400,
	.property = SIDE_TONE_MASK,
};

static struct platform_device msm_ihs_ffa_mono_rx_device = {
	.name = "snddev_icodec",
	.id = 5,
	.dev = { .platform_data = &snddev_ihs_ffa_mono_rx_data },
};

/* Headset + Speaker  Stereo Rx - audio*/
static struct adie_codec_action_unit
	ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[] =
	HEADSET_SPEAKER_AUDIO_AB_CPLS_48000_OSR_256;


static struct adie_codec_hwsetting_entry
	ihs_stereo_speaker_stereo_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions,
		.action_sz =
		ARRAY_SIZE(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ihs_stereo_speaker_stereo_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_stereo_speaker_stereo_rx_settings,
	.setting_sz = ARRAY_SIZE(ihs_stereo_speaker_stereo_rx_settings),
};

static struct snddev_icodec_data snddev_ihs_stereo_speaker_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_speaker_stereo_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_STEREO_PLUS_SPKR_STEREO_RX,
	.profile = &ihs_stereo_speaker_stereo_rx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_headset_speaker_audio,
	.pamp_off = set_amp_PowerDown,
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
	.max_voice_rx_vol[VOC_NB_INDEX] = -500,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2000,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400,
};

static struct platform_device msm_ihs_stereo_speaker_stereo_rx_device = {
	.name = "snddev_icodec",
	.id = 21,
	.dev = { .platform_data = &snddev_ihs_stereo_speaker_stereo_rx_data },
};

static struct adie_codec_action_unit ispk_dual_mic_bs_8KHz_osr256_actions[] =
	HS_DMIC2_STEREO_8000_OSR_256;

static struct adie_codec_hwsetting_entry ispk_dual_mic_bs_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = ispk_dual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispk_dual_mic_bs_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 16Khz */
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = ispk_dual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispk_dual_mic_bs_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 48KHz */
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispk_dual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispk_dual_mic_bs_8KHz_osr256_actions),
	},
};

static enum hsed_controller idual_mic_broadside_pmctl_id[] = {
	PM_HSED_CONTROLLER_0, PM_HSED_CONTROLLER_2
};

static struct adie_codec_dev_profile ispk_dual_mic_bs_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ispk_dual_mic_bs_settings,
	.setting_sz = ARRAY_SIZE(ispk_dual_mic_bs_settings),
};
static struct snddev_icodec_data snddev_spk_idual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_broadside_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC_BROADSIDE,
	.profile = &ispk_dual_mic_bs_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pmctl_id = idual_mic_broadside_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(idual_mic_broadside_pmctl_id),
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_spk_idual_mic_broadside_device = {
	.name = "snddev_icodec",
	.id = 15,
	.dev = { .platform_data = &snddev_spk_idual_mic_broadside_data },
};

static struct adie_codec_action_unit idual_mic_bs_8KHz_osr256_actions[] =
	HS_DMIC2_STEREO_8000_OSR_256;

static struct adie_codec_hwsetting_entry idual_mic_broadside_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = idual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_bs_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 16KHz */
	{
		.freq_plan = 16000,
		.osr = 256,
		.actions = idual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_bs_8KHz_osr256_actions),
	}, /* 8KHz profile can be used for 16KHz */
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = idual_mic_bs_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idual_mic_bs_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile idual_mic_broadside_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = idual_mic_broadside_settings,
	.setting_sz = ARRAY_SIZE(idual_mic_broadside_settings),
};

static struct snddev_icodec_data snddev_idual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_broadside_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_MIC_BROADSIDE,
	.profile = &idual_mic_broadside_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pmctl_id = idual_mic_broadside_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(idual_mic_broadside_pmctl_id),
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device msm_idual_mic_broadside_device = {
	.name = "snddev_icodec",
	.id = 13,
	.dev = { .platform_data = &snddev_idual_mic_broadside_data },
};

static struct snddev_mi2s_data snddev_mi2s_stereo_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "hdmi_stereo_rx",
	.copp_id = 3,
	.acdb_id = ACDB_ID_HDMI,
	.channel_mode = 2,
	.sd_lines = MI2S_SD_0,
	.route = msm_snddev_tx_route_config,
	.deroute = msm_snddev_tx_route_deconfig,
	.default_sample_rate = 48000,
};

static struct platform_device msm_snddev_mi2s_stereo_rx_device = {
	.name = "snddev_mi2s",
	.id = 0,
	.dev = { .platform_data = &snddev_mi2s_stereo_rx_data },
};

#else
///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////


//***************************************************************************************
//      RX VOICE PATH
//***************************************************************************************

//=======================================================================================
// Handset mono RX (voice call through receiver)    DEVICE_HANDSET_VOICE_RX 
//=======================================================================================
struct adie_codec_action_unit earpiece_voice_rx_actions[] =
	EAR_PRI_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry earpiece_voice_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = earpiece_voice_rx_actions,
		.action_sz = ARRAY_SIZE(earpiece_voice_rx_actions),
	}
};

static struct adie_codec_dev_profile earpiece_voice_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = earpiece_voice_rx_settings,
	.setting_sz = ARRAY_SIZE(earpiece_voice_rx_settings),
};

struct snddev_icodec_data earpiece_voice_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_voice_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_SPKR,
	.profile = &earpiece_voice_rx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_earpiece_voice,
	.pamp_off = set_amp_PowerDown,
	.property = SIDE_TONE_MASK,
#if 0
	.max_voice_rx_vol[VOC_NB_INDEX] = -200,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1700,
	.max_voice_rx_vol[VOC_WB_INDEX] = -200,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1700,
#else
	.max_voice_rx_vol[VOC_NB_INDEX] = 600,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1500,
	.max_voice_rx_vol[VOC_WB_INDEX] = 600,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1500,
#endif

};

static struct platform_device lge_device_earpiece_voice_rx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_HANDSET_VOICE_RX,
	.dev = { .platform_data = &earpiece_voice_rx_data },
};

//=======================================================================================
// Headset stereo RX : (voice call through headset)    DEVICE_HEADSET_STEREO_VOICE_RX
//==========================================================================

static struct adie_codec_action_unit headset_mono_voice_rx_actions[] =
	HEADSET_AB_CPLS_48000_OSR_256;

static struct adie_codec_hwsetting_entry headset_mono_voice_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_mono_voice_rx_actions,
		.action_sz = ARRAY_SIZE(headset_mono_voice_rx_actions),
	}
};

static struct adie_codec_dev_profile headset_mono_voice_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_mono_voice_rx_settings,
	.setting_sz = ARRAY_SIZE(headset_mono_voice_rx_settings),
};

static struct snddev_icodec_data headset_mono_voice_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_voice_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_SPKR_STEREO,
	.profile = &headset_mono_voice_rx_profile,
	.channel_mode = CHANNEL_MODE_STEREO,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_headset_mono_voice,
	.pamp_off = set_amp_PowerDown,
	.property = 0,//SIDE_TONE_MASK,
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
#if 0
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400,
#else
	.max_voice_rx_vol[VOC_NB_INDEX] = 600,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1200,
	.max_voice_rx_vol[VOC_WB_INDEX] = 600,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1200,
#endif
};

static struct platform_device lge_device_headset_mono_voice_rx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_HEADSET_VOICE_RX,
	.dev = { .platform_data = &headset_mono_voice_rx_data },
};


//=======================================================================================
// Speaker stereo RX : (voice call through speaker)    DEVICE_SPEAKER_STEREO_VOICE_RX
//=======================================================================================
static struct adie_codec_action_unit speaker_stereo_voice_rx_actions[] =
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_0)
	SPEAKER_INCALL_PRI_STEREO_48000_OSR_256;
#else
	SPEAKER_INCALL_LINOUT_PRI_STEREO_48000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry speaker_stereo_voice_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = speaker_stereo_voice_rx_actions,
		.action_sz = ARRAY_SIZE(speaker_stereo_voice_rx_actions),
	}
};

static struct adie_codec_dev_profile speaker_stereo_voice_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = speaker_stereo_voice_rx_settings,
	.setting_sz = ARRAY_SIZE(speaker_stereo_voice_rx_settings),
};

static struct snddev_icodec_data speaker_stereo_voice_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_stereo_voice_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_STEREO,
	.profile = &speaker_stereo_voice_rx_profile,
	.channel_mode = CHANNEL_MODE_STEREO,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_speaker_stereo_voice,
	.pamp_off = set_amp_PowerDown,
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_0)
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,	
#endif

#if 0
	.max_voice_rx_vol[VOC_NB_INDEX] = 500,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1500,
	.max_voice_rx_vol[VOC_WB_INDEX] = 500,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1500
#else
	.max_voice_rx_vol[VOC_NB_INDEX] = 800,
	.min_voice_rx_vol[VOC_NB_INDEX] =  -1000,
	.max_voice_rx_vol[VOC_WB_INDEX] = 800,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1000
#endif
};

static struct platform_device lge_device_speaker_stereo_voice_rx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_SPEAKER_VOICE_RX,
	.dev = { .platform_data = &speaker_stereo_voice_rx_data },
};

//=======================================================================================
// TTY RX : (TTY call)    DEVICE_TTY_RX
//=======================================================================================
struct adie_codec_action_unit tty_rx_actions[] =
	TTY_HEADSET_MONO_RX;

static struct adie_codec_hwsetting_entry tty_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = tty_rx_actions,
		.action_sz = ARRAY_SIZE(tty_rx_actions),
	}
};

static struct adie_codec_dev_profile tty_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = tty_rx_settings,
	.setting_sz = ARRAY_SIZE(tty_rx_settings),
};

static struct snddev_icodec_data tty_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_TTY_HEADSET_SPKR,
	.profile = &tty_rx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.default_sample_rate = 48000,
	.pamp_on = &set_amp_tty,
	.pamp_off = &set_amp_PowerDown,
	.max_voice_rx_vol[VOC_NB_INDEX] = -600,
	.min_voice_rx_vol[VOC_NB_INDEX] = -700,
	.max_voice_rx_vol[VOC_WB_INDEX] = -600,
	.min_voice_rx_vol[VOC_WB_INDEX] = -700,
};

static struct platform_device lge_device_tty_rx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_TTY_RX,
	.dev = { .platform_data = &tty_rx_data },
};


//=======================================================================================
// BT sco RX : (voice call through BT)    DEVICE_BT_SCO_VOICE_RX
//=======================================================================================
struct snddev_ecodec_data bt_sco_voice_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_voice_rx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_SPKR,
	.channel_mode = CHANNEL_MODE_MONO,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
	.max_voice_rx_vol[VOC_NB_INDEX] = 400,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1100,
	.max_voice_rx_vol[VOC_WB_INDEX] = 400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1100,
};

struct platform_device lge_device_bt_sco_voice_rx = {
	.name = "msm_snddev_ecodec",
	.id = DEVICE_ID_EXTERNAL_BT_SCO_VOICE_RX,
	.dev = { .platform_data = &bt_sco_voice_rx_data },
};


//***************************************************************************************
//      RX AUDIO PATH
//***************************************************************************************

//=======================================================================================
// Handset mono RX (audio through receiver)    DEVICE_HANDSET_AUDIO_RX 
//=======================================================================================
struct adie_codec_action_unit earpiece_audio_rx_actions[] =
	EAR_PRI_MONO_8000_OSR_256_AUDIO;

static struct adie_codec_hwsetting_entry earpiece_audio_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = earpiece_audio_rx_actions,
		.action_sz = ARRAY_SIZE(earpiece_audio_rx_actions),
	}
};

static struct adie_codec_dev_profile earpiece_audio_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = earpiece_audio_rx_settings,
	.setting_sz = ARRAY_SIZE(earpiece_audio_rx_settings),
};

static struct snddev_icodec_data earpiece_audio_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_audio_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_SPKR,
	.profile = &earpiece_audio_rx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_earpiece_voice,
	.pamp_off = set_amp_PowerDown,
	.max_voice_rx_vol[VOC_NB_INDEX] = -200,
	.min_voice_rx_vol[VOC_NB_INDEX] = -200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -200,
	.min_voice_rx_vol[VOC_WB_INDEX] = -200,
};

static struct platform_device lge_device_earpiece_audio_rx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_HANDSET_AUDIO_RX,
	.dev = { .platform_data = &earpiece_audio_rx_data },
};

//=======================================================================================
// Headset stereo RX : (audio through headset)    DEVICE_HEADSET_STEREO_AUDIO_RX
//=======================================================================================
static struct adie_codec_action_unit headset_stereo_audio_rx_actions[] =
	HEADSET_AUDIO_AB_CPLS_48000_OSR_256;

static struct adie_codec_hwsetting_entry headset_stereo_audio_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_stereo_audio_rx_actions,
		.action_sz = ARRAY_SIZE(headset_stereo_audio_rx_actions),
	}
};

static struct adie_codec_dev_profile headset_stereo_audio_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_stereo_audio_rx_settings,
	.setting_sz = ARRAY_SIZE(headset_stereo_audio_rx_settings),
};

static struct snddev_icodec_data headset_stereo_audio_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_audio_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_SPKR_STEREO,
	.profile = &headset_stereo_audio_rx_profile,
	.channel_mode = CHANNEL_MODE_STEREO,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_headset_stereo_audio,
	.pamp_off = set_amp_PowerDown,
	.property = 0,
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
	.max_voice_rx_vol[VOC_NB_INDEX] = -700,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2200,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400,
};

static struct platform_device lge_device_headset_stereo_audio_rx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_HEADSET_AUDIO_RX,
	.dev = { .platform_data = &headset_stereo_audio_rx_data },
};


//=======================================================================================
// Speaker stereo RX : (audio call through speaker)    DEVICE_SPEAKER_STEREO_AUDIO_RX
//=======================================================================================
static struct adie_codec_action_unit speaker_stereo_audio_rx_actions[] =
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_0)
	SPEAKER_AUDIO_PRI_STEREO_48000_OSR_256;
#else
	SPEAKER_AUDIO_LINOUT_PRI_MONO_48000_OSR_256;
#endif
static struct adie_codec_hwsetting_entry speaker_stereo_audio_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = speaker_stereo_audio_rx_actions,
		.action_sz = ARRAY_SIZE(speaker_stereo_audio_rx_actions),
	}
};

static struct adie_codec_dev_profile speaker_stereo_audio_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = speaker_stereo_audio_rx_settings,
	.setting_sz = ARRAY_SIZE(speaker_stereo_audio_rx_settings),
};

static struct snddev_icodec_data speaker_stereo_audio_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_stereo_audio_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MONO,
	.profile = &speaker_stereo_audio_rx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_speaker_stereo_audio,
	.pamp_off = set_amp_PowerDown,
#if (CONFIG_LGE_PCB_REVISION == LGE_REV_0)
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,	
#endif
	.max_voice_rx_vol[VOC_NB_INDEX] = 500,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1500,
	.max_voice_rx_vol[VOC_WB_INDEX] = 500,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1500
};

static struct platform_device lge_device_speaker_stereo_audio_rx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_SPEAKER_AUDIO_RX,
	.dev = { .platform_data = &speaker_stereo_audio_rx_data },
};

//=========================================================================================================
// Headset / Speaker dual RX : (audio through headset and speaker)    DEVICE_HEADSET_SPEAKER_AUDIO_RX
//=========================================================================================================
static struct adie_codec_action_unit headset_speaker_audio_rx_actions[] =
	HEADSET_SPEAKER_AUDIO_AB_CPLS_48000_OSR_256;


static struct adie_codec_hwsetting_entry 	headset_speaker_audio_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_speaker_audio_rx_actions,
		.action_sz =
		ARRAY_SIZE(headset_speaker_audio_rx_actions),
	}
};

static struct adie_codec_dev_profile headset_speaker_audio_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_speaker_audio_rx_settings,
	.setting_sz = ARRAY_SIZE(headset_speaker_audio_rx_settings),
};

static struct snddev_icodec_data headset_speaker_audio_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_speaker_stereo_rx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_STEREO_PLUS_SPKR_STEREO_RX,
	.profile = &headset_speaker_audio_rx_profile,
	.channel_mode = CHANNEL_MODE_STEREO,
	.default_sample_rate = 48000,
	.pamp_on = set_amp_headset_speaker_audio,
	.pamp_off = set_amp_PowerDown,
	.voltage_on = msm_snddev_hsed_voltage_on,
	.voltage_off = msm_snddev_hsed_voltage_off,
	.max_voice_rx_vol[VOC_NB_INDEX] = -500,
	.min_voice_rx_vol[VOC_NB_INDEX] = -2000,
	.max_voice_rx_vol[VOC_WB_INDEX] = -900,
	.min_voice_rx_vol[VOC_WB_INDEX] = -2400,
};

static struct platform_device lge_device_headset_speaker_audio_rx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_HEADSET_SPEAKER_RX,
	.dev = { .platform_data = &headset_speaker_audio_rx_data },
};


//=======================================================================================
// BT sco RX : (audio through BT)    DEVICE_BT_SCO_AUDIO_RX
//=======================================================================================
static struct snddev_ecodec_data bt_sco_audio_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_audio_rx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_SPKR,
	.channel_mode = CHANNEL_MODE_MONO,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
	.max_voice_rx_vol[VOC_NB_INDEX] = 400,
	.min_voice_rx_vol[VOC_NB_INDEX] = -1100,
	.max_voice_rx_vol[VOC_WB_INDEX] = 400,
	.min_voice_rx_vol[VOC_WB_INDEX] = -1100,
};

struct platform_device lge_device_bt_sco_audio_rx = {
	.name = "msm_snddev_ecodec",
	.id = DEVICE_ID_EXTERNAL_BT_SCO_AUDIO_RX,
	.dev = { .platform_data = &bt_sco_audio_rx_data },
};

//=======================================================================================
// BT A2DP RX : (audio through A2DP)    DEVICE_BT_A2DP_RX
//=======================================================================================
static struct snddev_virtual_data bt_a2dp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "a2dp_rx",
	.copp_id = 2,
	.acdb_id = PSEUDO_ACDB_ID,
};

static struct platform_device lge_device_bt_a2dp_rx = {
	.name = "snddev_virtual",
	.id = 0,
	.dev = { .platform_data = &bt_a2dp_rx_data },
};


//***************************************************************************************
//      TX VOICE PATH
//***************************************************************************************
static enum hsed_controller handset_mic1_pmctl_id[] = {PM_HSED_CONTROLLER_0};
//static enum hsed_controller handset_mic_aux_pmctl_id[] = {PM_HSED_CONTROLLER_2};

//=======================================================================================
// Handset voice TX (voice call through main mic)    DEVICE_HANDSET_VOICE_TX 
//=======================================================================================
struct adie_codec_action_unit handset_voice_tx_actions[] =
	AMIC_PRI_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry handset_voice_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = handset_voice_tx_actions,
		.action_sz = ARRAY_SIZE(handset_voice_tx_actions),
	}
};

static struct adie_codec_dev_profile handset_voice_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = handset_voice_tx_settings,
	.setting_sz = ARRAY_SIZE(handset_voice_tx_settings),
};

static struct snddev_icodec_data handset_voice_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_voice_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_MIC,  //ACDB_ID_HANDSET_MIC
	.profile = &handset_voice_tx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = handset_mic1_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(handset_mic1_pmctl_id),
	.default_sample_rate = 48000,
};

static struct platform_device lge_device_handset_voice_tx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_HANDSET_VOICE_TX,
	.dev = { .platform_data = &handset_voice_tx_data },
};


//=======================================================================================
// Handset loopback TX (loopback through main mic)    DEVICE_HANDSET_LOOPBACK_TX 
//=======================================================================================
struct adie_codec_action_unit handset_loopback_tx_actions[] =
	LOOPBACK_AMIC_PRI_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry handset_loopback_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = handset_loopback_tx_actions,
		.action_sz = ARRAY_SIZE(handset_loopback_tx_actions),
	}
};

static struct adie_codec_dev_profile handset_loopback_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = handset_loopback_tx_settings,
	.setting_sz = ARRAY_SIZE(handset_loopback_tx_settings),
};

static struct snddev_icodec_data handset_loopback_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_loopback_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_LOOPBACK_TX,  //ACDB_ID_HANDSET_MIC
	.profile = &handset_loopback_tx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = handset_mic1_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(handset_mic1_pmctl_id),
	.default_sample_rate = 48000,
};

static struct platform_device lge_device_handset_loopback_tx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_HANDSET_LOOPBACK_TX,
	.dev = { .platform_data = &handset_loopback_tx_data },
};


//=======================================================================================
// Headset voice TX (voice call through headset mic)    DEVICE_HEADSET_VOICE_TX 
//=======================================================================================
struct adie_codec_action_unit headset_voice_tx_actions[] =
	AMIC1_HEADSET_TX_MONO_PRIMARY_OSR256;

static struct adie_codec_hwsetting_entry headset_voice_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_voice_tx_actions,
		.action_sz = ARRAY_SIZE(headset_voice_tx_actions),
	}
};

static struct adie_codec_dev_profile headset_voice_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = headset_voice_tx_settings,
	.setting_sz = ARRAY_SIZE(headset_voice_tx_settings),
};

static struct snddev_icodec_data headset_voice_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_voice_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_MIC,
	.profile = &headset_voice_tx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device lge_device_headset_voice_tx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_HEADSET_VOICE_TX,
	.dev = { .platform_data = &headset_voice_tx_data },
};


//=======================================================================================
// Headset loopback TX (voice call through headset mic)    DEVICE_HEADSET_LOOPBACK_TX 
//=======================================================================================
struct adie_codec_action_unit headset_loopback_tx_actions[] =
	AMIC1_HEADSET_LOOPBACK_TX_MONO_PRIMARY_OSR256;

static struct adie_codec_hwsetting_entry headset_loopback_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_loopback_tx_actions,
		.action_sz = ARRAY_SIZE(headset_loopback_tx_actions),
	}
};

static struct adie_codec_dev_profile headset_loopback_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = headset_loopback_tx_settings,
	.setting_sz = ARRAY_SIZE(headset_loopback_tx_settings),
};

static struct snddev_icodec_data headset_loopback_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_loopback_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HEADSET_LOOPBACK_TX,
	.profile = &headset_loopback_tx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_tx_route_config,
	.pamp_off = msm_snddev_tx_route_deconfig,
};

static struct platform_device lge_device_headset_loopback_tx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_HEADSET_LOOPBACK_TX,
	.dev = { .platform_data = &headset_loopback_tx_data },
};


//=======================================================================================
// Speaker voice TX (voice call through aux mic)    DEVICE_SPEAKER_VOICE_TX 
//=======================================================================================
struct adie_codec_action_unit speaker_voice_tx_actions[] =
	SPEAKERPHONE_AMIC_PRI_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry speaker_voice_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = speaker_voice_tx_actions,
		.action_sz = ARRAY_SIZE(speaker_voice_tx_actions),
	}
};

static struct adie_codec_dev_profile speaker_voice_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = speaker_voice_tx_settings,
	.setting_sz = ARRAY_SIZE(speaker_voice_tx_settings),
};

static struct snddev_icodec_data speaker_voice_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_voice_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_SPKR_PHONE_MIC,
	.profile = &speaker_voice_tx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = handset_mic1_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(handset_mic1_pmctl_id),
	.default_sample_rate = 48000,
};

static struct platform_device lge_device_speaker_tx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_SPEAKER_VOICE_TX,
	.dev = { .platform_data = &speaker_voice_tx_data },
};

//=======================================================================================
// TTY TX : (TTY call)    DEVICE_TTY_TX
//=======================================================================================
struct adie_codec_action_unit tty_tx_actions[] =
	TTY_HEADSET_MONO_TX;

static struct adie_codec_hwsetting_entry tty_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = tty_tx_actions,
		.action_sz = ARRAY_SIZE(tty_tx_actions),
	}
};

static struct adie_codec_dev_profile tty_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = tty_tx_settings,
	.setting_sz = ARRAY_SIZE(tty_tx_settings),
};

static struct snddev_icodec_data tty_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_TTY_HEADSET_MIC,
	.profile = &tty_tx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.default_sample_rate = 48000,
	.pmctl_id = NULL,
	.pmctl_id_sz = 0,
	.pamp_on = NULL,
	.pamp_off = NULL,
};

static struct platform_device lge_device_tty_tx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_TTY_TX,
	.dev = { .platform_data = &tty_tx_data },
};

//=======================================================================================
// BT sco TX : (voice call through bt sco)    DEVICE_BT_SCO_VOICE_TX
//=======================================================================================
static struct snddev_ecodec_data bt_sco_voice_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_tx",
	.copp_id = 1,
	.acdb_id = ACDB_ID_BT_SCO_MIC,
	.channel_mode = CHANNEL_MODE_MONO,
	.conf_pcm_ctl_val = BT_SCO_PCM_CTL_VAL,
	.conf_aux_codec_intf = BT_SCO_AUX_CODEC_INTF,
	.conf_data_format_padding_val = BT_SCO_DATA_FORMAT_PADDING,
};

struct platform_device lge_device_bt_sco_voice_tx = {
	.name = "msm_snddev_ecodec",
	.id = DEVICE_ID_EXTERNAL_BT_SCO_VOICE_TX,
	.dev = { .platform_data = &bt_sco_voice_tx_data },
};


//***************************************************************************************
//      TX AUDIO PATH
//***************************************************************************************

//=======================================================================================
// Handset rec TX (recording through main mic)    DEVICE_MIC1_REC_TX 
//=======================================================================================
struct adie_codec_action_unit mic1_rec_tx_actions[] =
	SPEAKER_TX_48000_OSR_256_REC;

static struct adie_codec_hwsetting_entry mic1_rec_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = mic1_rec_tx_actions,
		.action_sz = ARRAY_SIZE(mic1_rec_tx_actions),
	}
};

static struct adie_codec_dev_profile mic1_rec_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = mic1_rec_tx_settings,
	.setting_sz = ARRAY_SIZE(mic1_rec_tx_settings),
};

static struct snddev_icodec_data mic1_rec_tx_data = {
	.capability = (SNDDEV_CAP_TX),
	.name = "mic1_rec_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_VR_TX,
	.profile = &mic1_rec_tx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = handset_mic1_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(handset_mic1_pmctl_id),
	.default_sample_rate = 48000,
};

static struct platform_device lge_device_mic1_rec_tx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_MIC1_REC,
	.dev = { .platform_data = &mic1_rec_tx_data },
};

//===========================================================================

static struct adie_codec_action_unit cam_rec_tx_actions[] =
	SPEAKER_TX_48000_OSR_256_CAM_REC;

static struct adie_codec_hwsetting_entry cam_rec_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = cam_rec_tx_actions,
		.action_sz = ARRAY_SIZE(cam_rec_tx_actions),
	}
};

static struct adie_codec_dev_profile cam_rec_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = cam_rec_tx_settings,
	.setting_sz = ARRAY_SIZE(cam_rec_tx_settings),
};

static struct snddev_icodec_data cam_rec_tx_data = {
	.capability = (SNDDEV_CAP_TX),
	.name = "cam_rec_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_REC_CAMCORDER_TX,
	.profile = &cam_rec_tx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = handset_mic1_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(handset_mic1_pmctl_id),
	.default_sample_rate = 48000,
};

static struct platform_device lge_device_cam_rec_tx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_CAM_REC,
	.dev = { .platform_data = &cam_rec_tx_data },
};

//===========================================================================

static struct adie_codec_action_unit voice_rec_tx_actions[] =
	SPEAKER_TX_48000_OSR_256_VOICE_REC;

static struct adie_codec_hwsetting_entry voice_rec_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = voice_rec_tx_actions,
		.action_sz = ARRAY_SIZE(voice_rec_tx_actions),
	}
};

static struct adie_codec_dev_profile voice_rec_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = voice_rec_tx_settings,
	.setting_sz = ARRAY_SIZE(voice_rec_tx_settings),
};

static struct snddev_icodec_data voice_rec_tx_data = {
	.capability = (SNDDEV_CAP_TX),
	.name = "voice_rec_tx",
	.copp_id = 0,
	.acdb_id = ACDB_ID_HANDSET_MIC,
	.profile = &voice_rec_tx_profile,
	.channel_mode = CHANNEL_MODE_MONO,
	.pmctl_id = handset_mic1_pmctl_id,
	.pmctl_id_sz = ARRAY_SIZE(handset_mic1_pmctl_id),
	.default_sample_rate = 48000,
};

static struct platform_device lge_device_voice_rec_tx = {
	.name = "snddev_icodec",
	.id = DEVICE_ID_INTERNAL_VOICE_REC,
	.dev = { .platform_data = &voice_rec_tx_data },
};

#endif

#if 0
static struct platform_device *snd_devices_m3s_bakup[] __initdata = {
	&msm_iearpiece_ffa_device,
	&msm_imic_ffa_device,
	&msm_ispkr_stereo_device,
	&msm_ispkr_audio_stereo_device,
	&msm_headset_mic_device,
	&msm_ihs_ffa_mono_rx_device,
	&msm_snddev_mi2s_fm_rx_device,
	&msm_snddev_mi2s_fm_tx_device,
	&msm_bt_sco_earpiece_device,
	&msm_bt_sco_mic_device,
	&msm_ispkr_mic_device,
	&msm_headset_stereo_device,
	&msm_headset_stereo_audio_device,
	&msm_idual_mic_endfire_device,
	&msm_spk_idual_mic_endfire_device,
	&msm_itty_mono_tx_device,
	&msm_itty_mono_rx_device,
	&msm_a2dp_rx_device,
	&msm_a2dp_tx_device,
	&msm_uplink_rx_device,
	&msm_real_stereo_tx_device,
	&msm_ihs_stereo_speaker_stereo_rx_device,
	&msm_spk_idual_mic_broadside_device,
	&msm_idual_mic_broadside_device,
	&msm_snddev_mi2s_stereo_rx_device,
	&msm_imic_speakerphone_device,
	&msm_headset_mic_incall_device,
};

#else
/////////////////////////////////////////////////////////////////////////////////////////
/* Configurations list */
/////////////////////////////////////////////////////////////////////////////////////////
// Bryce uses this config
static struct platform_device *snd_devices_m3s[] __initdata = {
  &lge_device_earpiece_voice_rx,
  &lge_device_headset_mono_voice_rx,
  &lge_device_speaker_stereo_voice_rx,
  &lge_device_tty_rx,
  &lge_device_bt_sco_voice_rx,
  &lge_device_earpiece_audio_rx,
  &lge_device_headset_stereo_audio_rx,
  &lge_device_speaker_stereo_audio_rx,
  &lge_device_headset_speaker_audio_rx,
  &lge_device_bt_sco_audio_rx,
  &lge_device_bt_a2dp_rx,
  &lge_device_handset_voice_tx,
  &lge_device_headset_voice_tx,
  &lge_device_speaker_tx,
  &lge_device_tty_tx,
  &lge_device_bt_sco_voice_tx,
  &lge_device_mic1_rec_tx,
  &lge_device_cam_rec_tx,
  &lge_device_voice_rec_tx,
   &lge_device_handset_loopback_tx,
   &lge_device_headset_loopback_tx
}; 
 
struct adie_codec_action_unit *codec_cal[] = {
  earpiece_voice_rx_actions,
  headset_mono_voice_rx_actions,
  speaker_stereo_voice_rx_actions,
  tty_rx_actions,
  headset_stereo_audio_rx_actions,
  headset_speaker_audio_rx_actions,
  speaker_stereo_audio_rx_actions,
  handset_voice_tx_actions,
  headset_voice_tx_actions,
  speaker_voice_tx_actions,
  tty_tx_actions,
  mic1_rec_tx_actions,
  cam_rec_tx_actions,
  voice_rec_tx_actions,
  handset_loopback_tx_actions,
  headset_loopback_tx_actions
};
#endif
void __ref msm_snddev_init_timpani(void)
{
	platform_add_devices(snd_devices_m3s,
			ARRAY_SIZE(snd_devices_m3s));
}
