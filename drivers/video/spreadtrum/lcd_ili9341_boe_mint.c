/* drivers/video/sc8810/lcd_ili9486.c
 *
 * Support for ili9486 LCD device
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
//#include <mach/lcd.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>
#include "lcdpanel.h"

#ifdef LCD_DEBUG
#define LCD_PRINT printk
#else
#define LCD_PRINT(...)
#endif

static uint32_t ili9486_read_id(struct panel_spec *self);

static int32_t ili9486_init(struct panel_spec *self)
{
	send_data_t send_cmd = self->info.mcu->ops->send_cmd;
	send_data_t send_data = self->info.mcu->ops->send_data;

	LCD_PRINT("ili9486_init\n");

#if 0
	/*Sleep out*/
	send_cmd(0x11); // (SLPOUT)

	mdelay(120);

	/* Initializing Seq */
	send_cmd(0xCF); 
	send_data(0x00);
	send_data(0xFB);
	send_data(0xFF);

	send_cmd(0xED); 
	send_data(0x64);
	send_data(0x03);	
	send_data(0x12);
	send_data(0x81);	

	send_cmd(0xE8); 
	send_data(0x85);
	send_data(0x00);	
	send_data(0x70);
	
	send_cmd(0xCB); 
	send_data(0x39);
	send_data(0x2D);	
	send_data(0x00);
	send_data(0x34);
	send_data(0x02);

	send_cmd(0xF7); 
	send_data(0x20);

	send_cmd(0xB1); 
	send_data(0x15);

	send_cmd(0xB6); 
	send_data(0x0A);
	send_data(0xA2);
	send_data(0x27);

	
	send_cmd(0x2A); 
	send_data(0x00);
	send_data(0x00);
	send_data(0x00);
	send_data(0xEF);

	send_cmd(0x2B); 
	send_data(0x00);
	send_data(0x00);
	send_data(0x01);
	send_data(0x3F);

	send_cmd(0x35); 
	send_data(0x00);

	send_cmd(0x36); 
	send_data(0x08);
	
	send_cmd(0x3A); 
	send_data(0x55);

	send_cmd(0xB4); 
	send_data(0x00);

	send_cmd(0xF2); 
	send_data(0x00);

	send_cmd(0x26); 
	send_data(0x01);

	send_cmd(0xF6); 
	send_data(0x01);
	send_data(0x30);
	send_data(0x00);
	

	/*Power Setting & Ram address Seq*/
	send_cmd(0xC0); 
	send_data(0x16);

	send_cmd(0xC1); 
	send_data(0x10);

	send_cmd(0xC5); 
	send_data(0x34);
	send_data(0x4D);	


	/*Gamma setting Seq*/
	send_cmd(0xE0); 
	send_data(0x00);
	send_data(0x1E);
	send_data(0x16);
	send_data(0x0C);
	send_data(0x0E);
	send_data(0x08);
	send_data(0x43);
	send_data(0x61);
	send_data(0x34);
	send_data(0x05);
	send_data(0x0C);
	send_data(0x02);
	send_data(0x13);
	send_data(0x12);
	send_data(0x0F);

	send_cmd(0xE1); 
	send_data(0x0F);	
	send_data(0x15);
	send_data(0x14);
	send_data(0x04);
	send_data(0x0F);
	send_data(0x05);
	send_data(0x28);
	send_data(0x22);
	send_data(0x3C);
	send_data(0x04);
	send_data(0x0A);
	send_data(0x0A);
	send_data(0x29);
	send_data(0x2B);
	send_data(0x00);

    //Display on

	send_cmd(0x29); // (DISPON)

	mdelay(100); // 100ms
