#ifndef DIAGICD_H
#define DIAGICD_H


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include <mach/lge_comdef.h>

/*===========================================================================

                      EXTERNAL FUNCTION AND VARIABLE DEFINITIONS

===========================================================================*/


/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

  This section contains local definitions for constants, macros, types,
  variables and other items needed by this module.

===========================================================================*/
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
#define LCD_MAIN_WIDTH   320 /*Cayman : 480	  VS920 : 736		VS910 : 480	VS660 : 320 */
#define LCD_MAIN_HEIGHT  480 /*Cayman : 800	  VS920 : 1280	VS910 : 800	VS660 : 480 */

#define ICD_MAX_STRING 10
#define ICD_DISP_TEXT_MAX_STRING 300
#define ICD_MSTR_TBL_SIZE 0xFF
#define ICD_SEND_BUF_SIZE		 65536	
#define ICD_SEND_SAVE_IMG_PATH_LEN 	20//Slate_ADB
#define ICD_SCRN_BUF_SIZE_MAX	 LCD_MAIN_WIDTH * LCD_MAIN_HEIGHT * 2 // icd_pixel_16_type, 2bytes
#define VARIABLE		50
typedef enum
{
	/** SAR : Sprint Automation Requirement - START **/
	ICD_GETDEVICEINFO_REQ_CMD						=0x01,	//Auto-025, Auto-027, Auto-030, Auto-040
	ICD_EXTENDEDVERSIONINFO_REQ_CMD					=0x02,	//Auto-222, Auto-223
	ICD_HANDSETDISPLAYTEXT_REQ_CMD					=0x03,	//Auto-224, Auto-225
	ICD_CAPTUREIMAGE_REQ_CMD						=0x04,	//Auto-015, Auto-226
	/** SAR : Sprint Automation Requirement - END **/

	/** ICDR : ICD Implementation Recommendation  - START **/
	ICD_GETAIRPLANEMODE_REQ_CMD						=0x20,	//Auto-016
	ICD_SETAIRPLANEMODE_REQ_CMD						=0x21,	//Auto-051
	ICD_GETBACKLIGHTSETTING_REQ_CMD					=0x22,	//Auto-017
	ICD_SETBACKLIGHTSETTING_REQ_CMD					=0x23,	//Auto-052
	ICD_GETBATTERYCHARGINGSTATE_REQ_CMD				=0x24,	//Auto-018
	ICD_SETBATTERYCHARGINGSTATE_REQ_CMD				=0x25,	//Auto-054
	ICD_GETBATTERYLEVEL_REQ_CMD						=0x26,	//Auto-019
	ICD_GETBLUETOOTHSTATUS_REQ_CMD					=0x27,	//Auto-020
	ICD_SETBLUETOOTHSTATUS_REQ_CMD					=0x28,	//Auto-053
	ICD_GETGPSSTATUS_REQ_CMD						=0x29,	//Auto-024
	ICD_SETGPSSTATUS_REQ_CMD						=0x2A,	//Auto-055
	ICD_GETKEYPADBACKLIGHT_REQ_CMD					=0x2B,	//Auto-026
	ICD_SETKEYPADBACKLIGHT_REQ_CMD					=0x2C,	//Auto-056
	ICD_GETROAMINGMODE_REQ_CMD						=0x30,	//Auto-037
	ICD_GETSTATEANDCONNECTIONATTEMPTS_REQ_CMD		=0x32,	//Auto-042
	ICD_GETUISCREENID_REQ_CMD						=0x33,	//Auto-204 ~ Auto214
	ICD_GETWIFISTATUS_REQ_CMD						=0x35,	//Auto-045
	ICD_SETWIFISTATUS_REQ_CMD						=0x36,	//Auto-059
	ICD_SETDISCHARGING_REQ_CMD						=0x37,	
	ICD_SETSCREENORIENTATIONLOCK_REQ_CMD			=0x38,	//
	ICD_GETRSSI_REQ_CMD								=0x39,	//Auto-038
//  20111125 [begin] suhyun.lee@lge.com SLATE ICD 231,232,238 Merge From LS840 
	ICD_GETUSBDEBUGSTATUSSTATUS_REQ_CMD				=0x3D,		//Auto-231
	ICD_SETUSBDEBUGSTATUSSTATUS_REQ_CMD				=0x3E,	//Auto-238
	ICD_GETLATITUDELONGITUDEVALUES_REQ_CMD	=0X3F,    //Auto-235
	ICD_GETSCREENLOCKSTATUS_REQ_CMD					=0x40,	//Auto-232
	ICD_SETSCREENLOCKSTATUS_REQ_CMD					=0x41,	//Auto-239
//  20111125 [end] suhyun.lee@lge.com SLATE ICD 231,232,238 Merge From LS840
	ICD_GETANDORIDIDENTIFIER_REQ_CMD				=0x47, 
	/** ICDR : ICD Implementation Recommendation  - END **/

	ICD_MAX_REQ_CMD									=0xff,
}icd_sub_cmd_type;

