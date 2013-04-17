/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include <linux/module.h>
#include <mach/lge_diagcmd.h>
#include <mach/lge_diag_mtc.h>

#include <linux/unistd.h> /*for open/close*/
#include <linux/fcntl.h> /*for O_RDWR*/

#include <linux/fb.h> /* to handle framebuffer ioctls */
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include <linux/syscalls.h> //for sys operations

#include <linux/input.h> // for input_event
#include <linux/fs.h> // for file struct
#include <linux/types.h> // for ssize_t
#include <linux/input.h> // for event parameters
#include <linux/jiffies.h>
#include <linux/delay.h>

#if 1 //SPRINT_SLATE_KEYPRESS_TEST
#include <linux/crc-ccitt.h>
#include <linux/delay.h>

#define ESC_CHAR     0x7D
#define CONTROL_CHAR 0x7E
#define ESC_MASK     0x20

#define CRC_16_L_SEED           0xFFFF

#define CRC_16_L_STEP(xx_crc, xx_c) \
	crc_ccitt_byte(xx_crc, xx_c)

void *lg_diag_mtc_req_pkt_ptr;
unsigned short lg_diag_mtc_req_pkt_length;
#endif //SPRINT_SLATE_KEYPRESS_TEST

// matthew.choi@lge.com 111003 prevent reset when try to start recording key input
static bool mtc_key_log_started = FALSE;

#ifndef LG_FW_DUAL_TOUCH
#define LG_FW_DUAL_TOUCH
#endif
///////////
typedef struct {
  unsigned int handset_7k_key_code;
  unsigned int Android_key_code;
}keycode_trans_type;

#define HANDSET_7K_KEY_TRANS_MAP_SIZE 60
keycode_trans_type handset_7k_keytrans_table[HANDSET_7K_KEY_TRANS_MAP_SIZE]=
{
   {KEY_BACK, '^'}, {KEY_1,'1'}, {KEY_2,'2'}, {KEY_3,'3'}, {KEY_4,'4'}, {KEY_5,'5'}, {KEY_6,'6'}, 
	{KEY_7,'7'}, {KEY_8,'8'}, {KEY_9,'9'}, {KEY_0,'0'}, {KEY_BACKSPACE,'Y'}, {KEY_HOME,'!'}, {KEY_MENU,'O'},
   {KEY_SEARCH, '+'}, {KEY_Q,'q'}, {KEY_W,'w'}, {KEY_E,'e'}, {KEY_R,'r'}, {KEY_T,'t'}, {KEY_Y,'y'}, 
	{KEY_U,'u'}, {KEY_I,'i'}, {KEY_O,'o'}, {KEY_P,'p'}, {KEY_LEFT,'/'},
	{KEY_LEFTALT,'$' }, {KEY_A,'a'}, {KEY_S,'s'}, {KEY_D,'d'}, {KEY_F,'f'}, {KEY_G,'g'}, {KEY_H,'h'}, 
	{KEY_J,'j'}, {KEY_K,'k'}, {KEY_L,'l'}, {KEY_ENTER,'='}, {KEY_UP,'L'}, {KEY_REPLY,32}, {KEY_DOWN,'R'},
	{KEY_LEFTSHIFT, '~'}, {KEY_Z,'z'}, {KEY_X,'x'}, {KEY_C,'c'}, {KEY_V,'v'}, {KEY_B,'b'}, {KEY_N,'n'}, 
	{KEY_M,'m'}, {KEY_DOT, 32}, {KEY_RIGHT,'V'},
	{KEY_MENU,'O'}, {KEY_HOME, '+'}, {KEY_BACK, '^'}, {KEY_SEARCH, '+'}, 
	{KEY_SEND,'S'}, {KEY_END,'E'},
	{KEY_VOLUMEUP,'U'}, {KEY_VOLUMEDOWN,'D'}, {KEY_VIDEO_PREV,'Z'}, {KEY_CAMERA,'A'}
};
///////////
/*===========================================================================

                      EXTERNAL FUNCTION AND VARIABLE DEFINITIONS

===========================================================================*/
extern PACK(void *) diagpkt_alloc (diagpkt_cmd_code_type code, unsigned int length);
extern PACK(void *) diagpkt_alloc2 (diagpkt_cmd_code_type code, unsigned int length, unsigned int packet_length);
extern PACK(void *) diagpkt_free (PACK(void *)pkt);
extern void send_to_arm9( void*	pReq, void	*pRsp);

#ifdef CONFIG_LGE_DIAG_ATS_ETA_MTC_KEY_LOGGING
extern unsigned long int ats_mtc_log_mask;
extern void diagpkt_commit (PACK(void *)pkt);
extern int event_log_start(void);
extern int event_log_end(void);
#endif

extern int base64_encode(char *text, int numBytes, char *encodedText);
/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

  This section contains local definitions for constants, macros, types,
  variables and other items needed by this module.

===========================================================================*/
#ifdef CONFIG_LGE_DIAG_ATS_ETA_MTC_KEY_LOGGING
#define JIFFIES_TO_MS(t) ((t) * 1000 / HZ)
#endif 

extern mtc_user_table_entry_type mtc_mstr_tbl[MTC_MSTR_TBL_SIZE];

//LGE_CHANGE_S [20110208] myeonggyu.son@lge.com [gelato] mtc touch logging [START]
#ifdef CONFIG_MACH_LGE_M3S
// temp blocked - myeonggyu.son@lge.com
//extern uint16_t max_x;
//extern uint16_t max_y;
#endif
//LGE_CHANGE_E [20110208] myeonggyu.son@lge.com [gelato] mtc touch logging [END]

unsigned char g_diag_mtc_check = 0;
unsigned char g_diag_mtc_capture_rsp_num = 0;

#if 1 //SPRINT_SLATE_KEYPRESS_TEST
static char mtc_running = 0;
extern int diagchar_ioctl(unsigned int iocmd, unsigned long ioarg);
#endif //SPRINT_SLATE_KEYPRESS_TEST

static mtc_lcd_info_type lcd_info;

static int ats_mtc_set_lcd_info (mtc_scrn_id_type ScreenType);
static ssize_t read_framebuffer(byte* pBuf);