#else
	// Hidden register setting
	send_cmd(0xCB); 
	send_data(0x39);
	send_data(0x2C);	
	send_data(0x00);
	send_data(0x34);
	send_data(0x02);
	
	send_cmd(0xCF); 
	send_data(0x00);
	send_data(0xc1);	
	send_data(0x30);

	send_cmd(0xE8); 
	send_data(0x85);
	send_data(0x00);
	send_data(0x78);	
	
	send_cmd(0xED); 
	send_data(0x64);
	send_data(0x03);	
	send_data(0x12);
	send_data(0x81);	
	
	send_cmd(0xEA); 
	send_data(0x00);
	send_data(0x00);
	
	send_cmd(0xF2); 
	send_data(0x00);
	
	send_cmd(0xF7); 
	send_data(0x20);

	send_cmd(0xC0); 
	send_data(0x1B);

	send_cmd(0xC1); 
	send_data(0x10);

	send_cmd(0xC5); 
	send_data(0x14);
	send_data(0x44);

	send_cmd(0xC7); 
	send_data(0xC2);
	
	send_cmd(0x26); 
	send_data(0x01);

	send_cmd(0x36); 
	send_data(0x08);	
	
	send_cmd(0x3A); 
	send_data(0x55);	
	
	send_cmd(0x35); 
	send_data(0x00);

	send_cmd(0xB1); 
	send_data(0x00);
	send_data(0x18);

	send_cmd(0xB6); 
	send_data(0x0A);
	send_data(0xC2);

	send_cmd(0xE0); 
	send_data(0x0F);
	send_data(0x1c);
	send_data(0x19);
	send_data(0x0b);
	send_data(0x0e);
	send_data(0x09);
	send_data(0x46);
	send_data(0x52);
	send_data(0x36);
	send_data(0x0a);
	send_data(0x14);
	send_data(0x06);
	send_data(0x0c);
	send_data(0x07);
	send_data(0x00);

	send_cmd(0xE1); 
	send_data(0x00);
	send_data(0x23);
	send_data(0x27);
	send_data(0x04);
	send_data(0x10);
	send_data(0x07);
	send_data(0x3a);
	send_data(0x00);
	send_data(0x4a);
	send_data(0x05);
	send_data(0x0b);
	send_data(0x09);
	send_data(0x33);
	send_data(0x37);
	send_data(0x0f);

    //Display on

	send_cmd(0x11); // (SLPOUT)

	mdelay(120); // 100ms

	send_cmd(0x29); // (DISPON)

	mdelay(100); // 100ms
	
#endif
	
	LCD_PRINT("ili9341_init: end\n");

	return 0;
}

static int32_t ili9486_set_window(struct panel_spec *self,
		uint16_t left, uint16_t top, uint16_t right, uint16_t bottom)
{
	send_data_t send_cmd = self->info.mcu->ops->send_cmd;
	send_data_t send_data = self->info.mcu->ops->send_data;

	LCD_PRINT("ili9486_set_window\n");

	send_cmd(0x2A); // col
	send_data((left >> 8));
	send_data((left & 0xFF));
	send_data((right >> 8));
	send_data((right & 0xFF));

	send_cmd(0x2B); // row
	send_data((top >> 8));
	send_data((top & 0xFF));
	send_data((bottom >> 8));
	send_data((bottom & 0xFF));

	send_cmd(0x2C);

	return 0;
}

static int32_t ili9486_invalidate(struct panel_spec *self)
{
	LCD_PRINT("ili9486_invalidate\n");

	return self->ops->panel_set_window(self, 0, 0,
			self->width-1, self->height-1);
	
}

static int32_t ili9486_invalidate_rect(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom)
{

	LCD_PRINT("ili9486_invalidate_rect : (%d, %d, %d, %d)\n",left, top, right, bottom);


	return self->ops->panel_set_window(self, left, top,
			right, bottom);
}

static int32_t ili9486_set_direction(struct panel_spec *self, uint16_t direction)
{

	LCD_PRINT("ili9486_set_direction\n");
	return 0;
}

