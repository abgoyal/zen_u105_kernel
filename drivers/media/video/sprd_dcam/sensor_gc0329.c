/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
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
#include <linux/delay.h>
#include "common/sensor.h"
#include "common/jpeg_exif_header_k.h"
#include "common/sensor_drv.h"


/**---------------------------------------------------------------------------*
 ** 						   Macro Define
 **---------------------------------------------------------------------------*/
#undef MYSENSORDEBUG
#ifdef MYSENSORDEBUG
#define SENSOR_PRINTK  SENSOR_PRINT
#else
#define SENSOR_PRINTK(...)
#endif



/**---------------------------------------------------------------------------*
 ** 						Const variables 								  *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 ** 						   Macro Define
 **---------------------------------------------------------------------------*/
#define GC0329_I2C_ADDR_W	0x31 // 0x42 --> 0x42 / 2
#define GC0329_I2C_ADDR_R	0x31 // 0x43 --> 0x43 / 2

#define SENSOR_GAIN_SCALE		16

static uint32_t  g_flash_mode_en = 0;
uint32_t Sensor_SetFlash(uint32_t flash_mode);
 
/**---------------------------------------------------------------------------*
 ** 					Local Function Prototypes							  *
 **---------------------------------------------------------------------------*/
LOCAL uint32_t GC0329_Power_On(uint32_t power_on);
LOCAL uint32_t set_GC0329_ae_enable(uint32_t enable);
LOCAL uint32_t set_hmirror_enable(uint32_t enable);
LOCAL uint32_t set_vmirror_enable(uint32_t enable);
LOCAL uint32_t set_preview_mode(uint32_t preview_mode);
LOCAL uint32_t GC0329_Identify(uint32_t param);
/*
LOCAL uint32_t GC0329_BeforeSnapshot(uint32_t param);
LOCAL uint32_t GC0329_After_Snapshot(uint32_t param);
*/
LOCAL uint32_t set_brightness(uint32_t level);
LOCAL uint32_t set_contrast(uint32_t level);
LOCAL uint32_t set_sharpness(uint32_t level);
LOCAL uint32_t set_saturation(uint32_t level);
LOCAL uint32_t set_image_effect(uint32_t effect_type);

LOCAL uint32_t read_ev_value(uint32_t value);
LOCAL uint32_t write_ev_value(uint32_t exposure_value);
LOCAL uint32_t read_gain_value(uint32_t value);
LOCAL uint32_t write_gain_value(uint32_t gain_value);
LOCAL uint32_t read_gain_scale(uint32_t value);
LOCAL uint32_t set_frame_rate(uint32_t param);
LOCAL uint32_t GC0329_set_work_mode(uint32_t mode);

LOCAL uint32_t set_GC0329_ev(uint32_t level);
LOCAL uint32_t set_GC0329_awb(uint32_t mode);
LOCAL uint32_t set_GC0329_anti_flicker(uint32_t mode);
LOCAL uint32_t set_GC0329_video_mode(uint32_t mode);

LOCAL uint32_t GC0329_BeforeSnapshot(uint32_t param);

LOCAL uint32_t GC0329_After_Snapshot(uint32_t param);
LOCAL uint32_t Sensor_flash(uint32_t param);


//LOCAL BOOLEAN gc_enter_effect = SENSOR_FALSE;

/**---------------------------------------------------------------------------*
 ** 						Local Variables 								 *
 **---------------------------------------------------------------------------*/
 typedef enum
{
	FLICKER_50HZ = 0,
	FLICKER_60HZ,
	FLICKER_MAX
}FLICKER_E;