static unsigned char bmp_data_array[MTC_SCRN_BUF_SIZE_MAX];

/*===========================================================================

                      INTERNAL FUNCTION DEFINITIONS

===========================================================================*/
PACK (void *)LGF_MTCProcess (
        PACK (void	*)req_pkt_ptr,	/* pointer to request packet  */
        unsigned short		pkt_len )		      /* length of request packet   */
{
	DIAG_MTC_F_req_type *req_ptr = (DIAG_MTC_F_req_type *) req_pkt_ptr;
  	DIAG_MTC_F_rsp_type *rsp_ptr = NULL;
  	mtc_func_type func_ptr= NULL;
  	int nIndex = 0;
  
  	g_diag_mtc_check = 1;
#if 1 //SPRINT_SLATE_KEYPRESS_TEST
  	mtc_running = 1;
#endif //SPRINT_SLATE_KEYPRESS_TEST

  	for( nIndex = 0 ; nIndex < MTC_MSTR_TBL_SIZE  ; nIndex++)
  	{
    	if( mtc_mstr_tbl[nIndex].cmd_code == req_ptr->hdr.sub_cmd)
    	{
      		if( mtc_mstr_tbl[nIndex].which_procesor == MTC_ARM11_PROCESSOR)
        		func_ptr = mtc_mstr_tbl[nIndex].func_ptr;
      		break;
    	}
    	else if (mtc_mstr_tbl[nIndex].cmd_code == MTC_MAX_CMD)
      		break;
    	else
      		continue;
  	}

  	printk(KERN_INFO "[MTC]cmd_code : [0x%X], sub_cmd : [0x%X]\n",req_ptr->hdr.cmd_code, req_ptr->hdr.sub_cmd);

  	if( func_ptr != NULL)
  	{
    	printk(KERN_INFO "[MTC]cmd_code : [0x%X], sub_cmd : [0x%X]\n",req_ptr->hdr.cmd_code, req_ptr->hdr.sub_cmd);
    	rsp_ptr = func_ptr((DIAG_MTC_F_req_type*)req_ptr);
  	}
//	else
//		send_to_arm9((void*)req_ptr, (void*)rsp_ptr);
//	diagpkt_free(rsp_ptr);

#if 1 //SPRINT_SLATE_KEYPRESS_TEST
	mtc_running = 0;
#endif //SPRINT_SLATE_KEYPRESS_TEST

	return (rsp_ptr);
}

EXPORT_SYMBOL(LGF_MTCProcess);

DIAG_MTC_F_rsp_type* mtc_info_req_proc(DIAG_MTC_F_req_type *pReq)
{
	unsigned int rsp_len;
	DIAG_MTC_F_rsp_type *pRsp;

	printk(KERN_INFO "[MTC]mtc_info_req_proc\n");

	rsp_len = sizeof(mtc_info_rsp_type);
  	printk(KERN_INFO "[MTC] mtc_info_req_proc rsp_len :(%d)\n", rsp_len);
   
  	pRsp = (DIAG_MTC_F_rsp_type *)diagpkt_alloc(DIAG_MTC_F, rsp_len);
  	if (pRsp == NULL) {
     	printk(KERN_ERR "[MTC] diagpkt_alloc failed\n");
  	}

  	pRsp->hdr.cmd_code = DIAG_MTC_F;
  	pRsp->hdr.sub_cmd = MTC_INFO_REQ_CMD;
  
  	if(pReq->mtc_req.info.screen_id == MTC_SUB_LCD) // N/A
  	{
    	printk(KERN_ERR "[MTC]mtc_info_req_proc, sub lcd is not supported\n");
    	return pRsp;
  	}

  	if(pReq->mtc_req.info.screen_id == MTC_MAIN_LCD)
  	{
    	ats_mtc_set_lcd_info(MTC_MAIN_LCD);
    	pRsp->mtc_rsp.info.scrn_id = MTC_MAIN_LCD;
  	}
#ifdef LGE_USES_SUBLCD
  	else if(pReq->mtc_req.info.screen_id == MTC_SUB_LCD)
  	{
    	ats_mtc_set_lcd_info(MTC_SUB_LCD);
    	pRsp->mtc_rsp.info.scrn_id = MTC_SUB_LCD;
  	}
#endif
  	else
  	{
    	printk(KERN_ERR "[MTC]mtc_info_req_proc, unknown screen_id type : %d\n", pRsp->mtc_rsp.info.scrn_id);
    	return pRsp;
  	}

  	pRsp->mtc_rsp.info.scrn_width = lcd_info.width_max;
  	pRsp->mtc_rsp.info.scrn_height = lcd_info.height_max;
  	pRsp->mtc_rsp.info.bits_pixel = 16;//lcd_info.bits_pixel;

  	return pRsp;
}

