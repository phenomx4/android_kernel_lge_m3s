#ifndef LG_DIAG_SCREEN_SECTION_SHOT_H
#define LG_DIAG_SCREEN_SECTION_SHOT_H
// LG_FW_DIAG_KERNEL_SERVICE

#include "lge_comdef.h"
#include "lge_diag_screen_shot.h"

#define SCREEN_SHOT_PACK_LEN  1024

typedef struct
{
  boolean  is_main_lcd;
  word     x;
  word     y;
  word     width;
  word     height;
  dword    total_bytes;
  dword    sended_bytes;
  boolean  update;
  boolean  updated;
  boolean  packed;
  boolean  is_fast_mode;
  boolean  full_draw;
  unsigned short buf[LCD_MAIN_WIDTH * LCD_MAIN_HEIGHT];
}PACKED lcd_section_buf_info_type;


typedef struct
{
  byte     cmd_code;
  byte     sub_cmd_code;
  boolean  main_onoff;
  byte     main_value;
  boolean  sub_onoff;
  byte     sub_value;
  boolean  ok;
}PACKED diag_lcd_section_backlight_ctrl_req_type;

typedef struct
{
  byte     cmd_code;
  byte     sub_cmd_code;
  boolean  ok;
  boolean  is_main_lcd;
  word     x;
  word     y;
  word     width;
  word     height;
  byte     seq_flow;
  dword    total_bytes;
  dword    sended_bytes;
  boolean  packed;
  boolean  is_fast_mode;
  boolean  full_draw;
  unsigned short buf[SCREEN_SHOT_PACK_LEN];
}PACKED diag_lcd_section_get_buf_req_type;


typedef union 
{
	byte cmd_code;
	diag_lcd_section_backlight_ctrl_req_type lcd_bk_ctrl;
	diag_lcd_section_get_buf_req_type lcd_buf;
}PACKED diag_screen_section_shot_type;

#endif /* LG_DIAG_SCREEN_SECTION_SHOT_H */
