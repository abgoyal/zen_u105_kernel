/******************************************************************************
 ** File Name:    Sensor_SP0A19.c                                         *
 ** Author:        superpix                                      	  *
 ** Date:           03/02/2011                                          *
 ** Copyright:    Spreadtrum All Rights Reserved.        *
 ** Description:   implementation of digital camera register interface       *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 **                                  *
 ******************************************************************************/

#include <linux/delay.h>
#include "common/sensor.h"
#include "common/jpeg_exif_header_k.h"
#include "common/sensor_drv.h"


#undef MYSENSOR_DEBUG
#ifdef MYSENSOR_DEBUG
#define SENSOR_PRINTK printk
#else
#define SENSOR_PRINTK(...)
#endif

#define SP0A19_I2C_ADDR_W    		0x21 //	0x42
#define SP0A19_I2C_ADDR_R    		0x21 //	0x43
#define SP0A19_I2C_ACK				       0x0
typedef enum
{
	DCAMERA_FLICKER_50HZ = 0,
	DCAMERA_FLICKER_60HZ,
	FLICKER_MAX
}FLICKER_E;


//LOCAL uint32_t			s_sensor_mclk		 = 0;
LOCAL uint32_t			Antiflicker		 = DCAMERA_FLICKER_50HZ;

static uint32_t  g_flash_mode_en = 0;
uint32_t Sensor_SetFlash(uint32_t flash_mode);
 
//AE target
#define  Pre_Value_P0_0xf7  0x8c-0x08
#define  Pre_Value_P0_0xf8  0x84-0x08
#define  Pre_Value_P0_0xf9  0x7c-0x08
#define  Pre_Value_P0_0xfa  0x74-0x08
//HEQ
#define  Pre_Value_P0_0xdd  0x78//0x84
#define  Pre_Value_P0_0xde  0xa0
//AWB pre gain
#define  Pre_Value_P1_0x28  0xc5
#define  Pre_Value_P1_0x29  0x9b
LOCAL uint32_t SP0A19_Power_On(uint32_t power_on);

//LOCAL uint32_t SP0A19_Power_On(uint32_t power_on);
LOCAL uint32_t SP0A19_flash(uint32_t param);