LOCAL SENSOR_REG_T GC0329_YUV_640X480[]=
{
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfc, 0x16},
	{0xfc, 0x16},
	{0xfc, 0x16},
	{0xfc, 0x16},
	{0xfe, 0x00},

	{0x73, 0x90},
	{0x74, 0x80}, 
	{0x75, 0x80}, 
	{0x76, 0x94}, 
	{0x42, 0x00}, 
	{0x77, 0x57}, 
	{0x78, 0x4d}, 
	{0x79, 0x45}, 
	//{0x42, 0xfc},
	////////////////////analog////////////////////
  {0x09,0x00},                            
  {0x0a,0x02},                            
  {0x0b,0x00},                            
  {0x0c,0x02},                            
	{0x17, 0x17},// 0x15 rotation 180  0x14

	{0x19, 0x05}, 
	{0x1b, 0x24}, 
	{0x1c, 0x04}, 
	{0x1e, 0x08}, 
  {0x1f,0x08}, //C8                       
  {0x20,0x01},                            
	{0x21, 0x48}, 
	{0x22, 0xba},
	{0x23, 0x22},
	{0x24, 0x16},	   
	////////////////////blk////////////////////
  {0x26,0xf7}, //BLK                      
  {0x28,0x7f}, //BLK limit                
  {0x29,0x00},                            
  {0x32,0x00}, //04 darkc                 
  {0x33,0x1c}, //blk ratio                
  {0x34,0x1c},                            
  {0x35,0x1c},                            
  {0x36,0x1c},                            
                                                                    
  {0x3b,0x04}, //manual offset            
  {0x3c,0x04},                            
  {0x3d,0x04},                            
  {0x3e,0x04},                            
                                                                    
	////////////////////ISP BLOCK ENABL////////////////////
  {0x40,0xff},                            
  {0x41,0x24},//[5]skin detection         
  {0x42,0xfa},//disable ABS               
  {0x46,0x02},                            
  {0x4b,0xca},                            
  {0x4d,0x01},                            
  {0x4f,0x01},                            
  {0x70,0x48},                            
                                                                    
	//{0xbc, 0x00},
	//{0xbd, 0x00},
	//{0xbe, 0x00},
	////////////////////DNDD////////////////////
  {0x80,0xe7},                            
  {0x82,0x5f}, //0717                     
  {0x83,0x03},//05 0717                   
  {0x87,0x4a},                            
	////////////////////INTPEE////////////////////
  {0x90,0x80}, //8c 0717                  
  {0x92,0x05},                            
  {0x94,0x05},                            
  {0x95,0x43},//45 0717                   
  {0x96,0x88},                            
                                                                    
	////////////////////ASDE////////////////////
  {0xfe,0x01},                            
  {0x18,0x22},                            
  {0xfe,0x00},                            
  {0x9c,0x0a},                            
  {0xa0,0xaf},                            
  {0xa2,0x33}, //ff 0717                  
  {0xa4,0x30}, //50 jambo                 
  {0xa5,0x31},                            
  {0xa7,0x20}, //35 0717                  
                                                                    
	////////////////////RGB gamma////////////////////
  {0xfe,0x00},                            
  {0xbf,0x0b},                            
  {0xc0,0x1d},                            
  {0xc1,0x33},                            
  {0xc2,0x49},                            
  {0xc3,0x5d},                            
  {0xc4,0x6e},                            
  {0xc5,0x7c},                            
  {0xc6,0x99},                            
  {0xc7,0xaf},                            
  {0xc8,0xc2},                            
  {0xc9,0xd0},                            
  {0xca,0xda},                            
  {0xcb,0xe2},                            
  {0xcc,0xe7},                            
  {0xcd,0xf0},                            
  {0xce,0xf7},                            
  {0xcf,0xff},                            
	//////////////////CC///////////////////
	{0xfe, 0x00},
	{0xb3, 0x44},
	{0xb4, 0xfd},
	{0xb5, 0x02},
	{0xb6, 0xfa},
	{0xb7, 0x48},
	{0xb8, 0xf0},
	// crop 						   
	{0x50, 0x01},
	////////////////////YCP////////////////////
	{0xfe, 0x00},
	{0xd1, 0x38},
	{0xd2, 0x38},
	{0xdd, 0x54},
	////////////////////AEC////////////////////
  {0xfe,0x01},                            
  {0x10,0x40}, // before Gamma            
  {0x11,0x21}, //                         
  {0x12,0x43},	// center weight *2       
  {0x13,0x54}, //58 0717                  
	                                                                  
  {0x17,0xa8},	//88, 08, c8, a8          
  {0x1a,0x21},                            
  {0x20,0x31},	//AEC stop margin         
  {0x21,0xd0},                            
  {0x22,0x60},                            
  {0x3c,0xe8},//th                        
  {0x3d,0x84}, //slope                    
  {0x3e,0x30}, //read 3f for status       
	
  {0xfe, 0x00},//////////banding
  {0x05, 0x02},
  {0x06, 0x2c},
  {0x07, 0x00},
  {0x08, 0xb8},
  {0xfe, 0x01},
  {0x29, 0x00},
  {0x2a, 0x60},
  {0x2b, 0x03},
  {0x2c, 0x00},
  {0x2d, 0x03},
  {0x2e, 0xc0},
  {0x2f, 0x04},
  {0x30, 0x80},
  {0x31, 0x05},
  {0x32, 0xa0},
  {0xfe, 0x00},
	////////////////////AWB////////////////////
  {0xfe,0x01},                            
  {0x06,0x12},                            
  {0x07,0x06},                            
  {0x08,0x9c},                            
  {0x09,0xee},                            
  {0x50,0xfc},                            
  {0x51,0x25},//28 0717                   
  {0x52,0x10},                            
  {0x53,0x25},//20 0717                   
  {0x54,0x15},//12 0717                   
  {0x55,0x16},                            
  {0x56,0x30},                            
  {0x58,0x50},//60 0717                   
  {0x59,0x08},                            
  {0x5a,0x02},                            
  {0x5b,0x63},                            
  {0x5c,0x37},//35 0717                   
  {0x5d,0x72},                            
  {0x5e,0x11},                            
  {0x5f,0x40},                            
  {0x60,0x40},                            
  {0x61,0xc8},                            
  {0x62,0xa0},                            
  {0x63,0x40},                            
  {0x64,0x50},                            
  {0x65,0x98},                            
  {0x66,0xfa},                            
  {0x67,0x78},//80 0717                   
  {0x68,0x58},//60 0717                   
  {0x69,0x90},                            
  {0x6a,0x40},                            
  {0x6b,0x39},                            
  {0x6c,0x30},//30 0717                   
  {0x6d,0x20},//60 0717                   
  {0x6e,0x41},                            
  {0x70,0x10},                            
  {0x71,0x00},                            
	{0x72, 0x10},
	{0x73, 0x40}, 
  {0x80,0x60},                            
  {0x81,0x50},                            
  {0x82,0x42},                            
  {0x83,0x40},                            
  {0x84,0x40},                            
  {0x85,0x40},                            
	{0x74, 0x40},
	{0x75, 0x58},
	{0x76, 0x24},
	{0x77, 0x40},
	{0x78, 0x20},
	{0x79, 0x60},
	{0x7a, 0x58},
	{0x7b, 0x20},
	{0x7c, 0x30},
	{0x7d, 0x35},
	{0x7e, 0x10},
	{0x7f, 0x08},
	{0x81, 0x50}, 
	{0x82, 0x42}, 
	{0x83, 0x40}, 
	{0x84, 0x40}, 
	{0x85, 0x40}, 
	
	////////////////////ABS////////////////////
  {0x9c,0x00},                            
  {0x9e,0xc0},                            
  {0x9f,0x40},                            
                                                                    
	////////////////////CC-AWB////////////////////
	{0xd0, 0x00},
	{0xd2, 0x2c},
	{0xd3, 0x80}, 
	////////////////////LSC///////////////////
	{0xfe, 0x01},
	{0xa0, 0x00},
	{0xa1, 0x3c},
	{0xa2, 0x50},
	{0xa3, 0x00},
	{0xa8, 0x0f},
	{0xa9, 0x08},
	{0xaa, 0x00},
	{0xab, 0x04},
	{0xac, 0x00},
	{0xad, 0x07},
	{0xae, 0x0e},
	{0xaf, 0x00},
	{0xb0, 0x00},
	{0xb1, 0x09},
	{0xb2, 0x00},
	{0xb3, 0x00},
	{0xb4, 0x31},
	{0xb5, 0x19},
	{0xb6, 0x24},
	{0xba, 0x3a},
	{0xbb, 0x24},
	{0xbc, 0x2a},
	{0xc0, 0x17},
	{0xc1, 0x13},
	{0xc2, 0x17},
	{0xc6, 0x21},
	{0xc7, 0x1c},
	{0xc8, 0x1c},
	{0xb7, 0x00},
	{0xb8, 0x00},
	{0xb9, 0x00},
	{0xbd, 0x00},
	{0xbe, 0x00},
	{0xbf, 0x00},
	{0xc3, 0x00},
	{0xc4, 0x00},
	{0xc5, 0x00},
	{0xc9, 0x00},
	{0xca, 0x00},
	{0xcb, 0x00},
	{0xa4, 0x00},
	{0xa5, 0x00},
	{0xa6, 0x00},
	{0xa7, 0x00},
	/////20120427/////////
	{0xfe, 0x01},
	{0x18, 0x22},
	{0x21, 0xc0},
	{0x06, 0x12},
	{0x08, 0x9c},
	{0x51, 0x28},
	{0x52, 0x10},
	{0x53, 0x20},
	{0x54, 0x40},
	{0x55, 0x16},
	{0x56, 0x30},
	{0x58, 0x60},
	{0x59, 0x08},
	{0x5c, 0x35},
	{0x5d, 0x72},
	{0x67, 0x80},
	{0x68, 0x60},
	{0x69, 0x90},
	{0x6c, 0x30},
	{0x6d, 0x60},
	{0x70, 0x10},
	{0xfe, 0x00},
	{0x9c, 0x0a},
	{0xa0, 0xaf},
	{0xa2, 0xff},
	{0xa4, 0x60},
	{0xa5, 0x31},
	{0xa7, 0x35},
	{0x42, 0xfe},
	{0xd1, 0x34},
	{0xd2, 0x34},
	{0xfe, 0x00},
	////////////////////asde ///////////////////
	//{0xa0, 0xaf},
	//{0xa2, 0xff},
	{0x44, 0xa2},

	{0xf0, 0x07},
	{0xf1, 0x01},
};


