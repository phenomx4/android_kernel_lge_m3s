#include <linux/module.h>
#include <linux/input.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fcntl.h> 
#include <linux/fs.h>
#include <linux/uaccess.h>

#include <mach/lge_diag_communication.h>
#include <mach/lge_diag_testmode.h>
#include <mach/lge_diagcmd.h>
#include <mach/lge_backup_items.h>

#include <linux/gpio.h>
#include <mach/board_lge.h>
/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode [START] */
#include <linux/parser.h>
#define WL_IS_WITHIN(min,max,expr)         (((min)<=(expr))&&((max)>(expr)))
/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode [END] */
//#define SYS_GPIO_SD_DET

#define FACTORY_RESET_STR       "FACT_RESET_"
#define FACTORY_RESET_STR_SIZE	11
#define FACTORY_RESET_BLK 1 // read / write on the first block

//[20101010] NFC, addy.kim@lge.com [START]
#define NFC_RESULT_PATH 	"/sys/class/lg_fw_diagclass/lg_fw_diagcmd/nfc_testmode_result"
//[20101010] NFC, addy.kim@lge.com [END]

//accel testmode 8.8
#define ACCEL_ENABLE "/sys/devices/platform/i2c-gpio.8/i2c-8/8-0008/bma222_enable"


#define MSLEEP_CNT 100

//typedef struct MmcPartition MmcPartition;
typedef struct MmcPartition {
    char *device_index;
    char *filesystem;
    char *name;
    unsigned dstatus;
    unsigned dtype ;
    unsigned dfirstsec;
    unsigned dsize;
} MmcPartition;

struct statfs_local {
 	__u32 f_type;
 	__u32 f_bsize;
 	__u32 f_blocks;
 	__u32 f_bfree;
 	__u32 f_bavail;
 	__u32 f_files;
 	__u32 f_ffree;
 	__kernel_fsid_t f_fsid;
 	__u32 f_namelen;
 	__u32 f_frsize;
 	__u32 f_spare[5];
};

typedef struct {
	char ret[32];
} testmode_rsp_from_diag_type;

uint8_t if_condition_is_on_air_plain_mode = 0;

char key_buf[MAX_KEY_BUFF_SIZE];
boolean if_condition_is_on_key_buffering = FALSE;
int count_key_buf = 0;


static struct diagcmd_dev *diagpdev;
static unsigned char test_mode_factory_reset_status = FACTORY_RESET_START;


extern int db_integrity_ready;
extern int fpri_crc_ready;
extern int file_crc_ready;
extern int code_partition_crc_ready;
extern int total_crc_ready;
extern int db_dump_ready;	
extern int db_copy_ready;
extern testmode_rsp_from_diag_type integrity_ret;

extern PACK(void *) diagpkt_alloc (diagpkt_cmd_code_type code, unsigned int length);
extern PACK(void *) diagpkt_free (PACK(void *)pkt);
extern void send_to_arm9( void*	pReq, void	*pRsp, unsigned int rsp_len);
extern void send_SMS_to_arm9( void*	pReq, void	*pRsp, unsigned int rsp_len);
extern testmode_user_table_entry_type testmode_mstr_tbl[TESTMODE_MSTR_TBL_SIZE];
extern int diag_event_log_start(void);
extern int diag_event_log_end(void);
extern unsigned int LGF_KeycodeTrans(word input);
extern void LGF_SendKey(word keycode);
extern void set_operation_mode(boolean isOnline);
extern struct input_dev* get_ats_input_dev(void);
extern int boot_complete_info;
extern void remote_set_operation_mode(int info);
extern void remote_set_ftm_boot(int info);
extern void remote_set_ftmboot_reset(uint32 info);

extern int lge_erase_block(int secnum, size_t size);
extern int lge_write_block(int secnum, unsigned char *buf, size_t size);
extern int lge_read_block(int secnum, unsigned char *buf, size_t size);
extern int lge_mmc_scan_partitions(void);
extern const MmcPartition *lge_mmc_find_partition_by_name(const char *name);