static int ats_mtc_set_lcd_info (mtc_scrn_id_type ScreenType)
{
  	struct fb_var_screeninfo fb_varinfo;
  	int fbfd;
  
  	if ((fbfd = sys_open ("/dev/graphics/fb0", O_RDWR, 0)) == -1)
  	{
    	printk(KERN_ERR "[MTC]ats_mtc_set_lcd_info, Can't open %s\n", "/dev/graphics/fb0");
    	return 0;
  	}

  	memset((void *)&fb_varinfo, 0, sizeof(struct fb_var_screeninfo));

  	if (sys_ioctl (fbfd, FBIOGET_VSCREENINFO, (long unsigned int)&fb_varinfo) < 0)
  	{
    	printk(KERN_ERR "[MTC]ats_mtc_set_lcd_info, ioctl failed\n");
    	return 0;
  	}

  	printk(KERN_INFO "[MTC]ats_mtc_set_lcd_info, fbvar.xres= %d, fbvar.yres= %d, fbvar.pixel= %d\n", fb_varinfo.xres, fb_varinfo.yres, fb_varinfo.bits_per_pixel);
  
  	sys_close(fbfd);
  
  	if (ScreenType == MTC_MAIN_LCD){
  		lcd_info.id = MTC_MAIN_LCD;
  		lcd_info.width_max = fb_varinfo.xres;
  		lcd_info.height_max =fb_varinfo.yres;
  	}
#if defined (LG_SUBLCD_INCLUDE)  
  	else if (ScreenType == MTC_SUB_LCD){
  		lcd_info.id = MTC_SUB_LCD;
  		lcd_info.width_max = fb_varinfo.xres;
  		lcd_info.height_max = fb_varinfo.yres;
  	}
#endif

  	//To Get the Bits Depth
  	lcd_info.bits_pixel = fb_varinfo.bits_per_pixel;

#if 0  
  	if( lcd_info.bits_pixel == MTC_BIT_65K )
  	{
  		lcd_info.mask.blue = MTC_65K_CMASK_BLUE;
  		lcd_info.mask.green = MTC_65K_CMASK_GREEN;
  		lcd_info.mask.red = MTC_65K_CMASK_RED;
  	}
  	else if( lcd_info.bits_pixel == MTC_BIT_262K )
  	{
  		lcd_info.mask.blue = MTC_262K_CMASK_BLUE;
  		lcd_info.mask.green = MTC_262K_CMASK_GREEN;
  		lcd_info.mask.red = MTC_262K_CMASK_RED;
  	}
  	else // default 16 bit
  	{
  		lcd_info.bits_pixel = MTC_BIT_65K;
  		lcd_info.mask.blue = MTC_65K_CMASK_BLUE;
  		lcd_info.mask.green = MTC_65K_CMASK_GREEN;
  		lcd_info.mask.red = MTC_65K_CMASK_RED;
  	}
#else
   lcd_info.bits_pixel = 16;
   lcd_info.mask.blue = MTC_65K_CMASK_BLUE;
   lcd_info.mask.green = MTC_65K_CMASK_GREEN;
   lcd_info.mask.red = MTC_65K_CMASK_RED;
#endif
  	lcd_info.degrees = 0; //No rotation .. manual Data	

  	return 1;
}

#if 1 //SLATE_CROPPED_CAPTURE
unsigned char tmp_img_block[MTC_SCRN_BUF_SIZE_MAX+16];
EXPORT_SYMBOL(tmp_img_block);
#endif //SLATE_CROPPED_CAPTURE

#define	FB_DEV	"/dev/graphics/fb0"

unsigned long convert_32_to_16_bpp(byte* target_buf, byte* src_buf)
{
  int x, y, start_pos;
  byte* src_buf_ptr;
  unsigned long dst_cnt = 0;
  uint8 red, green, blue;

  printk(KERN_INFO "%s", __func__);

  //memset((void *)target_buf, 0, LCD_BUFFER_SIZE);

  for (y = 0; y < 480; y++) 
  {
    start_pos = y*320*4;
    src_buf_ptr = (byte *)src_buf + start_pos;

    for (x = 0; x < (320*4); x += 4) 
    {
      red = ((src_buf_ptr[x]>>3)&0x1F);
      green = ((src_buf_ptr[x+1]>>2)&0x3F);
      blue = ((src_buf_ptr[x+2]>>3)&0x1F);
      // ignore alpa (every 4th bit)
      target_buf[dst_cnt++] = ((green&0x07)<<5)|blue;
      target_buf[dst_cnt++] = ((red<<3)&0xF8)|((green>>3)&0x07);					
    }
  }
  printk(KERN_INFO "%s, translated count : %ld", __func__, dst_cnt);
  
  return dst_cnt;
}

DIAG_MTC_F_rsp_type* mtc_capture_screen(DIAG_MTC_F_req_type *pReq)
{
  	unsigned int rsp_len,packet_len;
  	DIAG_MTC_F_rsp_type *pRsp;
  	ssize_t bmp_size;
#if 0
    int j, ret, x;
    int size;
    char *data;
    char *data_buf;
    uint16 h;
#endif

    uint8 red, green, blue;
    int width, height,x_offset,y_offset;
    int input;
    int y;

#if 1 //SLATE_CROPPED_CAPTURE
    int x_start=0, y_start=0, x_end=0, y_end=0, i;
	byte *pImgBlock;;
	
	x_start = pReq->mtc_req.capture.left; 			y_start = pReq->mtc_req.capture.top;
	x_end = x_start + pReq->mtc_req.capture.width;	y_end = y_start + pReq->mtc_req.capture.height;
#endif //SLATE_CROPPED_CAPTURE

	printk(KERN_INFO "[MTC]mtc_capture_screen : upper_left_x=%d, upper_left_y=%d, bottom_right_x=%d, bottom_right_y=%d\n",x_start,y_start,x_end,y_end);

  	rsp_len = sizeof(mtc_capture_rsp_type);
	packet_len = rsp_len - (( MTC_SCRN_BUF_SIZE_MAX) )+ ((x_end-x_start)*((y_end-y_start)*sizeof(unsigned short)));
	printk(KERN_INFO "[MTC] mtc_capture_screen rsp_len :(%d), packet_len :(%d)\n", rsp_len, packet_len);

#if 1 //SLATE_CROPPED_CAPTURE
	pRsp = (DIAG_MTC_F_rsp_type *)diagpkt_alloc2(DIAG_MTC_F, rsp_len, packet_len);
  	if (pRsp == NULL) {
		printk(KERN_ERR "[MTC] diagpkt_alloc failed\n");
		return NULL;
  	}
#else
	pRsp = (DIAG_MTC_F_rsp_type *)diagpkt_alloc(DIAG_MTC_F, rsp_len);
  	if (pRsp == NULL) {
		printk(KERN_ERR "[MTC] diagpkt_alloc failed\n");
		return NULL;
  	}
#endif //SLATE_CROPPED_CAPTURE

  	pRsp->hdr.cmd_code = DIAG_MTC_F;
  	pRsp->hdr.sub_cmd = MTC_CAPTURE_REQ_CMD;
  	g_diag_mtc_capture_rsp_num = MTC_CAPTURE_REQ_CMD;

#if 1 //SLATE_CROPPED_CAPTURE

#if 0//joosang.lee@lge.com
	memset(tmp_img_block, 0x00, sizeof(tmp_img_block));
	bmp_size = read_framebuffer((byte*)tmp_img_block);
	pImgBlock = pRsp->mtc_rsp.capture.bmp_data;

	memset(pRsp->mtc_rsp.capture.bmp_data, 0, MTC_SCRN_BUF_SIZE_MAX);
	for(j=0;j<480;j++)
	{
		for(i=0;i<320;i++)
		{
			if(((i>=x_start) && (i<x_end)) && ((j>=y_start) && (j<y_end)))
			{
				*pImgBlock++ = tmp_img_block[(j*320)+i];
			}
		}
	}
#else

	input = 0;

	x_offset = pReq->mtc_req.capture.left;
	y_offset = pReq->mtc_req.capture.top;
	
	if(pReq->mtc_req.capture.width > 0)		
		width = pReq->mtc_req.capture.width;
	else
		width = 320;
	
	if(pReq->mtc_req.capture.height > 0)		
		height = pReq->mtc_req.capture.height;
	else
		height = 480;

	memset(tmp_img_block, 0x00, sizeof(tmp_img_block));
	bmp_size = read_framebuffer((byte*)tmp_img_block);
    printk(KERN_INFO"bmp_size = %ld , width=%ld, height=%ld\n", bmp_size, width, height );
	pImgBlock = pRsp->mtc_rsp.capture.bmp_data;

	memset(pRsp->mtc_rsp.capture.bmp_data, 0, MTC_SCRN_BUF_SIZE_MAX);

#if 1
	for (y = y_offset; y < (y_offset + height); y++) 
	{
		x_start = (y*320*4) + x_offset*4;
		x_end = (y*320*4) + (x_offset+width)*4;
		for (i = x_start; i < x_end; i += 4) 
		{
         #if 1
         /*   GGL_PIXEL_FORMAT_RGBX_8888 : convert to RGB_565
         blue.offset = 8;
         green.offset = 16;
         red.offset = 24;
         transp.offset = 0;
         */
         #if 1
         red = ((tmp_img_block[i]>>3)&0x1F);  
         green = ((tmp_img_block[i+1]>>2)&0x3F);
         blue = ((tmp_img_block[i+2]>>3)&0x1F);
         // ignore alpa (every 4th bit)
         pImgBlock[input++] = ((green&0x07)<<5)|blue;
         pImgBlock[input++] = ((red<<3)&0xF8)|((green>>3)&0x07);
         #else
         pImgBlock[input++] = tmp_img_block[i];
         pImgBlock[input++] = tmp_img_block[i+2];
         pImgBlock[input++] = tmp_img_block[i+3];
         pImgBlock[input++] = 0xFF;// ignore alpa (every 4th bit)
         #endif
         
         #else
         red=tmp_img_block[i+3];
         green=tmp_img_block[i+2];
         blue=tmp_img_block[i+1];
         
         h=(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3));
         
         pImgBlock[i/4]=h;

         #endif
		}
	}   
