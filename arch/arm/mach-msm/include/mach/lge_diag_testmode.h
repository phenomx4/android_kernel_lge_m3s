#ifndef LG_DIAG_TESTMODE_H
#define LG_DIAG_TESTMODE_H

#include "lge_comdef.h"

/*********************** BEGIN PACK() Definition ***************************/
#if defined __GNUC__
  #define PACK(x)       x __attribute__((__packed__))
  #define PACKED        __attribute__((__packed__))
#elif defined __arm
  #define PACK(x)       __packed x
  #define PACKED        __packed
#else
  #error No PACK() macro defined for this compiler
#endif
/********************** END PACK() Definition *****************************/

/* BEGIN: 0014654 jihoon.lee@lge.com 20110124 */
/* MOD 0014654: [TESTMODE] SYNC UP TESTMODE PACKET STRUCTURE TO KERNEL */
//#define MAX_KEY_BUFF_SIZE    200
#define MAX_KEY_BUFF_SIZE    201
/* END: 0014654 jihoon.lee@lge.com 2011024 */

typedef enum
{
  	VER_SW=0,				//Binary Revision
  	VER_DSP,      			/* Camera DSP */
  	VER_MMS,
  	VER_CONTENTS,
  	VER_PRL,
  	VER_ERI,
  	VER_BREW,
  	VER_MODEL,  			// 250-0-7 Test Mode Version
  	VER_HW,
  	REV_DSP=9,
  	CONTENTS_SIZE,
  	JAVA_FILE_CNT=13,
  	JAVA_FILE_SIZE,
  	VER_JAVA,
  	BANK_ON_CNT=16,
  	BANK_ON_SIZE,
  	MODULE_FILE_CNT,
  	MODULE_FILE_SIZE,
  	MP3_DSP_OS_VER=21,
  	VER_MODULE  ,
  	VER_LCD_REVISION=24
} test_mode_req_version_type;
/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode function [START] */
#define WIFI_MAC_ADDR_CNT 12

typedef enum
{
  WLAN_TEST_MODE_54G_ON=4,
  WLAN_TEST_MODE_OFF,
  WLAN_TEST_MODE_RX_START,
  WLAN_TEST_MODE_RX_RESULT=9,
  WLAN_TEST_MODE_TX_START=10,
  WLAN_TEST_MODE_TXRX_STOP=13,
  WLAN_TEST_MODE_LF_RX_START=31,
  WLAN_TEST_MODE_MF_TX_START=44,
  WLAN_TEST_MODE_11B_ON=57,
  WLAN_TEST_MODE_11N_MIXED_LONG_GI_ON=69,
  WLAN_TEST_MODE_11N_MIXED_SHORT_GI_ON=77,
  WLAN_TEST_MODE_11N_GREEN_LONG_GI_ON=85,
  WLAN_TEST_MODE_11N_GREEN_SHORT_GI_ON=93,
  WLAN_TEST_MODE_11A_CH_RX_START=101, // not support
  WLAN_TEST_MODE_11BG_CH_TX_START=128,
  WLAN_TEST_MODE_11A_ON=155,
  WLAN_TEST_MODE_11AN_MIXED_LONG_GI_ON=163,
  WLAN_TEST_MODE_MAX=195,
}test_mode_req_wifi_type;

typedef enum
{
  WLAN_TEST_MODE_CTGRY_ON,
  WLAN_TEST_MODE_CTGRY_OFF,
  WLAN_TEST_MODE_CTGRY_RX_START,
  WLAN_TEST_MODE_CTGRY_RX_STOP,
  WLAN_TEST_MODE_CTGRY_TX_START,
  WLAN_TEST_MODE_CTGRY_TX_STOP,
  WLAN_TEST_MODE_CTGRY_NOT_SUPPORTED,
} test_mode_ret_wifi_ctgry_t;


typedef enum
{
    WIFI_MAC_ADDRESS_WRITE = 0,
    WIFI_MAC_ADDRESS_READ = 1,
}test_mode_req_wifi_addr_req_type;

typedef struct {
	//test_mode_req_wifi_addr_req_type req_type;
	byte req_type;
    byte wifi_mac_addr[WIFI_MAC_ADDR_CNT];
}test_mode_req_wifi_addr_type;

/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode function [END] */

typedef enum
{
  	MOTOR_OFF,
  	MOTOR_ON
}test_mode_req_motor_type;