LOCAL SENSOR_REG_TAB_INFO_T s_GC0329_resolution_Tab_YUV[]=
{
	// COMMON INIT
	{ADDR_AND_LEN_OF_ARRAY(GC0329_YUV_640X480), 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},
	
	// YUV422 PREVIEW 1	
	{PNULL, 0, 640, 480,24, SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	
	// YUV422 PREVIEW 2 
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}

};

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_GC0329_ioctl_func_tab = 
{
    // Internal 
    PNULL,
    GC0329_Power_On, // PNULL
    PNULL,
    GC0329_Identify,

    PNULL,			// write register
    PNULL,			// read  register	
    PNULL,
    PNULL,

    // External
    set_GC0329_ae_enable,
    PNULL, //set_hmirror_enable,
    PNULL, //set_vmirror_enable,

    set_brightness,
    set_contrast,
    PNULL, //set_sharpness,
    PNULL, //set_saturation,

    set_preview_mode,	
    set_image_effect,

    GC0329_BeforeSnapshot,
    GC0329_After_Snapshot,

    Sensor_flash,

    PNULL, //read_ev_value,
    PNULL, //write_ev_value,
    PNULL,//read_gain_value,
    PNULL, //write_gain_value,
    PNULL, //read_gain_scale,
    PNULL, //set_frame_rate,	
    PNULL,
    PNULL,
    set_GC0329_awb,
    PNULL,
    PNULL,
    set_GC0329_ev,
    PNULL,
    PNULL,
    PNULL,
    PNULL,
    PNULL,
    set_GC0329_anti_flicker,
    set_GC0329_video_mode,
    PNULL,
    PNULL,
    PNULL,
    PNULL,
    PNULL
};