LOCAL SENSOR_REG_T SP0A19_YUV_640X480[]=
{
//SP0A19 ini
          //init
		  {0xfd,0x00},
		  {0x1C,0x28},
		  {0x32,0x00},
		  {0x0f,0x2f},
		  {0x10,0x2e},
		  {0x11,0x00},
		  {0x12,0x18},
		  {0x13,0x2f},
		  {0x14,0x00},
		  {0x15,0x3f},
		  {0x16,0x00},
		  {0x17,0x18},
		  {0x25,0x40},
		  {0x1a,0x0b},
		  {0x1b,0xc },
		  {0x1e,0xb },
		  {0x20,0x3f}, // add
		  {0x21,0x13}, // 0x0c 24
		  {0x22,0x19},
		  {0x26,0x1a},
		  {0x27,0xab},
		  {0x28,0xfd},
		  {0x30,0x00},
		  #if((7635 == CONFIG_ZYT_PRJ)&&(CONFIG_ZYT_CUST==65))
		  {0x31,0x60},
		  #else
		  {0x31,0x00},
		  #endif
		  {0xfb,0x33},
		  {0x1f,0x08}, //0x10 2012-9-14
		  
		  
		//Blacklevel
		  {0xfd,0x00},
		  {0x65,0x00},//blue_suboffset 0x04
		  {0x66,0x00},//red_suboffset  0x05
		  {0x67,0x00},//gr_suboffset   0x06
		  {0x68,0x00},//gb_suboffset  0x07

		//ae setting
		  {0xfd,0x00},
		  {0x03,0x01},
		  {0x04,0x32},
		  {0x06,0x00},
		  {0x09,0x01},
		  {0x0a,0x46},
		  {0xf0,0x66},
		  {0xf1,0x00},
		  {0xfd,0x01},
		  {0x90,0x0c},
		  {0x92,0x01},
		  {0x98,0x66},
		  {0x99,0x00},
		  {0x9a,0x01},
		  {0x9b,0x00},
		  
		//Status
		  {0xfd,0x01},
		  {0xce,0xc8},
		  {0xcf,0x04},
		  {0xd0,0xc8},
		  {0xd1,0x04}, 
		  
		  {0xfd,0x01},
		  {0xc4,0x46},//70
		  {0xc5,0x8f},//74
		  {0xca,0x30},
		  {0xcb,0x45},
		  {0xcc,0x70},//rpc_heq_low
		  {0xcd,0x48},//rpc_heq_dummy
		  {0xfd,0x00},
		  //lsc  for st 
		  {0xfd,0x01},
		  {0x35,0x15},
		  {0x36,0x15}, //20
		  {0x37,0x15},
		  {0x38,0x15},
		  {0x39,0x15},
		  {0x3a,0x15}, //15
		  {0x3b,0x13},
		  {0x3c,0x15},
		  {0x3d,0x15},
		  {0x3e,0x15}, //12
		  {0x3f,0x15},
		  {0x40,0x18},
		  {0x41,0x00},
		  {0x42,0x04},
		  {0x43,0x04},
		  {0x44,0x00},
		  {0x45,0x00},
		  {0x46,0x00},
		  {0x47,0x00},
		  {0x48,0x00},
		  {0x49,0xfd},
		  {0x4a,0x00},
		  {0x4b,0x00},
		  {0x4c,0xfd},
		  {0xfd,0x00},
		 
  
		//awb 1
		  {0xfd,0x01},
		  {0x28,0xc5},
		  {0x29,0x9b},
		//{0x10,0x08},
		//{0x11,0x14},	
		//{0x12,0x14},
		  {0x2e,0x02},	
		  {0x2f,0x16},
		  {0x17,0x14},  
		  {0x18,0x19},	//0x29   0813
		  {0x19,0x45},	

		//{0x1a,0x9e},//a1;a5	  
		//{0x1b,0xae},//b0;9a
		//{0x33,0xef},
		  {0x2a,0xef},
		  {0x2b,0x15},

		  //awb2
		  {0xfd,0x01},
		  {0x73,0x80},
		  {0x1a,0x80},
		  {0x1b,0x80}, 
		//d65
		  {0x65,0xd5}, //d6
		  {0x66,0xfa}, //f0
		  {0x67,0x72}, //7a
		  {0x68,0x8a}, //9a
		//indoor
		  {0x69,0xc6}, //ab
		  {0x6a,0xee}, //ca
		  {0x6b,0x94}, //a3
		  {0x6c,0xab}, //c1
		//f 
		  {0x61,0x7a}, //82
		  {0x62,0xa9}, //a5
		  {0x63,0xc5}, //d6
		  {0x64,0xe6}, //ec
		  //cwf
		  {0x6d,0xb9}, //a5
		  {0x6e,0xde}, //c2
		  {0x6f,0xb2}, //a7
		  {0x70,0xd5}, //c5
		 
		//skin detect
		 {0xfd,0x01},
		 {0x08,0x15},
		 {0x09,0x04},
		 {0x0a,0x20},
		 {0x0b,0x12},
		 {0x0c,0x27},
		 {0x0d,0x06},
		 {0x0f,0x63},//0x5f   0813

		   //BPC_grad
		  {0xfd,0x00},
		  {0x79,0xf0},
		  {0x7a,0xf0},  
		  {0x7b,0xf0},  
		  {0x7c,0xf0},    
		  
		//smooth
		  {0xfd,0x00},
		  #if 0
			//μ￥í¨μà???????D?μ	
		  {0x57,0x04}, //raw_dif_thr_outdoor
		  {0x58,0x06}, //raw_dif_thr_normal
		  {0x56,0x0b}, //raw_dif_thr_dummy
		  {0x59,0x0d}, //raw_dif_thr_lowlight
			//GrGb?????D?μ
		  {0x89,0x04}, //raw_grgb_thr_outdoor 
		  {0x8a,0x06}, //raw_grgb_thr_normal  
		  {0x9c,0x0b}, //raw_grgb_thr_dummy   
		  {0x9d,0x0d}, //raw_grgb_thr_lowlight
		  #else
		  //μ￥í¨μà???????D?μ	
		  {0x57,0x06}, //raw_dif_thr_outdoor
		  {0x58,0x0d}, //raw_dif_thr_normal
		  {0x56,0x10}, //raw_dif_thr_dummy
		  {0x59,0x10}, //raw_dif_thr_lowlight
			//GrGb?????D?μ
		  {0x89,0x06}, //raw_grgb_thr_outdoor 
		  {0x8a,0x0d}, //raw_grgb_thr_normal  
		  {0x9c,0x14}, //raw_grgb_thr_dummy   
		  {0x9d,0x16}, //raw_grgb_thr_lowlight

		  #endif
			//Gr\Gb???????????è
		  {0x81,0xe0}, //raw_gflt_fac_outdoor
		  {0x82,0xd0}, //raw_gflt_fac_normal
		  {0x83,0x60}, //raw_gflt_fac_dummy
		  {0x84,0x30}, //raw_gflt_fac_lowlight
			//Gr?￠Gbμ￥í¨μà?ú???????è  
		  {0x85,0xe0}, //raw_gf_fac_outdoor  
		  {0x86,0xc0}, //raw_gf_fac_normal  
		  {0x87,0x80}, //raw_gf_fac_dummy   
		  {0x88,0x40}, //raw_gf_fac_lowlight
			//R?￠B???????è  
		  {0x5a,0xff},  //raw_rb_fac_outdoor
		  {0x5b,0xe0},  //raw_rb_fac_normal
		  {0x5c,0x80},  //raw_rb_fac_dummy
		  {0x5d,0x00},  //raw_rb_fac_lowlight
		  
		//sharpen 
		  {0xfd,0x01}, 
		  {0xe2,0x30}, //sharpen_y_base
		  {0xe4,0xa0}, //sharpen_y_max

		  {0xe5,0x04}, //rangek_neg_outdoor  //0x08
		  {0xd3,0x04}, //rangek_pos_outdoor   //0x08
		  {0xd7,0x04}, //range_base_outdoor   //0x08
		 
		  {0xe6,0x04}, //rangek_neg_normal   // 0x08
		  {0xd4,0x04}, //rangek_pos_normal   // 0x08
		  {0xd8,0x04}, //range_base_normal   // 0x08
		  
		  {0xe7,0x08}, //rangek_neg_dummy   // 0x10
		  {0xd5,0x08}, //rangek_pos_dummy   // 0x10
		  {0xd9,0x08}, //range_base_dummy    // 0x10
			
		  {0xd2,0x10}, //rangek_neg_lowlight
		  {0xd6,0x10}, //rangek_pos_lowlight
		  {0xda,0x10}, //range_base_lowlight
		 
		  {0xe8,0x36},//0x30  //sharp_fac_pos_outdoor  // 0x35
		  {0xec,0x3a},//0x35  //sharp_fac_neg_outdoor
		  {0xe9,0x2b},//0x28  //sharp_fac_pos_nr      // 0x35
		  {0xed,0x3a},//0x35  //sharp_fac_neg_nr
		  {0xea,0x12},//0x10  //sharp_fac_pos_dummy   // 0x30
		  {0xef,0x24},//0x20  //sharp_fac_neg_dummy   // 0x20
		  {0xeb,0x0f},//0x0c  //sharp_fac_pos_low
		  {0xf0,0x1b},//0x18  //sharp_fac_neg_low 
		  #if 0
		//CCM
		  {0xfd,0x01},
		  {0xa0,0x80},
		  {0xa1,0x00},
		  {0xa2,0x00},
		  {0xa3,0xf6},
		  {0xa4,0x99},
		  {0xa5,0xf2},
		  {0xa6,0x0d},
		  {0xa7,0xda},
		  {0xa8,0xa0},
		  {0xa9,0x00},
		  {0xaa,0x33},
		  {0xab,0x0c},
		  {0xfd,0x00},
		#else
		//CCM
		  {0xfd,0x01},
		  {0xa0,0xa6},
		  {0xa1,0xc0},
		  {0xa2,0x19},
		  {0xa3,0x00},
		  {0xa4,0xb3},
		  {0xa5,0xcd},
		  {0xa6,0x0d},
		  {0xa7,0xda},
		  {0xa8,0x98},
		  {0xa9,0x0c},
		  {0xaa,0x30},
		  {0xab,0x0c},
		  {0xfd,0x00},
		#endif
		  
		  #if 0
		  		//gamma  
		  {0xfd,0x00},
		  {0x8b,0x0 },  // 00;0 ;0 
		  {0x8c,0xC },  // 0f;C ;11
		  {0x8d,0x19},  // 1e;19;19
		  {0x8e,0x2C},  // 3d;2C;28
		  {0x8f,0x49},  // 6c;49;46
		  {0x90,0x61},  // 92;61;61
		  {0x91,0x77},  // aa;77;78
		  {0x92,0x8A},  // b9;8A;8A
		  {0x93,0x9B},  // c4;9B;9B
		  {0x94,0xA9},  // cf;A9;A9
		  {0x95,0xB5},  // d4;B5;B5
		  {0x96,0xC0},  // da;C0;C0
		  {0x97,0xCA},  // e0;CA;CA
		  {0x98,0xD4},  // e4;D4;D4
		  {0x99,0xDD},  // e8;DD;DD
		  {0x9a,0xE6},  // ec;E6;E6
		  {0x9b,0xEF},  // f1;EF;EF
		  {0xfd,0x01},  // 01;01;01
		  {0x8d,0xF7},  // f7;F7;F7
		  {0x8e,0xFF},  // ff;FF;FF		 
		  {0xfd,0x00},  //

		  #else
		//gamma  
		  {0xfd,0x00},
		  {0x8b,0x0 },  // 00;0 ;0 
		  {0x8c,0x19 },  // 0f;C ;11
		  {0x8d,0x26},  // 1e;19;19
		  {0x8e,0x37},  // 3d;2C;28
		  {0x8f,0x51},  // 6c;49;46
		  {0x90,0x65},  // 92;61;61
		  {0x91,0x77},  // aa;77;78
		  {0x92,0x87},  // b9;8A;8A
		  {0x93,0x95},  // c4;9B;9B
		  {0x94,0xA2},  // cf;A9;A9
		  {0x95,0xae},  // d4;B5;B5
		  {0x96,0xba},  // da;C0;C0
		  {0x97,0xC5},  // e0;CA;CA
		  {0x98,0xD0},  // e4;D4;D4
		  {0x99,0xDa},  // e8;DD;DD
		  {0x9a,0xE4},  // ec;E6;E6
		  {0x9b,0xEd},  // f1;EF;EF
		  {0xfd,0x01},  // 01;01;01
		  {0x8d,0xF7},  // f7;F7;F7
		  {0x8e,0xFF},  // ff;FF;FF		 
		  {0xfd,0x00},  //
		   #endif  
		   //rpc
		  {0xfd,0x00}, 
		  {0xe0,0x4c}, //  4c;44;4c;3e;3c;3a;38;rpc_1base_max
		  {0xe1,0x3c}, //  3c;36;3c;30;2e;2c;2a;rpc_2base_max
		  {0xe2,0x34}, //  34;2e;34;2a;28;26;26;rpc_3base_max
		  {0xe3,0x2e}, //  2e;2a;2e;26;24;22;rpc_4base_max
		  {0xe4,0x2e}, //  2e;2a;2e;26;24;22;rpc_5base_max
		  {0xe5,0x2c}, //  2c;28;2c;24;22;20;rpc_6base_max
		  {0xe6,0x2c}, //  2c;28;2c;24;22;20;rpc_7base_max
		  {0xe8,0x2a}, //  2a;26;2a;22;20;20;1e;rpc_8base_max
		  {0xe9,0x2a}, //  2a;26;2a;22;20;20;1e;rpc_9base_max 
		  {0xea,0x2a}, //  2a;26;2a;22;20;20;1e;rpc_10base_max
		  {0xeb,0x28}, //  28;24;28;20;1f;1e;1d;rpc_11base_max
		  {0xf5,0x28}, //  28;24;28;20;1f;1e;1d;rpc_12base_max
		  {0xf6,0x28}, //  28;24;28;20;1f;1e;1d;rpc_13base_max	

		//ae min gain  
		  {0xfd,0x01},
		  {0x94,0xc0},  //rpc_max_indr
		  {0x95,0x28},   // 1e rpc_min_indr 
		  {0x9c,0xc0},  //rpc_max_outdr
		  {0x9d,0x28},  //rpc_min_outdr    
		//ae target
		{0xfd,0x00},
		{0xed,Pre_Value_P0_0xf7+4},			
		{0xf7,Pre_Value_P0_0xf7},
		{0xf8,Pre_Value_P0_0xf8},	
		{0xec,Pre_Value_P0_0xf8-4},		
		{0xef,Pre_Value_P0_0xf9+4},	
		{0xf9,Pre_Value_P0_0xf9},
		{0xfa,Pre_Value_P0_0xfa},		
		{0xee,Pre_Value_P0_0xfa-4},		

		//gray detect
		  {0xfd,0x01},
		  {0x30,0x40},
		  //add 0813 
		  {0x31,0x70},
		  {0x32,0x40},
		  {0x33,0xef},
		  {0x34,0x05},
		  {0x4d,0x2f},
		  {0x4e,0x20},
		  {0x4f,0x16},

		//lowlight lum
		  {0xfd,0x00}, //
		  {0xb2,0x20}, //lum_limit  // 0x10
		  {0xb3,0x1f}, //lum_set
		  {0xb4,0x30}, //black_vt  // 0x20
		  {0xb5,0x45}, //white_vt

		//saturation
		  {0xfd,0x00}, 
		  {0xbe,0xff}, 
		  {0xbf,0x01}, 
		  {0xc0,0xff}, 
		  {0xc1,0xd8}, 
		  {0xd3,0x80}, //0x78
		  {0xd4,0x80}, //0x78
		  {0xd6,0x60}, //0x78        {0xd7,0x60}, //0x78
		  {0xd7,0x40},

		//HEQ
		  {0xfd,0x00}, 
		  {0xdc,0x00}, 
		  {0xdd,Pre_Value_P0_0xdd}, //0x80 0813  // 0x78
		  {0xde,Pre_Value_P0_0xde}, //80
		  {0xdf,0x80}, 
		   
		//func enable
		  {0xfd,0x00},  
		  {0x32,0x15},  //0x0d
		  {0x34,0x16},  
		  {0x35,0x40},  
		  {0x33,0xef},  
		  {0x5f,0x51},  

             //   {SENSOR_WRITE_DELAY, 200},//delay 200ms
	         {0xff , 0xff},	
};