typedef enum
{
  	ACOUSTIC_OFF=0,
  	ACOUSTIC_ON,
  	HEADSET_PATH_OPEN,
  	HANDSET_PATH_OPEN,
  	ACOUSTIC_LOOPBACK_ON,
  	ACOUSTIC_LOOPBACK_OFF
} test_mode_req_acoustic_type;

typedef enum
{
	CAM_TEST_MODE_OFF = 0,
	CAM_TEST_MODE_ON,
	CAM_TEST_SHOT,
	CAM_TEST_SAVE_IMAGE,
	CAM_TEST_CALL_IMAGE,
	CAM_TEST_ERASE_IMAGE,
	CAM_TEST_FLASH_ON,
	CAM_TEST_FLASH_OFF = 9,
	CAM_TEST_CAMCORDER_MODE_OFF,
	CAM_TEST_CAMCORDER_MODE_ON,
	CAM_TEST_CAMCORDER_SHOT,
	CAM_TEST_CAMCORDER_SAVE_MOVING_FILE,
	CAM_TEST_CAMCORDER_PLAY_MOVING_FILE,
	CAM_TEST_CAMCORDER_ERASE_MOVING_FILE,
	CAM_TEST_CAMCORDER_FLASH_ON,
	CAM_TEST_CAMCORDER_FLASH_OFF,
	CAM_TEST_STROBE_LIGHT_ON,
	CAM_TEST_STROBE_LIGHT_OFF,
	CAM_TEST_CAMERA_SELECT = 22,
} test_mode_req_cam_type;

typedef enum
{
  	EXTERNAL_SOCKET_MEMORY_CHECK,
  	EXTERNAL_FLASH_MEMORY_SIZE,
  	EXTERNAL_SOCKET_ERASE,
  	EXTERNAL_FLASH_MEMORY_USED_SIZE = 4,
  	EXTERNAL_SOCKET_ERASE_SDCARD_ONLY = 0xE,
  	EXTERNAL_SOCKET_ERASE_FAT_ONLY = 0xF,
} test_mode_req_socket_memory;

#ifndef LG_BTUI_TEST_MODE
typedef enum
{
  	BT_GET_ADDR, //no use anymore
  	BT_TEST_MODE_1=1,
  	BT_TEST_MODE_CHECK=2,
  	BT_TEST_MODE_RELEASE=5,
  	BT_TEST_MODE_11=11 // 11~42
} test_mode_req_bt_type;
#endif

//20110930, addy.kim@lge.com,  [START]
typedef enum
{
  NFC_TEST_MODE_ON=0,
  NFC_TEST_MODE_OFF,
  NFC_TEST_MODE_SWP,
  NFC_TEST_MODE_ANT,
  NFC_TEST_MODE_READER,
  NFC_TEST_MODE_FIRMWARE_FILE_VERSION,
  NFC_TEST_MODE_FIMEWARE_UPDATE,
  NFC_TEST_MODE_FIRMWARE_CHIP_VERSION
}test_mode_req_nfc_type;
//20110930, addy.kim@lge.com,  [END]


typedef enum
{
  	MP3_128KHZ_0DB,
  	MP3_128KHZ_0DB_L,
  	MP3_128KHZ_0DB_R,
  	MP3_MULTISINE_20KHZ,
  	MP3_PLAYMODE_OFF,
  	MP3_SAMPLE_FILE,
  	MP3_NoSignal_LR_128k
} test_mode_req_mp3_test_type;

typedef enum 
{
  	MANUAL_TEST_ON,
  	MANUAL_TEST_OFF,
  	MANUAL_MODE_CHECK,
  	MANUAL_MODE_ALLAUTO_RESULT
} test_mode_req_manual_test_mode_type;

typedef enum
{
  	MEMORY_TOTAL_SIZE_TEST = 0 ,
  	MEMORY_FORMAT_MEMORY_TEST = 1,
} test_mode_req_memory_size_type;

typedef enum
{
  	MEMORY_TOTAL_CAPA_TEST,
  	MEMORY_USED_CAPA_TEST,
  	MEMORY_REMAIN_CAPA_TEST
} test_mode_req_memory_capa_type;

typedef enum
{
  	SLEEP_MODE_ON,
 	AIR_PLAIN_MODE_ON,
  	FTM_BOOT_ON,
  	AIR_PLAIN_MODE_OFF
} test_mode_sleep_mode_type;

