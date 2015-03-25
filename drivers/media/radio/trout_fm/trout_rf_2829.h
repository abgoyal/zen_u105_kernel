#ifndef _TROUT_FM_RF_2829_H_
#define _TROUT_FM_RF_2829_H_
typedef enum
{
	RF_SWITCH_K2_1,
	RF_SWITCH_K2_2,
	RF_SWITCH_K2_3,
	RF_SWITCH_K2_4,
	RF_SWITCH_K2_5,
	RF_SWITCH_K2_6,
	RF_SWITCH_K2_7,
	RF_SWITCH_K2_8,
	RF_SWITCH_K4_1,
}RF_SWITCH_NAME_E;

typedef enum
{
	RF_SWITCH_MODE_DIS,
	RF_SWITCH_MODE_EN,
}RF_SWITCH_MODE_E;

#define SYS_REG_PAD_MIN       0
#define SYS_REG_PAD_SD_CLK 	  0
#define SYS_REG_PAD_SD_CMD    1
#define SYS_REG_PAD_SD_D0 	  2
#define SYS_REG_PAD_SD_D1 	  3
#define SYS_REG_PAD_SD_D2 	  4
#define SYS_REG_PAD_SD_D3 	  5
#define SYS_REG_PAD_IISDI     6
#define SYS_REG_PAD_IISDO     7
#define SYS_REG_PAD_IISCLK    8
#define SYS_REG_PAD_IISLRCK   9
#define SYS_REG_PAD_U0TXD     0xA
#define SYS_REG_PAD_U0RXD     0xB
#define SYS_REG_PAD_U0CTS     0xC
#define SYS_REG_PAD_U0RTS     0xD
#define SYS_REG_PAD_U1TXD     0xE
#define SYS_REG_PAD_U1RXD     0xF
#define SYS_REG_PAD_TINT      0x10
#define SYS_REG_PAD_CLK_32K   0x11
#define SYS_REG_PAD_ANTSEL0   0x12
#define SYS_REG_PAD_ANTSEL1   0x13
#define SYS_REG_PAD_ANTSEL2   0x14
#define SYS_REG_PAD_PTEST     0x15
#define SYS_REG_PAD_XTLEN     0x16
#define SYS_REG_PAD_RESETN    0x17
#define SYS_REG_PAD_JTAGTCK   0x18
#define SYS_REG_PAD_JTAGTMS   0x19
#define SYS_REG_PAD_JTAGTDI   0x1A
#define SYS_REG_PAD_JTAGTRSTN 0x1B
#define SYS_REG_PAD_JTAGTDO   0x1C

#define SYS_REG_PAD_MAX     SYS_REG_PAD_JTAGTDO

#define	SYS_REG_TEST_ANA_MODE			0x1D
#define	SYS_REG_MPLL_CTRL				0x1E
#define	SPI_MPLL_EN                     BIT_19
#define	SYS_REG_SPLL_CTRL				0x1F
#define	SPI_SPLL_EN                     BIT_19
#define	SYS_REG_CLK_CTRL0				0x20
#define	SYS_REG_CLK_CTRL1				0x21
#define	WIFI_SLEEP_CLK_SEL              BIT_16
#define	SPI_XTL_AUTOPD_EN               BIT_13
#define	SPI_XTL_FORCE_ON    			BIT_12
#define	SPI_FORCE_SLEEP					BIT_11
#define	SPI_FM_SLEEP                    BIT_10
#define	SYS_REG_POW_CTRL				0x22
#define	PMU_PD_INT                      BIT_0
#define	LDO_RF_PD                       BIT_1
#define	LDO_ANA_PD                      BIT_4
#define	PMU_PD_INT_AUTO                 BIT_8
#define	LDO_RF_PD_AUTO                  BIT_9
#define	LDO_ANA_PD_AUTO                 BIT_10
#define	SYS_REG_GPIO_IN					0x23
#define	SYS_REG_GPIO_OUT				0x24
#define	SYS_REG_GPIO_OE					0x25
#define	SYS_REG_GPIO_INEN				0x26
#define	SYS_REG_WPDI					0x27
#define	SYS_REG_ANA_TEST_CFG			0x2B
#define	SYS_REG_PIN_IN_SEL				0x2C
#define	SYS_REG_TEST_SEL				0x43
#define	SYS_REG_WRX_CFG					0x44
#define	SYS_REG_ADDA_FORCE				0x50
#define	SYS_REG_WIFI_SOFT_RST			0x51
#define	SYS_REG_BIST_CFG				0x52
#define	SYS_REG_DACCONST_CFG			0x53
#define	SYS_REG_IQ_SWAP					0x55
#define	SYS_REG_ROSYS_REG_BIST_RESULT0	0x56
#define	SYS_REG_ROSYS_REG_BIST_RESULT1	0x57
#define	SYS_REG_ANT_MODE				0x58
#define	SYS_REG_BIAS_MODE				0x59
#define	SYS_REG_HOST2ARSYS_REG_INFO0	0x5A
#define	SYS_REG_HOST2ARSYS_REG_INFO1	0x5B
#define	SYS_REG_HOST2ARSYS_REG_INFO2	0x5C
#define	SYS_REG_HOST2ARSYS_REG_INFO3	0x5D
#define	SYS_REG_INFO0_FROM_ARM			0x5E
#define	SYS_REG_INFO1_FROM_ARM			0x5F
#define	SYS_REG_INFO2_FROM_ARM			0x60
#define	SYS_REG_INFO3_FROM_ARM			0x61
#define	SYS_REG_WIFIDA_CFG				0x62
#define	SYS_REG_WIFIAD_CFG				0x63
#define	SYS_REG_BTAD_CFG				0x64
#define	SYS_REG_LDO_CFG					0x65
#define	SYS_REG_RFMODE_CFG				0x66
#define	SYS_REG_FPGA_DEBUG_CFG			0xC8
#define	SYS_REG_HOST_WAKE_UP			0xFF

#endif