/**---------------------------------------------------------------------------*
 **                         Function Definitions                              *
 **---------------------------------------------------------------------------*/
LOCAL void SP0A19_WriteReg( uint8_t  subaddr, uint8_t data )
{
    //uint8_t cmd[2];
    //cmd[0]	=	subaddr;
    //cmd[1]	=	data;
    Sensor_WriteReg_8bits(subaddr, data);
    //I2C_WriteCmdArr(SP0A19_I2C_ADDR_W, cmd, 2, SCI_TRUE);
    
    //	SENSOR_PRINTK("SENSOR: SP0A19_WriteReg reg/value(%x,%x) !!", subaddr, data);
}

LOCAL uint8_t SP0A19_ReadReg( uint8_t  subaddr)
{
    uint8_t value = 0;
    
    //I2C_WriteCmdArr(SP0A19_I2C_ADDR_W, &subaddr, 1, SCI_TRUE);
    //I2C_ReadCmd(SP0A19_I2C_ADDR_R, &value, SCI_TRUE);
    //SENSOR_PRINTK( "SP0A19 read ret is %X=%X",subaddr,value );
    value = Sensor_ReadReg( subaddr);
    return value;
}

#define id_reg 0x02
#define id_value 0xA6
#define TRY_COUNTS 2
LOCAL uint32_t SP0A19_Identify(uint32_t param)
{
	uint32_t i;
    
    for(i = 0; i<2;i++ )
    {
        if(SP0A19_ReadReg(id_reg)==id_value)
        {
            printk("%s:Success", __FUNCTION__);
            return  (uint32_t)SENSOR_SUCCESS;
        }
        msleep(5);
    }
    
    printk("%s:Fail", __FUNCTION__);
    return SENSOR_FAIL;
}

LOCAL uint32_t SP0A19_flash(uint32_t param)
{
    g_flash_mode_en = param;
    Sensor_SetFlash(param);

    return 0;
}

LOCAL uint32_t SP0A19_set_hmirror_enable(uint32_t enable);
LOCAL uint32_t SP0A19_set_vmirror_enable(uint32_t enable);

LOCAL SENSOR_REG_T SP0A19_brightness_tab[][3]=
{
	{
		{0xfd,0x00},
		{0xdc,0xd0},//level -3
		{0xff,0xff},
	},

	{
		{0xfd,0x00},
		{0xdc,0xe0},//level -2
		{0xff,0xff},
	},

	{
		{0xfd,0x00},
		{0xdc,0xf0},//level -1
		{0xff,0xff},
	},

	{
		{0xfd,0x00},
		{0xdc,0x00},//level 0
		{0xff,0xff},
	},

	{
		{0xfd,0x00},
		{0xdc,0x10},//level +1
		{0xff,0xff},
	},

	{
		{0xfd,0x00},
		{0xdc,0x20},//level +2
		{0xff,0xff},
	},
    {		
		{0xfd,0x00},
		{0xdc,0x30},//level +3
		{0xff,0xff},
	},
};
LOCAL uint32_t SP0A19_set_brightness(uint32_t level)
{ 
    uint16_t i;
    SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP0A19_brightness_tab[level];

  //  SCI_ASSERT(PNULL != sensor_reg_ptr);
    
    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++)
    {
        Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    } 
    return 0;
}