#else
#if 0
  	for(i=0; i<(320*480*4); i+=4){
		red=tmp_img_block[i+3];
		green=tmp_img_block[i+2];
		blue=tmp_img_block[i+1];
		
		h=(((red & 0x00F8) << 8) | ((green & 0x00FC) << 3) | ((blue & 0x00F8) >> 3));

		pImgBlock[i/4]=h;
	
	}
#else
   convert_32_to_16_bpp( pImgBlock, tmp_img_block);

#endif

#endif
   
#endif

#else
	bmp_size = read_framebuffer((byte*)pRsp->mtc_rsp.capture.bmp_data);

	printk(KERN_INFO "[MTC]mtc_capture_screen, Read framebuffer & Bmp convert complete.. %d\n", (int)bmp_size);
#endif //SLATE_CROPPED_CAPTURE

  	pRsp->mtc_rsp.capture.scrn_id = lcd_info.id;
#if 1 //SLATE_CROPPED_CAPTURE
	pRsp->mtc_rsp.capture.bmp_width = x_end - x_start;
	pRsp->mtc_rsp.capture.bmp_height = y_end - y_start;
#else
	pRsp->mtc_rsp.capture.bmp_width = lcd_info.width_max;
  	pRsp->mtc_rsp.capture.bmp_height = lcd_info.height_max;
#endif
  	pRsp->mtc_rsp.capture.bits_pixel = 16;//lcd_info.bits_pixel;
  	pRsp->mtc_rsp.capture.mask.blue = lcd_info.mask.blue;
  	pRsp->mtc_rsp.capture.mask.green = lcd_info.mask.green;
  	pRsp->mtc_rsp.capture.mask.red = lcd_info.mask.red;

//joosang.lee@lge.com
   printk(KERN_INFO "\n Complete Capture Screen \n");

  	return pRsp;
}

DEFINE_SPINLOCK( mtc_spinlock );

static ssize_t read_framebuffer(byte* pBuf)
{
  	struct file *phMscd_Filp = NULL;
  	ssize_t read_size = 0;

	unsigned long flags;

  	mm_segment_t old_fs=get_fs();

  	set_fs(get_ds());

  	phMscd_Filp = filp_open("/dev/graphics/fb0", O_RDONLY |O_LARGEFILE, 0);

  	if( !phMscd_Filp)
    	printk("open fail screen capture \n" );

   spin_lock_irqsave( &mtc_spinlock, flags );
  	read_size = phMscd_Filp->f_op->read(phMscd_Filp, pBuf, MTC_SCRN_BUF_SIZE_MAX, &phMscd_Filp->f_pos);
   spin_unlock_irqrestore( &mtc_spinlock, flags );
   
  	filp_close(phMscd_Filp,NULL);

  	set_fs(old_fs);

  	return read_size;
}