typedef enum
{
  	SPEAKER_PHONE_OFF,
  	SPEAKER_PHONE_ON,
  	NOMAL_Mic1,
  	NC_MODE_ON,
  	ONLY_MIC2_ON_NC_ON,
  	ONLY_MIC1_ON_NC_ON
} test_mode_req_speaker_phone_type;

typedef enum
{
  	TEST_SCRIPT_ITEM_SET,
  	TEST_SCRIPT_MODE_CHECK,
  	CAL_DATA_BACKUP,
  	CAL_DATA_RESTORE,
  	CAL_DATA_ERASE,
  	CAL_DATA_INFO
} test_mode_req_test_script_mode_type;

typedef enum
{
  	FACTORY_RESET_CHECK,
  	FACTORY_RESET_COMPLETE_CHECK,
  	FACTORY_RESET_STATUS_CHECK,
  	FACTORY_RESET_COLD_BOOT,
  	FACTORY_RESET_ERASE_USERDATA = 0x0F, // for NPST dll
} test_mode_req_factory_reset_mode_type;

typedef enum
{
  	FACTORY_RESET_START = 0,
  	FACTORY_RESET_INITIAL = 1,
  	FACTORY_RESET_ARM9_END = 2,
  	FACTORY_RESET_COLD_BOOT_START = 3,
  	FACTORY_RESET_COLD_BOOT_END = 5,
  	FACTORY_RESET_NA = 7,
} test_mode_factory_reset_status_type;

typedef enum
{
  	VOL_LEV_OFF,
  	VOL_LEV_MIN,
  	VOL_LEV_MEDIUM,
  	VOL_LEV_MAX
} test_mode_req_volume_level_type;

typedef enum
{
  	FIRST_BOOTING_COMPLETE_CHECK,
/* BEGIN: 0015566 jihoon.lee@lge.com 20110207 */
/* ADD 0015566: [Kernel] charging mode check command */
#ifdef CONFIG_LGE_CHARGING_MODE_INFO
  	FIRST_BOOTING_CHG_MODE_CHECK=0xF, // charging mode check, temporal
#endif
/* END: 0015566 jihoon.lee@lge.com 20110207 */
} test_mode_req_fboot;

/* TEST_MODE_PID_TEST */
typedef enum
{
  PID_WRITE,
  PID_READ
}test_mode_req_subcmd_type;

typedef struct{
  test_mode_req_subcmd_type	pid_subcmd;
  byte PID[30];
}test_mode_req_pid_type;
typedef enum
{
	DB_INTEGRITY_CHECK=0,
	FPRI_CRC_CHECK=1,
	FILE_CRC_CHECK=2,
	CODE_PARTITION_CRC_CHECK=3,
	TOTAL_CRC_CHECK=4,	
	DB_DUMP=5,  //hojung7.kim@lge.com Add  (MS910)
	DB_COPY=6   //hojung7.kim@lge.com Add  (MS910)
} test_mode_req_db_check;

typedef enum
{
  	FIRST_BOOTING_IN_CHG_MODE,
  	FIRST_BOOTING_NOT_IN_CHG_MODE
} test_mode_first_booting_chg_mode_type;

typedef enum
{
  	RESET_FIRST_PRODUCTION,
  	RESET_FIRST_PRODUCTION_CHECK,
  	RESET_REFURBISH=2,
  	RESET_REFURBISH_CHECK,
} test_mode_req_reset_production_type;

//LGE_FOTA_ID_CHECK
typedef enum
{
	FOTA_ID_CHECK,
	FOTA_ID_READ
}test_mode_req_fota_id_check_type;

typedef enum
{
	POWER_RESET,
	POWER_OFF,
}test_mode_req_power_reset_type;

typedef enum 
{
  ACCEL_SENSOR_OFF= 0,
  ACCEL_SENSOR_ON,
  ACCEL_SENSOR_SENSORDATA,
} test_mode_req_acceleration_sensor_type;

typedef enum 
{
  SENSOR_CALIBRATION_START = 0,
  SENSOR_CALIBRATION_RESULT,
}test_mode_req_calibration_sensor_type;


extern int key_lock;

typedef enum
{
	KEY_LOCK_REQ=0,
	KEY_UNLOCK_REQ=1,	
}test_mode_req_key_lock_type;

//LGE_FW_LCD_K_CAL
typedef struct
{
	byte MaxRGB[10];
}test_mode_req_lcd_cal;
//johny.kim
typedef enum
{
    MLT_DISABLE,
    MLT_ENABLE,
}test_mode_req_mlt_enable_type;
//johny.kim

