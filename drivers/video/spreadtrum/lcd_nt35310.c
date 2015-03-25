/* drivers/video/sc8810/lcd_nt35310.c
 *
 * Support for nt35310 LCD device
 *
 * Copyright (C) 2012 Freecom
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

#if (7036 == CONFIG_ZYT_PRJ) && (63 == CONFIG_ZYT_CUST)
	#define CONFIG_ZYT_nt35310_7036_XLL_S05              //Ï²À´ÀÖS05 LCM-G500T50-0000 V00£¨S05-5´çlcd£©
#endif

static int32_t nt35310_init(struct panel_spec *self)
{
	send_data_t	send_cmd = self->info.mcu->ops->send_cmd;
	send_data_t	send_data = self->info.mcu->ops->send_data;

	LCD_PRINT("nt35310_init\n");
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif
#if defined(CONFIG_ZYT_nt35310_7036_XLL_S05)
	send_cmd(0xED);
	send_data(0x01);
	send_data(0xFE); 

	send_cmd(0xEE);
	send_data(0xDE);
	send_data(0x21); 

	send_cmd(0xF1);
	send_data(0x01);
	 
	send_cmd(0xDF);
	send_data(0x10);
	 
	send_cmd(0xB7);
	send_data(0x00);

	send_cmd(0xC0);
	send_data(0x3c);
	send_data(0x3c);
	send_data(0x10);
	send_data(0x10);

	send_cmd(0xC2);
	send_data(0x44);
	send_data(0x44);
	send_data(0x44);

	send_cmd(0xC6);
	send_data(0x00);
	send_data(0xE4);
	send_data(0xE4);
	send_data(0xE4);
	send_cmd(0xC4);
	send_data(0x28);

	send_cmd(0xBF);
	send_data(0xAA); 

	send_cmd(0xB8);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);

	send_cmd(0xB7);
	send_data(0x00);
	send_data(0x00);
	send_data(0x3F);
	send_data(0x00);
	send_data(0x5E);
	send_data(0x00);
	send_data(0x64);
	send_data(0x00);
	send_data(0x8C);
	send_data(0x00);
	send_data(0xAC);
	send_data(0x00);
	send_data(0xDC);
	send_data(0x00);
	send_data(0x70);
	send_data(0x00);
	send_data(0x90);
	send_data(0x00);
	send_data(0xEB);
	send_data(0x00);
	send_data(0xDC);
	send_data(0x00);

	send_cmd(0xC2);
	send_data(0x0A);
	send_data(0x00);
	send_data(0x04);
	send_data(0x00);

	send_cmd(0xC7);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);

	send_cmd(0xC9);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);

	send_cmd(0xE0);
	send_data(0x01);
	send_data(0x00);
	send_data(0x03);
	send_data(0x00);
	send_data(0x0B);
	send_data(0x00);
	send_data(0x24);
	send_data(0x00);
	send_data(0x3B);
	send_data(0x00);
	send_data(0x4E);
	send_data(0x00);
	send_data(0x65);
	send_data(0x00);
	send_data(0x7D);
	send_data(0x00);
	send_data(0x8F);
	send_data(0x00);
	send_data(0x9A);
	send_data(0x00);
	send_data(0xA7);
	send_data(0x00);
	send_data(0xB5);
	send_data(0x00);
	send_data(0xBD);
	send_data(0x00);
	send_data(0xC1);
	send_data(0x00);
	send_data(0xC8);
	send_data(0x00);
	send_data(0xD1);
	send_data(0x00);
	send_data(0xE2);
	send_data(0x00);
	send_data(0xF3);
	send_data(0x00);

	send_cmd(0xE1);
	send_data(0x01);
	send_data(0x00);
	send_data(0x03);
	send_data(0x00);
	send_data(0x0B);
	send_data(0x00);
	send_data(0x24);
	send_data(0x00);
	send_data(0x3B);
	send_data(0x00);
	send_data(0x4E);
	send_data(0x00);
	send_data(0x65);
	send_data(0x00);
	send_data(0x7D);
	send_data(0x00);
	send_data(0x8F);
	send_data(0x00);
	send_data(0x9A);
	send_data(0x00);
	send_data(0xA7);
	send_data(0x00);
	send_data(0xB5);
	send_data(0x00);
	send_data(0xBD);
	send_data(0x00);
	send_data(0xC1);
	send_data(0x00);
	send_data(0xC8);
	send_data(0x00);
	send_data(0xD1);
	send_data(0x00);
	send_data(0xE2);
	send_data(0x00);
	send_data(0xF3);
	send_data(0x00); 

	send_cmd(0xE2);
	send_data(0x01);
	send_data(0x00);
	send_data(0x03);
	send_data(0x00);
	send_data(0x0B);
	send_data(0x00);
	send_data(0x24);
	send_data(0x00);
	send_data(0x3B);
	send_data(0x00);
	send_data(0x4E);
	send_data(0x00);
	send_data(0x65);
	send_data(0x00);
	send_data(0x7D);
	send_data(0x00);
	send_data(0x8F);
	send_data(0x00);
	send_data(0x9A);
	send_data(0x00);
	send_data(0xA7);
	send_data(0x00);
	send_data(0xB5);
	send_data(0x00);
	send_data(0xBD);
	send_data(0x00);
	send_data(0xC1);
	send_data(0x00);
	send_data(0xC8);
	send_data(0x00);
	send_data(0xD1);
	send_data(0x00);
	send_data(0xE2);
	send_data(0x00);
	send_data(0xF3);
	send_data(0x00); 

	send_cmd(0xE3);
	send_data(0x01);
	send_data(0x00);
	send_data(0x03);
	send_data(0x00);
	send_data(0x0B);
	send_data(0x00);
	send_data(0x24);
	send_data(0x00);
	send_data(0x3B);
	send_data(0x00);
	send_data(0x4E);
	send_data(0x00);
	send_data(0x65);
	send_data(0x00);
	send_data(0x7D);
	send_data(0x00);
	send_data(0x8F);
	send_data(0x00);
	send_data(0x9A);
	send_data(0x00);
	send_data(0xA7);
	send_data(0x00);
	send_data(0xB5);
	send_data(0x00);
	send_data(0xBD);
	send_data(0x00);
	send_data(0xC1);
	send_data(0x00);
	send_data(0xC8);
	send_data(0x00);
	send_data(0xD1);
	send_data(0x00);
	send_data(0xE2);
	send_data(0x00);
	send_data(0xF3);
	send_data(0x00); 

	send_cmd(0xE4);
	send_data(0x01);
	send_data(0x00);
	send_data(0x03);
	send_data(0x00);
	send_data(0x0B);
	send_data(0x00);
	send_data(0x24);
	send_data(0x00);
	send_data(0x3B);
	send_data(0x00);
	send_data(0x4E);
	send_data(0x00);
	send_data(0x65);
	send_data(0x00);
	send_data(0x7D);
	send_data(0x00);
	send_data(0x8F);
	send_data(0x00);
	send_data(0x9A);
	send_data(0x00);
	send_data(0xA7);
	send_data(0x00);
	send_data(0xB5);
	send_data(0x00);
	send_data(0xBD);
	send_data(0x00);
	send_data(0xC1);
	send_data(0x00);
	send_data(0xC8);
	send_data(0x00);
	send_data(0xD1);
	send_data(0x00);
	send_data(0xE2);
	send_data(0x00);
	send_data(0xF3);
	send_data(0x00);

	send_cmd(0xE5);
	send_data(0x01);
	send_data(0x00);
	send_data(0x03);
	send_data(0x00);
	send_data(0x0B);
	send_data(0x00);
	send_data(0x24);
	send_data(0x00);
	send_data(0x3B);
	send_data(0x00);
	send_data(0x4E);
	send_data(0x00);
	send_data(0x65);
	send_data(0x00);
	send_data(0x7D);
	send_data(0x00);
	send_data(0x8F);
	send_data(0x00);
	send_data(0x9A);
	send_data(0x00);
	send_data(0xA7);
	send_data(0x00);
	send_data(0xB5);
	send_data(0x00);
	send_data(0xBD);
	send_data(0x00);
	send_data(0xC1);
	send_data(0x00);
	send_data(0xC8);
	send_data(0x00);
	send_data(0xD1);
	send_data(0x00);
	send_data(0xE2);
	send_data(0x00);
	send_data(0xF3);
	send_data(0x00); 

	send_cmd(0xE6);
	send_data(0x11);
	send_data(0x00);
	send_data(0x44);
	send_data(0x00);
	send_data(0x99);
	send_data(0x00);
	send_data(0xAA);
	send_data(0x00);
	send_data(0x99);
	send_data(0x00);
	send_data(0x88);
	send_data(0x00);
	send_data(0x99);
	send_data(0x00);
	send_data(0x77);
	send_data(0x00);
	send_data(0xAA);
	send_data(0x00);
	send_data(0x77);
	send_data(0x00);
	send_data(0x55);
	send_data(0x00);
	send_data(0x33);
	send_data(0x00);
	send_data(0x44);
	send_data(0x00);
	send_data(0x66);
	send_data(0x00);
	send_data(0x77);
	send_data(0x00);
	send_data(0x33);
	send_data(0x00); 

	send_cmd(0xE7);
	send_data(0x11);
	send_data(0x00);
	send_data(0x44);
	send_data(0x00);
	send_data(0x99);
	send_data(0x00);
	send_data(0xAA);
	send_data(0x00);
	send_data(0x99);
	send_data(0x00);
	send_data(0x88);
	send_data(0x00);
	send_data(0x99);
	send_data(0x00);
	send_data(0x77);
	send_data(0x00);
	send_data(0xAA);
	send_data(0x00);
	send_data(0x77);
	send_data(0x00);
	send_data(0x55);
	send_data(0x00);
	send_data(0x33);
	send_data(0x00);
	send_data(0x44);
	send_data(0x00);
	send_data(0x66);
	send_data(0x00);
	send_data(0x77);
	send_data(0x00);
	send_data(0x33);
	send_data(0x00); 

	send_cmd(0xE8);
	send_data(0x11);
	send_data(0x00);
	send_data(0x44);
	send_data(0x00);
	send_data(0x99);
	send_data(0x00);
	send_data(0xAA);
	send_data(0x00);
	send_data(0x99);
	send_data(0x00);
	send_data(0x88);
	send_data(0x00);
	send_data(0x99);
	send_data(0x00);
	send_data(0x77);
	send_data(0x00);
	send_data(0xAA);
	send_data(0x00);
	send_data(0x77);
	send_data(0x00);
	send_data(0x55);
	send_data(0x00);
	send_data(0x33);
	send_data(0x00);
	send_data(0x44);
	send_data(0x00);
	send_data(0x66);
	send_data(0x00);
	send_data(0x77);
	send_data(0x00);
	send_data(0x33);
	send_data(0x00); 

	send_cmd(0xE9);
	send_data(0xAA);
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);  

	send_cmd(0x00);
	send_data(0xAA); 

	send_cmd(0xF2);
	send_data(0xF1);
	send_data(0x54);
	send_data(0xA5);
	send_data(0x04);
	send_data(0x1E);
	send_data(0x44);
	send_data(0x80);
	send_data(0x02);
	send_data(0x2C);
	send_data(0x2C);
	send_data(0x32);
	send_data(0x4A);

	send_cmd(0xF3);
	send_data(0x00);

	send_cmd(0xF9);
	send_data(0x06);
	send_data(0x10);
	send_data(0x29);
	send_data(0x00); 

	send_cmd(0x3A);
	send_data(0x55);

	send_cmd(0x2A);
	send_data(0x00);
	send_data(0x18);
	send_data(0x01);
	send_data(0x27);
	send_cmd(0x11);
	//zyt add
	send_cmd(0x36);  
	send_data(0xc0);  // 180¡ã
	LCD_DelayMS(120);
	send_cmd(0x29);
	send_cmd(0x35);
	send_data(0x00);
#else
    send_cmd(0xED);  
    send_data(0x01);  
    send_data( 0xFE); 
    
    send_cmd(0xEE);  
    send_data(0xDE);  
    send_data(0x21);
    
    send_cmd(0xB3);  
    send_data(0x20);
    
    send_cmd(0xF1);  
    send_data(0x01);
    
    send_cmd(0xDF);  
    send_data(0x10);
    
    send_cmd(0x51);  
    send_data(0xFF);  
    
    send_cmd(0x53);  
    send_data(0x2C);
    
    send_cmd(0x55);  
    send_data(0x02);  
    
    
    
    send_cmd(0xC0);  
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
    send_data(0X44);  //44
    send_data(0X44); //44
    send_data(0X10);  //10
    send_data(0X10);  //10
#else
    send_data(0X4c);  //44
    send_data(0X4c); //44
    send_data(0X11);  //10
    send_data(0X11);  //10
#endif
    
    send_cmd(0xC4);  
    send_data(0X6e);
    
    
    send_cmd(0xBF);  
    send_data(0xAA);
    
    send_cmd(0xB0);  
    send_data(0x0D);  
    send_data(0x00);  
    send_data(0x0D);  
    send_data(0x00);  
    send_data( 0x11);  
    send_data(0x00);  
    send_data( 0x19); 
    send_data(0x00);  
    send_data( 0x21);  
    send_data(0x00);  
    send_data( 0x2D);  
    send_data(0x00);  
    send_data( 0x3D);  
    send_data(0x00);  
    send_data( 0x5D);  
    send_data(0x00);  
    send_data( 0x5D);  
    send_data(0x00);
    
    send_cmd(0xB1);  
    send_data( 0x80);  
    send_data(0x00);  
    send_data( 0x8B);  
    send_data(0x00);  
    send_data( 0x96);  
    send_data(0x00);
    
    send_cmd(0xB2);  
    send_data( 0x00);  
    send_data(0x00);  
    send_data( 0x02);  
    send_data(0x00);  
    send_data( 0x03);  
    send_data(0x00);
    
    send_cmd(0xB3);  
    send_data( 0x00);  
    send_data(0x00);  
    send_data( 0x00);  
    send_data(0x00);  
    send_data( 0x00);  
    send_data(0x00);  
    send_data( 0x00); 
    send_data(0x00);  
    send_data( 0x00);  
    send_data(0x00);  
    send_data( 0x00);  
    send_data(0x00);  
    send_data( 0x00);  
    send_data(0x00);  
    send_data( 0x00);  
    send_data(0x00);  
    send_data( 0x00);  
    send_data(0x00);  
    send_data( 0x00); 
    send_data(0x00);  
    send_data( 0x00);  
    send_data(0x00);  
    send_data( 0x00);  
    send_data(0x00);
    
    send_cmd(0xB4);  
    send_data( 0x8B);  
    send_data(0x00);  
    send_data( 0x96);  
    send_data(0x00);  
    send_data( 0xA1);  
    send_data(0x00);
    
    send_cmd(0xB5);  
    send_data( 0x02);  
    send_data(0x00);  
    send_data( 0x03);  
    send_data(0x00);  
    send_data( 0x04);  
    send_data(0x00);
    
    send_cmd(0xB6);  
    send_data( 0x00);  
    send_data(0x00);
    
    send_cmd(0xB8);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);
    
    send_cmd(0xC2);  
    send_data(0x0A);  
    send_data(0x00);  
    send_data(0x04); 
    send_data(0x00);
    
    send_cmd(0xC7);  
    send_data(0x00); 
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);
    
    send_cmd(0xC9);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00); 
    send_data(0x00); 
    send_data(0x00); 
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00); 
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);
    
    send_cmd(0xB7);  
    send_data(0x3F);  
    send_data(0x00);  
    send_data(0x5E);  
    send_data(0x00);  
    send_data(0x9E);  
    send_data(0x00);  
    send_data(0x74);  
    send_data(0x00);  
    send_data(0x8C);  
    send_data(0x00);  
    send_data(0xAC);  
    send_data(0x00);  
    send_data(0xDC);  
    send_data(0x00);  
    send_data(0x70);  
    send_data(0x00);  
    send_data(0xB9);  
    send_data(0x00);  
    send_data(0xEC);  
    send_data(0x00);  
    send_data(0xDC);  
    send_data(0x00); 
    
    send_cmd(0xE0);  
    send_data(0x01);  
    send_data(0x00); 
    send_data(0x05); 
    send_data(0x00);  
    send_data(0x0F);  
    send_data(0x00);  
    send_data(0x2F);  
    send_data(0x00);  
    send_data(0x45);  
    send_data(0x00);  
    send_data(0x54); 
    send_data(0x00);  
    send_data(0x67);  
    send_data(0x00);  
    send_data(0x7C);  
    send_data(0x00);  
    send_data(0x8C);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0xA8);  
    send_data(0x00);  
    send_data(0xBA);  
    send_data(0x00);  
    send_data(0xC5);  
    send_data(0x00);  
    send_data(0xCB);  
    send_data(0x00);  
    send_data(0xD3);  
    send_data(0x00);  
    send_data(0xDA);  
    send_data(0x00);  
    send_data(0xE3);  
    send_data(0x00);  
    send_data(0xF3);  
    send_data(0x00);
    
    send_cmd(0xE2);  
    send_data(0x01);  
    send_data(0x00);  
    send_data(0x05);  
    send_data(0x00);  
    send_data(0x0F);  
    send_data(0x00);  
    send_data(0x2F);  
    send_data(0x00);  
    send_data(0x45);  
    send_data(0x00);  
    send_data(0x54);  
    send_data(0x00);  
    send_data(0x67);  
    send_data(0x00);  
    send_data(0x7C);  
    send_data(0x00);  
    send_data(0x8C);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0xA8);  
    send_data(0x00);  
    send_data(0xBA);  
    send_data(0x00);  
    send_data(0xC5); 
    send_data(0x00);  
    send_data(0xCB);  
    send_data(0x00);  
    send_data(0xD3);  
    send_data(0x00);  
    send_data(0xDA);  
    send_data(0x00);  
    send_data(0xE3);  
    send_data(0x00);  
    send_data(0xF3);  
    send_data(0x00);
    
    send_cmd(0xE4);  
    send_data(0x01);  
    send_data(0x00);  
    send_data(0x05);  
    send_data(0x00);  
    send_data(0x0F);  
    send_data(0x00);  
    send_data(0x2F);  
    send_data(0x00);  
    send_data(0x45);  
    send_data(0x00);  
    send_data(0x54);  
    send_data(0x00);  
    send_data(0x67);  
    send_data(0x00);  
    send_data(0x7C);  
    send_data(0x00);  
    send_data(0x8C);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0xA8);  
    send_data(0x00);  
    send_data(0xBA);  
    send_data(0x00);  
    send_data(0xC5);  
    send_data(0x00);  
    send_data(0xCB);  
    send_data(0x00);  
    send_data(0xD3);  
    send_data(0x00);  
    send_data(0xDA);  
    send_data(0x00);  
    send_data(0xE3);  
    send_data(0x00);  
    send_data(0xF3);  
    send_data(0x00);
    		
    send_cmd(0xE1);  
    send_data(0x00); 
    send_data(0x00);  
    send_data(0x04);  
    send_data(0x00);  
    send_data(0x0F);  
    send_data(0x00);  
    send_data(0x2F);  
    send_data(0x00); 
    send_data(0x45);  
    send_data(0x00);  
    send_data(0x54);  
    send_data(0x00);  
    send_data(0x66);  
    send_data(0x00);  
    send_data(0x7D);  
    send_data(0x00);  
    send_data(0x8B);  
    send_data(0x00);  
    send_data(0x99); 
    send_data(0x00);  
    send_data(0xA8);  
    send_data(0x00);  
    send_data(0xBA);  
    send_data(0x00);  
    send_data(0xC5); 
    send_data(0x00);  
    send_data(0xCB);  
    send_data(0x00);  
    send_data(0xD4);  
    send_data(0x00);  
    send_data(0xD9);  
    send_data(0x00);  
    send_data(0xE3);  
    send_data(0x00);  
    send_data(0xF3);  
    send_data(0x00);
    
    send_cmd(0xE3);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x04);  
    send_data(0x00);  
    send_data(0x0F);  
    send_data(0x00);  
    send_data(0x2F);  
    send_data(0x00);  
    send_data(0x45);  
    send_data(0x00);  
    send_data(0x54);  
    send_data(0x00);  
    send_data(0x66);  
    send_data(0x00); 
    send_data(0x7D);  
    send_data(0x00);  
    send_data(0x8B);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0xA8);  
    send_data(0x00);  
    send_data(0xBA); 
    send_data(0x00);  
    send_data(0xC5);  
    send_data(0x00);  
    send_data(0xCB);  
    send_data(0x00);  
    send_data(0xD4);  
    send_data(0x00);  
    send_data(0xD9);  
    send_data(0x00);  
    send_data(0xE3);  
    send_data(0x00);  
    send_data(0xF3);  
    send_data(0x00);
    
    send_cmd(0xE5);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x04);  
    send_data(0x00);  
    send_data(0x0F);  
    send_data(0x00);  
    send_data(0x2F);  
    send_data(0x00);  
    send_data(0x45);  
    send_data(0x00);  
    send_data(0x54);  
    send_data(0x00);  
    send_data(0x66);  
    send_data(0x00);  
    send_data(0x7D);  
    send_data(0x00);  
    send_data(0x8B);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0xA8); 
    send_data(0x00);  
    send_data(0xBA);  
    send_data(0x00);  
    send_data(0xC5);  
    send_data(0x00);  
    send_data(0xCB);  
    send_data(0x00);  
    send_data(0xD4);  
    send_data(0x00);  
    send_data(0xD9);  
    send_data(0x00);  
    send_data(0xE3);  
    send_data(0x00);  
    send_data(0xF3);  
    send_data(0x00);
    
    send_cmd(0xE6);  
    send_data(0x21);  
    send_data(0x00);  
    send_data(0x55);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0x77);  
    send_data(0x00);  
    send_data(0x77);  
    send_data(0x00);  
    send_data(0x76);  
    send_data(0x00);  
    send_data(0x78);  
    send_data(0x00);  
    send_data(0x98);  
    send_data(0x00);  
    send_data(0xBB);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0x66);  
    send_data(0x00);  
    send_data(0x54);  
    send_data(0x00);  
    send_data(0x45);  
    send_data(0x00);  
    send_data(0x34);  
    send_data(0x00);  
    send_data(0x44);  
    send_data(0x00);  
    send_data(0x34);  
    send_data(0x00);
    
    send_cmd(0xE7);  
    send_data(0x21);  
    send_data(0x00);  
    send_data(0x55);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0x77);  
    send_data(0x00);  
    send_data(0x77);  
    send_data(0x00);  
    send_data(0x76);  
    send_data(0x00);  
    send_data(0x78);  
    send_data(0x00);  
    send_data(0x98);  
    send_data(0x00);  
    send_data(0xBB);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0x66);  
    send_data(0x00);  
    send_data(0x54);  
    send_data(0x00);  
    send_data(0x45);  
    send_data(0x00);  
    send_data(0x34);  
    send_data(0x00);  
    send_data(0x44);  
    send_data(0x00);  
    send_data(0x34);  
    send_data(0x00);
    
    send_cmd(0xE8);  
    send_data(0x21);  
    send_data(0x00);  
    send_data(0x55);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0x77);  
    send_data(0x00);  
    send_data(0x77);  
    send_data(0x00);  
    send_data(0x76);  
    send_data(0x00);  
    send_data(0x78);  
    send_data(0x00);  
    send_data(0x98);  
    send_data(0x00);  
    send_data(0xBB);  
    send_data(0x00);  
    send_data(0x99);  
    send_data(0x00);  
    send_data(0x66);  
    send_data(0x00);  
    send_data(0x54);  
    send_data(0x00);  
    send_data(0x45);  
    send_data(0x00);  
    send_data(0x34);  
    send_data(0x00);  
    send_data(0x44);  
    send_data(0x00);  
    send_data(0x34);  
    send_data(0x00);
    
    send_cmd(0xE9);  
    send_data(0xAA);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);
    
    send_cmd(0xE9);  
    send_data(0xAA);  
    send_data(0x00);  
    send_data(0x00);  
    send_data(0x00);
    send_cmd(0x00);  
    send_data(0xAA);
    
    send_cmd(0xC6);  
    send_data(0x00);  
    send_data(0xE2);  
    send_data(0xE2);  
    send_data(0xE2 );
    
    send_cmd(0xF2);  
    send_data(0xF1);  
    send_data(0x54);  
    send_data(0xA5);  
    send_data(0x04);  
    send_data(0x1E);  
    send_data(0x44);  
    send_data(0x90);  
    send_data(0x02);  
    send_data(0x2C);  
    send_data(0x2C);  
    send_data(0x32);  
    send_data(0x4A );
    
    send_cmd(0xF3);  
    send_data(0x0C );
    
	//zyt add
	send_cmd(0x36);  
	send_data(0x00);  // 180¡ã
//123
    send_cmd(0x3A);//P242 set the interface pixel format
    send_data(0x55);
    
    send_cmd(0x35);  
    send_data(0x00);
    
    send_cmd(0x11);
    
    LCD_DelayMS(120);		
    
    send_cmd(0x29);
#endif
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif
	return 0;
}

static int32_t nt35310_set_window(struct panel_spec *self,
		uint16_t left, uint16_t top, uint16_t right, uint16_t bottom)
{
	send_data_t	send_cmd = self->info.mcu->ops->send_cmd;
	send_data_t	send_data = self->info.mcu->ops->send_data;

	LCD_PRINT("nt35310_set_window left=0x%x,top=0x%x,right=0x%x,bottom=0x%x\n",left, top, right, bottom);
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif    
	send_cmd(0x2A); // col
#if (7036 == CONFIG_ZYT_PRJ) && (63 == CONFIG_ZYT_CUST)
	send_data(((left+24) >> 8));
	send_data(((left+24) & 0xFF));
	send_data(((right+24) >> 8));
	send_data(((right+24) & 0xFF));
#else
	send_data((left >> 8));
	send_data((left & 0xFF));
	send_data((right >> 8));
	send_data((right & 0xFF));
#endif
	send_cmd(0x2B); // row
	send_data((top >> 8));
	send_data((top & 0xFF));
	send_data((bottom >> 8));
	send_data((bottom & 0xFF));
	
	send_cmd(0x2C); //Write data
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif
	return 0;
}


static int32_t nt35310_invalidate(struct panel_spec *self)
{
	LCD_PRINT("nt35310_invalidate\n");

	return self->ops->panel_set_window(self, 0, 0, 
			self->width-1, self->height-1);
	
}

static int32_t nt35310_invalidate_rect(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom)
{
	send_data_t	send_cmd = self->info.mcu->ops->send_cmd;
	read_data_t	read_data = self->info.mcu->ops->read_data;

	LCD_PRINT("nt35310_invalidate_rect : (%d, %d, %d, %d)\n",left, top, right, bottom);

	return self->ops->panel_set_window(self, left, top, 
			right, bottom);
}

static int32_t nt35310_set_direction(struct panel_spec *self, uint16_t direction)
{
	send_data_t	send_cmd = self->info.mcu->ops->send_cmd;
	read_data_t	read_data = self->info.mcu->ops->read_data;
	send_data_t	send_data = self->info.mcu->ops->send_data;
	LCD_PRINT("nt35310_set_direction direction =%d\n", direction);
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif	
	send_cmd(0x36);

	switch (direction) {
	case LCD_DIRECT_NORMAL:
		send_data(0xD4);
		break;
	case LCD_DIRECT_ROT_90:
		send_data(0xA0);
		break;
	case LCD_DIRECT_ROT_180:
		send_data(0x60);
		break;
	case LCD_DIRECT_ROT_270:
		send_data(0xB0);
		break;
	case LCD_DIRECT_MIR_H:
		send_data(0x40);
		break;
	case LCD_DIRECT_MIR_V:
		send_data(0x10);
		break;
	case LCD_DIRECT_MIR_HV:
		send_data(0xE0);
		break;
	default:
		LCD_PRINT("unknown lcd direction!\n");
		send_data(0x0);
		direction = LCD_DIRECT_NORMAL;
		break;
	}
	
	self->direction = direction;
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif
	return 0;
}

static int32_t nt35310_enter_sleep(struct panel_spec *self, uint8_t is_sleep)
{
	send_data_t	send_cmd = self->info.mcu->ops->send_cmd;
	read_data_t	read_data = self->info.mcu->ops->read_data;
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif
	if(is_sleep) 
	{	
		//Sleep In
		send_cmd(0x28);
		send_cmd(0x10);
		LCD_DelayMS(150);
	}
	else 
	{	
		//Sleep Out
		#ifndef LCD_ADJUST_PARAM
		send_cmd(0x29);
		LCD_DelayMS(60); 
		send_cmd(0x11);
		LCD_DelayMS(60); 
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

static uint32_t nt35310_read_id(struct panel_spec *self)
{
	send_data_t	send_cmd = self->info.mcu->ops->send_cmd;
	read_data_t	read_data = self->info.mcu->ops->read_data;

	uint32_t  value=0,value1=0,value2=0,value3=0;
#if 0
	send_cmd(0xDB);
	value = read_data();
	LCD_PRINT("nt35310_read_id(0xDB):id=0x%x.\n", value);	 // 0x80

	if((0x80==value) || (0x81==value) || (0x82==value))
		return 0x5310;
	else
		return 0;
#else
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(4, LCM_CTRL);
#endif
	send_cmd(0xD4);
	value = read_data();  //¡°01¡± means Novatek
	value1 = read_data();
	value2 = read_data();  //Chip ID code. ¡°5310¡± means NT35310
	value3 = read_data();  //Chip version code
	LCD_PRINT("nt35310_read_id(0xD4):value=0x%x.value1=0x%x.value2=0x%x.value3=0x%x.\n", value, value1, value2, value3);	 // 0x5310
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	lcdc_write(0, LCM_CTRL);
#endif	
	if((0x53 == value2) && (0x10 == value3))
	{
		return 0x5310;
	}
	else
	{
		return 0;
	}
#endif
}



static struct panel_operations lcd_nt35310_operations = {
	.panel_init            = nt35310_init,
	.panel_set_window      = nt35310_set_window,
	.panel_invalidate      = nt35310_invalidate,
	.panel_invalidate_rect = nt35310_invalidate_rect,
	.panel_set_direction   = nt35310_set_direction,
	.panel_enter_sleep     = nt35310_enter_sleep,
	.panel_readid          = nt35310_read_id,
};

static struct timing_mcu lcd_nt35310_timing[] = {
[LCD_REGISTER_TIMING] = {                // read/write register timing

		.rcss = 25,
		.rlpw = 70,
		.rhpw = 70,
		.wcss = 25,
		.wlpw = 75,
		.whpw = 75,

	},
[LCD_GRAM_TIMING] = {                // read/write gram timing

		.rcss = 25,
		.rlpw = 70,
		.rhpw = 70,
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
		.wcss = 10,
		.wlpw = 20,
		.whpw = 20,
#else
		.wcss = 15,
		.wlpw = 35,
		.whpw = 35,
#endif

	}
};

static struct info_mcu lcd_nt35310_info = {
	.bus_mode = LCD_BUS_8080,
#ifdef CONFIG_LCD_DATA_WIDTH_8BIT
	.bus_width = 8,
#else
	.bus_width = 16,
#endif
	.timing = lcd_nt35310_timing,
	.ops = NULL,
};

struct panel_spec lcd_panel_nt35310 = {

	.width = 320,

	.height = 480,//
	.mode = LCD_MODE_MCU,
	.direction = LCD_DIRECT_NORMAL,
	.info = {.mcu = &lcd_nt35310_info},
	.ops = &lcd_nt35310_operations,
};

struct panel_cfg lcd_nt35310 = {
    .lcd_cs = -1,
    .lcd_id = 0x5310,
    .lcd_name = "lcd_nt35310",
    .panel = &lcd_panel_nt35310,
};
static int __init lcd_nt35310_init(void)
{
    //lcd_regulator();
    return sprd_register_panel(&lcd_nt35310);
}
    
subsys_initcall(lcd_nt35310_init);