static int32_t ili9486_enter_sleep(struct panel_spec *self, uint8_t is_sleep)
{
	send_data_t send_cmd = self->info.mcu->ops->send_cmd;
	send_data_t send_data = self->info.mcu->ops->send_data;

	if(is_sleep) {
		//send_cmd(0x10);
		mdelay(120); 
	}
	else {
		//send_cmd(0x11);
		mdelay(120); 
	}
	return 0;
}

static uint32_t ili9486_read_id(struct panel_spec *self)
{
	uint32_t read_value = 0;
	send_data_t send_cmd = self->info.mcu->ops->send_cmd;
	read_data_t read_data = self->info.mcu->ops->read_data;
	
	send_cmd(0x04);

	read_data(); 
	read_value += read_data()<< 16;
	read_value += read_data()<< 8;
	read_value += read_data();
  
	return read_value; 
}

static struct panel_operations lcd_ili9486_operations = {
	.panel_init            = ili9486_init,
	.panel_set_window      = ili9486_set_window,
	.panel_invalidate      = ili9486_invalidate,
	.panel_invalidate_rect = ili9486_invalidate_rect,
	.panel_set_direction   = ili9486_set_direction,
	.panel_enter_sleep     = ili9486_enter_sleep,
	.panel_readid          = ili9486_read_id,
};

static struct timing_mcu lcd_ili9486_timing[] = {
	[LCD_REGISTER_TIMING] = {         
		.rcss = 0,
		.rlpw = 60,
		.rhpw = 101,
		.wcss = 0,
		.wlpw = 40,
		.whpw = 36,
	},
	[LCD_GRAM_TIMING] = {         
		.rcss = 0,
		.rlpw = 60,
		.rhpw = 101,
		.wcss = 0,
		.wlpw = 40,
		.whpw = 36,
	}
};

static struct info_mcu lcd_ili9486_info = {
	.bus_mode = LCD_BUS_8080,
	.bus_width = 16,
	.timing    = lcd_ili9486_timing,
	.ops = NULL,
};

struct panel_spec lcd_ili9486_spec = {
	.width = 240,
	.height = 320,
	.mode = LCD_MODE_MCU,
	.direction = LCD_DIRECT_NORMAL,
	.info = {.mcu = &lcd_ili9486_info},
	.ops = &lcd_ili9486_operations,
};

struct panel_cfg lcd_ili9486 = {
	.lcd_cs = -1,
#if defined(CONFIG_MACH_MINT)
	.lcd_id = 0x60b4,
#else
	.lcd_id = 0x61a4,
#endif	
	.lcd_name = "lcd_ili9486",
	.panel = &lcd_ili9486_spec,
};

static int lcd_regulator(void)
{
	int err;
	struct regulator *lcd_regulator = NULL;
	struct regulator *lcdio_regulator = NULL;

	lcd_regulator = regulator_get(NULL, REGU_NAME_LCD);
	if (IS_ERR(lcd_regulator)) {
		pr_err("ILI9486:could not get lcd regulator\n");
		return -1;
	}

	err = regulator_enable(lcd_regulator);
	if (err) {
		pr_err("ILI9486:could not enable lcd regulator\n");
		return -1;
	}
	err = regulator_set_voltage(lcd_regulator,3000000,3000000);
	if (err)
		pr_err("ILI9486:could not set lcd to 3000mv.\n");


	lcdio_regulator = regulator_get(NULL, REGU_NAME_LCDIO);
	if (IS_ERR(lcdio_regulator)) {
		pr_err("ILI9486:could not get lcdio regulator\n");
		return -1;
	}

	err = regulator_enable(lcdio_regulator);
	if (err) {
		pr_err("ILI9486:could not enable lcdio regulator\n");
		return -1;
	}

	err =regulator_set_voltage(lcdio_regulator,1800000,1800000);
	if (err)
		pr_err("ILI9486:could not set lcdio to 1800mv.\n");
	return 0;
}

static int __init lcd_ili9486_init(void)
{
	lcd_regulator();
	return sprd_register_panel(&lcd_ili9486);
}

subsys_initcall(lcd_ili9486_init);
