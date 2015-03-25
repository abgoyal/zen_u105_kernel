#ifndef _TROUT_FM_RF_55_H_
#define _TROUT_FM_RF_55_H_

#define SYS_REG_PAD_MIN    		0
#define SYS_REG_PAD_IISDO      	0
#define SYS_REG_PAD_IISCLK     	1
#define SYS_REG_PAD_IISLRCK   	2
#define SYS_REG_PAD_PTEST      	3
#define SYS_REG_PAD_GINT        4
#define SYS_REG_PAD_XTLEN     	5
#define SYS_REG_PAD_RESETN    	6
#define SYS_REG_PAD_GPIO        7
#define SYS_REG_PAD_U0CTS     	8
#define SYS_REG_PAD_U0RTS     	9
#define SYS_REG_PAD_U0RXD     	0xA
#define SYS_REG_PAD_U0TXD     	0xB
#define SYS_REG_PAD_CLK_32K  	0xC
#define SYS_REG_PAD_SD_CLK    	0xD
#define SYS_REG_PAD_SD_CMD   	0xE
#define SYS_REG_PAD_SD_D0 	    0xF
#define SYS_REG_PAD_SD_D1 	    0x10
#define SYS_REG_PAD_SD_D2 	    0x11
#define SYS_REG_PAD_SD_D3 	    0x12
#define SYS_REG_PAD_IISDI       0x13
#define SYS_REG_PAD_MAX     			SYS_REG_PAD_IISDI

#define	SYS_REG_FUNC_LOWSPEED 			0x40
#define	SYS_REG_CLK_CTRL0				0x43
#define	SYS_REG_CLK_CTRL1				0x44

#define	WIFI_SLEEP_CLK_SEL              BIT_16
#define	SPI_XTL_AUTOPD_EN              	BIT_13
#define	SPI_XTL_FORCE_ON    			BIT_12
#define	SPI_FORCE_SLEEP					BIT_11
#define	SPI_FM_SLEEP                    BIT_10

#define	SYS_REG_POW_CTRL				0x45

#define	PMU_PD_INT                      BIT_0
#define	LDO_ANA_PD                     	BIT_4
#define	PMU_PD_INT_AUTO                	BIT_8
#define	LDO_ANA_PD_AUTO                	BIT_10

#define	SYS_REG_GPIO_IN					0x46
#define	SYS_REG_GPIO_OUT				0x47
#define	SYS_REG_GPIO_OE					0x48
#define	SYS_REG_GPIO_INEN				0x49
#define	SYS_REG_TEST_SEL				0x4D
#define	SYS_REG_WIFI_CFG				0x4E
#define	SYS_REG_ADDA_FORCE				0x50
#define	SYS_REG_WIFI_SOFT_RST			0x51
#define	SYS_REG_BIST_CFG				0x52
#define	SYS_REG_DACCONST_CFG			0x53
#define	SYS_REG_UPSAMPLE_CFG			0x54
#define	SYS_REG_IQ_SWAP					0x55
#define	SYS_REG_ROSYS_REG_BIST_RESULT0	0x56
#define	SYS_REG_ROSYS_REG_BIST_RESULT1	0x57
#define	SYS_REG_ANT_MODE				0x58
#define	SYS_REG_HOST2ARSYS_REG_INFO0	0x5A
#define	SYS_REG_HOST2ARSYS_REG_INFO1	0x5B
#define	SYS_REG_HOST2ARSYS_REG_INFO2	0x5C
#define	SYS_REG_HOST2ARSYS_REG_INFO3	0x5D
#define	SYS_REG_INFO0_FROM_ARM			0x5E
#define	SYS_REG_INFO1_FROM_ARM			0x5F
#define	SYS_REG_INFO2_FROM_ARM			0x60
#define	SYS_REG_INFO3_FROM_ARM			0x61
#define	SYS_REG_WIFIAD_CFG				0x63
#define	SYS_REG_BTAD_CFG				0x65

/*
#define	SYS_REG_TROUT_FAIL_H1			0x67
#define	SYS_REG_TROUT_FAIL_H1			0x68
#define	SYS_REG_TROUT_FAIL_H1			0x69
*/
#define	SYS_REG_TROUT_FAIL_H1			0x6A

#define	SYS_REG_BWDAC_CFG0				0x6E
#define	SYS_REG_BWDAC_CFG1				0x6F
#define	SYS_REG_BWDAC_CFG2				0x70
#define	SYS_REG_BWDAC_STS0				0x71
#define	SYS_REG_BWDAC_STS1				0x72

#define	SYS_REG_ADDA_TEST_CFG			0xC7
#define	SYS_REG_FPGA_DEBUG_CFG			0xC8

#define MAX_FM_FREQ	1080
#define MIN_FM_FREQ	875   //for jni
#endif