#ifdef CONFIG_LGE_DIAG_ATS_ETA_MTC_KEY_LOGGING
DIAG_MTC_F_rsp_type* mtc_logging_mask_req_proc(DIAG_MTC_F_req_type *pReq)
{
  	unsigned int rsp_len;
  	DIAG_MTC_F_rsp_type *pRsp;

  	rsp_len = sizeof(mtc_log_req_type);
  	printk(KERN_INFO "[MTC] mtc_logging_mask_req_proc rsp_len :(%d)\n", rsp_len);

	pRsp = (DIAG_MTC_F_rsp_type *)diagpkt_alloc(DIAG_MTC_F, rsp_len);
  	if (pRsp == NULL) {
  		printk(KERN_ERR "[MTC] diagpkt_alloc failed\n");
    	return pRsp;
  	}

  	switch(pReq->mtc_req.log.mask)
  	{
    	case 0x00000000://ETA_LOGMASK_DISABLE_ALL:
			// matthew.choi@lge.com 111003 prevent reset when try to start recording key input
    		if (mtc_key_log_started == FALSE)
      		{
				pRsp->mtc_rsp.log.mask = ats_mtc_log_mask = 0xFFFFFFFF;
				mtc_key_log_started = TRUE;
    		}
			else
			{
	      		ats_mtc_log_mask = pReq->mtc_req.log.mask;
	      		pRsp->mtc_rsp.log.mask = ats_mtc_log_mask;
			}
			break;
    	case 0xFFFFFFFF://ETA_LOGMASK_ENABLE_ALL:
    	case 0x00000001://ETA_LOGITEM_KEY:
    	case 0x00000002://ETA_LOGITEM_TOUCHPAD:
    	case 0x00000003://ETA_LOGITME_KEYTOUCH:
      		ats_mtc_log_mask = pReq->mtc_req.log.mask;
      		pRsp->mtc_rsp.log.mask = ats_mtc_log_mask;
			// matthew.choi@lge.com 111003 prevent reset when try to start recording key input
			mtc_key_log_started = TRUE;
      		break;
    	default:
      		ats_mtc_log_mask = 0x00000000; // //ETA_LOGMASK_DISABLE_ALL
      		pRsp->mtc_rsp.log.mask = ats_mtc_log_mask;
			// matthew.choi@lge.com 111003 prevent reset when try to start recording key input
			mtc_key_log_started = FALSE;
      		break;
  	}

  	/* LGE_CHANGE
   	* support mtc using diag port
   	* 2010 07-11 taehung.kim@lge.com
   	*/
   	if(ats_mtc_log_mask & 0xFFFFFFFF)
   		event_log_start();
   	else
   	{
		// matthew.choi@lge.com 111003 prevent reset when try to start recording key input
		if (mtc_key_log_started == TRUE)
	   	{
			event_log_end();
			mtc_key_log_started = FALSE;
		}
   	}

  	return pRsp;
}

/////////////////////
unsigned int Handset_7k_KeycodeTrans(word input)
{
  	int index = 0;
  	unsigned int ret = (unsigned int)input;  // if we can not find, return the org value. 
 
  	for( index = 0; index < HANDSET_7K_KEY_TRANS_MAP_SIZE  ; index++)
  	{
	 	if( handset_7k_keytrans_table[index].handset_7k_key_code == input)
	 	{
			ret = handset_7k_keytrans_table[index].Android_key_code;
			printk(KERN_ERR "[MTC] input keycode %d, mapped keycode %d\n", input, ret);
			break;
	 	}
  	}  

  	return ret;
}
///////////////////

void mtc_send_key_log_data(struct ats_mtc_key_log_type* p_ats_mtc_key_log)
{
  	unsigned int rsp_len;
  	DIAG_MTC_F_rsp_type *pRsp;
  
  	rsp_len = sizeof(mtc_log_data_rsp_type);
  	printk(KERN_INFO "[MTC] mtc_send_key_log_data rsp_len :(%d)\n", rsp_len);

	pRsp = (DIAG_MTC_F_rsp_type *)diagpkt_alloc(DIAG_MTC_F, rsp_len);
  	if (pRsp == NULL) {
  		printk(KERN_ERR "[MTC] diagpkt_alloc failed\n");
  		diagpkt_commit (pRsp);
  	}

  	pRsp->hdr.cmd_code = DIAG_MTC_F;
  	pRsp->hdr.sub_cmd = MTC_LOG_REQ_CMD;

  	pRsp->mtc_rsp.log_data.log_id = p_ats_mtc_key_log->log_id; //LOG_ID, 1 key, 2 touch
  	pRsp->mtc_rsp.log_data.log_len = p_ats_mtc_key_log->log_len; //LOG_LEN
  
  	if(p_ats_mtc_key_log->log_id == 1) // key
  	{
    	pRsp->mtc_rsp.log_data.log_type.log_data_key.time = (unsigned long long)JIFFIES_TO_MS(jiffies);
    	pRsp->mtc_rsp.log_data.log_type.log_data_key.hold = (unsigned char)((p_ats_mtc_key_log->x_hold)&0xFF);// hold
		//pRsp->mtc_rsp.log_data.log_type.log_data_key.keycode = ((p_ats_mtc_key_log->y_code)&0xFF);//key code
    	pRsp->mtc_rsp.log_data.log_type.log_data_key.keycode = Handset_7k_KeycodeTrans((p_ats_mtc_key_log->y_code)&0xFF);//key code
    	pRsp->mtc_rsp.log_data.log_type.log_data_key.active_uiid = 0;
  	}
  	else // touch
  	{
    	pRsp->mtc_rsp.log_data.log_type.log_data_touch.time = (unsigned long long)JIFFIES_TO_MS(jiffies);
    	pRsp->mtc_rsp.log_data.log_type.log_data_touch.screen_id = (unsigned char)1; // MAIN LCD
    	pRsp->mtc_rsp.log_data.log_type.log_data_touch.action = (unsigned char)p_ats_mtc_key_log->action;
		pRsp->mtc_rsp.log_data.log_type.log_data_touch.x = (unsigned short)p_ats_mtc_key_log->x_hold;
    	pRsp->mtc_rsp.log_data.log_type.log_data_touch.y = (unsigned short)p_ats_mtc_key_log->y_code;
    	pRsp->mtc_rsp.log_data.log_type.log_data_touch.active_uiid = 0;
  	}

  	diagpkt_commit (pRsp);
}
EXPORT_SYMBOL(mtc_send_key_log_data);
#endif 

