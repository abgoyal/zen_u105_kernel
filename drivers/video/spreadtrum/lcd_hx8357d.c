/* drivers/video/sc8810/lcd_hx8357.c
 *
 * Support for hx8357 LCD device
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



static int32_t hx8357d_init(struct panel_spec *self)
{
	send_data_t	send_cmd = self->info.mcu->ops->send_cmd;
	send_data_t	send_data = self->info.mcu->ops->send_data;

	LCD_PRINT("hx8357d_init\n");
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif
send_cmd(0x11);      //Sleep Out 
  LCD_DelayMS(250); 

  send_cmd(0xB9);      //SET password 
  send_data(0xFF);     
  send_data(0x83);     
  send_data(0x57);  
    ////打开TE，解决切屏
#ifdef TE_ON  
  send_cmd(0xB0);         
  send_data(0x44);    
  send_data(0x01); 
#else
  send_cmd(0xB0);         
  send_data(0x66);    
  send_data(0x01); 
#endif  
   
  send_cmd(0xB1);        //SETPower 
  send_data(0x00);     //STB 
  send_data(0x14);     //VGH = 13V, VGL = -10V 
  send_data(0x18);     //VSPR = 4.5V 
  send_data(0x18);     //VSNR = -4.5V 
  send_data(0xC3);    //AP 
  send_data(0x77);     //FS 
   
  send_cmd(0xB4);      //SETCYC 
  send_data(0x22);         //2-dot 
  send_data(0x40);     //RTN 
  send_data(0x00);     //DIV 
  send_data(0x2A);     //N_DUM 
  send_data(0x2A);     //I_DUM 
  send_data(0x20);     //GDON 
  send_data(0x91);     //GDOFF 
   
  send_cmd(0xB6);    //VCOMDC 
  send_data(0x2a);     //0X3C
   
  send_cmd(0xC0);    //SETSTBA 
  send_data(0x50);     //N_OPON 
  send_data(0x50);     //I_OPON 
  send_data(0x00);     //STBA 
  send_data(0x3C);    //STBA 
  send_data(0xC4);    //STBA 
  send_data(0x08);     //GENON 
 
 
  send_cmd(0xC2);      // Set Gate EQ 
  send_data(0x00); 
  send_data(0x08); 
  send_data(0x04);  
   
  send_cmd(0xCC);     //Set Panel 
  send_data(0x09);     //SS_Panel = 1, BGR_Panel = 1 
   
  send_cmd(0xE0);      //Set Gamma 
  send_data(0x03);     //VRP0 
  send_data(0x07);      //VRP1 
  send_data(0x13);      //VRP2 
  send_data(0x23);      //VRP3 
  send_data(0x2B);      //VRP4 
  send_data(0x3C);      //VRP5 
  send_data(0x48);      //VRP6 
  send_data(0x50);      //VRP7 
  send_data(0x49);      //VRP8 
  send_data(0x42);      //VRP9 
  send_data(0x3C);      //VRP10 
  send_data(0x31);      //VRP11 
  send_data(0x2F);      //VRP12 
  send_data(0x29);      //VRP13 
  send_data(0x25);      //VRP14 
  send_data(0x1C);      //VRP15 
  send_data(0x03);      //VRN0 
  send_data(0x07);      //VRN1 
  send_data(0x13);      //VRN2 
  send_data(0x2B);      //VRN3 
  send_data(0x2B);      //VRN4 
  send_data(0x3C);      //VRN5 
  send_data(0x48);      //VRN6 
  send_data(0x50);      //VRN7 
  send_data(0x49);      //VRN8 
  send_data(0x42);      //VRN9 
  send_data(0x3C);      //VRN10 
  send_data(0x31);      //VRN11 
  send_data(0x2F);      //VRN12 
  send_data(0x29);      //VRN13 
  send_data(0x25);      //VRN14 
  send_data(0x1C);      //VRN15 
  send_data(0x00); 
  send_data(0x01);     //GMA_Reload 
   
  send_cmd(0x3A);      //COLMOD 
  send_data(0x05);        //RGB666     

  send_cmd(0x35);      //TE ON
  send_data(0x00);      //   

  //send_cmd(0x44);     
 // send_data(0x01);     
 // send_data(0x22);  
//LCD_DelayMS(200); 
 
send_cmd(0x29);      //Display On 
LCD_DelayMS(10); 
send_cmd(0x2C);    //Write SRAM Data
    
    send_cmd(0x2c);  //Write SRAM Data

#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif
	return 0;
}

static int32_t hx8357d_enter_sleep(struct panel_spec *self, uint8_t is_sleep)
{
	LCD_PRINT("hx8357d_enter_sleep\n");
	send_data_t	send_cmd = self->info.mcu->ops->send_cmd;

#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif

	if(is_sleep) {
          send_cmd(0x28);
          LCD_DelayMS(10);
          send_cmd(0x10); // 
          LCD_DelayMS(120);
	}
	else {
#if 1
		//Sleep Out
          send_cmd(0x11); // 
          LCD_DelayMS(120);
          send_cmd(0x29);
          LCD_DelayMS(10);
#else
		// re init	
		self->ops->lcd_reset(self);
		self->ops->lcd_init(self);
#endif
	}

#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif
	return 0;
}

static int32_t hx8357d_set_window(struct panel_spec *self,
		uint16_t left, uint16_t top, uint16_t right, uint16_t bottom)
{
	LCD_PRINT("hx8357d_set_window\n");
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

static int32_t hx8357d_invalidate(struct panel_spec *self)
{
	LCD_PRINT("hx8357d_invalidate\n");

	return self->ops->panel_set_window(self, 0, 0, 
			self->width-1, self->height-1);
}

static uint32_t hx8357d_readid(struct panel_spec *self)
{
	send_data_t send_cmd = self->info.mcu->ops->send_cmd;
	send_data_t send_data = self->info.mcu->ops->send_data;
	read_data_t read_data = self->info.mcu->ops->read_data;
    uint32_t ID0 =0,ID1=0;
	uint32_t    i = 0, result = 0;
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif
	
    send_cmd(0xB9); //SET password
    send_data(0xFF);
    send_data(0x83);
    send_data(0x57);
    LCD_DelayMS(1);

    send_cmd(0xD0);     
    LCD_DelayMS(10); 
    ID0 = read_data(); //???¨￠ 
#if (CONFIG_ZYT_CUST == 31)
    while(i < 6)
	{
		//ID0 = read_data(); //???¨￠ 
		ID1 = read_data(); //0X99    
		LCD_PRINT("kernel hx8357d_read_id(%d): id=0x%x\n", i, ID1);
		if (ID1 == 0X99)
		{
			result = 1;
		    break;
	    }
	    i++;
	}
#else
	//ID0 = read_data(); //???¨￠ 
	ID1 = read_data(); //0X99    
#endif   
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif	
	LCD_PRINT("kernel hx8357d_read_id=%x\n", ID1);
	return ID1;

}

static int32_t hx8357d_set_direction(struct panel_spec *self, uint16_t direction)
{
	LCD_PRINT("hx8357d_set_direction\n");
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

static int32_t hx8357d_invalidate_rect(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom)
{
	LCD_PRINT("hx8357d_invalidate_rect : (%d, %d, %d, %d)\n",left, top, right, bottom);

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

static struct panel_operations lcd_hx8357d_operations = {
	.panel_init = hx8357d_init,
	.panel_set_window = hx8357d_set_window,
	.panel_invalidate = hx8357d_invalidate,
	.panel_invalidate_rect = hx8357d_invalidate_rect,
	.panel_set_direction = hx8357d_set_direction,
	.panel_enter_sleep = hx8357d_enter_sleep,
	.panel_readid = hx8357d_readid,
};

static struct timing_mcu lcd_hx8357d_timing[] = {


[LCD_REGISTER_TIMING] = {                // read/write register timing
		.rcss = 25,  // 25 ns
		.rlpw =50,//70,
		.rhpw = 90,//70,
		.wcss = 50,//10,
		.wlpw = 55,//50,
		.whpw = 55,//50,
	},
[LCD_GRAM_TIMING] = {                // read/write gram timing
		.rcss = 25,
		.rlpw = 70,
		.rhpw = 70,
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
        .wcss = 15,
        .wlpw = 20,//这里不能改
        .whpw = 20,//这里不能改
#else
		.wcss = 15,
		.wlpw = 15,
		.whpw = 15,
#endif
	}

};

static struct info_mcu lcd_hx8357d_info = {
	.bus_mode = LCD_BUS_8080,
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	.bus_width = 8,
#else
	.bus_width = 16,
#endif
	.timing = lcd_hx8357d_timing,
	.ops = NULL,
};

struct panel_spec lcd_panel_hx8357d = {
	.width = 320,
	.height = 480,
	.mode = LCD_MODE_MCU,
	.direction = LCD_DIRECT_NORMAL,
	.info = {.mcu = &lcd_hx8357d_info},
	.ops = &lcd_hx8357d_operations,
};

struct panel_cfg lcd_hx8357d = {
    .lcd_cs = -1,
    .lcd_id = 0x99,
    .lcd_name = "lcd_hx8357d",
    .panel = &lcd_panel_hx8357d,
};
static int __init lcd_hx8357d_init(void)
{
    //lcd_regulator();
    return sprd_register_panel(&lcd_hx8357d);
}
    
subsys_initcall(lcd_hx8357d_init);