LOCAL SENSOR_REG_T SP0A19_contrast_tab[][4]=
{
	{
		{0xfd, 0x00},
		{0xdd,  Pre_Value_P0_0xdd-0x30},	//level -3
		{0xde,  Pre_Value_P0_0xde-0x30},
		{0xff, 0xff}
	},

	{
		{0xfd, 0x00},
		{0xdd,  Pre_Value_P0_0xdd-0x20},	//level -2
		{0xde,  Pre_Value_P0_0xde-0x20},
		{0xff, 0xff}
	},

	{
		{0xfd, 0x00},
		{0xdd, Pre_Value_P0_0xdd-0x10},	//level -1
		{0xde, Pre_Value_P0_0xde-0x10},
		{0xff, 0xff}
	},

	{
		{0xfd, 0x00},
		{0xdd, Pre_Value_P0_0xdd},	//level 0
		{0xde, Pre_Value_P0_0xde},
		{0xff, 0xff}
	},

	{
		{0xfd, 0x00},
		{0xdd,  Pre_Value_P0_0xdd+0x10},	//level +1
		{0xde,  Pre_Value_P0_0xde+0x10},
		{0xff, 0xff}
	},

	{
		{0xfd, 0x00},
		{0xdd,  Pre_Value_P0_0xdd+0x20},	//level +2
		{0xde,  Pre_Value_P0_0xde+0x20},
		{0xff, 0xff}
	},

	{
		{0xfd, 0x00},
		{0xdd,  Pre_Value_P0_0xdd+0x30},	//level +3
		{0xde,  Pre_Value_P0_0xde+0x30},
		{0xff, 0xff}
	}, 
};

LOCAL uint32_t SP0A19_set_contrast(uint32_t level)
{
    uint16_t i;    
    
    SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP0A19_contrast_tab[level];

//    SCI_ASSERT(PNULL != sensor_reg_ptr);
    
    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++)
    {
        Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    }
    return 0;
}

#if 0
LOCAL uint32_t Sensor_Write_Regs( SENSOR_REG_T* sensor_reg_ptr )
{
    uint32_t i;
    
    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value) ; i++)
    {
        Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    }
    return 0;
}


LOCAL SENSOR_REG_T SP0A19_mode_tab[][4]=
{
#if 1
{    
{0xfd,0x00},
{0xfd,0x00},
{0xfd,0x00}, 
{0xFF,0xFF},
}
#else
//capture preview daylight
{     
{0xfd,0x00},
{0xb2,NORMAL_Y0ffset}, 
{0xb3,0x1f}, //lum_set
{0xFF,0xFF},	//数组结束标志
},
//capture preview night
{    
{0xfd,0x00},
{0xb2,LOWLIGHT_Y0ffset}, 
{0xb3,0x1f}, //lum_set
{0xFF,0xFF},	//数组结束标志 
}
#endif
};  
#endif

LOCAL uint32_t SP0A19_set_preview_mode(uint32_t preview_mode)
{
	switch (preview_mode)
	{
		case DCAMERA_ENVIRONMENT_NORMAL: 
		case DCAMERA_ENVIRONMENT_SUNNY:
		{
			SP0A19_WriteReg(0xfd,0x0 );
			SP0A19_WriteReg(0xb2,0x20);
			SP0A19_WriteReg(0xb3,0x1f);
			if(Antiflicker== DCAMERA_FLICKER_50HZ)
			{
			SENSOR_PRINTK("normal mode 50hz\r\n");
			//capture preview daylight 24M 50hz 20-8FPS maxgain:0x70   
			  SP0A19_WriteReg(0xfd,0x00);
			  SP0A19_WriteReg(0x03,0x01);
			  SP0A19_WriteReg(0x04,0x32);
			  SP0A19_WriteReg(0x06,0x00);
			  SP0A19_WriteReg(0x09,0x01);
			  SP0A19_WriteReg(0x0a,0x46);
			  SP0A19_WriteReg(0xf0,0x66);
			  SP0A19_WriteReg(0xf1,0x00);
			  SP0A19_WriteReg(0xfd,0x01);
			  SP0A19_WriteReg(0x90,0x0c);
			  SP0A19_WriteReg(0x92,0x01);
			  SP0A19_WriteReg(0x98,0x66);
			  SP0A19_WriteReg(0x99,0x00);
			  SP0A19_WriteReg(0x9a,0x01);
			  SP0A19_WriteReg(0x9b,0x00);
			  SP0A19_WriteReg(0xfd,0x01);
			  SP0A19_WriteReg(0xce,0xc8);
			  SP0A19_WriteReg(0xcf,0x04);
			  SP0A19_WriteReg(0xd0,0xc8);
			  SP0A19_WriteReg(0xd1,0x04);
			  SP0A19_WriteReg(0xfd,0x00);		
			 }
			else	
			{
			SENSOR_PRINTK("normal mode 60hz\r\n");
			//capture preview daylight 24M 60hz 20-8FPS maxgain:0x70   
			  SP0A19_WriteReg(0xfd,0x00);
			  SP0A19_WriteReg(0x03,0x00);
			  SP0A19_WriteReg(0x04,0xff);
			  SP0A19_WriteReg(0x06,0x00);
			  SP0A19_WriteReg(0x09,0x01);
			  SP0A19_WriteReg(0x0a,0x46);
			  SP0A19_WriteReg(0xf0,0x55);
			  SP0A19_WriteReg(0xf1,0x00);
			  SP0A19_WriteReg(0xfd,0x01);
			  SP0A19_WriteReg(0x90,0x0f);
			  SP0A19_WriteReg(0x92,0x01);
			  SP0A19_WriteReg(0x98,0x55);
			  SP0A19_WriteReg(0x99,0x00);
			  SP0A19_WriteReg(0x9a,0x01);
			  SP0A19_WriteReg(0x9b,0x00);
			  SP0A19_WriteReg(0xfd,0x01);
			  SP0A19_WriteReg(0xce,0xfb);
			  SP0A19_WriteReg(0xcf,0x04);
			  SP0A19_WriteReg(0xd0,0xfb);
			  SP0A19_WriteReg(0xd1,0x04);
			  SP0A19_WriteReg(0xfd,0x00);			
			 }	
			break;
		}
		case DCAMERA_ENVIRONMENT_NIGHT:
		{
			SP0A19_WriteReg(0xfd,0x0 );
			SP0A19_WriteReg(0xb2,0x25);
			SP0A19_WriteReg(0xb3,0x1f);
			if(Antiflicker== DCAMERA_FLICKER_50HZ)
			{
			SENSOR_PRINTK("night mode 50hz\r\n");
			//capture preview night 24M 50hz 20-6FPS maxgain:0x78	 
			  SP0A19_WriteReg(0xfd,0x00);
			  SP0A19_WriteReg(0x03,0x01);
			  SP0A19_WriteReg(0x04,0x32);
			  SP0A19_WriteReg(0x06,0x00);
			  SP0A19_WriteReg(0x09,0x01);
			  SP0A19_WriteReg(0x0a,0x46);
			  SP0A19_WriteReg(0xf0,0x66);
			  SP0A19_WriteReg(0xf1,0x00);
			  SP0A19_WriteReg(0xfd,0x01);
			  SP0A19_WriteReg(0x90,0x10);
			  SP0A19_WriteReg(0x92,0x01);
			  SP0A19_WriteReg(0x98,0x66);
			  SP0A19_WriteReg(0x99,0x00);
			  SP0A19_WriteReg(0x9a,0x01);
			  SP0A19_WriteReg(0x9b,0x00);
			  SP0A19_WriteReg(0xfd,0x01);
			  SP0A19_WriteReg(0xce,0x60);
			  SP0A19_WriteReg(0xcf,0x06);
			  SP0A19_WriteReg(0xd0,0x60);
			  SP0A19_WriteReg(0xd1,0x06);
			  SP0A19_WriteReg(0xfd,0x00);		
			 }
			else
			{
			SENSOR_PRINTK("night mode 60hz\r\n");
			//capture preview night 24M 60hz 20-6FPS maxgain:0x78
			  SP0A19_WriteReg(0xfd,0x00);
			  SP0A19_WriteReg(0x03,0x00);
			  SP0A19_WriteReg(0x04,0xff);
			  SP0A19_WriteReg(0x06,0x00);
			  SP0A19_WriteReg(0x09,0x01);
			  SP0A19_WriteReg(0x0a,0x46);
			  SP0A19_WriteReg(0xf0,0x55);
			  SP0A19_WriteReg(0xf1,0x00);
			  SP0A19_WriteReg(0xfd,0x01);
			  SP0A19_WriteReg(0x90,0x14);
			  SP0A19_WriteReg(0x92,0x01);
			  SP0A19_WriteReg(0x98,0x55);
			  SP0A19_WriteReg(0x99,0x00);
			  SP0A19_WriteReg(0x9a,0x01);
			  SP0A19_WriteReg(0x9b,0x00);
			  SP0A19_WriteReg(0xfd,0x01);
			  SP0A19_WriteReg(0xce,0xa4);
			  SP0A19_WriteReg(0xcf,0x06);
			  SP0A19_WriteReg(0xd0,0xa4);
			  SP0A19_WriteReg(0xd1,0x06);
			  SP0A19_WriteReg(0xfd,0x00);		
			 }
			break;
		}
		
		default:
		{
			break;
		}

	SENSOR_Sleep(100);
	} 
return 0;
}


