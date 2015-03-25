/* drivers/video/sc8800g/sc8800g_lcd_st7796s.c
 *
 * Support for st7796s LCD device
 *
 * Copyright (C) 2010 Spreadtrum
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
//harry add
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>
#include "lcdpanel.h"
//#include <mach/lcd.h>
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
//#include <linux/io.h>
//#include "lcdc_reg.h"
#include "sprdfb.h"
//#include <mach/hardware.h>
#endif

#undef  LCD_DEBUG
#ifdef LCD_DEBUG
#define LCD_PRINT printk
#else
#define LCD_PRINT(...)
#endif

static int32_t st7796s_init(struct panel_spec *self)
{
	send_data_t	send_cmd = self->info.mcu->ops->send_cmd;
	send_data_t	send_data = self->info.mcu->ops->send_data;

	LCD_PRINT("st7796s_init\n");
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif
	 send_cmd(0x11);
         LCD_DelayMS(120);
        
        send_cmd(0xf0);
        send_data(0xc3);
        send_cmd(0xf0);
        send_data(0x96);
        
      send_cmd(0x35);    // TE ON
      send_data(0x00);
        
        send_cmd(0x36);
        send_data(0x48);//48
        send_cmd(0x3a);
        send_data(0x55);
        send_cmd(0xb4);
        send_data(0x01);
        send_cmd(0xe8);
        send_data(0x40);
        send_data(0x8a);
        send_data(0x00);
        send_data(0x00);
        send_data(0x29);
        send_data(0x19);
        send_data(0xa5);
        send_data(0x33);
        
        send_cmd(0xc1);
        send_data(0x0f);
        send_cmd(0xc2);
        send_data(0xa7);
        send_cmd(0xc5);
        send_data(0x27);
        send_cmd(0xe0);
        send_data(0xf0);
        send_data(0x09);
        send_data(0x0b);
        send_data(0x06);
        send_data(0x04);
        send_data(0x15);
        send_data(0x2f);
        send_data(0x54);
        send_data(0x42);
        send_data(0x3c);
        send_data(0x17);
        send_data(0x14);
        send_data(0x18);
        send_data(0x1b);
        
        send_cmd(0xe1);
        send_data(0xf0);
        send_data(0x09);
        send_data(0x0b);
        send_data(0x06);
        send_data(0x04);
        send_data(0x03);
        send_data(0x2d);
        send_data(0x43);
        send_data(0x42);
        send_data(0x3b);
        send_data(0x16);
        send_data(0x14);
        send_data(0x17);
        send_data(0x1b);
        
        send_cmd(0xf0);
        send_data(0x3c);
        send_cmd(0xf0);
        send_data(0x69);
         LCD_DelayMS(120);
        send_cmd(0x29); //Display On
         LCD_DelayMS( 25);
        
        send_cmd( 0x2C);
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif
	return 0;
}

static int32_t st7796s_enter_sleep(struct panel_spec *self, uint8_t is_sleep)
{
	LCD_PRINT("st7796_enter_sleep\n");
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif
	if ( is_sleep ){
		self->info.mcu->ops->send_cmd(0x0028);
		LCD_DelayMS(10);
		self->info.mcu->ops->send_cmd(0x0010);
		LCD_DelayMS(120);
	}
	else{
#ifndef LCD_ADJUST_PARAM
		self->info.mcu->ops->send_cmd(0x0011);
		LCD_DelayMS(120);
		self->info.mcu->ops->send_cmd(0x0029);
		self->info.mcu->ops->send_cmd(0x2C);
#else
		self->ops->lcd_reset(self);
		self->ops->lcd_init(self);
#endif

	}

#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif
	return 0;
}

static int32_t st7796s_set_window(struct panel_spec *self,
		uint16_t left, uint16_t top, uint16_t right, uint16_t bottom)
{
	LCD_PRINT("st7796s_set_window\n");

#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif    
	self->info.mcu->ops->send_cmd(0x002A);
	self->info.mcu->ops->send_data((left>>8) & 0xff);
	self->info.mcu->ops->send_data(left&0xff);
	self->info.mcu->ops->send_data((right>>8) & 0xff);
	self->info.mcu->ops->send_data(right&0xff);

	self->info.mcu->ops->send_cmd(0x2B); //power setting 
	self->info.mcu->ops->send_data((top>>8) & 0xff); 
	self->info.mcu->ops->send_data(top&0xff); 
	self->info.mcu->ops->send_data((bottom>>8) & 0xff); 
	self->info.mcu->ops->send_data(bottom&0xff); 
	
	self->info.mcu->ops->send_cmd(0x002c);
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif
	return 0;
}

static int32_t st7796s_invalidate(struct panel_spec *self)
{
	LCD_PRINT("st7796s_invalidate\n");

	return self->ops->panel_set_window(self, 0, 0, 
			self->width-1, self->height-1);
}

static uint32_t st7796s_readid(struct panel_spec *self)
{
	send_data_t	send_cmd = self->info.mcu->ops->send_cmd;
	read_data_t	read_data = self->info.mcu->ops->read_data;
	uint32_t	lcm_dev_id = 0, data_read;
	uint32_t i = 0;
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif

	send_cmd(0xD3);

	data_read = read_data();  //param1:dummy	
	LCD_PRINT("st7796s_readid(0): 0x%x", data_read);
	data_read = read_data();  //param2:0
	LCD_PRINT("st7796s_readid(1): 0x%x", data_read);

	data_read = read_data(); //param3:77
	LCD_PRINT("st7796s_readid(2): 0x%x", data_read);
	lcm_dev_id = data_read & 0xFF;
	
	data_read = read_data(); //param4:96
	LCD_PRINT("st7796s_readid(3): 0x%x\n", data_read);
	lcm_dev_id = (lcm_dev_id << 8) | (data_read & 0xFF);

	LCD_PRINT("st7796s_readid: 0x%x\n", lcm_dev_id);   
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif	

	return lcm_dev_id;
}

static int32_t st7796s_set_direction(struct panel_spec *self, uint16_t direction)
{
	LCD_PRINT("st7796s_set_direction\n");
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif	

	switch (direction) {
	case LCD_DIRECT_NORMAL:
		self->info.mcu->ops->send_cmd_data(0x0036, 0x08);
		break;
	case LCD_DIRECT_ROT_90:
		self->info.mcu->ops->send_cmd_data(0x0036, 0x00E8);
		break;
	case LCD_DIRECT_ROT_180:
	case LCD_DIRECT_MIR_HV:
		self->info.mcu->ops->send_cmd_data(0x0036, 0x0008);
		break;
	case LCD_DIRECT_ROT_270:
		self->info.mcu->ops->send_cmd_data(0x0036, 0x0028);
		break;
	case LCD_DIRECT_MIR_H:
		self->info.mcu->ops->send_cmd_data(0x0036, 0x0088);
		break;
	case LCD_DIRECT_MIR_V:
		self->info.mcu->ops->send_cmd_data(0x0036, 0x0048);
		break;
	default:
		LCD_PRINT("unknown lcd direction!\n");
		self->info.mcu->ops->send_cmd_data(0x0036, 0x00c8);
		direction = LCD_DIRECT_NORMAL;
		break;
	}

	self->direction = direction;
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif
	return 0;
}

static int32_t st7796s_invalidate_rect(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom)
{
	LCD_PRINT("st7796s_invalidate_rect : (%d, %d, %d, %d)\n",left, top, right, bottom);

	left 	= (left >= self->width)    ? (self->width-1) : left;
	right 	= (right >= self->width)   ? (self->width-1) : right;
	top 	= (top >= self->height)    ? (self->height-1) : top;
	bottom 	= (bottom >= self->height) ? (self->height-1) : bottom;

	/*
	if ( ( right < left ) || ( bottom < top ) )
	{
		return ERR_LCD_OPERATE_FAIL;
	}
	*/

	return self->ops->panel_set_window(self, left, top, 
			right, bottom);
}