typedef union
{
  	test_mode_req_version_type				version;
	test_mode_req_motor_type            	motor;
    test_mode_req_acoustic_type         	acoustic;
    test_mode_req_cam_type              	camera;
    boolean                             	key_test_start;
    test_mode_req_socket_memory         	esm;
#ifndef LG_BTUI_TEST_MODE
    test_mode_req_bt_type               	bt;
#endif
	//20110930, chan2.kim@lge.com,	[START]
	test_mode_req_nfc_type 					nfc;
	//20110930, chan2.kim@lge.com,	[END]
    test_mode_req_mp3_test_type         	mp3_play;
	test_mode_req_manual_test_mode_type		test_manual_mode;
    test_mode_req_memory_size_type      	memory_format;
	test_mode_req_pid_type			pid;
    word                                	key_data;
    test_mode_req_memory_capa_type      	mem_capa;
    test_mode_sleep_mode_type           	sleep_mode;
	test_mode_req_speaker_phone_type		speaker_phone;
	test_mode_req_test_script_mode_type 	test_mode_test_scr_mode;
	test_mode_req_factory_reset_mode_type  	factory_reset;
	test_mode_req_volume_level_type			volume_level;
	test_mode_req_fboot 					fboot;
	test_mode_req_db_check					db_check;
	test_mode_req_reset_production_type 	reset_production_cmd;
	/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode [START] */
	test_mode_req_wifi_type wifi;
	test_mode_req_wifi_addr_type wifi_mac_ad;
//LGE_FOTA_ID_CHECK
    test_mode_req_fota_id_check_type		fota_id_check;
//Testmode Key Lock
	test_mode_req_key_lock_type				req_key_lock;
	/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode [END] */
	test_mode_req_power_reset_type		power_reset;
	test_mode_req_acceleration_sensor_type accel;
	test_mode_req_calibration_sensor_type sensor_calibration;
	char sensor_data[256];
	char aat_result[16];
//johny.kim
    test_mode_req_mlt_enable_type mlt_enable;
//johny.kim
	test_mode_req_lcd_cal lcd_cal; //LGE_FW_LCD_K_CAL
} test_mode_req_type;

typedef struct diagpkt_header
{
	byte opaque_header;
} PACKED diagpkt_header_type;

typedef struct DIAG_TEST_MODE_F_req_tag {
	diagpkt_header_type						xx_header;
	word									sub_cmd_code;
	test_mode_req_type						test_mode_req;
} PACKED DIAG_TEST_MODE_F_req_type;

typedef enum
{
  	TEST_OK_S,
  	TEST_FAIL_S,
  	TEST_NOT_SUPPORTED_S
} PACKED test_mode_ret_stat_type;


/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode [START] */
typedef struct
{
	int packet;
	int per;
} PACKED WlRxResults;
/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode [END] */

typedef struct 
{
       unsigned int  count;
       float              x_data;                      //xAAC
       float              y_data;                      //yAAC
       float              z_data;                      //zAAC       
} PACKED test_mode_accel_rsp_type;

typedef union
{
  	test_mode_req_version_type				version;
  	byte									str_buf[15];
	char 									key_pressed_buf[MAX_KEY_BUFF_SIZE];
	int                                     manual_test;
	char  									memory_check;
	test_mode_req_pid_type				pid;
// BEGIN : munho.lee@lge.com 2011-01-15
// MOD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist   
  	uint32    								socket_memory_size;
  	uint32    								socket_memory_usedsize;
//  int    socket_memory_size
//  int    socket_memory_usedsize;
// END : munho.lee@lge.com 2011-01-15
	unsigned int 							mem_capa;
	/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode [START] */
	byte wlan_status;
	WlRxResults wlan_rx_results;
	/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode [END] */
	test_mode_req_test_script_mode_type 	test_mode_test_scr_mode;
	test_mode_req_factory_reset_mode_type	factory_reset;
    byte 									fota_id_read[30];
	test_mode_accel_rsp_type accel;
//johny.kim
    byte mlt_enable;
//johny.kim

 	test_mode_req_lcd_cal lcd_cal; //LGE_FW_LCD_K_CAL
  byte read_nfc_data[100];
} PACKED test_mode_rsp_type;