/**---------------------------------------------------------------------------*
 ** 						Global Variables								  *
 **---------------------------------------------------------------------------*/
 SENSOR_INFO_T g_GC0329_yuv_info =
{
	GC0329_I2C_ADDR_W,				// salve i2c write address
	GC0329_I2C_ADDR_R, 				// salve i2c read address
	
	0,								// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
									// bit2: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
									// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_P|\
	SENSOR_HW_SIGNAL_VSYNC_N|\
	SENSOR_HW_SIGNAL_HSYNC_P,		// bit0: 0:negative; 1:positive -> polarily of pixel clock
									// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
									// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
									// other bit: reseved											
											
	// preview mode
	SENSOR_ENVIROMENT_NORMAL|\
	SENSOR_ENVIROMENT_NIGHT|\
	SENSOR_ENVIROMENT_SUNNY,		
	
	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL|\
	SENSOR_IMAGE_EFFECT_BLACKWHITE|\
	SENSOR_IMAGE_EFFECT_RED|\
	SENSOR_IMAGE_EFFECT_GREEN|\
	SENSOR_IMAGE_EFFECT_BLUE|\
	SENSOR_IMAGE_EFFECT_YELLOW|\
	SENSOR_IMAGE_EFFECT_NEGATIVE|\
	SENSOR_IMAGE_EFFECT_CANVAS,
	
	// while balance mode
	0,
		
	7,								// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
									// bit[8:31] reseved
	
	SENSOR_LOW_PULSE_RESET,			// reset pulse level
	100,								// reset pulse width(ms)
	
	SENSOR_HIGH_LEVEL_PWDN,			// 1: high level valid; 0: low level valid	
		
	2,								// count of identify code
	{{0x00, 0xc0},						// supply two code to identify sensor.
	{0x00, 0xc0}},						// for Example: index = 0-> Device id, index = 1 -> version id
									
	SENSOR_AVDD_2800MV,				// voltage of avdd

	640,							// max width of source image
	480,							// max height of source image
	"GC0329",						// name of sensor												

	SENSOR_IMAGE_FORMAT_YUV422,		// define in SENSOR_IMAGE_FORMAT_E enum,
									// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T
	SENSOR_IMAGE_PATTERN_YUV422_YUYV,	// pattern of input image form sensor;			

	s_GC0329_resolution_Tab_YUV,	// point to resolution table information structure
	&s_GC0329_ioctl_func_tab,		// point to ioctl function table
			
	PNULL,							// information and table about Rawrgb sensor
	PNULL,							// extend information about sensor	
	SENSOR_AVDD_1800MV,                     // iovdd
	SENSOR_AVDD_1800MV,                      // dvdd
	2,
	1,
	0,
	0,                      // deci frame num during video preview
	0,                     // threshold enable(only analog TV)	
	0,                     // atv output mode 0 fix mode 1 auto mode	
	0,                    // atv output start postion	
	0,                     // atv output end postion
    	0,
	0 //{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1}
};

/**---------------------------------------------------------------------------*
 ** 							Function  Definitions
 **---------------------------------------------------------------------------*/
LOCAL void GC0329_WriteReg( uint8_t  subaddr, uint8_t data )
{
    //SENSOR_PRINTK(":enter GC0329_WriteReg\n");
    Sensor_WriteReg_8bits(subaddr, data);
    
    //SENSOR_PRINTK("SENSOR: GC0329_WriteReg reg/value(%x,%x) !!\n", subaddr, data);

}

LOCAL uint8_t GC0329_ReadReg( uint8_t  subaddr)
{
    uint8_t value = 0;
	
    value = Sensor_ReadReg( subaddr);

    //SENSOR_PRINTK("SENSOR: GC0329_ReadReg reg/value(%x,%x) !!\n", subaddr, value);

    return value;
}