PACK (void *)LGF_TestMode(PACK (void*)req_pkt_ptr, uint16 pkt_len);
void* LGF_TestModeFactoryReset(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_TestMotor(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type *pRsp);
void* LGF_TestAcoustic(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_TestCam(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGT_TestModeKeyTest(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type *pRsp);
void* LGF_ExternalSocketMemory(	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp);
#ifndef LG_BTUI_TEST_MODE
void* LGF_TestModeBlueTooth(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
#endif
void* LGF_TestModeMP3(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_MemoryFormatTest(test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type *pRsp);
void* LGF_TestModeKeyData(test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type *pRsp);
void* LGF_MemoryVolumeCheck(test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type *pRsp);
void* LGF_PowerSaveMode(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_PowerSaveMode(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_TestModeSpeakerPhone(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_TestScriptItemSet(test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp);
void* LGT_TestModeVolumeLevel(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_TestModeFboot(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_TestModeDBIntegrityCheck(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_TestModeFOTAIDCheck(test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_TestModePowerReset(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void* LGF_TestModeKEYLOCK(test_mode_req_type * pReq, DIAG_TEST_MODE_F_rsp_type * pRsp);
void* LGF_TestModeAccel(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);
void * LGF_TestModeManualModeSet(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp);

//[20111006] addy.kim@lge.com [START]
void* LGF_TestModeNFC( test_mode_req_type*	pReq, DIAG_TEST_MODE_F_rsp_type	*pRsp);
//[20111006] addy.kim@lge.com [END]		

void* linux_app_handler(test_mode_req_type*	pReq, DIAG_TEST_MODE_F_rsp_type	*pRsp);

void* LGF_TestModeMLTEnableSet(test_mode_req_type * pReq, DIAG_TEST_MODE_F_rsp_type * pRsp);

/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode function [START] */
typedef struct _rx_packet_info 
{
	int goodpacket;
	int badpacket;
} rx_packet_info_t;

enum {
	Param_none = -1,
	Param_goodpacket,
	Param_badpacket,
	Param_end,
	Param_err,
};

static const match_table_t param_tokens = {
	{Param_goodpacket, "good=%d"},
	{Param_badpacket, "bad=%d"},
	{Param_end,	"END"},
	{Param_err, NULL}
};

void* LGF_TestModeWLAN(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type *pRsp);
/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode function [END] */

//LGE_FW_LCD_K_CAL
void* LGF_TestLCD_Cal(
		test_mode_req_type* pReq ,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	char ptr[30];
	
	pRsp->ret_stat_code = TEST_OK_S;
	printk("<6>" "pReq->lcd_cal: (%d)\n", pReq->lcd_cal.MaxRGB[0]);

	if (diagpdev != NULL){
		if (pReq->lcd_cal.MaxRGB[0] != 5)
			update_diagcmd_state(diagpdev, "LCD_Cal", pReq->lcd_cal.MaxRGB[0]);
		else {
			printk("<6>" "pReq->MaxRGB string type : %s\n",pReq->lcd_cal.MaxRGB);
        	sprintf(ptr,"LCD_Cal,%s",&pReq->lcd_cal.MaxRGB[1]);
			printk("<6>" "%s \n", ptr);
			update_diagcmd_state(diagpdev, ptr, pReq->lcd_cal.MaxRGB[0]);
		}
	}
	else
	{
		printk("\n[%s] error LCD_cal", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
	return pRsp;
}

testmode_user_table_entry_type testmode_mstr_tbl[TESTMODE_MSTR_TBL_SIZE] =
{
	/* sub command							func_ptr						processor type */
	/* 0 ~ 5   */
	{TEST_MODE_VERSION,						NULL,							ARM9_PROCESSOR},
	{TEST_MODE_LCD,							linux_app_handler,				ARM11_PROCESSOR},
//LGE_FW_LCD_K_CAL
	{TEST_MODE_LCD_CAL,       LGF_TestLCD_Cal,  ARM11_PROCESSOR },
//	{TEST_MODE_LCD_CAL,       linux_app_handler,  ARM11_PROCESSOR },

	{TEST_MODE_MOTOR,						LGF_TestMotor,					ARM11_PROCESSOR},
	{TEST_MODE_ACOUSTIC,					LGF_TestAcoustic,				ARM11_PROCESSOR},
	/* 6 ~ 10  */
	{TEST_MODE_CAM,							LGF_TestCam,					ARM11_PROCESSOR},
	/* 11 ~ 15 */
	/* 16 ~ 20 */
	/* 21 ~ 25 */
	{TEST_MODE_KEY_TEST,					LGT_TestModeKeyTest,			ARM11_PROCESSOR},	
	{TEST_MODE_EXT_SOCKET_TEST,          	LGF_ExternalSocketMemory, 		ARM11_PROCESSOR},
#ifndef LG_BTUI_TEST_MODE
	{TEST_MODE_BLUETOOTH_TEST,           	LGF_TestModeBlueTooth,    		ARM11_PROCESSOR},
#endif
	/* 26 ~ 30 */
	{TEST_MODE_MP3_TEST,                 	LGF_TestModeMP3,          		ARM11_PROCESSOR},
	/* 31 ~ 35 */
	{TEST_MODE_ACCEL_SENSOR_TEST,        	linux_app_handler,        		ARM11_PROCESSOR},
	//20111006,  addy.kim@lge.com,	[START] 
	{ TEST_MODE_ALCOHOL_SENSOR_TEST,		LGF_TestModeNFC,				ARM11_PROCESSOR},		
	//20111006, addy.kim@lge.com,	[END]	
	/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode function [START] */
	{TEST_MODE_WIFI_TEST,                	LGF_TestModeWLAN,        		ARM11_PROCESSOR},
	/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode function [END] */
	/* 36 ~ 40 */
	{TEST_MODE_MANUAL_MODE_TEST, LGF_TestModeManualModeSet, ARM11_PROCESSOR},
	{TEST_MODE_KEY_DATA_TEST,            	LGF_TestModeKeyData,        	ARM11_PROCESSOR},
	{TEST_MODE_FACTORY_RESET_CHECK_TEST,	LGF_TestModeFactoryReset,		ARM11_PROCESSOR},
	/* 41 ~ 45 */
	{TEST_MODE_MEMORY_CAPA_TEST,         	LGF_MemoryVolumeCheck,    		ARM11_PROCESSOR},
	{TEST_MODE_SLEEP_MODE_TEST,          	LGF_PowerSaveMode,        		ARM11_PROCESSOR},
	{TEST_MODE_SPEAKER_PHONE_TEST,       	LGF_TestModeSpeakerPhone, 		ARM11_PROCESSOR},
	/* 46 ~ 50 */
    {TEST_MODE_VCO_SELF_TUNNING_TEST,       NULL,                     		ARM9_PROCESSOR},
	{TEST_MODE_MRD_USB_TEST,             	NULL,                     		ARM9_PROCESSOR},
	{TEST_MODE_TEST_SCRIPT_MODE,         	LGF_TestScriptItemSet,    		ARM11_PROCESSOR},
	{TEST_MODE_TEST_SCRIPT_MODE,         	NULL,							ARM9_PROCESSOR},

	{TEST_MODE_PROXIMITY_SENSOR_TEST,    	linux_app_handler,        		ARM11_PROCESSOR},
	{TEST_MODE_FORMAT_MEMORY_TEST,		  	LGF_MemoryFormatTest,			ARM11_PROCESSOR},
	/* 51 ~ 55 */
	{TEST_MODE_VOLUME_TEST,              	LGT_TestModeVolumeLevel,  		ARM11_PROCESSOR},
	{TEST_MODE_CGPS_MEASURE_CNO,			NULL,							ARM9_PROCESSOR},
	/* 56 ~ 60 */
	{TEST_MODE_FIRST_BOOT_COMPLETE_TEST,  	LGF_TestModeFboot,        		ARM11_PROCESSOR},
	{TEST_MODE_LED_TEST, 					linux_app_handler,				ARM11_PROCESSOR},
	/* 61 ~ 65 */
	/* 66 ~ 70 */
	{TEST_MODE_PID_TEST,             	 	NULL,  							ARM9_PROCESSOR},
	/* 71 ~ 75 */
	{TEST_MODE_SW_VERSION, 					NULL, 							ARM9_PROCESSOR},
	{TEST_MODE_IME_TEST, 					NULL, 							ARM9_PROCESSOR},
	{TEST_MODE_IMPL_TEST, 					NULL, 							ARM9_PROCESSOR},
	{TEST_MODE_UNLOCK_CODE_TEST, 			NULL, 							ARM9_PROCESSOR},
	/* 76 ~ 80 */
	{TEST_MODE_IDDE_TEST, 					NULL, 							ARM9_PROCESSOR},
	{TEST_MODE_FULL_SIGNATURE_TEST, 		NULL, 							ARM9_PROCESSOR},
	{TEST_MODE_NT_CODE_TEST, 				NULL, 							ARM9_PROCESSOR},
	/* 81 ~ 85 */
	{TEST_MODE_CAL_CHECK, 					NULL,							ARM9_PROCESSOR},
#ifndef LG_BTUI_TEST_MODE
	{TEST_MODE_BLUETOOTH_TEST_RW,			NULL,							ARM9_PROCESSOR},
#endif
	/* 86 ~ 90 */
	{TEST_MODE_SKIP_WELCOM_TEST, 			NULL,							ARM9_PROCESSOR},
	{TEST_MODE_MAC_READ_WRITE,    			linux_app_handler,        		ARM11_PROCESSOR},
	/* 91 ~ 95 */
	{TEST_MODE_DB_INTEGRITY_CHECK,         	LGF_TestModeDBIntegrityCheck,	ARM11_PROCESSOR},
	{TEST_MODE_NVCRC_CHECK, 				NULL,							ARM9_PROCESSOR},
	{TEST_MODE_RELEASE_CURRENT_LIMIT,		NULL,							ARM9_PROCESSOR},
	/* 96 ~ 100*/
	{TEST_MODE_RESET_PRODUCTION, 			NULL,							ARM9_PROCESSOR},
    //Virigin not support FOTA {TEST_MODE_FOTA_ID_CHECK, 				LGF_TestModeFOTAIDCheck,		ARM11_PROCESSOR},
	{TEST_MODE_KEY_LOCK,					LGF_TestModeKEYLOCK,			ARM11_PROCESSOR},
    {TEST_MODE_SENSOR_CALIBRATION_TEST,     linux_app_handler,              ARM11_PROCESSOR},
	{TEST_MODE_ACCEL_SENSOR_ONOFF_TEST,	LGF_TestModeAccel,			ARM11_PROCESSOR},
	{TEST_MODE_COMPASS_SENSOR_TEST,		linux_app_handler,			ARM11_PROCESSOR},
	{TEST_MODE_GNSS_MEASURE_CNO, 			NULL,							ARM9_PROCESSOR},
	{TEST_MODE_POWER_RESET, 				LGF_TestModePowerReset,	ARM11_PROCESSOR},
//johny.kim
    {TEST_MODE_MLT_ENABLE,                  LGF_TestModeMLTEnableSet,         ARM11_PROCESSOR},
//johny.kim

};

#if 1//def LGE_USB_ACCESS_LOCK
#define SECURITY_BINARY_CODE	696

PACK (void *)LGF_TFSBProcess (
			PACK (void	*)req_pkt_ptr,	/* pointer to request packet  */
			uint16		pkt_len )			  /* length of request packet	*/
{
	DIAG_TF_SB_F_req_type *req_ptr = (DIAG_TF_SB_F_req_type *) req_pkt_ptr;
	DIAG_TF_SB_F_rsp_type *rsp_ptr;
	unsigned int rsp_len;

	extern void set_usb_lock(int lock);

	if(req_ptr->seccode != SECURITY_BINARY_CODE)
	{
		set_usb_lock(1);
	}

	rsp_len = sizeof(DIAG_TF_SB_F_rsp_type);
	rsp_ptr = (DIAG_TF_SB_F_rsp_type *)diagpkt_alloc(DIAG_TF_SB_CMD_F, rsp_len);

	rsp_ptr->cmd_code = req_ptr->cmd_code;	
	return (rsp_ptr);
}
EXPORT_SYMBOL(LGF_TFSBProcess);   

PACK (void *)LGF_TFProcess (
			PACK (void	*)req_pkt_ptr,	/* pointer to request packet  */
			uint16		pkt_len )			  /* length of request packet	*/
{
	DIAG_TF_F_req_type *req_ptr = (DIAG_TF_F_req_type *) req_pkt_ptr;
	DIAG_TF_F_rsp_type *rsp_ptr;
	unsigned int rsp_len;
	extern int get_usb_lock(void);
	extern void set_usb_lock(int lock);
	extern void get_spc_code(char * spc_code);

	rsp_len = sizeof(DIAG_TF_F_rsp_type);
	rsp_ptr = (DIAG_TF_F_rsp_type *)diagpkt_alloc(DIAG_TF_CMD_F, rsp_len);
	rsp_ptr->sub_cmd = req_ptr->sub_cmd;
	
	switch(req_ptr->sub_cmd)
	{
		case TF_SUB_CHECK_PORTLOCK:
			if (get_usb_lock())
				rsp_ptr->result = TF_STATUS_PORT_UNLOCK;
			else
				rsp_ptr->result = TF_STATUS_PORT_LOCK;
			break;

		case TF_SUB_LOCK_PORT:
			set_usb_lock(1);
			rsp_ptr->result = TF_STATUS_SUCCESS;
			break;

		case TF_SUB_UNLOCK_PORT:
		{
			char spc_code[6];
			
			get_spc_code(spc_code);

			if (memcmp((byte *)spc_code,req_ptr->buf.keybuf, PPE_UART_KEY_LENGTH )==0)
			{
				set_usb_lock(0);
				rsp_ptr->result = TF_STATUS_SUCCESS;
			}
			else
				rsp_ptr->result = TF_STATUS_FAIL;

			break;
		}
	}

	return (rsp_ptr);
}
EXPORT_SYMBOL(LGF_TFProcess);
#endif

PACK (void *)LGF_TestMode(PACK (void*)req_pkt_ptr, uint16 pkt_len)
{
	DIAG_TEST_MODE_F_req_type *req_ptr = (DIAG_TEST_MODE_F_req_type *) req_pkt_ptr;
  	DIAG_TEST_MODE_F_rsp_type *rsp_ptr;
	unsigned int rsp_len=0;
  	testmode_func_type func_ptr= NULL;
  	int nIndex = 0;
	int is_valid_arm9_command = 1;

  	diagpdev = diagcmd_get_dev();

	for(nIndex = 0 ; nIndex < TESTMODE_MSTR_TBL_SIZE  ; nIndex++)
  	{
    	if(testmode_mstr_tbl[nIndex].cmd_code == req_ptr->sub_cmd_code)
    	{
        	if( testmode_mstr_tbl[nIndex].which_procesor == ARM11_PROCESSOR)
          		func_ptr = testmode_mstr_tbl[nIndex].func_ptr;
      		break;
    	}
		else if(testmode_mstr_tbl[nIndex].cmd_code == MAX_TEST_MODE_SUBCMD)
		{
			break;
		}
		else
		{
			continue;
		}
  	}

	if( func_ptr != NULL)
	{
		switch(req_ptr->sub_cmd_code) {
			case TEST_MODE_FACTORY_RESET_CHECK_TEST:
				rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type) - sizeof(test_mode_rsp_type);
				break;

//BGH			case TEST_MODE_TEST_SCRIPT_MODE:
//BGH      			rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type) - sizeof(test_mode_rsp_type) + sizeof(test_mode_req_test_script_mode_type);
//BGH      			break;		

			default:
				rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type);
				break;
		}

		rsp_ptr = (DIAG_TEST_MODE_F_rsp_type *)diagpkt_alloc(DIAG_TEST_MODE_F, rsp_len);
		if(rsp_ptr == NULL)
        	return rsp_ptr;

		rsp_ptr->sub_cmd_code = req_ptr->sub_cmd_code;
  		rsp_ptr->ret_stat_code = TEST_OK_S; // test ok	
	
		return func_ptr( &(req_ptr->test_mode_req), rsp_ptr);
	}
	else
	{
		switch(req_ptr->sub_cmd_code) {
			case TEST_MODE_VERSION:
			case TEST_MODE_VCO_SELF_TUNNING_TEST:	
			case TEST_MODE_MRD_USB_TEST:	
			case TEST_MODE_RESET_PRODUCTION: 
			case TEST_MODE_BATT_TEST:
			case TEST_MODE_CAL_CHECK:
			case TEST_MODE_PID_TEST:
			case TEST_MODE_RELEASE_CURRENT_LIMIT:
			case TEST_MODE_CGPS_MEASURE_CNO:
			case TEST_MODE_GNSS_MEASURE_CNO:
			case TEST_MODE_SW_VERSION:
			case TEST_MODE_TEST_SCRIPT_MODE:				
			case TEST_MODE_BLUETOOTH_TEST_RW:			
				rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type);
				break;	

			default:
				is_valid_arm9_command=0;
				break;
		}
		
		if(is_valid_arm9_command == 1)
		{
			rsp_ptr = (DIAG_TEST_MODE_F_rsp_type *)diagpkt_alloc(DIAG_TEST_MODE_F, rsp_len);
			if(rsp_ptr == NULL)
				return rsp_ptr;

			rsp_ptr->sub_cmd_code = req_ptr->sub_cmd_code;
  			rsp_ptr->ret_stat_code = TEST_OK_S; // test ok	
			
			send_to_arm9((void*)req_ptr, (void*)rsp_ptr, rsp_len);
		}
	}

	return (rsp_ptr);
}
EXPORT_SYMBOL(LGF_TestMode);

void* linux_app_handler(test_mode_req_type*	pReq, DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	diagpkt_free(pRsp);
  	return 0;
}

/* LCD QTEST */
PACK (void *)LGF_LcdQTest (
        PACK (void	*)req_pkt_ptr,	/* pointer to request packet  */
        uint16		pkt_len )		      /* length of request packet   */
{
	printk("[%s] LGF_LcdQTest\n", __func__ );

	/* Returns 0 for executing lg_diag_app */
	return 0;
}
EXPORT_SYMBOL(LGF_LcdQTest);

#ifdef CONFIG_LGE_PCB_VERSION
byte CheckHWRev(void)
{
	// request packet of send_to_arm9 should be DIAG_TEST_MODE_F_req_type
	//test_mode_req_type Req;
	DIAG_TEST_MODE_F_req_type Req;
	DIAG_TEST_MODE_F_rsp_type Rsp;

	/*
	char *pReq = (char *) &Req;
	char *pRsp = (char *) &Rsp;

	pReq[0] = 250;
	pReq[1] = 0;
	pReq[2] = 0;
	pReq[3] = 8;

	send_to_arm9(pReq , pRsp);
	printk("CheckHWRev> 0x%x 0x%x 0x%x 0x%x 0x%x\n", pRsp[0],pRsp[1],pRsp[2],pRsp[3],pRsp[4]);
	*/

	Req.sub_cmd_code = TEST_MODE_VERSION;
	Req.test_mode_req.version = VER_HW;

	send_to_arm9((void*)&Req, (void*)&Rsp, sizeof(DIAG_TEST_MODE_F_rsp_type));
	/*
	 * previous kmsg : CheckHWRev> 0xfa 0x0 0x0 0x0 0x44
	 * current kmsg : CheckHWRev> 0xfa 0x0 0x0 0x44
	 * last packet matches to the previous, so this modification seems to be working fine
	*/
	printk("CheckHWRev> 0x%x 0x%x 0x%x 0x%x\n", Rsp.xx_header.opaque_header, \
		Rsp.sub_cmd_code, Rsp.ret_stat_code, Rsp.test_mode_rsp.str_buf[0]);

	return Rsp.test_mode_rsp.str_buf[0];
/* END: 0014656 jihoon.lee@lge.com 2011024 */	
}

void CheckHWRevStr(char *buf, int str_size)
{
	DIAG_TEST_MODE_F_req_type Req;
	DIAG_TEST_MODE_F_rsp_type Rsp;

	Req.sub_cmd_code = TEST_MODE_VERSION;
	Req.test_mode_req.version = VER_HW;

	send_to_arm9((void*)&Req, (void*)&Rsp, sizeof(DIAG_TEST_MODE_F_rsp_type));
       memcpy(buf, Rsp.test_mode_rsp.str_buf, (str_size <= sizeof(Rsp.test_mode_rsp.str_buf) ? str_size: sizeof(Rsp.test_mode_rsp.str_buf)));
}
EXPORT_SYMBOL(CheckHWRev);
#endif

boolean lgf_factor_key_test_rsp (char key_code)
{
    /* sanity check */
    if (count_key_buf>=MAX_KEY_BUFF_SIZE)
        return FALSE;

    key_buf[count_key_buf++] = key_code;
    return TRUE;
}
EXPORT_SYMBOL(lgf_factor_key_test_rsp);

void* LGF_TestModeFactoryReset(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	unsigned char pbuf[50]; //no need to have huge size, this is only for the flag
  	const MmcPartition *pMisc_part; 
  	unsigned char startStatus = FACTORY_RESET_NA; 
  	int mtd_op_result = 0;
  	unsigned long factoryreset_bytes_pos_in_emmc = 0;

	/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
  	DIAG_TEST_MODE_F_req_type req_ptr;

  	req_ptr.sub_cmd_code = TEST_MODE_FACTORY_RESET_CHECK_TEST;
  	req_ptr.test_mode_req.factory_reset = pReq->factory_reset;
  
	/* handle operation or rpc failure as well */
  	pRsp->ret_stat_code = TEST_FAIL_S;
  
  	lge_mmc_scan_partitions();
  	pMisc_part = lge_mmc_find_partition_by_name("misc"); // LS696_me : persist -> misc
  	factoryreset_bytes_pos_in_emmc = (pMisc_part->dfirstsec*512)+PTN_FRST_PERSIST_POSITION_IN_MISC_PARTITION;
  
  	printk("LGF_TestModeFactoryReset> mmc info sec : 0x%x, size : 0x%x type : 0x%x frst sec: 0x%lx\n", 
													pMisc_part->dfirstsec, pMisc_part->dsize, pMisc_part->dtype, factoryreset_bytes_pos_in_emmc);

	/* MOD 0013861: [FACTORY RESET] emmc_direct_access factory reset flag access */
	/* add carriage return and change flag size for the platform access */
  	switch(pReq->factory_reset)
  	{
    	case FACTORY_RESET_CHECK :
			/* MOD 0014110: [FACTORY RESET] stability */
			/* handle operation or rpc failure as well */
      		memset((void*)pbuf, 0, sizeof(pbuf));
      		mtd_op_result = lge_read_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+2);

      		if( mtd_op_result != (FACTORY_RESET_STR_SIZE+2) )
      		{
        		printk(KERN_ERR "[Testmode]lge_read_block, read data  = %d \n", mtd_op_result);
        		pRsp->ret_stat_code = TEST_FAIL_S;
        		break;
      		}
      		else
      		{
        		//printk(KERN_INFO "\n[Testmode]factory reset memcmp\n");
        		if(memcmp(pbuf, FACTORY_RESET_STR, FACTORY_RESET_STR_SIZE) == 0) // tag read sucess
        		{
          			startStatus = pbuf[FACTORY_RESET_STR_SIZE] - '0';
          			printk(KERN_INFO "[Testmode]factory reset backup status = %d \n", startStatus);
        		}
        		else
        		{
          			// if the flag storage is erased this will be called, start from the initial state
          			printk(KERN_ERR "[Testmode] tag read failed :  %s \n", pbuf);
        		}
      		}  

	      	test_mode_factory_reset_status = FACTORY_RESET_INITIAL;
      		memset((void *)pbuf, 0, sizeof(pbuf));
      		sprintf(pbuf, "%s%d\n",FACTORY_RESET_STR, test_mode_factory_reset_status);
      		printk(KERN_INFO "[Testmode]factory reset status = %d\n", test_mode_factory_reset_status);

      		mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc, FACTORY_RESET_STR_SIZE+2);	

			/* handle operation or rpc failure as well */
      		if(mtd_op_result!= (FACTORY_RESET_STR_SIZE+2))
      		{
        		printk(KERN_ERR "[Testmode]lge_erase_block, error num = %d \n", mtd_op_result);
        		pRsp->ret_stat_code = TEST_FAIL_S;
        		break;
      		}
      		else
      		{	
        		mtd_op_result = lge_write_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+2);
        		if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
        		{
          			printk(KERN_ERR "[Testmode]lge_write_block, error num = %d \n", mtd_op_result);
          			pRsp->ret_stat_code = TEST_FAIL_S;
          			break;
        		}
      		}

#if 1 
			/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
      		//send_to_arm9((void*)(((byte*)pReq) -sizeof(diagpkt_header_type) - sizeof(word)) , pRsp);
      		send_to_arm9((void*)&req_ptr, (void*)pRsp, sizeof(DIAG_TEST_MODE_F_rsp_type) - sizeof(test_mode_rsp_type));

			/* handle operation or rpc failure as well */
      		if(pRsp->ret_stat_code != TEST_OK_S)
      		{
        		printk(KERN_ERR "[Testmode]send_to_arm9 response : %d\n", pRsp->ret_stat_code);
        		pRsp->ret_stat_code = TEST_FAIL_S;
        		break;
      		}
#endif

      		/*LG_FW khlee 2010.03.04 -If we start at 5, we have to go to APP reset state(3) directly */
      		if( startStatus == FACTORY_RESET_COLD_BOOT_END)
        		test_mode_factory_reset_status = FACTORY_RESET_COLD_BOOT_START;
      		else
        		test_mode_factory_reset_status = FACTORY_RESET_ARM9_END;

      		memset((void *)pbuf, 0, sizeof(pbuf));
      		sprintf(pbuf, "%s%d\n",FACTORY_RESET_STR, test_mode_factory_reset_status);
      		printk(KERN_INFO "[Testmode]factory reset status = %d\n", test_mode_factory_reset_status);

      		mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc, FACTORY_RESET_STR_SIZE+2);
			/* handle operation or rpc failure as well */
      		if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
      		{
        		printk(KERN_ERR "[Testmode]lge_erase_block, error num = %d \n", mtd_op_result);
        		pRsp->ret_stat_code = TEST_FAIL_S;
        		break;
      		}
      		else
      		{
         		mtd_op_result = lge_write_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+2);
         		if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
         		{
          			printk(KERN_ERR "[Testmode]lge_write_block, error num = %d \n", mtd_op_result);
          			pRsp->ret_stat_code = TEST_FAIL_S;
          			break;
         		}
      		}

      		printk(KERN_INFO "%s, factory reset check completed \n", __func__);
      		pRsp->ret_stat_code = TEST_OK_S;
      		break;

		case FACTORY_RESET_COMPLETE_CHECK:
      		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
      		printk(KERN_ERR "[Testmode]not supported\n");
      		break;

    	case FACTORY_RESET_STATUS_CHECK:
      		memset((void*)pbuf, 0, sizeof(pbuf));
      		mtd_op_result = lge_read_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+2 );
			/* handle operation or rpc failure as well */
      		if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
      		{
      	 		printk(KERN_ERR "[Testmode]lge_read_block, error num = %d \n", mtd_op_result);
      	 		pRsp->ret_stat_code = TEST_FAIL_S;
      	 		break;
      		}
      		else
      		{
      	 		if(memcmp(pbuf, FACTORY_RESET_STR, FACTORY_RESET_STR_SIZE) == 0) // tag read sucess
      	 		{
      	   			test_mode_factory_reset_status = pbuf[FACTORY_RESET_STR_SIZE] - '0';
      	   			printk(KERN_INFO "[Testmode]factory reset status = %d \n", test_mode_factory_reset_status);
      	   			pRsp->ret_stat_code = test_mode_factory_reset_status;
      	 		}
      	 		else
      	 		{
      	   			printk(KERN_ERR "[Testmode]factory reset tag fail, set initial state\n");
      	   			test_mode_factory_reset_status = FACTORY_RESET_START;
      	   			pRsp->ret_stat_code = test_mode_factory_reset_status;
      	   			break;
      	 		}
      		}
      		break;

    	case FACTORY_RESET_COLD_BOOT:
      		/* Erase Internal FAT*/
      		update_diagcmd_state(diagpdev, "MMCFORMAT", EXTERNAL_SOCKET_ERASE_FAT_ONLY);		
      		msleep(8000);

			// remove requesting sync to CP as all sync will be guaranteed on their own.
      		test_mode_factory_reset_status = FACTORY_RESET_COLD_BOOT_START;
      		memset((void *)pbuf, 0, sizeof(pbuf));
      		sprintf(pbuf, "%s%d",FACTORY_RESET_STR, test_mode_factory_reset_status);
      		printk(KERN_INFO "[Testmode]factory reset status = %d\n", test_mode_factory_reset_status);
      		mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc,  FACTORY_RESET_STR_SIZE+2);
			/* handle operation or rpc failure as well */
      		if(mtd_op_result!=( FACTORY_RESET_STR_SIZE+2))
      		{
        		printk(KERN_ERR "[Testmode]lge_erase_block, error num = %d \n", mtd_op_result);
        		pRsp->ret_stat_code = TEST_FAIL_S;
        		break;
      		}
      		else
      		{
        		mtd_op_result = lge_write_block(factoryreset_bytes_pos_in_emmc, pbuf,  FACTORY_RESET_STR_SIZE+2);
        		if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
        		{
          			printk(KERN_ERR "[Testmode]lge_write_block, error num = %d \n", mtd_op_result);
          			pRsp->ret_stat_code = TEST_FAIL_S;
        		}
      		}
      		pRsp->ret_stat_code = TEST_OK_S;
      		break;

    	case FACTORY_RESET_ERASE_USERDATA:
      		test_mode_factory_reset_status = FACTORY_RESET_COLD_BOOT_START;
      		memset((void *)pbuf, 0, sizeof(pbuf));
      		sprintf(pbuf, "%s%d",FACTORY_RESET_STR, test_mode_factory_reset_status);
      		printk(KERN_INFO "[Testmode-erase userdata]factory reset status = %d\n", test_mode_factory_reset_status);
      		mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc , FACTORY_RESET_STR_SIZE+2);
			/* handle operation or rpc failure as well */
      		if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
      		{
        		printk(KERN_ERR "[Testmode]lge_erase_block, error num = %d \n", mtd_op_result);
        		pRsp->ret_stat_code = TEST_FAIL_S;
        		break;
      		}
      		else
      		{
        		mtd_op_result = lge_write_block(factoryreset_bytes_pos_in_emmc, pbuf, FACTORY_RESET_STR_SIZE+2);
        		if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))
        		{
          			printk(KERN_ERR "[Testmode]lge_write_block, error num = %d \n", mtd_op_result);
          			pRsp->ret_stat_code = TEST_FAIL_S;
          			break;
        		}
      		}
    		pRsp->ret_stat_code = TEST_OK_S;
    		break;

     	default:
        	pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
        	break;
	}

  	return pRsp;
}

//20110930, addy.kim@lge.com,  [START]

static int lg_diag_nfc_result_file_read(int i_testcode ,int *ptrRenCode, char *sz_extra_buff )
{
	int read;
	int read_size;
	char buf[100] = {0,};

	
	mm_segment_t oldfs;
	

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	printk("[NFC] HELLO START FILE READ");
	read = sys_open((const char __user *)NFC_RESULT_PATH, O_RDONLY , 0);

	if(read < 0) {
		printk(KERN_ERR "%s, NFC Result File Open Fail\n",__func__);
		goto nfc_read_err;
		
	}else {
		printk(KERN_ERR "%s, NFC Result File Open Success\n",__func__);
		 
	}

	read_size = 0;
	printk("[_NFC_] copy read to buf variable From read\n");
	while( sys_read(read, &buf[read_size], 1) == 1){
		//printk("[_NFC_] READ  buf[%d]:%c \n",read_size,buf[read_size]);

		if(buf[read_size] == 0x0A)
		{
				   buf[read_size] = 0x00;
				   break;
		}
		
		read_size++;
	}	

	printk("[_NFC_] READ char %d\n",buf[0]-48);
	
	*ptrRenCode = buf[0]-48; //change ASCII Code to int Number
	
	printk("[_NFC_] lg_diag_nfc_result_file_read : i_result_status == %d\n",*ptrRenCode);

	if((strlen(buf) > 1))
	{
		if(i_testcode == 5 || i_testcode == 7 || i_testcode == 9)
		{
			if(buf == NULL){
				printk(KERN_ERR "ADDY.KIM@lge.com : [_NFC_] BUFF is NULL\n");
				goto nfc_read_err;
			}
			printk("[_NFC_] lg_diag_nfc_result_file_read : Start Copy From szExtraData -> buf buf\n");
			strcpy( sz_extra_buff,(char*)(&buf[1]) );
		}
		else if( i_testcode == 10)
		{
			if(buf == NULL){
				printk(KERN_ERR "ADDY.KIM@lge.com : [_NFC_] BUFF is NULL\n");
				goto nfc_read_err;
			}
			printk("[_NFC_] lg_diag_nfc_result_file_read : Start Copy From szExtraData -> buf buf\n");
			strcpy( sz_extra_buff,(char*)(&buf[1]) );
		}
	}
	
	set_fs(oldfs);
	sys_close(read);

	return 1;

	nfc_read_err:
		set_fs(oldfs);
		sys_close(read);
		return 0;
		
}


void* LGF_TestModeNFC(
		test_mode_req_type*	pReq,
		DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	int nfc_result = 0;
	
	char szExtraData[100] = {0,};
	pRsp->ret_stat_code = TEST_FAIL_S;
	printk(KERN_ERR "ADDY.KIM@lge.com : [_NFC_] [%s:%d] SubCmd=<%d>\n", __func__, __LINE__, pReq->nfc);
/*
	if(pReq->nfc == 7)
	{
		pReq->nfc = 10;
	}
*/
	if (diagpdev != NULL && (pReq->nfc != 3) && (pReq->nfc != 2)){ 
		update_diagcmd_state(diagpdev, "NFC_TEST_MODE", pReq->nfc);
		
		if(pReq->nfc == 0)
			msleep(3000);
		else if(pReq->nfc == 1)
			msleep(6000);
		else if(pReq->nfc == 2)
			msleep(5000);
		else if(pReq->nfc == 3)
			msleep(4000);
		else if(pReq->nfc == 4)
			msleep(7000);
		else if(pReq->nfc == 5)
			msleep(5000);
		else if(pReq->nfc == 6)
			msleep(9000);
		else if(pReq->nfc == 7)
			msleep(5000);
		else
			msleep(7000);		
	
		if( (lg_diag_nfc_result_file_read(pReq->nfc,&nfc_result, szExtraData)) == 0 ){
			pRsp->ret_stat_code = TEST_FAIL_S;
			printk(KERN_ERR "addy.kim@lge.com , [NFC] FIle Open Error");
			goto nfc_test_err;
		}
/*
		if(pReq->nfc == 10)
		{
			pReq->nfc = 7;
		}			
*/
		printk(KERN_ERR "ADDY.KIM@lge.com : [_NFC_] [%s:%d] Result Value=<%d>\n", __func__, __LINE__, nfc_result);

		if( nfc_result == 0 || nfc_result == 9 ){
			pRsp->ret_stat_code = TEST_OK_S;
			printk(KERN_ERR "ADDY.KIM@lge.com : [_NFC_] [%s:%d] DAIG RETURN VALUE=<%s>\n", __func__, __LINE__, "TEST_OK");
		}else if(nfc_result == 1){
			pRsp->ret_stat_code = TEST_FAIL_S;
			printk(KERN_ERR "ADDY.KIM@lge.com : [_NFC_] [%s:%d] DAIG RETURN VALUE=<%s>\n", __func__, __LINE__, "TEST_FAIL");
			goto nfc_test_err;
		}else{
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
			printk(KERN_ERR "ADDY.KIM@lge.com : [_NFC_] [%s:%d] DAIG RETURN VALUE=<%s>\n", __func__, __LINE__, "NOT_SUPPORT");
			goto nfc_test_err;
		}
			
		if( pReq->nfc == 5 || pReq->nfc == 7 || pReq->nfc == 9 || pReq->nfc == 10 )
		{
			if(szExtraData == NULL ){
				printk(KERN_ERR "[_NFC_] [%s:%d] response Data is NULL \n", __func__, __LINE__);
				pRsp->ret_stat_code = TEST_FAIL_S;
				goto nfc_test_err;
			}

			printk("[_NFC_] Start Copy From szExtraData : [%s] -> test_mode_rsp buf\n",szExtraData);


			//save data to response to Diag
			strcpy(pRsp->test_mode_rsp.read_nfc_data,(byte*)szExtraData);
	
			printk(KERN_INFO "%s\n", pRsp->test_mode_rsp.read_nfc_data);
			pRsp->ret_stat_code = TEST_OK_S;
		}
	}
	else
	{
		printk(KERN_ERR "[_NFC_] [%s:%d] SubCmd=<%d> ERROR\n", __func__, __LINE__, pReq->nfc);
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
	return pRsp;

	nfc_test_err:

		return pRsp;		
}
//20110930, addy.kim@lge.com,  [END]


void* LGF_TestMotor(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;

	if (diagpdev != NULL)
	{
		update_diagcmd_state(diagpdev, "MOTOR", pReq->motor);
	}
	else
	{
		printk("\n[%s] error MOTOR", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}

  	return pRsp;
}

void* LGF_TestAcoustic(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
    pRsp->ret_stat_code = TEST_OK_S;

	if (diagpdev != NULL){
		if(pReq->acoustic > ACOUSTIC_LOOPBACK_OFF)
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
			
		update_diagcmd_state(diagpdev, "ACOUSTIC", pReq->acoustic);
	}
	else
	{
		printk("\n[%s] error ACOUSTIC", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  
	return pRsp;
}

void* LGF_TestCam(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;

	switch(pReq->camera)
	{
// Start LGE_BSP_CAMERA::tao.jin@lge.com 2011-11-08  enable CAM_TEST command for Testmode v 8.7
#if 0
		case CAM_TEST_SAVE_IMAGE:
		case CAM_TEST_FLASH_ON:
		case CAM_TEST_FLASH_OFF:
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
			break;
#endif
// End LGE_BSP_CAMERA::tao.jin@lge.com 2011-11-08  enable CAM_TEST command for Testmode v 8.7
		default:
			if (diagpdev != NULL)
			{
				update_diagcmd_state(diagpdev, "CAMERA", pReq->camera);
			}
			else
			{
				printk("\n[%s] error CAMERA", __func__ );
				pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
			}
			break;
	}
	
	return pRsp;
}

void* LGT_TestModeKeyTest(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type *pRsp)
{
  	pRsp->ret_stat_code = TEST_OK_S;

  	if(pReq->key_test_start){
		memset((void *)key_buf,0x00,MAX_KEY_BUFF_SIZE);
		count_key_buf=0;
		diag_event_log_start();
  	}
  	else
  	{
		memcpy((void *)((DIAG_TEST_MODE_KEY_F_rsp_type *)pRsp)->key_pressed_buf, (void *)key_buf, MAX_KEY_BUFF_SIZE);
		memset((void *)key_buf,0x00,MAX_KEY_BUFF_SIZE);
		diag_event_log_end();
  	}
  
	return pRsp;
}

char external_memory_copy_test(void)
{
	char return_value = 1;
	char *src = (void *)0;
	char *dest = (void *)0;
	off_t fd_offset;
	int fd;
	mm_segment_t old_fs=get_fs();
    set_fs(get_ds());

// BEGIN : munho.lee@lge.com 2010-12-30
// MOD: 0013315: [SD-card] SD-card drectory path is changed in the testmode
	if ( (fd = sys_open((const char __user *) "/sdcard/_ExternalSD/SDTest.txt", O_CREAT | O_RDWR, 0) ) < 0 )
/*
	if ( (fd = sys_open((const char __user *) "/sdcard/SDTest.txt", O_CREAT | O_RDWR, 0) ) < 0 )
*/	
// END : munho.lee@lge.com 2010-12-30
	{
		printk(KERN_ERR "[ATCMD_EMT] Can not access SD card\n");
		goto file_fail;
	}

	if ( (src = kmalloc(10, GFP_KERNEL)) )
	{
		sprintf(src,"TEST");
		if ((sys_write(fd, (const char __user *) src, 5)) < 0)
		{
			printk(KERN_ERR "[ATCMD_EMT] Can not write SD card \n");
			goto file_fail;
		}
		fd_offset = sys_lseek(fd, 0, 0);
	}
	if ( (dest = kmalloc(10, GFP_KERNEL)) )
	{
		if ((sys_read(fd, (char __user *) dest, 5)) < 0)
		{
			printk(KERN_ERR "[ATCMD_EMT]Can not read SD card \n");
			goto file_fail;
		}
		if ((memcmp(src, dest, 4)) == 0)
			return_value = 0;
		else
			return_value = 1;
	}

	kfree(src);
	kfree(dest);
file_fail:
	sys_close(fd);
    set_fs(old_fs);
// BEGIN : munho.lee@lge.com 2010-12-30
// MOD: 0013315: [SD-card] SD-card drectory path is changed in the testmode
	sys_unlink((const char __user *)"/sdcard/_ExternalSD/SDTest.txt");
/*
	sys_unlink((const char __user *)"/sdcard/SDTest.txt");
*/
// END : munho.lee@lge.com 2010-12-30
	return return_value;
}

void* LGF_ExternalSocketMemory(	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
    struct statfs_local  sf;
    pRsp->ret_stat_code = TEST_OK_S;

    switch( pReq->esm){
	case EXTERNAL_SOCKET_MEMORY_CHECK:
// BEGIN : munho.lee@lge.com 2011-01-15
// ADD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist 	
		if(gpio_get_value(SYS_GPIO_SD_DET))
		{
			pRsp->test_mode_rsp.memory_check = TEST_FAIL_S;
			break;
		}		
// END : munho.lee@lge.com 2011-01-15		
        pRsp->test_mode_rsp.memory_check = external_memory_copy_test();
        break;

	case EXTERNAL_FLASH_MEMORY_SIZE:
// BEGIN : munho.lee@lge.com 2010-12-30
// MOD: 0013315: [SD-card] SD-card drectory path is changed in the testmode
// BEGIN : munho.lee@lge.com 2011-01-15
// ADD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist 
		if(gpio_get_value(SYS_GPIO_SD_DET))
		{
			pRsp->test_mode_rsp.socket_memory_size = 0;
			break;
		}
// END : munho.lee@lge.com 2011-01-15		
   		if (sys_statfs("/sdcard/_ExternalSD", (struct statfs *)&sf) != 0)		
/*
        if (sys_statfs("/sdcard", (struct statfs *)&sf) != 0)
*/       
// END : munho.lee@lge.com 2010-12-30
        {
			printk(KERN_ERR "[Testmode]can not get sdcard infomation \n");
			pRsp->ret_stat_code = TEST_FAIL_S;
			break;
        }

// BEGIN : munho.lee@lge.com 2011-01-15
// MOD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist 
//		pRsp->test_mode_rsp.socket_memory_size = ((long long)sf.f_blocks * (long long)sf.f_bsize);  // needs byte
		pRsp->test_mode_rsp.socket_memory_size = ((long long)sf.f_blocks * (long long)sf.f_bsize) >> 20; // needs Mb.
// END : munho.lee@lge.com 2011-01-15
        break;

	case EXTERNAL_SOCKET_ERASE:

        if (diagpdev != NULL){
// BEGIN : munho.lee@lge.com 2010-11-27
// MOD : 0011477: [SD-card] Diag test mode			
// BEGIN : munho.lee@lge.com 2011-01-15
// ADD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist 	

/*  remove  //START : munho.lee@lge.com 2011-02-24 0016976: [Testmode] Without external memory internal memory can be formatted via diag command. 
			if(gpio_get_value(SYS_GPIO_SD_DET))
			{
				pRsp->ret_stat_code = TEST_FAIL_S;
				break;
			}
*/			// END : munho.lee@lge.com 2011-02-24
// END : munho.lee@lge.com 2011-01-15
			update_diagcmd_state(diagpdev, "MMCFORMAT", EXTERNAL_SOCKET_ERASE_SDCARD_ONLY);
//			update_diagcmd_state(diagpdev, "FACTORY_RESET", 3);
// END : munho.lee@lge.com 2010-11-27			
			msleep(5000);
			pRsp->ret_stat_code = TEST_OK_S;
        }
        else 
        {
			printk("\n[%s] error FACTORY_RESET", __func__ );
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
        }
        break;

	case EXTERNAL_FLASH_MEMORY_USED_SIZE:
// BEGIN : munho.lee@lge.com 2010-12-30
// MOD: 0013315: [SD-card] SD-card drectory path is changed in the testmode
// BEGIN : munho.lee@lge.com 2011-01-15
// ADD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist	

		if(gpio_get_value(SYS_GPIO_SD_DET))
		{
			pRsp->test_mode_rsp.socket_memory_usedsize = 0;
			break;
		}		
// END : munho.lee@lge.com 2011-01-15
		if (sys_statfs("/sdcard/_ExternalSD", (struct statfs *)&sf) != 0)		
/*
		if (sys_statfs("/sdcard", (struct statfs *)&sf) != 0)
*/			
// END : munho.lee@lge.com 2010-12-30
		{
			printk(KERN_ERR "[Testmode]can not get sdcard information \n");
			pRsp->ret_stat_code = TEST_FAIL_S;
			break;
		}
// BEGIN : munho.lee@lge.com 2011-01-15
// MOD: 0013541: 0014142: [Test_Mode] To remove Internal memory information in External memory test when SD-card is not exist	
		pRsp->test_mode_rsp.socket_memory_usedsize = ((long long)(sf.f_blocks - (long long)sf.f_bfree) * sf.f_bsize); // needs byte
// END : munho.lee@lge.com 2011-01-15
/*
		pRsp->test_mode_rsp.socket_memory_usedsize = ((long long)(sf.f_blocks - (long long)sf.f_bfree) * sf.f_bsize) >> 20;
*/		
		break;

	case EXTERNAL_SOCKET_ERASE_SDCARD_ONLY: /*0xE*/
		if (diagpdev != NULL){
			update_diagcmd_state(diagpdev, "MMCFORMAT", EXTERNAL_SOCKET_ERASE_SDCARD_ONLY);		
			msleep(5000);
			pRsp->ret_stat_code = TEST_OK_S;
		}
		else 
		{
			printk("\n[%s] error EXTERNAL_SOCKET_ERASE_SDCARD_ONLY", __func__ );
			pRsp->ret_stat_code = TEST_FAIL_S;
		}
		break;
	case EXTERNAL_SOCKET_ERASE_FAT_ONLY: /*0xF*/
		if (diagpdev != NULL){
			update_diagcmd_state(diagpdev, "MMCFORMAT", EXTERNAL_SOCKET_ERASE_FAT_ONLY);		
			msleep(5000);
			pRsp->ret_stat_code = TEST_OK_S;
		}
		else 
		{
			printk("\n[%s] error EXTERNAL_SOCKET_ERASE_FAT_ONLY", __func__ );
			pRsp->ret_stat_code = TEST_FAIL_S;
		}
		break;

	default:
        pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
        break;
	}

    return pRsp;
}

#ifndef LG_BTUI_TEST_MODE
void* LGF_TestModeBlueTooth(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	// 2011.04.30 sunhee.kang@lge.com BT TestMode merge from Gelato [START]
	if (diagpdev != NULL){
		update_diagcmd_state(diagpdev, "BT_TEST_MODE", pReq->bt);
		//if(pReq->bt==1) msleep(4900); //6sec timeout
		//else if(pReq->bt==2) msleep(4900);
		//else msleep(4900);
		printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%d>\n", __func__, __LINE__, pReq->bt);
		msleep(6000);
		printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%d>\n", __func__, __LINE__, pReq->bt);
	
		pRsp->ret_stat_code = TEST_OK_S;
	}
	else
	{
		printk(KERN_ERR "[_BTUI_] [%s:%d] BTSubCmd=<%d> ERROR\n", __func__, __LINE__, pReq->bt);
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
	return pRsp;
	// 2011.04.30 sunhee.kang@lge.com BT TestMode merge from Gelato [END]
}
#endif

void* LGF_TestModeMP3 (test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;
	printk("\n[%s] diagpdev 0x%x", __func__, diagpdev );

	if (diagpdev != NULL){
		if(pReq->mp3_play == MP3_SAMPLE_FILE)
		{
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
		}
		else
		{
			update_diagcmd_state(diagpdev, "MP3", pReq->mp3_play);
		}
	}
	else
	{
		printk("\n[%s] error MP3", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  
	return pRsp;
}

void* LGF_MemoryFormatTest(	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
  	struct statfs_local  sf;
  	unsigned int remained = 0;
  	
	pRsp->ret_stat_code = TEST_OK_S;
  	
	if (sys_statfs("/data", (struct statfs *)&sf) != 0)
  	{
    	printk(KERN_ERR "[Testmode]can not get sdcard infomation \n");
    	pRsp->ret_stat_code = TEST_FAIL_S;
  	}
  	else
  	{	
    	switch(pReq->memory_format)
    	{
      		case MEMORY_TOTAL_SIZE_TEST:
				break;	
      
			case MEMORY_FORMAT_MEMORY_TEST:
	  			/*
		  		For code of format memory
				*/		  
		  		pRsp->ret_stat_code = TEST_OK_S;
          		break;

	      	default :
          		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
          		break;
    	}
  	}

  	return pRsp;
}

void* LGF_TestModeKeyData(	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;

	LGF_SendKey(LGF_KeycodeTrans(pReq->key_data));

	return pRsp;
}

void* LGF_MemoryVolumeCheck(	test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
  	struct statfs_local  sf;
 	unsigned int total = 0;
  	unsigned int used = 0;
  	unsigned int remained = 0;
  	
	pRsp->ret_stat_code = TEST_OK_S;

  	if (sys_statfs("/data", (struct statfs *)&sf) != 0)
  	{
    	printk(KERN_ERR "[Testmode]can not get sdcard infomation \n");
    	pRsp->ret_stat_code = TEST_FAIL_S;
  	}
  	else
  	{
    	total = (sf.f_blocks * sf.f_bsize) >> 20;
    	remained = (sf.f_bavail * sf.f_bsize) >> 20;
    	used = total - remained;

    	switch(pReq->mem_capa)
    	{
      		case MEMORY_TOTAL_CAPA_TEST:
          		pRsp->test_mode_rsp.mem_capa = total;
          		break;

      		case MEMORY_USED_CAPA_TEST:
          		pRsp->test_mode_rsp.mem_capa = used;
          		break;

      		case MEMORY_REMAIN_CAPA_TEST:
          		pRsp->test_mode_rsp.mem_capa = remained;
          		break;

      		default :
          		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
          		break;
    	}
  	}

  	return pRsp;
}

void* LGF_TestModeSpeakerPhone(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;

	if (diagpdev != NULL)
	{
		if((pReq->speaker_phone == NOMAL_Mic1) || (pReq->speaker_phone == NC_MODE_ON)
			|| (pReq->speaker_phone == ONLY_MIC2_ON_NC_ON) || (pReq->speaker_phone == ONLY_MIC1_ON_NC_ON))
		{
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
		}
		else
		{
			update_diagcmd_state(diagpdev, "SPEAKERPHONE", pReq->speaker_phone);
		}
	}
	else
	{
		printk("\n[%s] error SPEAKERPHONE", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}
  	
	return pRsp;
}

extern int lm3530_get_state(void);


void* LGF_PowerSaveMode(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;
	pReq->sleep_mode = (pReq->sleep_mode & 0x00FF);     // 2011.06.21 biglake for power test after cal

	switch(pReq->sleep_mode){
		case SLEEP_MODE_ON:
			if(lm3530_get_state() ==1)
			LGF_SendKey(KEY_END);
			break;

		case AIR_PLAIN_MODE_ON:
// 201212 jungseob.kim@lge.com FRAMEWORK After Airplain mode on with the test mode, LCD should NOT be turned on when the USB becomes disconnected [START]
			if (diagpdev != NULL){
				update_diagcmd_state(diagpdev, "LCD_KEEP_OFF", 1);
			}
// 201212 jungseob.kim@lge.com FRAMEWORK After Airplain mode on with the test mode, LCD should NOT be turned on when the USB becomes disconnected [END]
			if(lm3530_get_state() ==1)
				LGF_SendKey(KEY_END);

      		remote_set_ftm_boot(0);
      		if_condition_is_on_air_plain_mode = 1;
      		remote_set_operation_mode(0);
			break;

		case AIR_PLAIN_MODE_OFF:
	  		remote_set_ftm_boot(0);
	  		if_condition_is_on_air_plain_mode = 0;
	  		remote_set_operation_mode(1);
			break;

		default:
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
			break;
	}

	return pRsp;
}

void* LGF_TestScriptItemSet(test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type* pRsp)
{
// BEGIN: 0009720 sehyuny.kim@lge.com 2010-10-06
// MOD 0009720: [Modem] It add RF X-Backup feature
  	int mtd_op_result = 0;

  	const MmcPartition *pMisc_part; 
  	unsigned long factoryreset_bytes_pos_in_emmc = 0;
/* BEGIN: 0014656 jihoon.lee@lge.com 20110124 */
/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
  	DIAG_TEST_MODE_F_req_type req_ptr;

  	req_ptr.sub_cmd_code = TEST_MODE_TEST_SCRIPT_MODE;
  	req_ptr.test_mode_req.test_mode_test_scr_mode = pReq->test_mode_test_scr_mode;
/* END: 0014656 jihoon.lee@lge.com 2011024 */

  	lge_mmc_scan_partitions();
  	pMisc_part = lge_mmc_find_partition_by_name("misc");
  	factoryreset_bytes_pos_in_emmc = (pMisc_part->dfirstsec*512)+PTN_FRST_PERSIST_POSITION_IN_MISC_PARTITION;

  	printk("LGF_TestScriptItemSet> mmc info sec : 0x%x, size : 0x%x type : 0x%x frst sec: 0x%lx\n", pMisc_part->dfirstsec, pMisc_part->dsize, pMisc_part->dtype, factoryreset_bytes_pos_in_emmc);

  	switch(pReq->test_mode_test_scr_mode)
  	{
    	case TEST_SCRIPT_ITEM_SET:
#if 1 // def CONFIG_LGE_MTD_DIRECT_ACCESS
      		mtd_op_result = lge_erase_block(factoryreset_bytes_pos_in_emmc, (FACTORY_RESET_STR_SIZE+2) );
/* BEGIN: 0014110 jihoon.lee@lge.com 20110115 */
/* MOD 0014110: [FACTORY RESET] stability */
/* handle operation or rpc failure as well */
      		if(mtd_op_result!=(FACTORY_RESET_STR_SIZE+2))  // LS696_me : Clear factory reset flag 
      		{
      	 		printk(KERN_ERR "[Testmode]lge_erase_block, error num = %d \n", mtd_op_result);
      	 		pRsp->ret_stat_code = TEST_FAIL_S;
      	 		break;
/* END: 0014110 jihoon.lee@lge.com 20110115 */
      		} else
#endif /*CONFIG_LGE_MTD_DIRECT_ACCESS*/
      // LG_FW khlee 2010.03.16 - They want to ACL on state in test script state.
      		{
//				if(diagpdev != NULL)		
//	      	 		update_diagcmd_state(diagpdev, "ALC", 1);
/* BEGIN: 0014656 jihoon.lee@lge.com 20110124 */
/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
      	 		//send_to_arm9((void*)(((byte*)pReq) -sizeof(diagpkt_header_type) - sizeof(word)) , pRsp);
      	 		send_to_arm9((void*)&req_ptr, (void*)pRsp, sizeof(DIAG_TEST_MODE_F_rsp_type) - sizeof(test_mode_rsp_type) + sizeof(test_mode_req_test_script_mode_type));
        		printk(KERN_INFO "%s, result : %s\n", __func__, pRsp->ret_stat_code==TEST_OK_S?"OK":"FALSE");
/* END: 0014656 jihoon.lee@lge.com 2011024 */
      		}
      		break;
#if 0		
  		case CAL_DATA_BACKUP:
  		case CAL_DATA_RESTORE:
  		case CAL_DATA_ERASE:
  		case CAL_DATA_INFO:
  			diagpkt_free(pRsp);
  			return 0;			
  			break;
#endif		
    	default:
/* BEGIN: 0014656 jihoon.lee@lge.com 20110124 */
/* MOD 0014656: [LG RAPI] OEM RAPI PACKET MISMATCH KERNEL CRASH FIX */
      		//send_to_arm9((void*)(((byte*)pReq) -sizeof(diagpkt_header_type) - sizeof(word)) , pRsp);
      		send_to_arm9((void*)&req_ptr, (void*)pRsp, sizeof(DIAG_TEST_MODE_F_rsp_type));
      		printk(KERN_INFO "%s, cmd : %d, result : %s\n", __func__, pReq->test_mode_test_scr_mode, \
	  										pRsp->ret_stat_code==TEST_OK_S?"OK":"FALSE");
      		if(pReq->test_mode_test_scr_mode == TEST_SCRIPT_MODE_CHECK)
      		{
        		switch(pRsp->test_mode_rsp.test_mode_test_scr_mode)
        		{
          			case 0:
            			printk(KERN_INFO "%s, mode : %s\n", __func__, "USER SCRIPT");
            			break;
          			case 1:
            			printk(KERN_INFO "%s, mode : %s\n", __func__, "TEST SCRIPT");
            			break;
          			default:
            			printk(KERN_INFO "%s, mode : %s, returned %d\n", __func__, "NO PRL", pRsp->test_mode_rsp.test_mode_test_scr_mode);
            			break;
        		}
      		}
/* END: 0014656 jihoon.lee@lge.com 2011024 */
      		break;
  	}  
      
// END: 0009720 sehyuny.kim@lge.com 2010-10-06
  	return pRsp;
}

void* LGT_TestModeVolumeLevel(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;

	if (diagpdev != NULL){
		update_diagcmd_state(diagpdev, "VOLUMELEVEL", pReq->volume_level);
	}
	else
	{
		printk("\n[%s] error VOLUMELEVEL", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}

  	return pRsp;
}

static int first_booting_chg_mode_status = -1;
void set_first_booting_chg_mode_status(int status)
{
	first_booting_chg_mode_status = status;
	printk("%s, status : %d\n", __func__, first_booting_chg_mode_status);
}

int get_first_booting_chg_mode_status(void)
{
	printk("%s, status : %d\n", __func__, first_booting_chg_mode_status);
	return first_booting_chg_mode_status;
}

void* LGF_TestModeFboot(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	switch( pReq->fboot)
	{
		case FIRST_BOOTING_COMPLETE_CHECK:
			printk("[Testmode] First Boot info ?? ====> %d \n", boot_complete_info);
			if (boot_complete_info)
				pRsp->ret_stat_code = TEST_OK_S;
			else
				pRsp->ret_stat_code = TEST_FAIL_S;
			break;
/* BEGIN: 0015566 jihoon.lee@lge.com 20110207 */
/* ADD 0015566: [Kernel] charging mode check command */
/*
 * chg_status 0 : in the charging mode
 * chg_status 1 : normal boot mode
 */
#ifdef CONFIG_LGE_CHARGING_MODE_INFO
		case FIRST_BOOTING_CHG_MODE_CHECK:
			if(get_first_booting_chg_mode_status() == 1)
				pRsp->ret_stat_code = FIRST_BOOTING_IN_CHG_MODE;
			else
				pRsp->ret_stat_code = FIRST_BOOTING_NOT_IN_CHG_MODE;
			break;
#endif
/* END: 0015566 jihoon.lee@lge.com 20110207 */
	    default:
			pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
			break;
	}

    return pRsp;
}
void* LGF_TestModePowerReset(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	
	if(diagpdev !=NULL)
	{
		switch(pReq->power_reset)
		{
			case POWER_RESET:
		update_diagcmd_state(diagpdev, "REBOOT", 0);
		pRsp->ret_stat_code = TEST_OK_S;
				break;
			case POWER_OFF:
				pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
				break;
		}
	}else
	{
		pRsp->ret_stat_code = TEST_FAIL_S;
	}
	
	return pRsp;
}

void * LGF_TestModeManualModeSet(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	int read;
	int i=0;
	const char* src ="/mpt/aat_result.txt";
	char buf[16];
	mm_segment_t old_fs;
	unsigned int rsp_len=0;
	DIAG_TEST_MODE_F_req_type req_ptr;

	memset(&buf,0x0,sizeof(buf));
       memset(&req_ptr.test_mode_req.aat_result,0x0,sizeof(req_ptr.test_mode_req.aat_result));

	req_ptr.test_mode_req.test_manual_mode=pReq->test_manual_mode;
	req_ptr.sub_cmd_code = TEST_MODE_MANUAL_MODE_TEST;

	printk(KERN_ERR "\n[%s]", __func__ );

	switch(pReq->test_manual_mode)
	{
		case MANUAL_TEST_ON:
		case MANUAL_TEST_OFF: 
		case MANUAL_MODE_CHECK:
			rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type);
                     send_to_arm9((void*)&req_ptr, (void*)pRsp,rsp_len);
			break;
		case MANUAL_MODE_ALLAUTO_RESULT:
			old_fs=get_fs();
     			set_fs(get_ds());
				
		       read = sys_open((const char __user *)src, O_RDONLY,0);
			if(read<0){
               		printk(KERN_ERR "\n [Testmode Manual Test ] sys_open() failed. cannot create %s\n",src);
		 		pRsp->ret_stat_code = TEST_FAIL_S;
               		goto file_fail;
       		}
			
			 if ((sys_read(read, buf, 14)) < 0)
                       {
                               printk(KERN_ERR "[%s] Can not read file.\n", __func__ );
                               pRsp->ret_stat_code = TEST_FAIL_S;
                               goto file_fail;
                       }else{
                               printk(KERN_ERR "[%s] sys_read:%s \n", __func__,buf );
                       }
			  strncpy(req_ptr.test_mode_req.aat_result,buf,strlen(buf));
                       printk(KERN_ERR "[%s] aat_result:%s (len:%d)\n", __func__,pReq->aat_result,strlen(buf) );
                      /* if(strlen(buf)>0){
				   rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type);
                               send_to_arm9((void*)&req_ptr, (void*)pRsp,rsp_len);
                       }else
                               pRsp->ret_stat_code = TEST_FAIL_S;*/
                       if(sizeof(req_ptr.test_mode_req.aat_result)==0)
				pRsp->ret_stat_code = TEST_FAIL_S;
			else
			{
				for(i=0; i<14; i++)
				{
					if(req_ptr.test_mode_req.aat_result[i]=='0' ||req_ptr.test_mode_req.aat_result[i]=='2' )
					{
						printk(KERN_ERR "MANUAL_MODE_ALLAUTO_RESULT-Fail");
						pRsp->ret_stat_code = TEST_FAIL_S;
				       sys_close(read);
				       set_fs(old_fs);
						return pRsp;
					}
				}
				printk(KERN_ERR "MANUAL_MODE_ALLAUTO_RESULT-Success");
				pRsp->ret_stat_code = TEST_OK_S;
				sys_close(read);
				set_fs(old_fs);
				break;
			}        
                   

               default:
                       pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
                       break;
					   
	}
	return pRsp;
file_fail:
       sys_close(read);
       set_fs(old_fs);
       sys_unlink((const char __user *)src);
       return pRsp;
	
}

static int write_sysfs(const char *path, const char *value, size_t num_bytes)
{
	int write;
	mm_segment_t old_fs;
		
	old_fs=get_fs();
       set_fs(get_ds());

	write = sys_open((const char __user *)path, O_WRONLY,0);
	if(write<0){
       		printk(KERN_ERR "\n [write_sysfs] sys_open() failed. cannot create %s\n",path);
			goto file_fail;
       }
	 if ((sys_write(write, value, 1)) < 0)
       {
                     printk(KERN_ERR "[%s] Can not write file.\n", __func__ );
                     goto file_fail;
        }else{
                     printk(KERN_ERR "[%s] sys_write:%s \n", __func__,value );
        }
	

	sys_close(write);
	set_fs(old_fs);

	return 0;
	
file_fail:
       sys_close(write);
       set_fs(old_fs);
       sys_unlink((const char __user *)path);
       return 0;
}

static int read_sysfs(const char *path, char *value)
{

	int read;
	mm_segment_t old_fs;
	char buf[16];
	
	old_fs=get_fs();
       set_fs(get_ds());

	read = sys_open((const char __user *)path, O_RDONLY,0);
	if(read<0){
       		printk(KERN_ERR "\n [read_sysfs] sys_open() failed. cannot create %s\n",path);
			goto file_fail;
       }
	 if ((sys_read(read, buf, 1)) < 0)
       {
                     printk(KERN_ERR "[%s] Can not read file.\n", __func__ );
                     goto file_fail;
        }else{
                     printk(KERN_ERR "[%s] sys_read:%s \n", __func__,buf );
        }
	strncpy(value,buf,strlen(buf));

	sys_close(read);
	set_fs(old_fs);

	return 0;
	
file_fail:
       sys_close(read);
       set_fs(old_fs);
       sys_unlink((const char __user *)path);
       return 0;
}

void* LGF_TestModeAccel(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	int destfile;
       const char* src = "/data/misc/sensors/diag_sensor_result";
       char buf [256];
       mm_segment_t old_fs;
	unsigned int rsp_len=0;
       DIAG_TEST_MODE_F_req_type req_ptr;
       
       memset(&pRsp->test_mode_rsp.accel, 0x0, sizeof(pRsp->test_mode_rsp.accel));
       memset(&buf,0x0,sizeof(buf));
       memset(&req_ptr.test_mode_req.sensor_data,0x0,sizeof(req_ptr.test_mode_req.sensor_data));

       req_ptr.sub_cmd_code = TEST_MODE_ACCEL_SENSOR_ONOFF_TEST;
       req_ptr.test_mode_req.accel = pReq->accel;
       pRsp->ret_stat_code = TEST_FAIL_S;

       printk(KERN_ERR "\n[%s]", __func__ );

       old_fs=get_fs();
       set_fs(get_ds());

       destfile = sys_open((const char __user *)src, O_CREAT | O_RDONLY, 0666) ;
       if(destfile<0){
               printk(KERN_ERR "\n [Testmode Accel ] sys_open() failed. cannot create %s\n",src);
               goto file_fail;
       }

       if (diagpdev != NULL){
               printk(KERN_ERR "\n[%s] ACCEL test start:%d", __func__ ,pReq->accel);
               update_diagcmd_state(diagpdev, "ACCEL_TEST_MODE", pReq->accel);
               pRsp->ret_stat_code = TEST_OK_S;

       }else{
               printk(KERN_ERR "\n[%s] error ACCEL", __func__ );
               pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
               goto file_fail;
       }
       switch(pReq->accel){
               case ACCEL_SENSOR_OFF:
			  write_sysfs(ACCEL_ENABLE,"0",1);	//only accel_count_flag disable
			   pRsp->ret_stat_code = TEST_OK_S;
                       break;
               case ACCEL_SENSOR_ON:
			  write_sysfs(ACCEL_ENABLE,"1",1); 	//only accel_count_flag enable
                       pRsp->ret_stat_code = TEST_OK_S;
                       break;
               case ACCEL_SENSOR_SENSORDATA:
                       msleep(800);            //result file is read after application write .
                       if ((sys_read(destfile, buf, sizeof(buf)-1)) < 0)
                       {
                               printk(KERN_ERR "[%s] Can not read file.\n", __func__ );
                               pRsp->ret_stat_code = TEST_FAIL_S;
                               goto file_fail;
                       }else{
                               printk(KERN_ERR "[%s] sys_read:%s \n", __func__,buf );
                       }
                       strncpy(req_ptr.test_mode_req.sensor_data,buf,strlen(buf));
                       printk(KERN_ERR "[%s] sensor_data:%s (len:%d)\n", __func__,req_ptr.test_mode_req.sensor_data,strlen(buf) );
                       if(strlen(buf)>0){
				   rsp_len = sizeof(DIAG_TEST_MODE_F_rsp_type);
                               send_to_arm9((void*)&req_ptr, (void*)pRsp,rsp_len);

                               if(pRsp->ret_stat_code != TEST_OK_S)
                                       pRsp->ret_stat_code = TEST_FAIL_S;
                               else
                                       pRsp->ret_stat_code = TEST_OK_S;
                       }else
                               pRsp->ret_stat_code = TEST_FAIL_S;
                       break;
               default:
                       pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
                       break;
       }
               
file_fail:
       sys_close(destfile);
       set_fs(old_fs);
       sys_unlink((const char __user *)src);
       return pRsp;
}



void* LGF_TestModeDBIntegrityCheck(test_mode_req_type* pReq, DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	printk(KERN_ERR "[_DBCHECK_] [%s:%d] DBCHECKSubCmd=<%d>\n", __func__, __LINE__, pReq->bt);

	memset(integrity_ret.ret, 0, 32);

	if (diagpdev != NULL){
		update_diagcmd_state(diagpdev, "DBCHECK", pReq->db_check);
		switch(pReq->db_check)
		{
			case DB_INTEGRITY_CHECK:
				while ( !db_integrity_ready )
					msleep(10);
				db_integrity_ready = 0;

				msleep(100); // wait until the return value is written to the file
				{
					unsigned long crc_val;
					//crc_val = simple_strtoul(integrity_ret.ret+1,NULL,10);
					crc_val = simple_strtoul(integrity_ret.ret+1,NULL,16);
					sprintf(pRsp->test_mode_rsp.str_buf, "0x%08X", crc_val);
						
					printk(KERN_INFO "%s\n", integrity_ret.ret);
					printk(KERN_INFO "%ld\n", crc_val);
					printk(KERN_INFO "%s\n", pRsp->test_mode_rsp.str_buf);
				}

				/* MANUFACTURE requested not to check the status, just return CRC
				if ( integrity_ret.ret[0] == '0' )
					pRsp->ret_stat_code = TEST_OK_S;
				else
					pRsp->ret_stat_code = TEST_FAIL_S;
				*/
				pRsp->ret_stat_code = TEST_OK_S;
				break;
					
			case FPRI_CRC_CHECK:
				while ( !fpri_crc_ready )
					msleep(10);
				fpri_crc_ready = 0;

				msleep(100); // wait until the return value is written to the file
				{
					unsigned long crc_val;
					crc_val = simple_strtoul(integrity_ret.ret+1,NULL,10);
					sprintf(pRsp->test_mode_rsp.str_buf, "0x%08X", crc_val);
					
					printk(KERN_INFO "%s\n", integrity_ret.ret);
					printk(KERN_INFO "%ld\n", crc_val);
					printk(KERN_INFO "%s\n", pRsp->test_mode_rsp.str_buf);
				}

				/* MANUFACTURE requested not to check the status, just return CRC
				if ( integrity_ret.ret[0] == '0' )
					pRsp->ret_stat_code = TEST_OK_S;
				else
					pRsp->ret_stat_code = TEST_FAIL_S;
				*/
					
				/*
				if ( integrity_ret.ret[0] == '0' )
				{
					unsigned long crc_val;
					pRsp->ret_stat_code = TEST_OK_S;
					memcpy(pRsp->test_mode_rsp.str_buf,integrity_ret.ret, 1);
					
					crc_val = simple_strtoul(integrity_ret.ret+1,NULL,10);
					sprintf(pRsp->test_mode_rsp.str_buf + 1, "%08x", crc_val);
				} else {
					pRsp->ret_stat_code = TEST_FAIL_S;
				}
				*/
				pRsp->ret_stat_code = TEST_OK_S;
				break;
					
			case FILE_CRC_CHECK:
			{
				while ( !file_crc_ready )
					msleep(10);
				file_crc_ready = 0;

				msleep(100); // wait until the return value is written to the file

				{
					unsigned long crc_val;
					crc_val = simple_strtoul(integrity_ret.ret+1,NULL,10);
					sprintf(pRsp->test_mode_rsp.str_buf, "0x%08X", crc_val);
						
					printk(KERN_INFO "%s\n", integrity_ret.ret);
					printk(KERN_INFO "%ld\n", crc_val);
					printk(KERN_INFO "%s\n", pRsp->test_mode_rsp.str_buf);
				}

				/* MANUFACTURE requested not to check the status, just return CRC
				if ( integrity_ret.ret[0] == '0' )
					pRsp->ret_stat_code = TEST_OK_S;
				else
					pRsp->ret_stat_code = TEST_FAIL_S;
				*/
				pRsp->ret_stat_code = TEST_OK_S;
				break;
					
				/*
				int mtd_op_result = 0;
				char sec_buf[512];
				const MmcPartition *pSystem_part; 
				unsigned long system_bytes_pos_in_emmc = 0;
				unsigned long system_sec_remained = 0;
					
				printk(KERN_INFO"FILE_CRC_CHECK read block1\n");
					
				lge_mmc_scan_partitions();
					
				pSystem_part = lge_mmc_find_partition_by_name("system");
				if ( pSystem_part == NULL )
				{
					
					printk(KERN_INFO"NO System\n");
					return 0;
				}
				system_bytes_pos_in_emmc = (pSystem_part->dfirstsec*512);
				system_sec_remained = pSystem_part->dsize;
				memset(sec_buf, 0 , 512);

				do 
				{
					mtd_op_result = lge_read_block(system_bytes_pos_in_emmc, sec_buf, 512);
					system_bytes_pos_in_emmc += mtd_op_result;
					system_sec_remained -= 1;
					printk(KERN_INFO"FILE_CRC_CHECK> system_sec_remained %d \n", system_sec_remained);
				} while ( mtd_op_result != 0 && system_sec_remained != 0 );
				*/	
#if 0					
				while ( !file_crc_ready )
					msleep(10);
				file_crc_ready = 0;
#else
//					pRsp->ret_stat_code = TEST_OK_S;

#endif					
				break;
			}
			case CODE_PARTITION_CRC_CHECK:
#if 0					
				while ( !code_partition_crc_ready )
					msleep(10);
				code_partition_crc_ready = 0;
#else
				pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;					
#endif					
				break;
			case TOTAL_CRC_CHECK:
#if 0 					
				while ( !total_crc_ready )
					msleep(10);
				total_crc_ready = 0;
#else
				pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;										
#endif					
				break;

// hojung7.kim@lge.com Add  (MS910)
			case DB_DUMP:
#if 0 					
				while ( !total_crc_ready )
					msleep(10);
				total_crc_ready = 0;
#else
				pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;													
#endif					
				break;

			case DB_COPY:
#if 0 					
				while ( !total_crc_ready )
					msleep(10);
				total_crc_ready = 0;
#else
				pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;										
#endif					
				break;
// hojung7.kim@lge.com Add  (MS910)
		}
	}
	else
	{
		printk("\n[%s] error DBCHECK", __func__ );
		pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
	}

	printk(KERN_ERR "[_DBCHECK_] [%s:%d] DBCHECK Result=<%s>\n", __func__, __LINE__, integrity_ret.ret);

	return pRsp;
}

/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode function [START] */
static char wifi_get_rx_packet_info(rx_packet_info_t* rx_info)
{
	const char* src = "/data/misc/wifi/diag_wifi_result";
	char return_value = TEST_FAIL_S;
	char *dest = (void *)0;
	char buf[30];
	off_t fd_offset;
	int fd;
	char *tok, *holder = NULL;
	char *delimiter = ":\r\n";
	substring_t args[MAX_OPT_ARGS];	
	int token;	
	char tmpstr[10];

    mm_segment_t old_fs=get_fs();
    set_fs(get_ds());

	if (rx_info == NULL) {
		goto file_fail;
	}
	
	memset(buf, 0x00, sizeof(buf));

    if ( (fd = sys_open((const char __user *)src, O_CREAT | O_RDWR, 0) ) < 0 )
    {
        printk(KERN_ERR "[Testmode Wi-Fi] sys_open() failed!!\n");
        goto file_fail;
    }

    if ( (dest = kmalloc(30, GFP_KERNEL)) )
    {
        fd_offset = sys_lseek(fd, 0, 0);

        if ((sys_read(fd, (char __user *) dest, 30)) < 0)
        {
            printk(KERN_ERR "[Testmode Wi-Fi] can't read path %s \n", src);
            goto file_fail;
        }

#if 0 
		/*	sscanf(dest, "%d:%d", &(rx_info->goodpacket), &(rx_info->badpacket));    */
		strncpy(buf, (const char *)dest, sizeof(buf) - 1) ;

		tok = strtok_r(dest, delimiter, &holder);

		if ( holder != NULL && tok != NULL)
		{
			rx_info->goodpacket = simple_strtoul(tok, (char**)NULL, 10);
			tok = strtok_r(NULL, delimiter, &holder);
			rx_info->badpacket = simple_strtoul(tok, (char**)NULL, 10);
			printk(KERN_ERR "[Testmode Wi-Fi] rx_info->goodpacket %lu, rx_info->badpacket = %lu \n",
				rx_info->goodpacket, rx_info->badpacket);
			return_value = TEST_OK_S;
		}
#else
		if ((memcmp(dest, "30", 2)) == 0) {
			printk(KERN_INFO "rx_packet_cnt read error \n");
			goto file_fail;
		}

		strncpy(buf, (const char *)dest, sizeof(buf) - 1);
		buf[sizeof(buf)-1] = 0;
		holder = &(buf[2]); // skip index, result
		
		while (holder != NULL) {
			tok = strsep(&holder, delimiter);
			
			if (!*tok)
				continue;

			token = match_token(tok, param_tokens, args);
			switch (token) {
			case Param_goodpacket:
				memset(tmpstr, 0x00, sizeof(tmpstr));
				if (0 == match_strlcpy(tmpstr, &args[0], sizeof(tmpstr)))
				{
					printk(KERN_ERR "Error GoodPacket %s", args[0].from);
					continue;
				}
				rx_info->goodpacket = simple_strtol(tmpstr, NULL, 0);
				printk(KERN_INFO "[Testmode Wi-Fi] rx_info->goodpacket = %d", rx_info->goodpacket);
				break;

			case Param_badpacket:
				memset(tmpstr, 0x00, sizeof(tmpstr));
				if (0 == match_strlcpy(tmpstr, &args[0], sizeof(tmpstr)))
				{
					printk(KERN_ERR "Error BadPacket %s\n", args[0].from);
					continue;
				}

				rx_info->badpacket = simple_strtol(tmpstr, NULL, 0);
				printk(KERN_INFO "[Testmode Wi-Fi] rx_info->badpacket = %d", rx_info->badpacket);
				return_value = TEST_OK_S;
				break;

			case Param_end:
			case Param_err:
			default:
				/* silently ignore unknown settings */
				printk(KERN_ERR "[Testmode Wi-Fi] ignore unknown token %s\n", tok);
				break;
			}
		}
#endif
    }

	printk(KERN_INFO "[Testmode Wi-Fi] return_value %d!!\n", return_value);
	
file_fail:    
    kfree(dest);
    sys_close(fd);
    set_fs(old_fs);
    sys_unlink((const char __user *)src);
    return return_value;
}


static char wifi_get_test_results(int index)
{
	const char* src = "/data/misc/wifi/diag_wifi_result";
    char return_value = TEST_FAIL_S;
    char *dest = (void *)0;
	char buf[4]={0};
    off_t fd_offset;
    int fd;
    mm_segment_t old_fs=get_fs();
    set_fs(get_ds());

    if ( (fd = sys_open((const char __user *)src, O_CREAT | O_RDWR, 0) ) < 0 )
    {
        printk(KERN_ERR "[Testmode Wi-Fi] sys_open() failed!!\n");
        goto file_fail;
    }

    if ( (dest = kmalloc(20, GFP_KERNEL)) )
    {
        fd_offset = sys_lseek(fd, 0, 0);

        if ((sys_read(fd, (char __user *) dest, 20)) < 0)
        {
            printk(KERN_ERR "[Testmode Wi-Fi] can't read path %s \n", src);
            goto file_fail;
        }

		sprintf(buf, "%d""1", index);
		buf[3]='\0';
        printk(KERN_INFO "[Testmode Wi-Fi] result %s!!\n", buf);

        if ((memcmp(dest, buf, 2)) == 0)
            return_value = TEST_OK_S;
        else
            return_value = TEST_FAIL_S;
		
        printk(KERN_ERR "[Testmode Wi-Fi] return_value %d!!\n", return_value);

    }
	
file_fail:
    kfree(dest);
    sys_close(fd);
    set_fs(old_fs);
    sys_unlink((const char __user *)src);

    return return_value;
}


static test_mode_ret_wifi_ctgry_t divide_into_wifi_category(test_mode_req_wifi_type input)
{
	test_mode_ret_wifi_ctgry_t sub_category = WLAN_TEST_MODE_CTGRY_NOT_SUPPORTED;
	
	if ( input == WLAN_TEST_MODE_54G_ON || 
		WL_IS_WITHIN(WLAN_TEST_MODE_11B_ON, WLAN_TEST_MODE_11A_CH_RX_START, input)) {
		sub_category = WLAN_TEST_MODE_CTGRY_ON;
	} else if ( input == WLAN_TEST_MODE_OFF ) {
		sub_category = WLAN_TEST_MODE_CTGRY_OFF;
	} else if ( input == WLAN_TEST_MODE_RX_RESULT ) {
		sub_category = WLAN_TEST_MODE_CTGRY_RX_STOP;
	} else if ( WL_IS_WITHIN(WLAN_TEST_MODE_RX_START, WLAN_TEST_MODE_RX_RESULT, input) || 
			WL_IS_WITHIN(WLAN_TEST_MODE_LF_RX_START, WLAN_TEST_MODE_MF_TX_START, input)) {
        sub_category = WLAN_TEST_MODE_CTGRY_RX_START;
	} else if ( WL_IS_WITHIN(WLAN_TEST_MODE_TX_START, WLAN_TEST_MODE_TXRX_STOP, input) || 
			WL_IS_WITHIN( WLAN_TEST_MODE_MF_TX_START, WLAN_TEST_MODE_11B_ON, input)) {
		sub_category = WLAN_TEST_MODE_CTGRY_TX_START;
	} else if ( input == WLAN_TEST_MODE_TXRX_STOP) {
		sub_category = WLAN_TEST_MODE_CTGRY_TX_STOP;
	}
	
	printk(KERN_INFO "[divide_into_wifi_category] input = %d, sub_category = %d!!\n", input, sub_category );
	
	return sub_category;	
}


void* LGF_TestModeWLAN(
        test_mode_req_type*	pReq,
        DIAG_TEST_MODE_F_rsp_type	*pRsp)
{
	int i;
	static int first_on_try = 10;
	test_mode_ret_wifi_ctgry_t wl_category;

	if (diagpdev != NULL)
	{
		update_diagcmd_state(diagpdev, "WIFI_TEST_MODE", pReq->wifi);

		printk(KERN_ERR "[WI-FI] [%s:%d] WiFiSubCmd=<%d>\n", __func__, __LINE__, pReq->wifi);

		wl_category = divide_into_wifi_category(pReq->wifi);

		/* Set Test Mode */
		switch (wl_category) {

			case WLAN_TEST_MODE_CTGRY_ON:
				//[10sec timeout] when wifi turns on, it takes about 9seconds to bring up FTM mode.
				msleep(7000);
				
				first_on_try = 5;

				pRsp->ret_stat_code = wifi_get_test_results(wl_category);
				pRsp->test_mode_rsp.wlan_status = !(pRsp->ret_stat_code);
				break;

			case WLAN_TEST_MODE_CTGRY_OFF:
				//5sec timeout
				msleep(3000);
				pRsp->ret_stat_code = wifi_get_test_results(wl_category);
				break;

			case WLAN_TEST_MODE_CTGRY_RX_START:
				msleep(2000);
				pRsp->ret_stat_code = wifi_get_test_results(wl_category);
				pRsp->test_mode_rsp.wlan_status = !(pRsp->ret_stat_code);
				break;

			case WLAN_TEST_MODE_CTGRY_RX_STOP:
			{
				rx_packet_info_t rx_info;
				int total_packet = 0;
				int m_rx_per = 0;
				// init
				rx_info.goodpacket = 0;
				rx_info.badpacket = 0;
				// wait 3 sec
				msleep(3000);
				
				pRsp->test_mode_rsp.wlan_rx_results.packet = 0;
				pRsp->test_mode_rsp.wlan_rx_results.per = 0;

				pRsp->ret_stat_code = wifi_get_rx_packet_info(&rx_info);
				if (pRsp->ret_stat_code == TEST_OK_S) {
					total_packet = rx_info.badpacket + rx_info.goodpacket;
					if(total_packet > 0) {
						m_rx_per = (rx_info.badpacket * 1000 / total_packet);
						printk(KERN_INFO "[WI-FI] per = %d, rx_info.goodpacket = %d, rx_info.badpacket = %d ",
							m_rx_per, rx_info.goodpacket, rx_info.badpacket);
					}
					pRsp->test_mode_rsp.wlan_rx_results.packet = rx_info.goodpacket;
					pRsp->test_mode_rsp.wlan_rx_results.per = m_rx_per;
				}				
				break;
			}

			case WLAN_TEST_MODE_CTGRY_TX_START:
				for (i = 0; i< 2; i++)
					msleep(1000);
				pRsp->ret_stat_code = wifi_get_test_results(wl_category);
				pRsp->test_mode_rsp.wlan_status = !(pRsp->ret_stat_code);
				break;

			case WLAN_TEST_MODE_CTGRY_TX_STOP:
				for (i = 0; i< 2; i++)
					msleep(1000);
				pRsp->ret_stat_code = wifi_get_test_results(wl_category);
				pRsp->test_mode_rsp.wlan_status = !(pRsp->ret_stat_code);
				break;

			default:
				pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
				break;
		}
	}
	else
	{
		printk(KERN_ERR "[WI-FI] [%s:%d] diagpdev %d ERROR\n", __func__, __LINE__, pReq->wifi);
		pRsp->ret_stat_code = TEST_FAIL_S;
	}

	return pRsp;
}

/* 2011-10-13, dongseok.ok@lge.com, Add Wi-Fi Testmode function [END] */

extern int fota_id_check;
extern char fota_id_read[20];
void* LGF_TestModeFOTAIDCheck(test_mode_req_type* pReq ,DIAG_TEST_MODE_F_rsp_type* pRsp)
{
	int i;
	
	pRsp->ret_stat_code = TEST_OK_S; // LGE_FOTA_BSP miracle.kim@lge.com temp block for TEST_OK result
    if (diagpdev != NULL)
    {
        switch( pReq->fota_id_check)
        {
            case FOTA_ID_CHECK:
                fota_id_check = 1;
                update_diagcmd_state(diagpdev, "FOTAIDCHECK", 0);
                msleep(500);

                if(fota_id_check == 0)
                {
					printk("[Testmode] TEST_OK_S\n");
				    pRsp->ret_stat_code = TEST_OK_S;
                }
				else
				{
                   	printk("[Testmode] TEST_FAIL_S\n");
					pRsp->ret_stat_code = TEST_FAIL_S;
				}
                break;

			case FOTA_ID_READ:
				memset(fota_id_read, 0x00, sizeof(fota_id_read));

                update_diagcmd_state(diagpdev, "FOTAIDREAD", 0);
                msleep(500);

                for(i = 0; i < sizeof(fota_id_read) ; i++)
                    pRsp->test_mode_rsp.fota_id_read[i] = fota_id_read[i];

                printk(KERN_ERR "%s, rsp.read_fota_id : %s\n", __func__, (char *)pRsp->test_mode_rsp.fota_id_read);
                pRsp->ret_stat_code = TEST_OK_S;
                break;

            default:
               	printk("[Testmode] TEST_NOT_SUPPORTED_S\n");
				pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
                break;
        }
    }
    else
        pRsp->ret_stat_code = TEST_FAIL_S;

    return pRsp;
}

int lgf_key_lock=0;
extern void lm3530_backlight_off(void);
extern void lm3530_backlight_on(void);

void* LGF_TestModeKEYLOCK(test_mode_req_type * pReq, DIAG_TEST_MODE_F_rsp_type * pRsp)
{
	pRsp->ret_stat_code = TEST_OK_S;
    if (diagpdev != NULL)
    {
        switch( pReq->req_key_lock)
        {
            case KEY_LOCK_REQ:				
				pRsp->ret_stat_code = TEST_OK_S;
				printk("[Testmode KEY_LOCK] key lock on\n");
				lm3530_backlight_off();				
				lgf_key_lock = 1;
				break;
			case KEY_UNLOCK_REQ:
				pRsp->ret_stat_code = TEST_OK_S;
				printk("[Testmode KEY_LOCK] key lock off\n");
				lm3530_backlight_on();
				mdelay(50);
				lgf_key_lock = 0;
				break;
			default:
				printk("[Testmode KEY_LOCK]not support\n");
				pRsp->ret_stat_code = TEST_NOT_SUPPORTED_S;
                break;				
        }
    }
	else
        pRsp->ret_stat_code = TEST_FAIL_S;

	printk("%s() : lgf_key_lock = %d, resp = %d\n", __func__, lgf_key_lock, pRsp->ret_stat_code);
	
    return pRsp;
}

//ssoo.kim@lge.com 2011-12-08 : SMS Test Tool [FEATURE_SMS_PC_TEST]
PACK (void *)LGF_SMSTest (PACK (void *)req_pkt_ptr, uint16 pkg_len){

	unsigned int rsp_len=0;
	//byte* req_ptr = (byte*) req_pkt_ptr;
  	DIAG_SMS_mode_rsp_type *rsp_ptr;

	printk("entry LGF_SMSTest \n");

	rsp_len = sizeof(DIAG_SMS_mode_rsp_type);
	
	rsp_ptr = (DIAG_SMS_mode_rsp_type *)diagpkt_alloc(DIAG_SMS_TEST_F, rsp_len);
	if(rsp_ptr == NULL){
		printk("LGF_SMSTest rsp_ptr is null!!\n");
		return rsp_ptr;
	}
	
	rsp_ptr->ret_stat_code = TEST_OK_S;		
	send_SMS_to_arm9((void*)req_pkt_ptr, (void*)rsp_ptr, rsp_len);
	printk("End LGF_SMSTest \n");
	return (rsp_ptr);
}
EXPORT_SYMBOL(LGF_SMSTest);

//johny.kim
void* LGF_TestModeMLTEnableSet(test_mode_req_type * pReq, DIAG_TEST_MODE_F_rsp_type * pRsp)
{
    char *src = (void *)0;
    char *dest = (void *)0;
    off_t fd_offset;
    int fd;

    mm_segment_t old_fs=get_fs();
    set_fs(get_ds());

    pRsp->ret_stat_code = TEST_FAIL_S;

    if (diagpdev != NULL)
    {
        if ( (fd = sys_open((const char __user *) "/mpt/enable", O_CREAT | O_RDWR, 0) ) < 0 )
        {
            printk(KERN_ERR "[Testmode MPT] Can not access MPT\n");
            goto file_fail;
        }
#if 1
		if(pReq->mlt_enable == 2)
		{
			if ( (dest = kmalloc(5, GFP_KERNEL)) )
			{
				if ((sys_read(fd, (char __user *) dest, 2)) < 0)
				{
					printk(KERN_ERR "[Testmode MPT] Can not read MPT \n");
					goto file_fail;
				}

				if ((memcmp("1", dest, 2)) == 0)
				{
					pRsp->test_mode_rsp.mlt_enable = 1;
					pRsp->ret_stat_code = TEST_OK_S;
				}
				else if ((memcmp("0", dest, 2)) == 0)
				{
					pRsp->test_mode_rsp.mlt_enable = 0;
					pRsp->ret_stat_code = TEST_OK_S;
				}
				else
				{
					//pRsp->test_mode_rsp = 1;
					pRsp->ret_stat_code = TEST_FAIL_S;
				}
			}
		}
		else
#endif
		{
		  pRsp->test_mode_rsp.mlt_enable = pReq->mlt_enable;
			if ( (src = kmalloc(5, GFP_KERNEL)) )
			{
				sprintf(src, "%d", pReq->mlt_enable);
				if ((sys_write(fd, (const char __user *) src, 2)) < 0)
				{
					printk(KERN_ERR "[Testmode MPT] Can not write MPT \n");
					goto file_fail;
				}

				fd_offset = sys_lseek(fd, 0, 0);
			}

			if ( (dest = kmalloc(5, GFP_KERNEL)) )
			{
				if ((sys_read(fd, (char __user *) dest, 2)) < 0)
				{
					printk(KERN_ERR "[Testmode MPT] Can not read MPT \n");
					goto file_fail;
				}

				if ((memcmp(src, dest, 2)) == 0)
					pRsp->ret_stat_code = TEST_OK_S;
				else
					pRsp->ret_stat_code = TEST_FAIL_S;
			}
		}
			
        file_fail:
          kfree(src);
          kfree(dest);
          sys_close(fd);
          set_fs(old_fs);
//          sys_unlink((const char __user *)"/mpt/enable");
    }

    return pRsp;
}
//johny.kim

