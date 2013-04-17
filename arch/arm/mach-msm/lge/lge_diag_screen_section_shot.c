/*
  Program : Screen Shot

  Author : khlee

  Date : 2010.01.26
*/
/* ==========================================================================*/
#include <linux/module.h>
#include <mach/lge_diag_screen_section_shot.h>
#include <linux/fcntl.h> 
#include <linux/fs.h>
#include <mach/lge_diagcmd.h>
#include <linux/uaccess.h>
#include <mach/lge_diag_mtc.h>


/* ==========================================================================
===========================================================================*/
#define LCD_BUFFER_SIZE LCD_MAIN_WIDTH * LCD_MAIN_HEIGHT * 2
/* ==========================================================================
===========================================================================*/
extern PACK(void *) diagpkt_alloc (diagpkt_cmd_code_type code, unsigned int length);
/*==========================================================================*/
/*==========================================================================*/

lcd_section_buf_info_type lcd_section_buf_info;
extern unsigned char tmp_img_block[MTC_SCRN_BUF_SIZE_MAX];
unsigned short tmp_img_block_2[320*480*2];

/*==========================================================================*/
static void to_rgb565(byte* from, u16* to)
{
	int i;
	int r,g,b;
	u16 h;

	for(i=0; i<320*480*4; i+=4){
		r=from[i];
		g=from[i+1];
		b=from[i+2];
		
		h=(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));

		to[i/4]=h;
	
	}
}

static void read_framebuffer(byte* pBuf)
{
  struct file *phMscd_Filp = NULL;
  mm_segment_t old_fs=get_fs();

  set_fs(get_ds());

  phMscd_Filp = filp_open("/dev/graphics/fb0", O_RDONLY |O_LARGEFILE, 0);

  if( !phMscd_Filp) {
		printk("open fail screen capture \n" );
		return;
  	}

  phMscd_Filp->f_op->read(phMscd_Filp, pBuf, 320*480*4, &phMscd_Filp->f_pos);
  filp_close(phMscd_Filp,NULL);

  set_fs(old_fs);

}