LOCAL uint32_t GC0329_Power_On(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val=g_GC0329_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val=g_GC0329_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val=g_GC0329_yuv_info.iovdd_val;  
	BOOLEAN power_down=g_GC0329_yuv_info.power_down_level;        
	BOOLEAN reset_level=g_GC0329_yuv_info.reset_pulse_level;
	uint32_t reset_width=g_GC0329_yuv_info.reset_pulse_width;        

	SENSOR_PRINTK("-------------------%s\n", __func__);
	if(SENSOR_TRUE==power_on)
	{
		Sensor_PowerDown(power_down);	
		// Open power
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		//Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		SENSOR_Sleep(20);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK); 
		SENSOR_Sleep(20);
		Sensor_PowerDown(!power_down);
		// Reset sensor		
		Sensor_Reset(reset_level);		
	}
	else
	{
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);    
		SENSOR_Sleep(20);
		Sensor_PowerDown(power_down);
		SENSOR_Sleep(20);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);         
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);  
		//Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}

	SENSOR_PRINTK("%s:-----------------power_on= %d\n", __func__, power_on);    

	return SENSOR_SUCCESS;
}

#define id_reg 0x00
#define id_value 0xc0

LOCAL uint32_t GC0329_Identify(uint32_t param)
{
    uint32_t i;
    SENSOR_PRINT("%s\n", __FUNCTION__);
    GC0329_WriteReg(0xfc, 0x16);

    for(i = 0; i<2;i++ )
    {
        if(GC0329_ReadReg(id_reg)==id_value)
        {
		SENSOR_PRINT("GC0329 Success\n");
		return (uint32_t)SENSOR_SUCCESS;
        }
		SENSOR_Sleep(5);
    }

    SENSOR_PRINT("%s:failed\n", __FUNCTION__);
    return SENSOR_FAIL;
}
LOCAL uint32_t GC0329_BeforeSnapshot(uint32_t param)
{
    if(g_flash_mode_en)
    {
        Sensor_SetFlash(1);
    }

    return 0;
}
LOCAL uint32_t GC0329_After_Snapshot(uint32_t param)
{
    return 0;
}

LOCAL uint32_t Sensor_flash(uint32_t param)
{
    g_flash_mode_en = param;
    Sensor_SetFlash(param);
    return 0;
}

LOCAL uint32_t set_GC0329_ae_enable(uint32_t enable)
{
	SENSOR_PRINTK("set_GC0329_ae_enable: enable = %d\n", enable);
	return 0;
}


LOCAL uint32_t set_hmirror_enable(uint32_t enable)
{
#if 0
 	uint8_t value = 0;	
	value = GC0329_ReadReg(0x17);
	value = (value & 0xFE) | (enable == 1 ? 0 : 1); //landscape
	SENSOR_PRINTK("set_hmirror_enable: enable = %d, 0x14: 0x%x.\n", enable, value);
	GC0329_WriteReg(0x17, value);
#endif
	return 0;
}


LOCAL uint32_t set_vmirror_enable(uint32_t enable)
{
#if 0
	uint8_t value = 0;	
	value = GC0329_ReadReg(0x17);
	value = (value & 0xFD) | ((enable & 0x1) << 1); //portrait
	SENSOR_PRINTK("set_vmirror_enable: enable = %d, 0x14: 0x%x.\n", enable, value);
	GC0329_WriteReg(0x17, value);
#endif
	return 0;
}

/******************************************************************************/
// Description: set brightness 
// Global resource dependence: 
// Author:
// Note:
//		level  must smaller than 8
/******************************************************************************/
//__align(4) const SENSOR_REG_T GC0329_brightness_tab[][2]=
LOCAL SENSOR_REG_T GC0329_brightness_tab[][5]=
{	
	{{0xfe, 0x01}, {0x13, 0x38}, {0xfe, 0x00}, {0xd5, 0xd0}, {0xff, 0xff}},
	{{0xfe, 0x01}, {0x13, 0x40}, {0xfe, 0x00}, {0xd5, 0xe0}, {0xff, 0xff}},
	{{0xfe, 0x01}, {0x13, 0x48}, {0xfe, 0x00}, {0xd5, 0xf0}, {0xff, 0xff}},
	{{0xfe, 0x01}, {0x13, 0x54}, {0xfe, 0x00}, {0xd5, 0x00}, {0xff, 0xff}},
	{{0xfe, 0x01}, {0x13, 0x58}, {0xfe, 0x00}, {0xd5, 0x20}, {0xff, 0xff}},
	{{0xfe, 0x01}, {0x13, 0x60}, {0xfe, 0x00}, {0xd5, 0x30}, {0xff, 0xff}},
	{{0xfe, 0x01}, {0x13, 0x68}, {0xfe, 0x00}, {0xd5, 0x40}, {0xff, 0xff}},
};


LOCAL uint32_t set_brightness(uint32_t level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC0329_brightness_tab[level];

	//SENSOR_ASSERT(level < 7);
	//SENSOR_ASSERT(PNULL != sensor_reg_ptr);
	
	for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++)
	{
		GC0329_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}
	SENSOR_Sleep(10); 
	SENSOR_PRINTK("set_brightness: level = %d\n", level);
	
	return 0;
}