LOCAL SENSOR_REG_T SP0A19_image_effect_tab[][5]=	
{
    // effect normal
    {
		{0xfd, 0x00},
		{0x62, 0x00},
		{0x63, 0x80},
		{0x64, 0x80},
		{0xff, 0xff}
    },
    //effect BLACKWHITE//GRAYSCALE
    {
		{0xfd, 0x00},
		{0x62, 0x20},
		{0x63, 0x80},
		{0x64, 0x80},
		{0xff, 0xff}
    },
    // effect RED pink
    {
		{0xfd, 0x00},
		{0x62, 0x10},
		{0x63, 0xb0},
		{0x64, 0x90},
		{0xff, 0xff}
    },
    // effect GREEN//SEPIAGREEN
    {
		{0xfd, 0x00},
		{0x62, 0x10},
		{0x63, 0x20},
		{0x64, 0x20},
		{0xff, 0xff}
    },
    // effect  BLUE//SEPIABLUE
    {
		{0xfd, 0x00},
		{0x62, 0x10},
		{0x63, 0x20},
		{0x64, 0xf0},
		{0xff, 0xff}
    },
    // effect  YELLOW//SEPIAYELLOW
    {
		{0xfd, 0x00},
		{0x62, 0x10},
		{0x63, 0xb0},
		{0x64, 0x00},
		{0xff, 0xff}

    },  
    // effect NEGATIVE//COLORINV
    {	     
		{0xfd, 0x00},
		{0x62, 0x04},
		{0x63, 0x80},
		{0x64, 0x80},
		{0xff, 0xff}
    },    
    //effect ANTIQUE//SEPIA
    {
		{0xfd, 0x00},
		{0x62, 0x10},
		{0x63, 0xc0},
		{0x64, 0x20},
		{0xff, 0xff}
    },
};
LOCAL uint32_t SP0A19_set_image_effect(uint32_t effect_type)
{
    uint16_t  i;
    
    SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP0A19_image_effect_tab[effect_type];

   // SCI_ASSERT(PNULL != sensor_reg_ptr);

    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++)
    {
        Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    }
    return 0;
}




LOCAL void Delayms (uint32_t ms)	
{
}

LOCAL uint32_t SP0A19_before_snapshot(uint32_t para)
{

	return 0;
}

LOCAL uint32_t SP0A19_after_snapshot(uint32_t para)
{


	return 0;
}

//@ Chenfeng for adding AWB & AE functions
LOCAL SENSOR_REG_T SP0A19_awb_tab[][7]=
{
	   	//AUTO  // AUTO 3000K~7000K 
	   	{
			{0xfd,0x01},                      
			{0x28,0xc5},		                  
			{0x29,0x9b},                      
			{0xfd,0x00}, 		
			{0x32,0x15},   
			{0xfd,0x00},  
		{0xff,0xff} 
		},    
		//INCANDESCENCE:  //2800K~3000K  
		{
			{0xfd,0x00}, 
			{0x32,0x05},                 
			{0xfd,0x01},                 
			{0x28,0x89},		             
			{0x29,0xb8},		             
			{0xfd,0x00}, 
		{0xff,0xff} 
		},
		//U30 //3000K
		{
			{0xfd,0x00},
			{0x32,0x05},       
			{0xfd,0x01},       
			{0x28,0x90},	     
			{0x29,0xc7},	     
			{0xfd,0x00},   	
		{0xff,0xff} 
		},  
		//CWF  //4150K
		{
			{0xfd,0x00},
			{0x32,0x05},       
			{0xfd,0x01},       
			{0x28,0x9f},	     
			{0x29,0xb8},	     
			{0xfd,0x00}, 		
		{0xff,0xff} 
		},    
		//FLUORESCENT://4200K~5000K   
		{
			{0xfd,0x00},  
			{0x32,0x05},                  
			{0xfd,0x01},                  
			{0x28,0xaf},		              
			{0x29,0x99},		              
			{0xfd,0x00},		
		    {0xff,0xff} 
		},
		//SUN: //DAYLIGHT//6500K   
		{
			{0xfd,0x00}, 
			{0x32,0x05},           
			{0xfd,0x01},           
			{0x28,0xbc},	         
			{0x29,0x5d},	         
			{0xfd,0x00}, 		
		    {0xff,0xff} 
		},
                //CLOUD: //7000K 
		{
			{0xfd,0x00}, 
			{0x32,0x05},          
			{0xfd,0x01},          
			{0x28,0xbf},	        
			{0x29,0x89},	        
			{0xfd,0x00},		
		    {0xff,0xff} 
		},	
};

LOCAL uint32_t SP0A19_set_awb(uint32_t mode)
{
	uint8_t awb_en_value;
	uint16_t  i;
	
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP0A19_awb_tab[mode];

	SP0A19_WriteReg(0xfd,0x00);
	awb_en_value = SP0A19_ReadReg(0x32);

	//SCI_ASSERT(mode < 7);
	//SCI_ASSERT(PNULL != sensor_reg_ptr);
	
	for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value); i++)
	{
	       if(0x32 == sensor_reg_ptr[i].reg_addr)
       	{
			if(mode == 0)
		   		SP0A19_WriteReg(0x32, awb_en_value |0x10 );
			else
               	SP0A19_WriteReg(0x32, awb_en_value &0xef );
       	}
      else
       {
           SP0A19_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	   }
	}
	Delayms(100); 
	return 0;
}