PACK (void *)LGE_ScreenSectionShot (
        PACK (void	*)req_pkt_ptr,	/* pointer to request packet  */
        uint16		pkt_len )		      /* length of request packet   */
{
	diag_screen_section_shot_type *req_ptr = (diag_screen_section_shot_type *)req_pkt_ptr;
  	diag_screen_section_shot_type *rsp_ptr = 0;
  	int rsp_len, packet_len;
	int x_start=0, y_start=0, x_end=0, y_end=0, i, j;	
	short *pImgBlock;	

#if 0
	unsigned char tmp_buf[20];

	memset(tmp_buf, 0x00, sizeof(tmp_buf));	
	memcpy(tmp_buf, req_pkt_ptr, sizeof(tmp_buf));	
	printk("\n\n");	
	for(i=0;i<20;i++)		
		printk("[0x%x] ",tmp_buf[i]);	
	printk("\n\n");
#endif
	printk("[UTS] %s, %d : cmd = %d, sub_cmd = %d\n",__func__,__LINE__,req_ptr->lcd_bk_ctrl.sub_cmd_code,req_ptr->lcd_buf.seq_flow);

	switch(req_ptr->lcd_bk_ctrl.sub_cmd_code)
  	{
    	case SCREEN_SHOT_BK_CTRL:
	  		break;
    	case SCREEN_SHOT_SECTION_LCD_BUF:
      		switch(req_ptr->lcd_buf.seq_flow)
      		{
        		case SEQ_START:
					printk("[UTS] x = %d, y = %d, width = %d, height = %d\n",
						req_ptr->lcd_buf.x,req_ptr->lcd_buf.y,req_ptr->lcd_buf.width,req_ptr->lcd_buf.height);

					x_start = req_ptr->lcd_buf.x;				y_start = req_ptr->lcd_buf.y;
					x_end = x_start + req_ptr->lcd_buf.width;	y_end = y_start + req_ptr->lcd_buf.height;
#if 0
					printk("[UTS] x_start = %d, y_start = %d, x_end = %d, y_end = %d\n",x_start,y_start,x_end,y_end);
					if(((x_end - x_start <= 0) || (y_end - y_start <= 0)) || ((x_end > LCD_MAIN_WIDTH) || (y_end >LCD_MAIN_HEIGHT))) 
					{						
						printk("[UTS] %s : invalid parameter\n",__func__);						
						return 0;					
					}
#endif
       			rsp_len = sizeof(diag_lcd_section_get_buf_req_type);
       			rsp_ptr = (diag_screen_section_shot_type *)diagpkt_alloc(DIAG_LGE_SCREEN_SECTION_SHOT_F, rsp_len - SCREEN_SHOT_PACK_LEN*sizeof(unsigned short));
		  			if (!rsp_ptr)
		  				return 0;

	          	lcd_section_buf_info.is_fast_mode = req_ptr->lcd_buf.is_fast_mode;
	          	lcd_section_buf_info.full_draw = req_ptr->lcd_buf.full_draw;
	          	lcd_section_buf_info.update = TRUE;
	          	lcd_section_buf_info.updated = FALSE;
					lcd_section_buf_info.x = x_start;
					lcd_section_buf_info.y = y_start;
	          	lcd_section_buf_info.width = x_end - x_start;
	          	lcd_section_buf_info.height = y_end - y_start;
	          	lcd_section_buf_info.total_bytes = lcd_section_buf_info.width * lcd_section_buf_info.height *sizeof(unsigned short);
	          	lcd_section_buf_info.sended_bytes = 0;
	          	lcd_section_buf_info.update = FALSE;
	          	lcd_section_buf_info.updated = TRUE;

					if((x_start == 0) && (y_start == 0) && (x_end == 0) && (y_end ==0))
					{
						printk("[UTS] %s : send lcd info\n",__func__);
						rsp_ptr->lcd_buf.x = 0;
           			rsp_ptr->lcd_buf.y = 0;
						rsp_ptr->lcd_buf.width = LCD_MAIN_WIDTH;
						rsp_ptr->lcd_buf.height = LCD_MAIN_HEIGHT;
					}
					else
					{
						printk("[UTS] %s : read framebuffer : [%d, %d] ~ [%d,%d]\n",__func__,x_start,y_start,x_end,y_end);
#if 0
						memset(tmp_img_block, 0x00, 320*480*2);
						read_framebuffer(tmp_img_block);

						pImgBlock = lcd_section_buf_info.buf;

						for(j=0;j<LCD_MAIN_HEIGHT;j++)					
						{						
							for(i=0;i<LCD_MAIN_WIDTH;i++)						
							{							
								if(((i>=x_start) && (i<x_end)) && ((j>=y_start) && (j<y_end)))							
								{
									*pImgBlock++ = tmp_img_block[(j*320)+i];
								}
							}					
						}
#else
                  memset(tmp_img_block, 0x00, 320*480*4);
                  memset(tmp_img_block_2,0x00,320*480*2);
                  read_framebuffer(tmp_img_block);

                  to_rgb565(tmp_img_block,tmp_img_block_2);

                  pImgBlock = lcd_section_buf_info.buf;

                  for(j=0;j<LCD_MAIN_HEIGHT;j++)					
                  {						
                  	for(i=0;i<LCD_MAIN_WIDTH;i++)						
                  	{							
                  		if(((i>=x_start) && (i<x_end)) && ((j>=y_start) && (j<y_end)))							
                  		{
                  			*pImgBlock++ = tmp_img_block_2[(j*320)+i];
                  		}
                  	}					
                  }
#endif
						rsp_ptr->lcd_buf.x = lcd_section_buf_info.x;
              			rsp_ptr->lcd_buf.y = lcd_section_buf_info.y;
						rsp_ptr->lcd_buf.width = lcd_section_buf_info.width;
						rsp_ptr->lcd_buf.height = lcd_section_buf_info.height;
					}

					rsp_ptr->lcd_buf.cmd_code = DIAG_LGE_SCREEN_SECTION_SHOT_F;
		          	rsp_ptr->lcd_buf.sub_cmd_code = SCREEN_SHOT_SECTION_LCD_BUF;
		          	rsp_ptr->lcd_buf.ok = TRUE;
					rsp_ptr->lcd_buf.is_main_lcd = FALSE;
		          	rsp_ptr->lcd_buf.seq_flow = SEQ_START;
		          	break;
        		case SEQ_REGET_BUF:
          			lcd_section_buf_info.sended_bytes = 0;
        		case SEQ_GET_BUF:
          			if(lcd_section_buf_info.updated == TRUE)
          			{
              			rsp_len = sizeof(diag_lcd_section_get_buf_req_type);
              			rsp_ptr = (diag_screen_section_shot_type *)diagpkt_alloc(DIAG_LGE_SCREEN_SECTION_SHOT_F, rsp_len);

						rsp_ptr->lcd_buf.cmd_code = DIAG_LGE_SCREEN_SECTION_SHOT_F;
						rsp_ptr->lcd_buf.sub_cmd_code = SCREEN_SHOT_SECTION_LCD_BUF;
						rsp_ptr->lcd_buf.ok = TRUE;
						rsp_ptr->lcd_buf.is_main_lcd = TRUE;
              			rsp_ptr->lcd_buf.x = lcd_section_buf_info.x;
              			rsp_ptr->lcd_buf.y = lcd_section_buf_info.y;
              			rsp_ptr->lcd_buf.width = lcd_section_buf_info.width;
              			rsp_ptr->lcd_buf.height = lcd_section_buf_info.height;
              			rsp_ptr->lcd_buf.total_bytes = lcd_section_buf_info.total_bytes;
              			rsp_ptr->lcd_buf.sended_bytes = lcd_section_buf_info.sended_bytes;
              			rsp_ptr->lcd_buf.packed = FALSE;
              			rsp_ptr->lcd_buf.is_fast_mode = lcd_section_buf_info.is_fast_mode;
              			rsp_ptr->lcd_buf.full_draw = lcd_section_buf_info.full_draw;
              			if(lcd_section_buf_info.total_bytes < SCREEN_SHOT_PACK_LEN*sizeof(unsigned short))
              			{
                        	// completed
                        	printk("[myeonggyu] %s %d\n",__func__,__LINE__);
                  			lcd_section_buf_info.sended_bytes = 0;
                  			rsp_ptr->lcd_buf.sended_bytes = lcd_section_buf_info.sended_bytes*sizeof(unsigned short);

                  			memcpy((void *)&(rsp_ptr->lcd_buf.buf[0]), (void *)&(lcd_section_buf_info.buf[lcd_section_buf_info.sended_bytes/sizeof(unsigned short)]), lcd_section_buf_info.total_bytes*sizeof(unsigned short));
                  			rsp_ptr->lcd_buf.seq_flow = SEQ_GET_BUF_COMPLETED;
                  			lcd_section_buf_info.update = TRUE;
                  			lcd_section_buf_info.updated = FALSE;
                		}
              			else if(lcd_section_buf_info.total_bytes <= lcd_section_buf_info.sended_bytes)
              			{
                    		// completed
                    		printk("[myeonggyu] %s %d\n",__func__,__LINE__);
                			lcd_section_buf_info.sended_bytes -= SCREEN_SHOT_PACK_LEN*sizeof(unsigned short);
                			rsp_ptr->lcd_buf.sended_bytes = lcd_section_buf_info.sended_bytes;

                			memcpy((void *)&(rsp_ptr->lcd_buf.buf[0]), (void *)&(lcd_section_buf_info.buf[lcd_section_buf_info.sended_bytes/sizeof(unsigned short)]), lcd_section_buf_info.total_bytes - lcd_section_buf_info.sended_bytes);

                			rsp_ptr->lcd_buf.seq_flow = SEQ_GET_BUF_COMPLETED;
                			lcd_section_buf_info.update = TRUE;
                			lcd_section_buf_info.updated = FALSE;
              			}
              			else
              			{
                    		// getting
                    		printk("[myeonggyu] %s %d\n",__func__,__LINE__);
                			memcpy((void *)&(rsp_ptr->lcd_buf.buf[0]), (void *)&(lcd_section_buf_info.buf[lcd_section_buf_info.sended_bytes/sizeof(unsigned short)]), SCREEN_SHOT_PACK_LEN*sizeof(unsigned short));
                			lcd_section_buf_info.sended_bytes += SCREEN_SHOT_PACK_LEN*sizeof(unsigned short);
                			rsp_ptr->lcd_buf.seq_flow = SEQ_GET_BUF;
              			}
            		}
            		else
            		{
	            		printk("[myeonggyu] %s %d\n",__func__,__LINE__);
              			rsp_len = sizeof(diag_lcd_section_get_buf_req_type);
              			rsp_ptr = (diag_screen_section_shot_type *)diagpkt_alloc(DIAG_LGE_SCREEN_SECTION_SHOT_F, rsp_len - (SCREEN_SHOT_PACK_LEN*sizeof(unsigned short)));

						rsp_ptr->lcd_buf.cmd_code = DIAG_LGE_SCREEN_SECTION_SHOT_F;
						rsp_ptr->lcd_buf.sub_cmd_code = SCREEN_SHOT_SECTION_LCD_BUF;
						rsp_ptr->lcd_buf.is_main_lcd = TRUE;
              			rsp_ptr->lcd_buf.x = lcd_section_buf_info.x;
              			rsp_ptr->lcd_buf.y = lcd_section_buf_info.y;
              			rsp_ptr->lcd_buf.width = lcd_section_buf_info.width;
              			rsp_ptr->lcd_buf.height = lcd_section_buf_info.height;
              			rsp_ptr->lcd_buf.total_bytes = lcd_section_buf_info.total_bytes;
              			rsp_ptr->lcd_buf.sended_bytes = lcd_section_buf_info.sended_bytes;
              			rsp_ptr->lcd_buf.packed = FALSE;
              			rsp_ptr->lcd_buf.is_fast_mode = lcd_section_buf_info.is_fast_mode;
              			rsp_ptr->lcd_buf.full_draw = lcd_section_buf_info.full_draw;
              			rsp_ptr->lcd_buf.seq_flow = SEQ_GET_BUF_SUSPEND;
            		}
 					break;
        		case SEQ_STOP:
           			rsp_len = sizeof(diag_lcd_section_get_buf_req_type);
           			rsp_ptr = (diag_screen_section_shot_type *)diagpkt_alloc(DIAG_LGE_SCREEN_SECTION_SHOT_F, rsp_len - (SCREEN_SHOT_PACK_LEN*sizeof(unsigned short)));

					rsp_ptr->lcd_buf.cmd_code = DIAG_LGE_SCREEN_SECTION_SHOT_F;
					rsp_ptr->lcd_buf.sub_cmd_code = SCREEN_SHOT_SECTION_LCD_BUF;
           			rsp_ptr->lcd_buf.ok = TRUE;
           			rsp_ptr->lcd_buf.seq_flow = SEQ_STOP;

           			lcd_section_buf_info.update = FALSE;
           			lcd_section_buf_info.updated = FALSE;
        			break;		
      		}
      		break;  		
	}
	return (rsp_ptr);	
}        

EXPORT_SYMBOL(LGE_ScreenSectionShot);        