//__align(4) const SENSOR_REG_T GC0329_ev_tab[][3]=
LOCAL SENSOR_REG_T GC0329_ev_tab[][5]=
{   
	    {{0xfe, 0x01}, {0x13, 0x38}, {0xfe, 0x00}, {0xd5, 0xd0}, {0xff, 0xff}},
	    {{0xfe, 0x01}, {0x13, 0x40}, {0xfe, 0x00}, {0xd5, 0xe0}, {0xff, 0xff}},
	    {{0xfe, 0x01}, {0x13, 0x48}, {0xfe, 0x00}, {0xd5, 0xf0}, {0xff, 0xff}},
	    {{0xfe, 0x01}, {0x13, 0x54}, {0xfe, 0x00}, {0xd5, 0x00}, {0xff, 0xff}},
	    {{0xfe, 0x01}, {0x13, 0x58}, {0xfe, 0x00}, {0xd5, 0x20}, {0xff, 0xff}},
	    {{0xfe, 0x01}, {0x13, 0x60}, {0xfe, 0x00}, {0xd5, 0x30}, {0xff, 0xff}},
	    {{0xfe, 0x01}, {0x13, 0x68}, {0xfe, 0x00}, {0xd5, 0x40}, {0xff, 0xff}},
    
};


LOCAL uint32_t set_GC0329_ev(uint32_t level)
{
    uint16_t i; 
    SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC0329_ev_tab[level];

    //SENSOR_ASSERT(PNULL != sensor_reg_ptr);
    //SENSOR_ASSERT(level < 7);
 
    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) ||(0xFF != sensor_reg_ptr[i].reg_value) ; i++)
    {
        GC0329_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    }

    
    SENSOR_PRINTK("SENSOR: set_ev: level = %d\n", level);

    return 0;
}

/******************************************************************************/
// Description: anti 50/60 hz banding flicker
// Global resource dependence: 
// Author:
// Note:
//		level  must smaller than 8
/******************************************************************************/
LOCAL uint32_t set_GC0329_anti_flicker(uint32_t param )
{
    switch (param)
    {
        case FLICKER_50HZ:
    GC0329_WriteReg(0xfe,0x00);
		GC0329_WriteReg(0x05,0x02);
		GC0329_WriteReg(0x06,0x2c);
		GC0329_WriteReg(0x07,0x00);
		GC0329_WriteReg(0x08,0xb8);
		GC0329_WriteReg(0xfe,0x01);
		GC0329_WriteReg(0x29,0x00);
		GC0329_WriteReg(0x2a,0x60);
		GC0329_WriteReg(0x2b,0x03);
		GC0329_WriteReg(0x2c,0x00);
		GC0329_WriteReg(0x2d,0x03);
		GC0329_WriteReg(0x2e,0xc0);
		GC0329_WriteReg(0x2f,0x04);
		GC0329_WriteReg(0x30,0x80);
		GC0329_WriteReg(0x31,0x05);
		GC0329_WriteReg(0x32,0xa0);
		GC0329_WriteReg(0xfe,0x00);

            break;

        case FLICKER_60HZ:
		GC0329_WriteReg(0xfe,0x00);
		GC0329_WriteReg(0x05,0x02);
		GC0329_WriteReg(0x06,0x2c);
		GC0329_WriteReg(0x07,0x00);
		GC0329_WriteReg(0x08,0x98);
		GC0329_WriteReg(0xfe,0x01);
		GC0329_WriteReg(0x29,0x00);
		GC0329_WriteReg(0x2a,0x50);
		GC0329_WriteReg(0x2b,0x03);
		GC0329_WriteReg(0x2c,0x20);
		GC0329_WriteReg(0x2d,0x03);
		GC0329_WriteReg(0x2e,0xc0);
		GC0329_WriteReg(0x2f,0x04);
		GC0329_WriteReg(0x30,0xb0);
		GC0329_WriteReg(0x31,0x05);
		GC0329_WriteReg(0x32,0xa0);
		GC0329_WriteReg(0xfe,0x00);
            break;

        default:
            break;
    }

    return 0;
}

/******************************************************************************/
// Description: set video mode
// Global resource dependence: 
// Author:
// Note:
//		 
/******************************************************************************/
//__align(4) const SENSOR_REG_T GC0329_video_mode_nor_tab[][15]=
LOCAL SENSOR_REG_T GC0329_video_mode_nor_tab[][18]=
{
    // normal mode      14.3 fps
    {
    		{0xfe,0x00},
    		{0x05,0x02},
    		{0x06,0x2c},
    		{0x07,0x00},
    		{0x08,0xb8},
    		{0xfe,0x01},
    		{0x29,0x00},
    		{0x2a,0x60},
    		{0x2b,0x03},
    		{0x2c,0x00},
    		{0x2d,0x03},
    		{0x2e,0xc0},
    		{0x2f,0x04},
    		{0x30,0x80},
    		{0x31,0x05},
    		{0x32,0xa0},
    		{0xfe,0x00},
    		{0xff,0xff}
	},
    //vodeo mode
    {
    		{0xfe,0x00},
    		{0x05,0x02},
    		{0x06,0x2c},
    		{0x07,0x00},
    		{0x08,0xb8},
    		{0xfe,0x01},
    		{0x29,0x00},
    		{0x2a,0x60},
    		{0x2b,0x03},
    		{0x2c,0x00},
    		{0x2d,0x03},
    		{0x2e,0x00},
    		{0x2f,0x03},
    		{0x30,0x00},
    		{0x31,0x05},
    		{0x32,0xa0},
    		{0xfe,0x00},
    		{0xff,0xff}
    }

};    