DIAG_MTC_F_rsp_type* mtc_serialized_data_req_proc(DIAG_MTC_F_req_type *pReq)
{
  	unsigned int rsp_len;
  	DIAG_MTC_F_rsp_type *pRsp;
  	static unsigned long bmp_sent_cnt = 0;

  	rsp_len = sizeof(mtc_serialized_data_rsp_type);
  	printk(KERN_INFO "[MTC] mtc_serialized_data_req_proc rsp_len :(%d)\n", rsp_len);

  	pRsp = (DIAG_MTC_F_rsp_type *)diagpkt_alloc(DIAG_MTC_F, rsp_len);
  	if (pRsp == NULL) {
  		printk(KERN_ERR "[MTC] diagpkt_alloc failed\n");
    	return pRsp;
  	}

  	pRsp->hdr.cmd_code = DIAG_MTC_F;
  	pRsp->hdr.sub_cmd = MTC_SERIALIZED_DATA_REQ_CMD;
  	g_diag_mtc_capture_rsp_num = MTC_SERIALIZED_DATA_REQ_CMD;

  	if((bmp_sent_cnt+1)*MTC_SCRN_BUF_SIZE <= MTC_SCRN_BUF_SIZE_MAX)
  	{
    	pRsp->mtc_rsp.serial_data.seqeunce = bmp_sent_cnt;
    	pRsp->mtc_rsp.serial_data.length = MTC_SCRN_BUF_SIZE;

		memset((void*)pRsp->mtc_rsp.serial_data.bmp_data, 0, MTC_SCRN_BUF_SIZE);
    	memcpy((void*)pRsp->mtc_rsp.serial_data.bmp_data, (void*)(bmp_data_array+bmp_sent_cnt*MTC_SCRN_BUF_SIZE), MTC_SCRN_BUF_SIZE);
  
    	bmp_sent_cnt++;
  	}

  	if(bmp_sent_cnt*MTC_SCRN_BUF_SIZE >= MTC_SCRN_BUF_SIZE_MAX)
  	{
    	pRsp->mtc_rsp.serial_data.seqeunce = 0xFFFF;
    	bmp_sent_cnt = 0;
  	}
  
  	return pRsp;
}

DIAG_MTC_F_rsp_type* mtc_serialized_capture_req_proc(DIAG_MTC_F_req_type *pReq)
{
  	unsigned int rsp_len;
  	DIAG_MTC_F_rsp_type *pRsp;
  	ssize_t bmp_size;
  
  	rsp_len = sizeof(mtc_serialized_capture_rsp_type);
  	printk(KERN_INFO "[MTC] mtc_serialized_data_req_proc rsp_len :(%d)\n", rsp_len);

  	pRsp = (DIAG_MTC_F_rsp_type *)diagpkt_alloc(DIAG_MTC_F, rsp_len);
  	if (pRsp == NULL) {
  		printk(KERN_ERR "[MTC] diagpkt_alloc failed\n");
    	return pRsp;
  	}

  	pRsp->hdr.cmd_code = DIAG_MTC_F;
  	pRsp->hdr.sub_cmd = MTC_SERIALIZED_CAPTURE_REQ_CMD;

  	pRsp->mtc_rsp.serial_capture.screen_id = lcd_info.id;
  	pRsp->mtc_rsp.serial_capture.bmp_size = lcd_info.width_max*lcd_info.height_max*2;
  	pRsp->mtc_rsp.serial_capture.bmp_width = lcd_info.width_max;
  	pRsp->mtc_rsp.serial_capture.bmp_height = lcd_info.height_max;
  	pRsp->mtc_rsp.serial_capture.bits_pixel = lcd_info.bits_pixel;
  	pRsp->mtc_rsp.serial_capture.mask.blue = lcd_info.mask.blue;
  	pRsp->mtc_rsp.serial_capture.mask.green = lcd_info.mask.green;
  	pRsp->mtc_rsp.serial_capture.mask.red = lcd_info.mask.red;

  	memset(bmp_data_array, 0, MTC_SCRN_BUF_SIZE_MAX);

  	bmp_size = read_framebuffer((byte*)bmp_data_array);
  	printk(KERN_INFO "[MTC]mtc_serialized_capture_req_proc, Read framebuffer & Bmp convert complete.. %d\n", (int)bmp_size);

  	return pRsp;
}

DIAG_MTC_F_rsp_type* mtc_null_rsp(DIAG_MTC_F_req_type *pReq)
{
  	unsigned int rsp_len;
  	DIAG_MTC_F_rsp_type *pRsp;

  	rsp_len = sizeof(mtc_req_hdr_type);
  	printk(KERN_INFO "[MTC] mtc_null_rsp rsp_len :(%d)\n", rsp_len);
   
  	pRsp = (DIAG_MTC_F_rsp_type *)diagpkt_alloc(DIAG_MTC_F, rsp_len);
  	if (pRsp == NULL) {
     	printk(KERN_ERR "[MTC] diagpkt_alloc failed\n");
  	}

  	pRsp->hdr.cmd_code = pReq->hdr.cmd_code;
  	pRsp->hdr.sub_cmd = pReq->hdr.sub_cmd;

  	return pRsp;
}