typedef struct DIAG_TEST_MODE_F_rsp_tag {
	diagpkt_header_type		xx_header;
	word					sub_cmd_code;
	test_mode_ret_stat_type	ret_stat_code;
	test_mode_rsp_type		test_mode_rsp;
} PACKED DIAG_TEST_MODE_F_rsp_type;

typedef enum
{
	TEST_MODE_VERSION=0,
	TEST_MODE_LCD=1,
	TEST_MODE_LCD_CAL = 2, //khyun.kim for test //LGE_FW_LCD_K_CAL	
	TEST_MODE_MOTOR=3,
	TEST_MODE_ACOUSTIC=4,
	TEST_MODE_CAM=7,
	TEST_MODE_KEY_TEST=22,
	TEST_MODE_EXT_SOCKET_TEST=23,
	TEST_MODE_BATT_TEST=25,
	TEST_MODE_MP3_TEST=27,
#ifndef LG_BTUI_TEST_MODE
  	TEST_MODE_BLUETOOTH_TEST=24,
#endif
	TEST_MODE_ACCEL_SENSOR_TEST=31,
	TEST_MODE_ALCOHOL_SENSOR_TEST=32, //[2011-10-6] addy.kim@lge.com, add Test Number 32
	TEST_MODE_WIFI_TEST=33,
	TEST_MODE_MANUAL_MODE_TEST=36,
	TEST_MODE_FORMAT_MEMORY_TEST=38,
	TEST_MODE_KEY_DATA_TEST=40,
	TEST_MODE_MEMORY_CAPA_TEST=41,
	TEST_MODE_SLEEP_MODE_TEST=42,
	TEST_MODE_SPEAKER_PHONE_TEST=43,
    TEST_MODE_VCO_SELF_TUNNING_TEST=46,
    TEST_MODE_MRD_USB_TEST=47,
	TEST_MODE_TEST_SCRIPT_MODE=48,
	TEST_MODE_PROXIMITY_SENSOR_TEST=49, 
 	TEST_MODE_FACTORY_RESET_CHECK_TEST=50,
	TEST_MODE_VOLUME_TEST=51,
	TEST_MODE_CGPS_MEASURE_CNO=54,
	TEST_MODE_FIRST_BOOT_COMPLETE_TEST = 58,
	TEST_MODE_LED_TEST=60,	
	TEST_MODE_PID_TEST=70,
	TEST_MODE_SW_VERSION=71,
	TEST_MODE_IME_TEST=72,
	TEST_MODE_IMPL_TEST=73,
	TEST_MODE_UNLOCK_CODE_TEST=75,
	TEST_MODE_IDDE_TEST=76,
	TEST_MODE_FULL_SIGNATURE_TEST=77,
	TEST_MODE_NT_CODE_TEST=78,
	TEST_MODE_CAL_CHECK=82,
#ifndef LG_BTUI_TEST_MODE
  	TEST_MODE_BLUETOOTH_TEST_RW=83,
#endif
	TEST_MODE_SKIP_WELCOM_TEST=87,
	TEST_MODE_MAC_READ_WRITE=88,
	TEST_MODE_DB_INTEGRITY_CHECK=91,
	TEST_MODE_NVCRC_CHECK=92,
    TEST_MODE_SENSOR_CALIBRATION_TEST = 93,
	TEST_MODE_RELEASE_CURRENT_LIMIT=94,
	TEST_MODE_RESET_PRODUCTION=96,
	//LGE_FOTA_ID_CHECK
	TEST_MODE_FOTA_ID_CHECK = 98,
	// Key lock
	TEST_MODE_KEY_LOCK =99,
	TEST_MODE_ACCEL_SENSOR_ONOFF_TEST=100,
	TEST_MODE_COMPASS_SENSOR_TEST=102,
	TEST_MODE_GNSS_MEASURE_CNO = 103,
	TEST_MODE_POWER_RESET=105,
//johny.kim
	TEST_MODE_MLT_ENABLE=106,
//johny.kim
  	MAX_TEST_MODE_SUBCMD = 0xFFFF
} PACKED test_mode_sub_cmd_type;
 
#define TESTMODE_MSTR_TBL_SIZE   128

#define ARM9_PROCESSOR		0
#define ARM11_PROCESSOR		1

typedef void*(* testmode_func_type)(test_mode_req_type * , DIAG_TEST_MODE_F_rsp_type * );

typedef struct
{
  	word cmd_code;
	testmode_func_type func_ptr;
  	byte  which_procesor;             // to choose which processor will do act.
} testmode_user_table_entry_type;