LOCAL uint32_t set_GC0329_video_mode(uint32_t mode)
{
    //uint8_t data=0x00;
    uint16_t i;
    SENSOR_REG_T* sensor_reg_ptr = PNULL;
    //uint8_t tempregval = 0;

    //SENSOR_ASSERT(mode <= 1);

    sensor_reg_ptr = (SENSOR_REG_T*)GC0329_video_mode_nor_tab[mode];

    //SENSOR_ASSERT(PNULL != sensor_reg_ptr);

    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value); i++)
    {
    	GC0329_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    }
//ziji	tempregval = GC0329_ReadReg(0xd2);
//    SENSOR_PRINTK("SENSOR: GC0329_ReadReg(0xd2) = %x\n", tempregval);
//    SENSOR_PRINTK("SENSOR: set_video_mode: mode = %d\n", mode);
    return 0;
}
//__align(4) const SENSOR_REG_T GC0329_awb_tab[][5]=
LOCAL SENSOR_REG_T GC0329_awb_tab[][5]=
{
	   	//AUTO
	   	{
			{0x77,0x57},
			{0x78,0x4d},
			{0x79,0x45},
			{0x42, 0xfe},
			{0xff, 0xff}
		},
		//INCANDESCENCE:
		{
			{0x42, 0xfd},    // Disable AWB
			{0x77, 0x48},
			{0x78, 0x40},
			{0x79, 0x5c},
			{0xff, 0xff}
		},
		//U30
		{
			{0x42, 0xfd},   // Disable AWB
			{0x77, 0x40},
			{0x78, 0x54},
			{0x79, 0x70},
			{0xff, 0xff}
		},
		//CWF  //
		{
			{0x42, 0xfd},   // Disable AWB
			{0x77, 0x40},
			{0x78, 0x54},
			{0x79, 0x70},
			{0xff, 0xff}
		},
		//FLUORESCENT:
		{
			{0x42, 0xfd},   // Disable AWB
			{0x77, 0x40},
			{0x78, 0x42},
			{0x79, 0x50},
			{0xff, 0xff} 
		},
		//SUN:
		{
			{0x42, 0xfd},   // Disable AWB
			{0x77, 0x50},
			{0x78, 0x45},
			{0x79, 0x40},
			{0xff, 0xff} 
		},
	            //CLOUD:
		{
			{0x42, 0xfd},   // Disable AWB
			{0x77, 0x5a},
			{0x78, 0x42},
			{0x79, 0x40},
			{0xff, 0xff}
		},
};
	
LOCAL uint32_t set_GC0329_awb(uint32_t mode)
{
	//uint8_t awb_en_value;
	uint32_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC0329_awb_tab[mode];

	//SENSOR_ASSERT(mode < 7);
	//SENSOR_ASSERT(PNULL != sensor_reg_ptr);


    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++)
    {
    	GC0329_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    }
	SENSOR_Sleep(10); 
	SENSOR_PRINTK("SENSOR: set_awb_mode: mode = %d\n", mode);
	
	return 0;
}

//__align(4) const SENSOR_REG_T GC0329_contrast_tab[][2]=
LOCAL SENSOR_REG_T GC0329_contrast_tab[][2]=
{
		{
			{0xd3,0x34}, 	{0xff,0xff},
		},

		{
			{0xd3,0x38}, 	{0xff,0xff},
		},

		{
			{0xd3,0x3d}, 	{0xff,0xff},
		},

		{
			{0xd3,0x40}, 	{0xff,0xff},
		},

		{
			{0xd3,0x44}, 	{0xff,0xff},
		},

		{
			{0xd3,0x48}, 	{0xff,0xff},
		},

		{
			{0xd3,0x50}, 	{0xff,0xff},
		},
};


LOCAL uint32_t set_contrast(uint32_t level)
{
    uint32_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC0329_contrast_tab[level];
	//SENSOR_ASSERT(level < 7);
	//SENSOR_ASSERT(PNULL != sensor_reg_ptr);

    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++)
    {
    	GC0329_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    }
    SENSOR_Sleep(20);
    SENSOR_PRINTK("set_contrast: level = %d\n", level);
    return 0;
}


LOCAL uint32_t set_sharpness(uint32_t level)
{
	
	return 0;
}


LOCAL uint32_t set_saturation(uint32_t level)
{

	
	return 0;
}

/******************************************************************************/
// Description: set brightness 
// Global resource dependence: 
// Author:
// Note:
//		level  must smaller than 8
/******************************************************************************/