DIAG_MTC_F_rsp_type* mtc_execute(DIAG_MTC_F_req_type *pReq)
{
  	int ret;
  	char cmdstr[100];
	int fd;

  	unsigned int req_len = 0;
  	unsigned int rsp_len = 0;
  	DIAG_MTC_F_rsp_type *pRsp = (void *)NULL;
  	unsigned char *mtc_cmd_buf_encoded = NULL;
  	int lenb64 = 0;
  
  	char *envp[] = {
  		"HOME=/",
  		"TERM=linux",
  		NULL,
  	};
  
  	char *argv[] = {
		"/system/bin/mtc",
  		cmdstr,
  		NULL,
  	};

  	printk(KERN_INFO "[MTC]mtc_execute\n");
  
  	switch(pReq->hdr.sub_cmd)
  	{
    	case MTC_KEY_EVENT_REQ_CMD:
      		req_len = sizeof(mtc_key_req_type);
      		rsp_len = sizeof(mtc_key_req_type);
      		printk(KERN_INFO "[MTC] KEY_EVENT_REQ rsp_len :(%d)\n", rsp_len);

	  		pRsp = (DIAG_MTC_F_rsp_type *)diagpkt_alloc(DIAG_MTC_F, rsp_len);
      		if (pRsp == NULL) {
        		printk(KERN_ERR "[MTC] diagpkt_alloc failed\n");
        		return pRsp;
      		}
      		pRsp->mtc_rsp.key.hold = pReq->mtc_req.key.hold;
      		pRsp->mtc_rsp.key.key_code = pReq->mtc_req.key.key_code;
      		break;

    	case MTC_TOUCH_REQ_CMD:
      		req_len = sizeof(mtc_touch_req_type);
      		rsp_len = sizeof(mtc_touch_req_type);
      		printk(KERN_INFO "[MTC] TOUCH_EVENT_REQ rsp_len :(%d)\n", rsp_len);

	  		pRsp = (DIAG_MTC_F_rsp_type *)diagpkt_alloc(DIAG_MTC_F, rsp_len);
      		if (pRsp == NULL) {
        		printk(KERN_ERR "[MTC] diagpkt_alloc failed\n");
        		return pRsp;
      		}
      		pRsp->mtc_rsp.touch.screen_id = pReq->mtc_req.touch.screen_id;
      		pRsp->mtc_rsp.touch.action = pReq->mtc_req.touch.action;
      		pRsp->mtc_rsp.touch.x = pReq->mtc_req.touch.x;
      		pRsp->mtc_rsp.touch.y = pReq->mtc_req.touch.y;
      		break;

    	default:
      		printk(KERN_ERR "[MTC] unknown sub_cmd : (%d)\n", pReq->hdr.sub_cmd);
      		break;
  	}

  	pRsp->hdr.cmd_code = pReq->hdr.cmd_code;
  	pRsp->hdr.sub_cmd = pReq->hdr.sub_cmd;

  	mtc_cmd_buf_encoded = kmalloc(sizeof(unsigned char)*50, GFP_KERNEL);
  	memset(cmdstr,0x00, 50);
  	memset(mtc_cmd_buf_encoded,0x00, 50);

  	lenb64 = base64_encode((char *)pReq, req_len, (char *)mtc_cmd_buf_encoded);

  	if ( (fd = sys_open((const char __user *) "/system/bin/mtc", O_RDONLY ,0) ) < 0 )
  	{
  		printk("\n [MTC]can not open /system/bin/mtc - execute /system/bin/mtc\n");
  		sprintf(cmdstr, "/system/bin/mtc ");
  	}
  	else
  	{
		memcpy((void*)cmdstr, (void*)mtc_cmd_buf_encoded, lenb64);
#if 0
		printk("[MTC] cmdstr[16] : %d, cmdstr[17] : %d, cmdstr[18] : %d", cmdstr[16], cmdstr[17], cmdstr[18]);
		printk("[MTC] cmdstr[19] : %d, cmdstr[20] : %d, cmdstr[21] : %d", cmdstr[19], cmdstr[20], cmdstr[21]);
#endif	
		printk("\n [MTC]execute /system/bin/mtc, %s\n", cmdstr);
  		sys_close(fd);
  	}
  	// END: eternalblue@lge.com.2009-10-23
  
  	printk(KERN_INFO "[MTC]execute mtc : data - %s\n\n", cmdstr);
  	if ((ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC)) != 0) {
	  	printk(KERN_ERR "[MTC]MTC failed to run : %i\n", ret);
  	}
  	else
  		printk(KERN_INFO "[MTC]execute ok, ret = %d\n", ret);

  	kfree(mtc_cmd_buf_encoded);
  
  	return pRsp;
}
EXPORT_SYMBOL(mtc_execute);

#if 1 //ifdef SPRINT_SLATE_KEYPRESS_TEST
static void add_hdlc_packet(struct mtc_data_buffer *mb, char data)
{
	mb->data[mb->data_length++] = data;

	//if (mb->data_length == BUFFER_MAX_SIZE) {
	if (mb->data_length >= BUFFER_MAX_SIZE) {
		mb->data_length = BUFFER_MAX_SIZE;

		msleep(10);

		if (diagchar_ioctl (DIAG_IOCTL_BULK_DATA, (unsigned long)mb)) {
			printk(KERN_ERR "[MTC] %s: diagchar_ioctl error\n", __func__);
		} 

		mb->data_length = 0;
	}

	return;
}

/*
 * FUNCTION	add_hdlc_esc_packet.
 */
static void add_hdlc_esc_packet(struct mtc_data_buffer *mb, char data)
{
	if (data == ESC_CHAR || data == CONTROL_CHAR) {
		add_hdlc_packet(mb, ESC_CHAR);
		add_hdlc_packet(mb, (data ^ ESC_MASK));
	} 
	else {
		add_hdlc_packet(mb, data);
	}

	return;
}

/*
 * FUNCTION	mtc_send_hdlc_packet.
 */
static void mtc_send_hdlc_packet(byte * pBuf, int len)
{
	int i;
	struct mtc_data_buffer *mb;
	word crc = CRC_16_L_SEED;

	mb = kzalloc(sizeof(struct mtc_data_buffer), GFP_ATOMIC);
	if (mb == NULL) {
		printk(KERN_ERR "[MTC] %s: failed to alloc memory\n", __func__);
		return;
	}

	//Generate crc data.
	for (i = 0; i < len; i++) {
		add_hdlc_esc_packet(mb, pBuf[i]);
		crc = CRC_16_L_STEP(crc, (word) pBuf[i]);
	}

	crc ^= CRC_16_L_SEED;
	add_hdlc_esc_packet(mb, ((unsigned char)crc));
	add_hdlc_esc_packet(mb, ((unsigned char)((crc >> 8) & 0xFF)));
	add_hdlc_packet(mb, CONTROL_CHAR);

	if (diagchar_ioctl(DIAG_IOCTL_BULK_DATA, (unsigned long)mb)) {
		printk(KERN_ERR "[MTC] %s: ioctl ignored\n", __func__);
	}

	kfree(mb);
	mb = NULL;

	return;
}

/*
 * FUNCTION	translate_key_code.
 */
dword translate_key_code(dword keycode)
{
	if (keycode == KERNELFOCUSKEY)
		return KERNELCAMERAKEY;
	else
		return keycode;
}

/*
 * FUNCTION	mtc_send_key_log_packet.
 */