LOCAL uint32_t SP0A19_set_ae_enable(uint32_t enable)
{
   #define AE_ENABLE  (0x32)
    unsigned char ae_value;
		SP0A19_WriteReg(0xfd,0x00);
        ae_value=SP0A19_ReadReg(AE_ENABLE);

        if(0x00==enable)
        {
            ae_value &= 0xfa;
            SP0A19_WriteReg(AE_ENABLE,ae_value);
        }
        else if(0x01==enable)
        {
            ae_value|=0x05;
            SP0A19_WriteReg(AE_ENABLE,ae_value);
        }
	return 0;
}

/******************************************************************************/
// Description: anti 50/60 hz banding flicker
// Global resource dependence: 
// Author:
// Note:
//		
/******************************************************************************/
LOCAL uint32_t SP0A19_set_anti_flicker(uint32_t mode)
{ 
     switch(mode)
    {
        case DCAMERA_FLICKER_50HZ:			
		Antiflicker = DCAMERA_FLICKER_50HZ;
		//SENSOR_PRINTK( " SP0A19_set_anti_flicker  50hz \r\n" );
		break;
            
        case DCAMERA_FLICKER_60HZ:
		Antiflicker = DCAMERA_FLICKER_60HZ;
		//SENSOR_PRINTK( " SP0A19_set_anti_flicker  60hz \r\n" );
		break;
            
        default:
            break;
    }
    
    Delayms(100); 
    return 0;
}

#if 0
/******************************************************************************/
// Description: set video mode
// Global resource dependence: 
// Author:
// Note:
//		 
/******************************************************************************/
LOCAL SENSOR_REG_T SP0A19_video_mode_nand_tab[][9]=
{
#if 1
    {		
		{0xfd, 0x00},{0xfd, 0x00},{0xfd, 0x00},{0xfd, 0x00},{0xfd, 0x00},{0xfd, 0x00},{0xfd, 0x00},{0xfd, 0x00},{0xff,0xff},
	},

#else
    {
        {0x01,0x6a},{0x02,0x32},{0x0f,0x20},{0xe2,0x00},{0xe3,0x96},{0xe8,0x04},{0xe9,0x1a}, {0xec,0x20},{0xff, 0xff} 
    },    
    //vodeo mode
    {
        {0x01,0x6a},{0x02,0x70},{0x0f,0x00},{0xe2,0x00},{0xe3,0x96},{0xe8,0x02},{0xe9,0x58}, {0xec,0x20},{0xff, 0xff}      
    }
#endif
};
#endif

/******************************************************************************/
// Description: set video mode
// Global resource dependence: 
// Author:
// Note:
//		 
/******************************************************************************/
LOCAL SENSOR_REG_T SP0A19_video_mode_nor_tab_50hz[][20]=
{
          //Video preview
           {
			//capture preview daylight 24M 50hz 20-8FPS maxgain:0x70   
			  {0xfd,0x00},
			  {0x03,0x01},
			  {0x04,0x32},
			  {0x06,0x00},
			  {0x09,0x01},
			  {0x0a,0x46},
			  {0xf0,0x66},
			  {0xf1,0x00},
			  {0xfd,0x01},
			  {0x90,0x0c},
			  {0x92,0x01},
			  {0x98,0x66},
			  {0x99,0x00},
			  {0x9a,0x01},
			  {0x9b,0x00},
			  {0xfd,0x01},
			  {0xce,0xc8},
			  {0xcf,0x04},
			  {0xd0,0xc8},
			  {0xd1,0x04},
			  {0xfd,0x00},                   
			  {0xFF,0xFF},	//êy×é?áê?±ê??	
               },    
//Video record
{
			//Video record daylight 24M 50hz 14-14FPS maxgain:0x80
			  {0xfd,0x00},
			  {0x03,0x00},
			  {0x04,0xd8},
			  {0x06,0x00},
			  {0x09,0x03},
			  {0x0a,0x31},
			  {0xf0,0x48},
			  {0xf1,0x00},
			  {0xfd,0x01},
			  {0x90,0x07},
			  {0x92,0x01},
			  {0x98,0x48},
			  {0x99,0x00},
			  {0x9a,0x01},
			  {0x9b,0x00},
			  {0xfd,0x01},
			  {0xce,0xf8},
			  {0xcf,0x01},
			  {0xd0,0xf8},
			  {0xd1,0x01},
			  {0xfd,0x00},             
			  {0xFF,0xFF},	//êy×é?áê?±ê??		
},
// UPCC  mode
{
#if 0
			//Video record daylight 24M 50hz 14-14FPS maxgain:0x80
			  {0xfd,0x00},
			  {0x03,0x00},
			  {0x04,0xd8},
			  {0x06,0x00},
			  {0x09,0x03},
			  {0x0a,0x31},
			  {0xf0,0x48},
			  {0xf1,0x00},
			  {0xfd,0x01},
			  {0x90,0x07},
			  {0x92,0x01},
			  {0x98,0x48},
			  {0x99,0x00},
			  {0x9a,0x01},
			  {0x9b,0x00},
			  {0xfd,0x01},
			  {0xce,0xf8},
			  {0xcf,0x01},
			  {0xd0,0xf8},
			  {0xd1,0x01},
			  {0xfd,0x00},             
                       {0xFF,0xFF},	//êy×é?áê?±ê??	
#else
			//capture preview daylight 24M 50hz 20-8FPS maxgain:0x70   
			  {0xfd,0x00},
			  {0x03,0x01},
			  {0x04,0x32},
			  {0x06,0x00},
			  {0x09,0x01},
			  {0x0a,0x46},
			  {0xf0,0x66},
			  {0xf1,0x00},
			  {0xfd,0x01},
			  {0x90,0x0c},
			  {0x92,0x01},
			  {0x98,0x66},
			  {0x99,0x00},
			  {0x9a,0x01},
			  {0x9b,0x00},
			  {0xfd,0x01},
			  {0xce,0xc8},
			  {0xcf,0x04},
			  {0xd0,0xc8},
			  {0xd1,0x04},
			  {0xfd,0x00},                   
			  {0xFF,0xFF},	//êy×é?áê?±ê??	

#endif
}  
};    