LOCAL uint32_t set_preview_mode(uint32_t preview_mode)
{
	SENSOR_PRINTK("set_preview_mode: preview_mode = %d\n", preview_mode);
	
	
	switch (preview_mode)
	{
		case DCAMERA_ENVIRONMENT_NORMAL: 
		{
			GC0329_set_work_mode(0);
			break;
		}
		case DCAMERA_ENVIRONMENT_NIGHT:
		{
			GC0329_set_work_mode(1);
			break;
		}
		case DCAMERA_ENVIRONMENT_SUNNY:
		{
			GC0329_set_work_mode(0);
			break;
		}
		default:
		{
			break;
		}
			
	}
	
	SENSOR_Sleep(50);
	
	return 0;
}

//__align(4) const SENSOR_REG_T GC0329_image_effect_tab[][11]=	
LOCAL SENSOR_REG_T GC0329_image_effect_tab[][6]=
{
		// effect normal
			{
				{0x43,0x00},

				{0x95,0x65},
				{0xd3,0x42},

				{0xda,0x00},
				{0xdb,0x00},
				{0xff,0xff}
			},
			//effect BLACKWHITE
			{
				{0x43,0x02},
				{0xd3,0x40},
				{0xda,0x00},
				{0xdb,0x00},
				{0xff,0xff},
				{0xff,0xff}
			},
			// effect RED pink
			{
				{0x43,0x02},
				{0x95,0x88},
				{0xd3,0x40},
				{0xda,0x10},
				{0xdb,0x50},
				{0xff, 0xff},
			},
			// effect GREEN
			{
				{0x43,0x02},
				{0x95,0x88},
				{0xd3,0x40},
				{0xda,0xc0},
				{0xdb,0xc0},
				{0xff, 0xff},
			},
			// effect  BLUE
			{
				{0x43,0x02},
				{0xd3,0x40},
				{0xda,0x50},
				{0xdb,0xe0},
				{0xff, 0xff},
				{0xff, 0xff}
			},
			// effect  YELLOW
			{
				{0x43,0x02},
				{0x95,0x88},
				{0xd3,0x40},
				{0xda,0x80},
				{0xdb,0x20},
				{0xff, 0xff}
			},
			// effect NEGATIVE
			{
				{0x43,0x01},
				{0xd3,0x40},
				{0xda,0x00},
				{0xdb,0x00},
				{0xff, 0xff},
				{0xff, 0xff}
			},
			//effect ANTIQUE
			{
				{0x43,0x02},
				{0xd3,0x40},
				{0xda,0xd2},
				{0xdb,0x28},
				{0xff, 0xff},
				{0xff, 0xff}
			},
};
LOCAL uint32_t set_image_effect(uint32_t effect_type)
{
    uint16_t i;
    
    SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC0329_image_effect_tab[effect_type];

    //SENSOR_ASSERT(PNULL != sensor_reg_ptr);

    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++)
    {
        Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    }
    SENSOR_PRINTK("-----------set_image_effect: effect_type = %d------------\n", effect_type);
    
    return 0;
}


LOCAL uint32_t read_ev_value(uint32_t value)
{
	return 0;
}

LOCAL uint32_t write_ev_value(uint32_t exposure_value)
{
	
	return 0;	
}

LOCAL uint32_t read_gain_value(uint32_t value)
{

	
	return 0;
}

LOCAL uint32_t write_gain_value(uint32_t gain_value)
{

	
	return 0;
}

LOCAL uint32_t read_gain_scale(uint32_t value)
{
	return SENSOR_GAIN_SCALE;
	
}


LOCAL uint32_t set_frame_rate(uint32_t param)
    
{
	//GC0329_WriteReg( 0xd8, uint8_t data );
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence: 
// Author:
// Note:
//		mode 0:normal;	 1:night 
/******************************************************************************/
//__align(4) const SENSOR_REG_T GC0329_mode_tab[][8]=
LOCAL SENSOR_REG_T GC0329_mode_tab[][8]=
{
	{{0xfe, 0x01},{0x33, 0x20},{0xfe, 0x00},{0xFF, 0xFF},},
	{{0xfe, 0x01},{0x33, 0x30},{0xfe, 0x00},{0xFF, 0xFF},},
};

LOCAL uint32_t GC0329_set_work_mode(uint32_t mode)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC0329_mode_tab[mode];

	//SENSOR_ASSERT(mode <= 1);
	//SENSOR_ASSERT(PNULL != sensor_reg_ptr);
	
	for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value); i++)
	{
		GC0329_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINTK("set_work_mode: mode = %d\n", mode);
	return 0;
}

struct sensor_drv_cfg sensor_gc0329 = {                                                                                                                         
    .sensor_pos = 1,                                                                                                                
    .sensor_name = "gc0329",                                                                                                                                    
    .driver_info = &g_GC0329_yuv_info,                                                                                                                          
};                                                                                                                                                              
                                                                                                                                                                
static int __init sensor_gc0329_init(void)                                                                                                                      
{                                                                                                                                                               
    printk("in sensor_gc0329_init \n");                                                                                                                         
    return dcam_register_sensor_drv(&sensor_gc0329);                                                                                                            
}                                                                                                                                                               
                                                                                                                                                                
subsys_initcall(sensor_gc0329_init); 