void mtc_send_key_log_packet(unsigned long keycode, unsigned long state)
{
	key_msg_type msg;
	dword sendKeyValue = 0;

	/* LGE_CHANGE [dojip.kim@lge.com] 2010-06-04 [LS670]
	 * don't send a raw diag packet in running MTC
	 */
	if (mtc_running)
		return;

	memset(&msg, 0, sizeof(key_msg_type));

	sendKeyValue = translate_key_code(keycode);

	msg.cmd_code = 121;
	msg.ts_type = 0;	//2;
	msg.num_args = 2;
	msg.drop_cnt = 0;
	//ts_get(&msg.time[0]); 
	msg.time[0] = 0;
	msg.time[1] = 0;
	msg.line_number = 261;
	msg.ss_id = LGE_DIAG_ICD_LOGGING_SSID;
	msg.ss_mask = LGE_DIAG_ICD_LOGGING_SSID_MASK;
	msg.args[0] = sendKeyValue;
	msg.args[1] = state;

	memcpy(&msg.code[0], "Debounced %d", sizeof("Debounced %d"));
	//msg.code[12] = '\0';

	memcpy(&msg.file_name[0], "DiagDaemon.c", sizeof("DiagDaemon.c"));
	//msg.fle_name[13] = '\0';

	mtc_send_hdlc_packet((byte *) & msg, sizeof(key_msg_type));

	return;
}
EXPORT_SYMBOL(mtc_send_key_log_packet);

/*
 * FUNCTION	mtc_send_touch_log_packet.
 */
void mtc_send_touch_log_packet(unsigned long touch_x, unsigned long touch_y, unsigned long status)
{
	touch_msg_type msg;
	
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-06-04 [LS670]
	 * don't send a raw diag packet in running MTC
	 */
	if (mtc_running)
		return;

	memset(&msg, 0, sizeof(touch_msg_type));

	msg.cmd_code = 121;
	msg.ts_type = 0;	//2;
	msg.num_args = 3;
	msg.drop_cnt = 0;
	//ts_get(&msg.time[0]); 
	msg.time[0] = 0;
	msg.time[1] = 0;
	msg.line_number = 261;
	msg.ss_id = LGE_DIAG_ICD_LOGGING_SSID;
	msg.ss_mask = LGE_DIAG_ICD_LOGGING_SSID_MASK;
	if(status == 1) // push - "DWN"
		msg.args[0] = 0x004E5744;
	else	// release - "UP"
		msg.args[0] = 0x00005055;
#if 0 // temp blocked - myeonggyu.son@lge.com
	msg.args[1] = touch_x*320/max_x;
	msg.args[2] = touch_y*480/max_y;
#else
	msg.args[1] = touch_x;
	msg.args[2] = touch_y;
#endif
	memcpy(&msg.code[0], "PenEvent %d,%d", sizeof("PenEvent %d,%d"));
	//msg.code[12] = '\0';

	memcpy(&msg.file_name[0], "DiagDaemon.c", sizeof("DiagDaemon.c"));
	//msg.fle_name[13] = '\0';

	mtc_send_hdlc_packet((byte *) & msg, sizeof(touch_msg_type));
	
	return;
}
EXPORT_SYMBOL(mtc_send_touch_log_packet);
#endif /*SPRINT_SLATE_KEYPRESS_TEST*/

/*  USAGE (same as testmode
  *    1. If you want to handle at ARM9 side, you have to insert fun_ptr as NULL and mark ARM9_PROCESSOR
  *    2. If you want to handle at ARM11 side , you have to insert fun_ptr as you want and mark AMR11_PROCESSOR.
  */
mtc_user_table_entry_type mtc_mstr_tbl[MTC_MSTR_TBL_SIZE] =
{ 
/*	sub_command							fun_ptr								which procesor              */
	{ MTC_INFO_REQ_CMD					,mtc_info_req_proc					, MTC_ARM11_PROCESSOR},
	{ MTC_CAPTURE_REQ_CMD				,mtc_capture_screen					, MTC_ARM11_PROCESSOR},
	{ MTC_KEY_EVENT_REQ_CMD				,mtc_execute						, MTC_ARM11_PROCESSOR},
	{ MTC_TOUCH_REQ_CMD					,mtc_execute						, MTC_ARM11_PROCESSOR},
#ifdef CONFIG_LGE_DIAG_ATS_ETA_MTC_KEY_LOGGING
	{ MTC_LOGGING_MASK_REQ_CMD			,mtc_logging_mask_req_proc			, MTC_ARM11_PROCESSOR},
	{ MTC_LOG_REQ_CMD					,mtc_null_rsp						, MTC_ARM11_PROCESSOR}, /*mtc_send_key_log_data*/
#endif 
	{ MTC_SERIALIZED_DATA_REQ_CMD		,mtc_serialized_data_req_proc		, MTC_ARM11_PROCESSOR},
	{ MTC_SERIALIZED_CAPTURE_REQ_CMD 	,mtc_serialized_capture_req_proc	, MTC_ARM11_PROCESSOR},
	{ MTC_PHONE_RESTART_REQ_CMD			,mtc_null_rsp						, MTC_ARM9_PROCESSOR},
	{ MTC_FACTORY_RESET					,mtc_null_rsp						, MTC_ARM9_ARM11_BOTH},
	{ MTC_PHONE_REPORT					,mtc_null_rsp						, MTC_ARM9_PROCESSOR},
	{ MTC_PHONE_STATE					,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_CAPTURE_PROP					,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_NOTIFICATION_REQUEST			,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_CUR_PROC_NAME_REQ_CMD			,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_KEY_EVENT_UNIV_REQ_CMD		,mtc_null_rsp						, MTC_NOT_SUPPORTED}, /*ETA command*/
	{ MTC_MEMORY_DUMP					,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_BATTERY_POWER					,mtc_null_rsp						, MTC_ARM9_PROCESSOR},
	{ MTC_BACKLIGHT_INFO				,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_FLASH_MODE					,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_MODEM_MODE					,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_CELL_INFORMATION				,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_HANDOVER						,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_ERROR_CMD						,mtc_null_rsp						, MTC_NOT_SUPPORTED},
	{ MTC_MAX_CMD						,mtc_null_rsp						, MTC_NOT_SUPPORTED},
};
