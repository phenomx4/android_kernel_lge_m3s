#include <linux/module.h>
#include <linux/delay.h>
#include <mach/lge_diagcmd.h>
#include <mach/lge_diag_keypress.h>
#include <linux/input.h>
#include <mach/gpio.h>
/*==========================================================================*/
#define HS_RELEASE_K 0xFFFF
/* 
enum {
	GPIO_SLIDE_CLOSE=0,
	GPIO_SLIDE_OPEN,
};
*/
#define KEY_TRANS_MAP_SIZE 77
/* Virtual Key */
#define V_KEY_DIAL	0x60
#define V_KEY_SEND	0x61
#define V_KEY_STAR	227
#define V_KEY_POUND	228

#define DIAL_TOUCH_X  137	
#define DIAL_TOUCH_Y  1843

extern struct input_dev* get_ats_input_dev(void);
typedef struct {
	  word LG_common_key_code;
	    unsigned int Android_key_code;
}keycode_trans_type;

keycode_trans_type keytrans_table[KEY_TRANS_MAP_SIZE]={
/* index = 0 */	{0x30, KEY_0},	
/* index = 1 */	{0x31, KEY_1},	
/* index = 2 */	{0x32, KEY_2},	
/* index = 3 */	{0x33, KEY_3},	
/* index = 4 */	{0x34, KEY_4},	
/* index = 5 */	{0x35, KEY_5},	
/* index = 6 */	{0x36, KEY_6},	
/* index = 7 */	{0x37, KEY_7},	
/* index = 8 */	{0x38, KEY_8},	
/* index = 9 */	{0x39, KEY_9},	
/* index = 10 */	{0x2A, V_KEY_STAR},	
/* index = 11 */	{0x23, V_KEY_POUND},
/* index = 12 */	{0x50, KEY_HOME},		
/* index = 13 */	{0x51, KEY_MENU},	
/* index = 14 */	{0x52, KEY_BACK},
/* index = 15 */	{0x53, KEY_SEARCH},
/* index = 16 */	{0x73, KEY_VOLUMEUP},
/* index = 17 */	{0x72, KEY_VOLUMEDOWN},
/* index = 18 */	{0x74, KEY_POWER},
/* index = 19 */	{0x60, KEY_SEND},
/* index = 20 */	{0x61, KEY_SEND},
					//{0x62, KEY_LEFT},
					//{0x63, KEY_RIGHT},
					//{0x64, KEY_UP},
					//{0x65, KEY_DOWN},
};
unsigned int LGF_KeycodeTrans(word input)
{
  int index = 0;
  unsigned int ret = (unsigned int)input;  // if we can not find, return the org value. 
 
  for( index = 0; index < KEY_TRANS_MAP_SIZE ; index++)
  {
    if( keytrans_table[index].LG_common_key_code == input)
    {
      ret = keytrans_table[index].Android_key_code;
      break;
    }
  }  

  return ret;
}

EXPORT_SYMBOL(LGF_KeycodeTrans);
/* ==========================================================================
===========================================================================*/
extern PACK(void *) diagpkt_alloc (diagpkt_cmd_code_type code, unsigned int length);
//extern unsigned int LGF_KeycodeTrans(word input);
extern void Send_Touch( unsigned int x, unsigned int y);
/*==========================================================================*/

static unsigned saveKeycode =0 ;

void SendKey(unsigned int keycode, unsigned char bHold)
{
  extern struct input_dev *get_ats_input_dev(void);
  struct input_dev *idev = get_ats_input_dev();

  if( keycode != HS_RELEASE_K)
    input_report_key( idev,keycode , 1 ); // press event

  if(bHold)
  {
    saveKeycode = keycode;
  }
  else
  {
    if( keycode != HS_RELEASE_K)
      input_report_key( idev,keycode , 0 ); // release  event
    else
      input_report_key( idev,saveKeycode , 0 ); // release  event
  }
}

void LGF_SendKey(word keycode)
{
	struct input_dev* idev = NULL;

	idev = get_ats_input_dev();

	if(idev == NULL)
		printk("%s: input device addr is NULL\n",__func__);
	
	input_report_key(idev,(unsigned int)keycode, 1);
	input_report_key(idev,(unsigned int)keycode, 0);
}

EXPORT_SYMBOL(LGF_SendKey);

PACK (void *)LGF_KeyPress (
        PACK (void	*)req_pkt_ptr,	/* pointer to request packet  */
        uint16		pkt_len )		      /* length of request packet   */
{
  DIAG_HS_KEY_F_req_type *req_ptr = (DIAG_HS_KEY_F_req_type *) req_pkt_ptr;
  DIAG_HS_KEY_F_rsp_type *rsp_ptr;
  unsigned int keycode = 0;
  const int rsp_len = sizeof( DIAG_HS_KEY_F_rsp_type );

  rsp_ptr = (DIAG_HS_KEY_F_rsp_type *) diagpkt_alloc( DIAG_HS_KEY_F, rsp_len );
  if (!rsp_ptr)
  	return 0;

  if((req_ptr->magic1 == 0xEA2B7BC0) && (req_ptr->magic2 == 0xA5B7E0DF))
  {
    rsp_ptr->magic1 = req_ptr->magic1;
    rsp_ptr->magic2 = req_ptr->magic2;
    rsp_ptr->key = 0xff; //ignore byte key code
    rsp_ptr->ext_key = req_ptr->ext_key;

    keycode = LGF_KeycodeTrans((word) req_ptr->ext_key);
  }
  else
  {
    rsp_ptr->key = req_ptr->key;
    keycode = LGF_KeycodeTrans((word) req_ptr->key);

  }

  if( keycode == 0xff)
    keycode = HS_RELEASE_K;  // to mach the size
  

  switch (keycode){
	default:
    	SendKey(keycode , req_ptr->hold);
		break;
  	}
  	
  return (rsp_ptr);
}

EXPORT_SYMBOL(LGF_KeyPress);