/** ICD requset type**/
typedef struct
{
	unsigned char cmd_code;
	unsigned char sub_cmd;
} PACKED icd_req_hdr_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char screen_id;
} PACKED icd_handset_disp_text_req_type;

//slate ICD type image capture
typedef enum
{
	LastImageNoHeader				= 0x00,
	LastImageHeader					= 0x01,		
	ContinueImageNoHeader			= 0x02,
	ContinueImageHeader				= 0x03,	
}icd_rsp_flag;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char screen_id;
	unsigned short expected_width;
	unsigned short expected_height;
	unsigned short upper_left_x;
	unsigned short upper_left_y;
	unsigned short lower_right_x;
	unsigned short lower_right_y;
	unsigned char bit_per_pixel;
	unsigned short seq_num;
} PACKED icd_screen_capture_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char airplane_mode;
} PACKED icd_set_airplane_mode_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned short item_data;
} PACKED icd_set_backlight_setting_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char charging_state;
} PACKED icd_set_battery_charging_state_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char bluetooth_status;
} PACKED icd_set_bluetooth_status_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char gps_status;
} PACKED icd_set_gps_status_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned short keypad_backlight;
} PACKED icd_set_keypadbacklight_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char physical_screen;
} PACKED icd_get_ui_screen_id_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char wifi_status;
} PACKED icd_set_wifi_status_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char orientation_mode;
} PACKED icd_set_screen_orientationlock_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	uint32 off_time;
} icd_set_discharger_req_type;
typedef struct {
	icd_req_hdr_type hdr;
}PACKED icd_get_usbdebug_status_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char usbdebug_status;
}PACKED icd_set_usbdebug_status_req_type;

typedef struct {
	icd_req_hdr_type hdr;
}PACKED icd_get_screenlock_status_req_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char msl[6];
	unsigned char screenlock_status;
}PACKED icd_set_screenlock_status_req_type;

typedef struct {
	icd_req_hdr_type hdr;
}PACKED icd_get_android_identifier_req_type;

typedef union
{
	icd_handset_disp_text_req_type disp_req_info;
	icd_screen_capture_req_type capture_req_info;
	icd_set_airplane_mode_req_type set_aiplane_mode_req_info;
	icd_set_backlight_setting_req_type set_backlight_setting_req_info;
	icd_set_battery_charging_state_req_type set_battery_charging_state_req_info;
	icd_set_bluetooth_status_req_type set_bluetooth_status_req_info;
	icd_set_gps_status_req_type set_gps_status_req_info;
	icd_set_keypadbacklight_req_type set_keypadbacklight_req_info;
	icd_get_ui_screen_id_req_type get_ui_srceen_id_req_info;
	icd_set_wifi_status_req_type set_wifi_status_req_info;
	icd_set_screen_orientationlock_req_type set_screenorientationlock_req_info;
	icd_set_discharger_req_type set_discharger_req_info;
	icd_get_usbdebug_status_req_type get_usbdebug_status_req_info;
	icd_set_usbdebug_status_req_type set_usbdebug_status_req_info;
	icd_get_screenlock_status_req_type get_screenlock_status_req_info;
	icd_set_screenlock_status_req_type set_screenlock_status_req_info;
	icd_get_android_identifier_req_type get_android_identifier_req_info;
} PACKED icd_req_type;