LOCAL SENSOR_REG_T SP0A19_video_mode_nor_tab_60hz[][25]=
{
//Video preview
{
			//capture preview daylight 24M 60Hz 20-8FPS maxgain:0x70   
			  {0xfd,0x00},
			  {0x03,0x00},
			  {0x04,0xff},
			  {0x06,0x00},
			  {0x09,0x01},
			  {0x0a,0x46},
			  {0xf0,0x55},
			  {0xf1,0x00},
			  {0xfd,0x01},
			  {0x90,0x0f},
			  {0x92,0x01},
			  {0x98,0x55},
			  {0x99,0x00},
			  {0x9a,0x01},
			  {0x9b,0x00},
			  {0xfd,0x01},
			  {0xce,0xfb},
			  {0xcf,0x04},
			  {0xd0,0xfb},
			  {0xd1,0x04},
			  {0xfd,0x00},
                       {0xFF,0xFF},	//êy×é?áê?±ê??
},    
//Video record
{
			//Video record daylight 24M 60Hz 14-14FPS maxgain:0x80
			  {0xfd,0x00},
			  {0x03,0x00},
			  {0x04,0xb4},
			  {0x06,0x00},
			  {0x09,0x03},
			  {0x0a,0x31},
			  {0xf0,0x3c},
			  {0xf1,0x00},
			  {0xfd,0x01},
			  {0x90,0x08},
			  {0x92,0x01},
			  {0x98,0x3c},
			  {0x99,0x00},
			  {0x9a,0x01},
			  {0x9b,0x00},
			  {0xfd,0x01},
			  {0xce,0xe0},
			  {0xcf,0x01},
			  {0xd0,0xe0},
			  {0xd1,0x01},
			  {0xfd,0x00},
                       {0xFF,0xFF},	//êy×é?áê?±ê??	       
},
// UPCC  mode
{	
#if 0
			//Video record daylight 24M 60Hz 14-14FPS maxgain:0x80
			  {0xfd,0x00},
			  {0x03,0x00},
			  {0x04,0xb4},
			  {0x06,0x00},
			  {0x09,0x03},
			  {0x0a,0x31},
			  {0xf0,0x3c},
			  {0xf1,0x00},
			  {0xfd,0x01},
			  {0x90,0x08},
			  {0x92,0x01},
			  {0x98,0x3c},
			  {0x99,0x00},
			  {0x9a,0x01},
			  {0x9b,0x00},
			  {0xfd,0x01},
			  {0xce,0xe0},
			  {0xcf,0x01},
			  {0xd0,0xe0},
			  {0xd1,0x01},
			  {0xfd,0x00},
                       {0xFF,0xFF},	//êy×é?áê?±ê??	     
#else
			//capture preview daylight 24M 60Hz 20-8FPS maxgain:0x70   
			  {0xfd,0x00},
			  {0x03,0x00},
			  {0x04,0xff},
			  {0x06,0x00},
			  {0x09,0x01},
			  {0x0a,0x46},
			  {0xf0,0x55},
			  {0xf1,0x00},
			  {0xfd,0x01},
			  {0x90,0x0f},
			  {0x92,0x01},
			  {0x98,0x55},
			  {0x99,0x00},
			  {0x9a,0x01},
			  {0x9b,0x00},
			  {0xfd,0x01},
			  {0xce,0xfb},
			  {0xcf,0x04},
			  {0xd0,0xfb},
			  {0xd1,0x04},
			  {0xfd,0x00},
                       {0xFF,0xFF},	//êy×é?áê?±ê??
#endif
}  
};


LOCAL uint32_t SP0A19_set_video_mode(uint32_t mode)
{
    //uint8_t data=0x00;
    uint16_t  i;
    SENSOR_REG_T* sensor_reg_ptr = PNULL;
    //uint8_t tempregval = 0;
    //SENSOR_PRINTK(" xg:SP0A19_set_video_mode ,%d,%d\r\n",mode,Antiflicker);
    // SCI_ASSERT(mode <=DCAMERA_MODE_MAX);   
    
    if(Antiflicker ==DCAMERA_FLICKER_50HZ )
    {
    
    sensor_reg_ptr = (SENSOR_REG_T*)SP0A19_video_mode_nor_tab_50hz[mode];
    } 
    else
    {
    
    sensor_reg_ptr = (SENSOR_REG_T*)SP0A19_video_mode_nor_tab_60hz[mode];
    
    }
    
   // SCI_ASSERT(PNULL != sensor_reg_ptr);

    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value); i++)
    {
    	SP0A19_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    }
    return 0;
}

LOCAL SENSOR_REG_T SP0A19_ev_tab[][10]=
{   
    {
		{0xfd,0x00},   
		{0xed,Pre_Value_P0_0xf7-0x18+0x04},			
		{0xf7,Pre_Value_P0_0xf7-0x18},//level -3
		{0xf8,Pre_Value_P0_0xf8-0x18},	
		{0xec,Pre_Value_P0_0xf8-0x18-0x04},		
		{0xef,Pre_Value_P0_0xf9-0x18+0x04},	
		{0xf9,Pre_Value_P0_0xf9-0x18},
		{0xfa,Pre_Value_P0_0xfa-0x18},		
		{0xee,Pre_Value_P0_0xfa-0x18-0x04},			
		{0xff,0xff},        
	},
    {
		{0xfd,0x00},   
		{0xed,Pre_Value_P0_0xf7-0x10+0x04},			
		{0xf7,Pre_Value_P0_0xf7-0x10},//level -2
		{0xf8,Pre_Value_P0_0xf8-0x10},	
		{0xec,Pre_Value_P0_0xf8-0x10-0x04},		
		{0xef,Pre_Value_P0_0xf9-0x10+0x04},	
		{0xf9,Pre_Value_P0_0xf9-0x10},
		{0xfa,Pre_Value_P0_0xfa-0x10},		
		{0xee,Pre_Value_P0_0xfa-0x10-0x04},			
		{0xff,0xff},      
	},
    {
		{0xfd,0x00},   
		{0xed,Pre_Value_P0_0xf7-0x08+0x04},			
		{0xf7,Pre_Value_P0_0xf7-0x08},//level -1
		{0xf8,Pre_Value_P0_0xf8-0x08},	
		{0xec,Pre_Value_P0_0xf8-0x08-0x04},		
		{0xef,Pre_Value_P0_0xf9-0x08+0x04},	
		{0xf9,Pre_Value_P0_0xf9-0x08},
		{0xfa,Pre_Value_P0_0xfa-0x08},		
		{0xee,Pre_Value_P0_0xfa-0x08-0x04},			
		{0xff,0xff},    
	},
    {
		{0xfd,0x00},   
		{0xed,Pre_Value_P0_0xf7+4},			
		{0xf7,Pre_Value_P0_0xf7},//level 0
		{0xf8,Pre_Value_P0_0xf8},	
		{0xec,Pre_Value_P0_0xf8-4},		
		{0xef,Pre_Value_P0_0xf9+4},	
		{0xf9,Pre_Value_P0_0xf9},
		{0xfa,Pre_Value_P0_0xfa},		
		{0xee,Pre_Value_P0_0xfa-4},			
		{0xff,0xff},    
	},
    {
		{0xfd,0x00},   
		{0xed,Pre_Value_P0_0xf7+0x08+0x04},			
		{0xf7,Pre_Value_P0_0xf7+0x08},//level -1
		{0xf8,Pre_Value_P0_0xf8+0x08},	
		{0xec,Pre_Value_P0_0xf8+0x08-0x04},		
		{0xef,Pre_Value_P0_0xf9+0x08+0x04},	
		{0xf9,Pre_Value_P0_0xf9+0x08},
		{0xfa,Pre_Value_P0_0xfa+0x08},		
		{0xee,Pre_Value_P0_0xfa+0x08-0x04},			
		{0xff,0xff},    
	},
    {
		{0xfd,0x00},   
		{0xed,Pre_Value_P0_0xf7+0x10+0x04},			
		{0xf7,Pre_Value_P0_0xf7+0x10},//level -2
		{0xf8,Pre_Value_P0_0xf8+0x10},	
		{0xec,Pre_Value_P0_0xf8+0x10-0x04},		
		{0xef,Pre_Value_P0_0xf9+0x10+0x04},	
		{0xf9,Pre_Value_P0_0xf9+0x10},
		{0xfa,Pre_Value_P0_0xfa+0x10},		
		{0xee,Pre_Value_P0_0xfa+0x10-0x04},			
		{0xff,0xff},       
	},
    {
		{0xfd,0x00},   
		{0xed,Pre_Value_P0_0xf7+0x18+0x04},			
		{0xf7,Pre_Value_P0_0xf7+0x18},//level -3
		{0xf8,Pre_Value_P0_0xf8+0x18},	
		{0xec,Pre_Value_P0_0xf8+0x18-0x04},		
		{0xef,Pre_Value_P0_0xf9+0x18+0x04},	
		{0xf9,Pre_Value_P0_0xf9+0x18},
		{0xfa,Pre_Value_P0_0xfa+0x18},		
		{0xee,Pre_Value_P0_0xfa+0x18-0x04},			
		{0xff,0xff},    
	},
};