static struct panel_operations lcd_st7796s_operations = {
	.panel_init = st7796s_init,
	.panel_set_window = st7796s_set_window,
	.panel_invalidate_rect = st7796s_invalidate_rect,
	.panel_invalidate = st7796s_invalidate,
	.panel_set_direction = st7796s_set_direction,
	.panel_enter_sleep = st7796s_enter_sleep,
	.panel_readid = st7796s_readid,
};

static struct timing_mcu lcd_st7796s_timing[] = {
[LCD_REGISTER_TIMING] = {                // read/write register timing
		.rcss = 45,
		.rlpw = 60,
		.rhpw = 100,
		.wcss = 10,
		.wlpw = 28,
		.whpw = 28,
	},
[LCD_GRAM_TIMING] = {                // read/write gram timing
		.rcss = 25,
		.rlpw = 70,
		.rhpw = 70,

		.wcss = 10,
		.wlpw = 28,
		.whpw = 28,

	}
};

static struct info_mcu lcd_st7796s_info = {
	.bus_mode = LCD_BUS_8080,
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	.bus_width = 8,
#else
	.bus_width = 16,
#endif
	.timing = lcd_st7796s_timing,
	.ops = NULL,
};

struct panel_spec lcd_panel_st7796s = {
	.width = 320,
	.height = 480,
	.mode = LCD_MODE_MCU,
	.direction = LCD_DIRECT_NORMAL,
	.info = {.mcu = &lcd_st7796s_info},
	.ops = &lcd_st7796s_operations,
};

struct panel_cfg lcd_st7796s = {
    .lcd_cs = -1,
    .lcd_id = 0x7796,
    .lcd_name = "lcd_st7796s",
    .panel = &lcd_panel_st7796s,
};
static int __init lcd_st7796s_init(void)
{
    //lcd_regulator();
    return sprd_register_panel(&lcd_st7796s);
}
    
subsys_initcall(lcd_st7796s_init);