typedef union{
	icd_req_hdr_type hdr;
	icd_req_type icd_req;
} PACKED DIAG_ICD_F_req_type;
/** ICD requset type**/

/** ICD response type**/
typedef struct {
	icd_req_hdr_type hdr;
	char manf_string[ICD_MAX_STRING];
	char model_string[ICD_MAX_STRING];
	char hw_ver_string[ICD_MAX_STRING];
	char sw_ver_string[ICD_MAX_STRING];
} PACKED icd_device_info_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	char ver_string[ICD_DISP_TEXT_MAX_STRING];
} PACKED icd_extended_info_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char screen_id;
	//char text_string[ICD_DISP_TEXT_MAX_STRING];
} PACKED icd_handset_disp_text_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char screen_id;
	unsigned short actual_width;
	unsigned short actual_height;	
	unsigned short upper_left_x;
	unsigned short upper_left_y;
	unsigned short lower_right_x;
	unsigned short lower_right_y;
	unsigned char flags;
	unsigned char bit_per_pixel;
	unsigned short seq_num;
//	byte image_data_block[ICD_SEND_BUF_SIZE];
	byte image_data_block[ICD_SEND_SAVE_IMG_PATH_LEN];//Slate_ADB
} PACKED icd_screen_capture_rsp_type;


typedef struct {
	icd_req_hdr_type hdr;
	unsigned char airplane_mode;
} PACKED icd_get_airplane_mode_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
} PACKED icd_set_airplane_mode_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned short item_data;
} PACKED icd_get_backlight_setting_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
} PACKED icd_set_backlight_setting_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char charging_state;
} PACKED icd_get_battery_charging_state_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
} PACKED icd_set_battery_charging_state_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char battery_level;
} PACKED icd_get_battery_level_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char bluetooth_status;
} PACKED icd_get_bluetooth_status_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
} PACKED icd_set_bluetooth_status_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char gps_status;
} PACKED icd_get_gps_status_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
} PACKED icd_set_gps_status_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned short keypad_backlight;
} PACKED icd_get_keypadbacklight_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
} PACKED icd_set_keypadbacklight_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char roaming_mode;
} PACKED icd_get_roamingmode_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char status;
	unsigned char numbar;
	unsigned short rx_power;
	unsigned short rx_ec_io;
} PACKED icd_get_rssi_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	char latitude_longitude[20];
	/*
	char latitude[5];
	char longitude[5];
	*/
} PACKED icd_get_latitude_longitude_values_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char at_state;
	unsigned char session_state;
	unsigned char almp_state;
	unsigned char init_state;
	unsigned char idle_state;
	unsigned char conn_state;
	unsigned char rup_state;
	unsigned char ovhd_state;
	char hybrid_mode;
	
	unsigned char trans_id;
	unsigned char msg_seq;
	unsigned char result;
	unsigned short duration;
	unsigned int success_count;
	unsigned int failure_count;
	unsigned int attempts_count;
	unsigned short pn;
	unsigned short sector_id_lsw;
	unsigned char sector_id_usw;
	unsigned char color_code;
	unsigned char num_ho;
} PACKED icd_get_state_connect_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char physical_screen;
	unsigned char ui_screen_id;
} PACKED icd_get_ui_screen_id_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char wifi_status;
} PACKED icd_get_wifi_status_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
} PACKED icd_set_wifi_status_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
} PACKED icd_set_screenorientationlock_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
} icd_set_discharger_rsp_type;
//  20111125 [begin] suhyun.lee@lge.com SLATE ICD 231,232,238 Merge From LS840 
typedef struct {
	icd_req_hdr_type hdr;
	unsigned char usbdebug_status;
}PACKED icd_get_usbdebug_status_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
}PACKED icd_set_usbdebug_status_rsp_type;