LOCAL uint32_t SP0A19_set_ev(uint32_t level)
{
    uint16_t  i; 
    SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP0A19_ev_tab[level];

  //  SCI_ASSERT(PNULL != sensor_reg_ptr);
  //  SCI_ASSERT(level < 7);
 
    for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) ||(0xFF != sensor_reg_ptr[i].reg_value) ; i++)
    {
        SP0A19_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
    }
    return 0;
}

LOCAL SENSOR_REG_TAB_INFO_T s_SP0A19_resolution_Tab_YUV[]=
{   
	// COMMON INIT
	{ ADDR_AND_LEN_OF_ARRAY(SP0A19_YUV_640X480),    640,    480, 24,  SENSOR_IMAGE_FORMAT_YUV422 },

	// YUV422 PREVIEW 1
	{ PNULL,                    0,   640,    480,    24   ,        SENSOR_IMAGE_FORMAT_YUV422 },
	{ PNULL,                    0,      0,  0  ,        0,        0      },   
	{ PNULL,                    0,      0,  0  ,        0,        0      },   
	{ PNULL,                    0,      0,  0  ,        0,        0      },   


	// YUV422 PREVIEW 2 
	{ PNULL,                    0,      0,  0  ,        0,        0      },   
	{ PNULL,                    0,      0,  0  ,        0,        0      },   
	{ PNULL,                    0,      0,  0  ,        0,        0      },   
	{ PNULL,                    0,      0,  0  ,        0,        0      }

};

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_SP0A19_ioctl_func_tab = 
{
    // Internal 
    NULL, //reset,
    SP0A19_Power_On, //power,
    NULL, //enter_sleep,
    SP0A19_Identify,
    NULL, //write_reg,			// write register
    NULL, //read_reg,			// read  register	
    NULL, //cus_func_1,
    NULL, //get_trim,
    SP0A19_set_ae_enable,
    SP0A19_set_hmirror_enable,
    SP0A19_set_vmirror_enable,
    SP0A19_set_brightness,
    SP0A19_set_contrast,
    NULL, //set_sharpness,
    NULL, //set_saturation,
    SP0A19_set_preview_mode,
    SP0A19_set_image_effect,
	SP0A19_before_snapshot,
    SP0A19_after_snapshot,
    SP0A19_flash,
    NULL, //read_ae_value,
    NULL, //write_ae_value,
    NULL, //read_gain_value,
    NULL, //write_gain_value,
    NULL, //read_gain_scale,
    NULL, //set_frame_rate,
    NULL, //af_enable,
    NULL, //af_get_status,
    SP0A19_set_awb,
    NULL, //get_skip_frame,
    NULL, //set_iso,
    SP0A19_set_ev,
    NULL, //check_image_format_support,
    NULL, //change_image_format,
    NULL, //set_zoom,
    NULL, //get_exif,
    NULL, //set_focus,
    SP0A19_set_anti_flicker,
    SP0A19_set_video_mode,
    NULL, //pick_jpeg_stream,
    NULL, //set_meter_mode,
};


SENSOR_INFO_T g_SP0A19_yuv_info =
{
	SP0A19_I2C_ADDR_W,				// salve i2c write address
	SP0A19_I2C_ADDR_R, 				// salve i2c read address
	
	0,								// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
									// bit2: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
									// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N|\
	SENSOR_HW_SIGNAL_VSYNC_P|\
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
	
	SENSOR_LOW_PULSE_RESET,		// reset pulse level
	100,								// reset pulse width(ms)
	
	SENSOR_HIGH_LEVEL_PWDN,			// 1: high level valid; 0: low level valid
	2,
	{{0x02, 0xa6},
	{0x02, 0xa6}},
	
	SENSOR_AVDD_2800MV,				// voltage of avdd	
	
	640,							// max width of source image
	480,							// max height of source image
	"SP0A19",						// name of sensor												

	SENSOR_IMAGE_FORMAT_YUV422,		// define in SENSOR_IMAGE_FORMAT_E enum,
									// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T
	SENSOR_IMAGE_PATTERN_YUV422_YUYV,	// pattern of input image form sensor;			

	s_SP0A19_resolution_Tab_YUV,	// point to resolution table information structure
	&s_SP0A19_ioctl_func_tab,		// point to ioctl function table
			
	PNULL,							// information and table about Rawrgb sensor
	PNULL,							// extend information about sensor	
	SENSOR_AVDD_2800MV,                     // iovdd
	SENSOR_AVDD_2800MV,                      // dvdd   //SENSOR_AVDD_1800MV
	3,								//skip frame num before preview
	3,								//skip frame num before capture
	0,			//skip frame num during preview
	2			//skip frame num during video preview
};


LOCAL uint32_t SP0A19_Power_On(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E	dvdd_val=g_SP0A19_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E	avdd_val=g_SP0A19_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E	iovdd_val=g_SP0A19_yuv_info.iovdd_val;  
	BOOLEAN				power_down=g_SP0A19_yuv_info.power_down_level;	    
	BOOLEAN				reset_level=g_SP0A19_yuv_info.reset_pulse_level;
	uint32_t			reset_width=g_SP0A19_yuv_info.reset_pulse_width;	    

	if(SENSOR_TRUE==power_on)
	{
		Sensor_PowerDown(power_down);	
		// Open power
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		//Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		SENSOR_Sleep(50);
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

	return SENSOR_SUCCESS;
}
#if 0
LOCAL uint32_t SP0A19_Power_On(uint32_t power_on)
{
	return 0;
}
#endif

LOCAL uint32_t SP0A19_set_hmirror_enable(uint32_t enable)
{
	return 0;
}
LOCAL uint32_t SP0A19_set_vmirror_enable(uint32_t enable)
{
	return 0;
}

struct sensor_drv_cfg sensor_sp0a19 = {                                                                                                                         
    .sensor_pos = CONFIG_DCAM_SENSOR_POS_SP0A19,                                                                                                                
    .sensor_name = "sp0a19",                                                                                                                                    
    .driver_info = &g_SP0A19_yuv_info,                                                                                                                          
};                                                                                                                                                              
                                                                                                                                                                
static int __init sensor_gc0329_init(void)                                                                                                                      
{                                                                                                                                                               
    printk("in sensor_sp0a19_init \n");                                                                                                                         
    return dcam_register_sensor_drv(&sensor_sp0a19);                                                                                                            
}                                                                                                                                                               
                                                                                                                                                                
subsys_initcall(sensor_gc0329_init); 