typedef struct DIAG_TEST_MODE_KEY_F_rsp_tag {
  	diagpkt_header_type		xx_header;
  	word					sub_cmd_code;
  	test_mode_ret_stat_type	ret_stat_code;
  	char key_pressed_buf[MAX_KEY_BUFF_SIZE];
} PACKED DIAG_TEST_MODE_KEY_F_rsp_type;

#if 1	//def LG_FW_USB_ACCESS_LOCK
#define PPE_UART_KEY_LENGTH 6
#define PPE_DES3_KEY_LENGTH 128

typedef enum {
  TF_SUB_CHECK_PORTLOCK = 0,
  TF_SUB_LOCK_PORT,
  TF_SUB_UNLOCK_PORT,
  TF_SUB_KEY_VERIFY,  
  TF_SUB_GET_CARRIER,
  TF_SUB_PROD_FLAG_STATUS,  
  TF_SUB_PROD_KEY_VERIFY,  
} nvdiag_tf_sub_cmd_type;

// oskwon 090606 : Lock 상태에서 SUB 커맨드 3개만 허용, 패킷 길이는 3가지 다 허용 
typedef struct	
{
  byte cmd_code;                      /* Command code */
  byte sub_cmd;                       /* Sub Command */
} PACKED DIAG_TF_F_req_type1;

typedef struct
{
  byte cmd_code;                      /* Command code */
  byte sub_cmd;                       /* Sub Command */
  byte keybuf[PPE_UART_KEY_LENGTH];   /* Uart Lock Key - 6 Digit */
} PACKED DIAG_TF_F_req_type2;

typedef struct
{
  byte cmd_code;                      /* Command code */
  byte sub_cmd;                       /* Sub Command */
  union {
  byte keybuf[PPE_UART_KEY_LENGTH];   /* Uart Lock Key - 16 Digit */
  byte probuf[PPE_DES3_KEY_LENGTH];   /* Production Key - 128 Byte */
  } PACKED buf;
} PACKED DIAG_TF_F_req_type;

typedef struct
{
  byte cmd_code;                      /* Command code */ 
  byte sub_cmd;                       /* Sub Command */
  byte result;                        /* Status of operation */
} PACKED DIAG_TF_F_rsp_type;

typedef enum {
  TF_STATUS_FAIL = 0,       	//Fail Unknown Reason
  TF_STATUS_SUCCESS,        	//General Success
  TF_STATUS_PORT_LOCK = 12,    	//TF_SUB_CHECK_PORTLOCK -> LOCK
  TF_STATUS_PORT_UNLOCK,    	//TF_SUB_CHECK_PORTLOCK -> UNLOCK
  TF_STATUS_VER_KEY_OK,     	//TF_SUB_KEY_VERIFY -> OK 
  TF_STATUS_VER_KEY_NG,     	//TF_SUB_KEY_VERIFY -> NG
  TF_STATUS_P_FLAG_ENABLE,   	//PRODUCTION FLAG 1 
  TF_STATUS_P_FLAG_DISABLE,		//PRODUCTION FLAG 0
  TF_STATUS_VER_P_KEY_OK,		// PPE KEY OK
  TF_STATUS_VER_P_KEY_NG,		// PPE KEY NG
} DIAG_TF_F_sub_cmd_result_type;

typedef struct
{
  byte cmd_code;                      /* Command code */
  dword seccode;                       /* security code */
} PACKED DIAG_TF_SB_F_req_type;

typedef struct
{
  byte cmd_code;                      /* Command code */ 
} PACKED DIAG_TF_SB_F_rsp_type;

#endif

//ssoo.kim@lge.com 2011-12-08 : SMS Test Tool [FEATURE_SMS_PC_TEST]
typedef union
{
	boolean retvalue;

} sms_mode_rsp_type;
typedef struct
{
  word sub_cmd_code;  /* Use test_mode_sub_cmd_type. */
  byte ret_stat_code; /* Status to return. Use diag_test_mode_ret_stat_type. */
  sms_mode_rsp_type sms_mode_rsp;
} DIAG_SMS_mode_rsp_type;

typedef struct
{
  word sub_cmd_code;  /* Use test_mode_sub_cmd_type. */
  byte ret_stat_code; /* Status to return. Use diag_test_mode_ret_stat_type. */
  sms_mode_rsp_type sms_mode_rsp;
} DIAG_SMS_mode_req_type;

#endif /* LG_DIAG_TESTMODE_H */
