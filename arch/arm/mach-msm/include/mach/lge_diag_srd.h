#ifndef LG_DIAG_SRD_H
#define LG_DIAG_SRD_H

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


#define MAX_DLL_VER_SIZE       32
#define MAX_FW_VER_SIZE        32
#define MAX_PRL_VER_SIZE       32
#define MAX_SCRIPT_VER_SIZE    32
#define MAX_RESERVED_SIZE      32

#define MAX_NV_ITEM_SIZE            128
#define MAX_USER_DATA_IN_PAGE		16
#define MAX_USER_DATA_IN_BLOCK		1024
#define MAX_PRL_ITEM_SIZE			2048
#define MAX_INFO_COUNT		50
#define MAX_BLOCK_IN_DID	 	 7 //DID Partition Data + Padding
#define MIN_NEED_BLOCK_SRD	  	 2 //DID Partition Padding

/*===========================================================================
                     DIAG TYPE DEFINITIONS
===========================================================================*/

typedef struct {
       unsigned char    cmd_code;         // DIAG_PST_API_F = 249
       unsigned char    sub_cmd;           // sub command
       dword   err_code;         // error code
       dword   packet_version;  // version information for this packet. currently, 0x1000
       dword   dwsize;            // size of packet - header
} PACKED srd_header_type;

//
// System time is represented with the following structure:
//
typedef struct {
    word wYear;
    word wMonth;
    word wDay;
    word wHour;
    word wMinute;
    word wSecond;
} PACKED srd_time_type;

//
// Download Information is represented with the following structure:
//
typedef struct {
    char dll_version[MAX_DLL_VER_SIZE];             // dll version ex) 1,0,0,0
    char fw_version[MAX_FW_VER_SIZE];          // new binary version for download
    char prl_version[MAX_PRL_VER_SIZE];         // new prl version for download
    char script_version[MAX_SCRIPT_VER_SIZE];  // new script version for download
    srd_time_type dl_time;                         // download start time for log
    char reserved1[MAX_RESERVED_SIZE];         // reserved 1 for extra use
    char reserved2[MAX_RESERVED_SIZE];         // reserved 2 for extra use
    char reserved3[MAX_RESERVED_SIZE];        // reserved 3 for extra use
} PACKED srd_dl_information_type;


//
// Download entry command is represented with the following structure:
//
typedef struct {
       unsigned char backup_used;           // CS, Factory, World Factory - refer to PST_BACKUP_USE_XXXX
       unsigned char binary_class;             // Normal, Master Binary - refer to PST_BINARY_CLASS_XXX
       unsigned char factory_reset_required;  // Is required factory reset ? after flashing completed.
       byte device_srd_reset_required;       // Is required reset ?
       word srd_backup_nv_counter ; 		//backup write couter .
       srd_dl_information_type information;
} PACKED srd_entry_req_type;

//
// Extra NV Operation command is represented with the following structure:
//
typedef struct {
    unsigned char    bOperation;
    dword 	dwNvEnumValue;
	dword	dwNam;
    unsigned char    bData[MAX_NV_ITEM_SIZE];  // same as DIAG_NV_ITEM_SIZE
} PACKED srd_extra_nv_operation_req_type;

typedef struct {
        unsigned char    bMore;
        dword   dwNam;
		dword   dwseqNum;
        dword   dwTotalSizeOfPrlFile;
        dword   dwSizeOfPrlData;
        unsigned char    bData[MAX_PRL_ITEM_SIZE];
} PACKED srd_extra_prl_operation_req_type;


typedef struct {
    srd_dl_information_type information;
    unsigned char backup_used;             // CS, Factory, World Factory refer to PST_BACKUP_USE_XXXX
} PACKED srd_get_information_req_type;

typedef union {
	srd_entry_req_type       			 do_dl_entry;
	srd_get_information_req_type        get_dl_information;
	srd_extra_nv_operation_req_type     extra_nv_operation;
	srd_extra_prl_operation_req_type    extra_prl_operation;
} PACKED srd_req_data_type;

typedef  struct {
	srd_header_type header; // common header
	srd_req_data_type req_data;
} PACKED srd_req_type;

// RESPONSE --------------------------------------------------------------------------------------

typedef union {
	word 	write_sector_counter;  //kabjoo.choi
} PACKED srd_rsp_data_type;

typedef struct {
	srd_header_type header; // common header
	srd_rsp_data_type rsp_data;
} PACKED srd_rsp_type;

typedef enum {
	MODEM_BACKUP=0,
	MDM_BACKUP=1,
	
} script_process_type;
/*===========================================================================
                     BACKUP DATA TYPE DEFINITIONS
===========================================================================*/
typedef  struct {
	srd_entry_req_type info;
	int dl_info_cnt;
	dword nv_esn;
	qword nv_meid;
	//nv_bd_addr_type nv_bt_address;
}PACKED user_data_backup_info;

typedef  struct 
{
	user_data_backup_info	info_table;
//	user_data_backup_table	nv_table;
}PACKED user_data_backup_item;

#endif /* LG_DIAG_SRD_H */
