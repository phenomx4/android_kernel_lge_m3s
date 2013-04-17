#ifndef LG_DIAG_KERNEL_SERVICE_H
#define LG_DIAG_KERNEL_SERVICE_H

#include <mach/lge_comdef.h>

//#define LG_DIAG_DEBUG

#define DIAGPKT_HDR_PATTERN (0xDEADD00DU)
#define DIAGPKT_OVERRUN_PATTERN (0xDEADU)
#define DIAGPKT_USER_TBL_SIZE 10
#define READ_BUF_SIZE 8004

#define DIAG_DATA_TYPE_EVENT         0
#define DIAG_DATA_TYPE_F3            1
#define DIAG_DATA_TYPE_LOG           2
#define DIAG_DATA_TYPE_RESPONSE      3
#define DIAG_DATA_TYPE_DELAYED_RESPONSE   4

#define DIAGPKT_NO_SUBSYS_ID 0xFF

#define TRUE 1
#define FALSE 0


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

typedef struct
{
  word cmd_code_lo;
  word cmd_code_hi;
  PACK(void *)(*func_ptr) (PACK(void *)req_pkt_ptr, uint16 pkt_len);
} diagpkt_user_table_entry_type;

typedef struct
{
	 uint16 cmd_code;
	 uint16 subsys_id;
	 uint16 cmd_code_lo;
	 uint16 cmd_code_hi;
	 uint16 proc_id;
	 uint32 event_id;
	 uint32 log_code;
	 uint32 client_id;
} bindpkt_params;

#define MAX_SYNC_OBJ_NAME_SIZE 32
typedef struct
{
	 char sync_obj_name[MAX_SYNC_OBJ_NAME_SIZE]; /* Name of the synchronization object associated with this process */
	 uint32 count; /* Number of entries in this bind */
	 bindpkt_params *params; /* first bind params */
}
bindpkt_params_per_process;

/* Note: the following 2 items are used internally via the macro below. */

/* User table type */
typedef struct
{ uint16 delay_flag;  /* 0 means no delay and 1 means with delay */
  uint16 cmd_code;
  word subsysid;
  word count;
  uint16 proc_id;
  const diagpkt_user_table_entry_type *user_table;
} diagpkt_user_table_type;

typedef struct
{
  uint8 command_code;
}
diagpkt_hdr_type;

typedef struct
{
  uint8 command_code;
  uint8 subsys_id;
  uint16 subsys_cmd_code;
}
diagpkt_subsys_hdr_type;

typedef struct
{
  uint8 command_code;
  uint8 subsys_id;
  uint16 subsys_cmd_code;
  uint32 status;  
  uint16 delayed_rsp_id;
  uint16 rsp_cnt; /* 0, means one response and 1, means two responses */
}
diagpkt_subsys_hdr_type_v2;

typedef struct
{
  unsigned int pattern;     /* Pattern to check validity of committed pointers. */
  unsigned int size;        /* Size of usable buffer (diagpkt_q_type->pkt) */
  unsigned int length;      /* Size of packet */

/* LGE_CHANGES_S [kyuhyung.lee@lge.com] 2010-02-08, LG_FW_DIAG_SCREEN_CAPTURE */
#if defined (CONFIG_MACH_MSM7X27_GELATO) || defined (LG_FW_DIAG_SCREEN_CAPTURE) || defined (LG_FW_MTC)
  byte pkt[4096];               /*LG_FW size up*/
#else
  byte pkt[1024];               /* Sized by 'length' field. */
#endif
/* LGE_CHANGES_E [kyuhyung.lee@lge.com] 2010-02-08, LG_FW_DIAG_SCREEN_CAPTURE */
} diagpkt_rsp_type;

typedef void (*diag_cmd_rsp) (const byte *rsp, unsigned int length, void *param);

typedef struct
{
  diag_cmd_rsp rsp_func; /* If !NULL, this is called in lieu of comm layer */
  void *rsp_func_param;

  diagpkt_rsp_type rsp; /* see diagi.h */
} diagpkt_lsm_rsp_type;

typedef struct
{
  uint32 diag_data_type; /* This will be used to identify whether the data passed to DCM is an event, log, F3 or response.*/
  uint8 rest_of_data;
} diag_data;

typedef struct
{
	uint16 countresult;
	uint16 wlan_status;
	uint16 g_wlan_status;
	uint16 rx_channel;
	uint16 rx_per;
	uint16 tx_channel;
	uint32 goodFrames;
	uint16 badFrames;
	uint16 rxFrames;
	uint16 wlan_data_rate;
	uint16 wlan_payload;
	uint16 wlan_data_rate_recent;
	unsigned long pktengrxducast_old;
	unsigned long pktengrxducast_new;
	unsigned long rxbadfcs_old;
	unsigned long rxbadfcs_new;
	unsigned long rxbadplcp_old;
	unsigned long rxbadplcp_new;
}wlan_status;

#define FPOS( type, field ) \
    /*lint -e545 */ ( (dword) &(( type *) 0)-> field ) /*lint +e545 */

#define DIAG_REST_OF_DATA_POS (FPOS(diag_data, rest_of_data))

#define DIAGPKT_DISPATCH_TABLE_REGISTER(xx_subsysid, xx_entry) \
	do { \
		static const diagpkt_user_table_type xx_entry##_table = { \
		 0, 0xFF, xx_subsysid, sizeof (xx_entry) / sizeof (xx_entry[0]), 1, xx_entry \
		}; \
	 /*lint -save -e717 */ \
		diagpkt_tbl_reg (&xx_entry##_table); \
	} while (0)

#define DIAGPKT_PKT2LSMITEM(p) \
		((diagpkt_lsm_rsp_type *) (((byte *) p) - FPOS (diagpkt_lsm_rsp_type, rsp.pkt)))

#if 0 //def LG_FW_USB_ACCESS_LOCK
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

#endif /* LG_DIAG_KERNEL_SERVICE_H */