typedef struct {
	icd_req_hdr_type hdr;
	unsigned char screenlock_status;	
}PACKED icd_get_screenlock_status_rsp_type;
typedef struct {
	icd_req_hdr_type hdr;
	unsigned char cmd_status;
}PACKED icd_set_screenlock_status_rsp_type;
//  20111125 [end] suhyun.lee@lge.com SLATE ICD 231,232,238 Merge From LS840 
typedef struct {
	icd_req_hdr_type hdr;
	char android_id_string[VARIABLE];
}PACKED icd_get_android_identifier_rsp_type;

typedef union
{
	icd_device_info_rsp_type dev_rsp_info;
	icd_extended_info_rsp_type extended_rsp_info;
	icd_handset_disp_text_rsp_type disp_rsp_info;
	icd_screen_capture_rsp_type capture_rsp_info;
	icd_get_airplane_mode_rsp_type get_airplane_mode_rsp_info;
	icd_set_airplane_mode_rsp_type set_airplane_mode_rsp_info;
	icd_get_backlight_setting_rsp_type get_backlight_setting_rsp_info;
	icd_set_backlight_setting_rsp_type set_backlight_setting_rsp_info;
	icd_get_battery_charging_state_rsp_type get_battery_charging_state_rsp_info;
	icd_set_battery_charging_state_rsp_type set_battery_charging_state_rsp_info;
	icd_get_battery_level_rsp_type get_battery_level_rsp_info;
	icd_get_bluetooth_status_rsp_type get_bluetooth_status_rsp_info;
	icd_set_bluetooth_status_rsp_type set_bluetooth_status_rsp_info;
	icd_get_gps_status_rsp_type get_gps_status_rsp_info;
	icd_set_gps_status_rsp_type set_gps_status_rsp_info;
	icd_get_keypadbacklight_rsp_type get_keypadbacklight_rsp_info;
	icd_set_keypadbacklight_rsp_type set_keypadbacklight_rsp_info;
	icd_get_roamingmode_rsp_type get_roamingmode_rsp_info;
	icd_get_rssi_rsp_type get_rssi_rsp_info;
	icd_get_state_connect_rsp_type get_state_connect_rsp_info;
	icd_get_ui_screen_id_rsp_type get_ui_screen_id_rsp_info;
	icd_get_wifi_status_rsp_type get_wifi_status_rsp_info;
	icd_set_wifi_status_rsp_type set_wifi_status_rsp_info;
	icd_set_screenorientationlock_rsp_type set_screenorientation_rsp_info;
	icd_set_discharger_rsp_type set_discharger_rsp_info;
	icd_get_usbdebug_status_rsp_type get_usbdebug_status_rsp_info;
	icd_set_usbdebug_status_rsp_type set_usbdebug_status_rsp_info;
	icd_get_screenlock_status_rsp_type get_screenlock_status_rsp_info;
	icd_set_screenlock_status_rsp_type set_screenlock_status_rsp_info;
	icd_get_latitude_longitude_values_rsp_type get_latitude_longitude_values_rsp_info;
	icd_get_android_identifier_rsp_type get_android_identifier_rsp_info;
} PACKED icd_rsp_type;

typedef union
{
	icd_req_hdr_type hdr;
	icd_rsp_type icd_rsp;
} PACKED DIAG_ICD_F_rsp_type;
/** ICD response type**/

// define which processor will handle the sub commands
#if !defined (ARM9_PROCESSOR) && !defined (ARM11_PROCESSOR)
typedef enum{
	ICD_ARM9_PROCESSOR = 0,
	ICD_ARM11_PROCESSOR = 1,
	ICD_ARM9_ARM11_BOTH = 2,
	ICD_NOT_SUPPORTED = 0xFF
}icd_which_processor_type;
#endif

typedef DIAG_ICD_F_rsp_type*(* icd_func_type)(DIAG_ICD_F_req_type *);

typedef struct
{
	unsigned short cmd_code;
	icd_func_type func_ptr;
	unsigned char which_procesor;             // to choose which processor will do act.
}icd_user_table_entry_type;

/*===========================================================================
                      INTERNAL FUNCTION DEFINITIONS
===========================================================================*/


#endif /* DIAGMTC_H */
