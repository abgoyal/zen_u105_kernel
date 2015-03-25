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

#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/skbuff.h>
#include <linux/wlan_plat.h>
#include <mach/board.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>

#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/mmc/host.h>

#define GET_WIFI_MAC_ADDR_FROM_NV_ITEM	        1

#define PREALLOC_WLAN_NUMBER_OF_SECTIONS	4
#define PREALLOC_WLAN_NUMBER_OF_BUFFERS		160
#define PREALLOC_WLAN_SECTION_HEADER		24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 1024)

#define WLAN_SKB_BUF_NUM	16
#define IFHWADDRLEN        	6

#define CUSTOMER_MAC_FILE "/data/wifimac.txt"

extern void * dhd_os_open_image(char *filename);
extern int  dhd_os_get_image_block(char *buf, int len, void *image);
extern void dhd_os_close_image(void *image);


static int wlan_device_cd = 0; /* WIFI virtual 'card detect' status */
static int wlan_device_power_state;
static int wlan_device_reset_state;
static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;
static struct regulator *wlan_regulator_18 = NULL;

int wlan_device_power(int on);
int wlan_device_reset(int on);
int wlan_device_set_carddetect(int on);

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

typedef struct wifi_mem_prealloc_struct {
	void *mem_ptr;
	unsigned long size;
} wifi_mem_prealloc_t;

static wifi_mem_prealloc_t wifi_mem_array[PREALLOC_WLAN_NUMBER_OF_SECTIONS] = {
	{ NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER) }
};

static void *wlan_device_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_NUMBER_OF_SECTIONS)
		return wlan_static_skb;
	if ((section < 0) || (section > PREALLOC_WLAN_NUMBER_OF_SECTIONS))
		return NULL;
	if (wifi_mem_array[section].size < size)
		return NULL;
	return wifi_mem_array[section].mem_ptr;
}

int __init init_wifi_mem(void)
{
	int i;

	for(i=0;( i < WLAN_SKB_BUF_NUM );i++) {
		if (i < (WLAN_SKB_BUF_NUM/2))
			wlan_static_skb[i] = dev_alloc_skb(4096);
		else
			wlan_static_skb[i] = dev_alloc_skb(8192);
	}
	for(i=0;( i < PREALLOC_WLAN_NUMBER_OF_SECTIONS );i++) {
		wifi_mem_array[i].mem_ptr = kmalloc(wifi_mem_array[i].size,
				GFP_KERNEL);
		if (wifi_mem_array[i].mem_ptr == NULL)
			return -ENOMEM;
	}
	return 0;
}

/* Control the BT_VDDIO and WLAN_VDDIO
Always power on  According to spec
*/
static int wlan_ldo_enable(void)
{
	int err;
	wlan_regulator_18 = regulator_get(NULL, "vddsd1");

	if (IS_ERR(wlan_regulator_18)) {
		pr_err("can't get wlan 1.8V regulator\n");
		return -1;
	}

	err = regulator_set_voltage(wlan_regulator_18,1800000,1800000);
	if (err){
		pr_err("can't set wlan to 1.8V.\n");
		return -1;
	}
	regulator_enable(wlan_regulator_18);
}

static void wlan_clk_init(void)
{
	struct clk *wlan_clk;
	struct clk *clk_parent;
	wlan_clk = clk_get(NULL, "clk_aux0");
	if (IS_ERR(wlan_clk)) {
		printk("clock: failed to get clk_aux0\n");
	}

	clk_parent = clk_get(NULL, "ext_32k");
	if (IS_ERR(clk_parent)) {
		printk("failed to get parent ext_32k\n");
	}

	clk_set_parent(wlan_clk, clk_parent);
	clk_set_rate(wlan_clk, 32000);
	clk_enable(wlan_clk);
}

int wlan_device_power(int on)
{
	pr_info("%s:%d \n", __func__, on);

	if(on) {

		gpio_direction_output(GPIO_WIFI_SHUTDOWN, 0);
		mdelay(10);
		gpio_direction_output(GPIO_WIFI_SHUTDOWN, 1);
		mdelay(200);
	}
	else {
	/* disale SDIO clock */
	#ifdef CONFIG_WLAN_SDIO
		sdhci_device_attach(0);
	#endif
		gpio_direction_output(GPIO_WIFI_SHUTDOWN, 0);

	}

	wlan_device_power_state = on;
	return 0;
}

int wlan_device_reset(int on)
{
	pr_info("%s: do nothing\n", __func__);
	wlan_device_reset_state = on;
	return 0;
}


int wlan_device_status_register(
		void (*callback)(int card_present, void *dev_id),
		void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

EXPORT_SYMBOL(wlan_device_status_register);

static unsigned int wlan_device_status(struct device *dev)
{
	return wlan_device_cd;
}

int wlan_device_set_carddetect(int val)
{
	pr_info("%s: %d\n", __func__, val);
	mdelay(100);
#if 0
	wlan_device_cd = val;
	if (wifi_status_cb) {
		wifi_status_cb(val, wifi_status_cb_devid);
	} else
		pr_warning("%s: Nobody to notify\n", __func__);
#endif

#ifdef CONFIG_WLAN_SDIO
    /*    sdhci_bus_scan(); depend on MMC Auto rescan */
#endif
	return 0;
}



#ifdef GET_WIFI_MAC_ADDR_FROM_NV_ITEM
static unsigned char wlan_mac_addr[IFHWADDRLEN] = { 0x11,0x22,0x33,0x44,0x55,0x66 };

static unsigned char char2bin( char m)
{
	if (( m >= 'a') && (m <= 'f')) {
		return m - 'a' + 10;
	}
	if (( m >= 'A') && (m <= 'F')) {
		return m - 'A' + 10;
	}
	if (( m >= '0') && (m <= '9')) {
		return m - '0';
	}
}

static int wlan_device_get_mac_addr(unsigned char *buf)
{
	int rc = 0;

	char macaddr[20];
	int mac_len = 0;
	void *fp;

	if (!buf){
		pr_err("%s, null parameter !!\n", __func__);
		return -EFAULT;
	}
	fp = dhd_os_open_image(CUSTOMER_MAC_FILE);
	if (fp == NULL)
		return -EFAULT;
	mac_len = dhd_os_get_image_block(macaddr, 17, fp);
	if (mac_len < 17)
		return -EFAULT;


	wlan_mac_addr[0] = (unsigned char)((char2bin(macaddr[0]) << 4) | char2bin(macaddr[1]));
	wlan_mac_addr[1] = (unsigned char)((char2bin(macaddr[3]) << 4) | char2bin(macaddr[4]));
	wlan_mac_addr[2] = (unsigned char)((char2bin(macaddr[6]) << 4) | char2bin(macaddr[7]));
	wlan_mac_addr[3] = (unsigned char)((char2bin(macaddr[9]) << 4) | char2bin(macaddr[10]));
	wlan_mac_addr[4] = (unsigned char)((char2bin(macaddr[12]) << 4) | char2bin(macaddr[13]));
	wlan_mac_addr[5] = (unsigned char)((char2bin(macaddr[15]) << 4) | char2bin(macaddr[16]));

	memcpy(buf, wlan_mac_addr, IFHWADDRLEN);
	pr_info("wifi mac: %x:%x:%x:%x:%x:%x\n", wlan_mac_addr[0], wlan_mac_addr[1], wlan_mac_addr[2], wlan_mac_addr[3], wlan_mac_addr[4], wlan_mac_addr[5]);

	dhd_os_close_image(fp);

	return 0;

}
#endif

static struct wifi_platform_data wlan_device_control = {
	.set_power      = wlan_device_power,
	.set_reset      = wlan_device_reset,
	.set_carddetect = wlan_device_set_carddetect,
	.mem_prealloc	= wlan_device_mem_prealloc,
#ifdef GET_WIFI_MAC_ADDR_FROM_NV_ITEM
	.get_mac_addr	= wlan_device_get_mac_addr,
#endif
};

static struct resource wlan_resources[] = {
#ifdef CONFIG_WLAN_SDIO
	[0] = {
		.start  = SPRD_SDIO1_PHYS,
		.end    = SPRD_SDIO1_PHYS + SZ_4K - 1,
		.flags  = IORESOURCE_MEM,
	},
#else
	[0] = {
		.start  = SPRD_SPI0_PHYS,
		.end    = SPRD_SPI0_PHYS + SZ_4K - 1,
		.flags  = IORESOURCE_MEM,
	},
#endif
	[1] = {
		.name = "bcmdhd_wlan_irq",
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
	},
};

static struct platform_device sprd_wlan_device = {
	.name   = "bcmdhd_wlan",
	.id     = 1,
	.dev    = {
		.platform_data = &wlan_device_control,
	},
	.resource	= wlan_resources,
	.num_resources	= ARRAY_SIZE(wlan_resources),
};

static int __init wlan_device_init(void)
{
	int ret;

	init_wifi_mem();
	wlan_ldo_enable();
	wlan_clk_init();
	gpio_request(GPIO_WIFI_IRQ, "oob_irq");
	gpio_direction_input(GPIO_WIFI_IRQ);

	wlan_resources[1].start = gpio_to_irq(GPIO_WIFI_IRQ);
	wlan_resources[1].end = gpio_to_irq(GPIO_WIFI_IRQ);

	gpio_request(GPIO_WIFI_SHUTDOWN,"wifi_pwd");
	gpio_direction_output(GPIO_WIFI_SHUTDOWN, 0);

	ret = platform_device_register(&sprd_wlan_device);

	return ret;
}

late_initcall(wlan_device_init);

MODULE_DESCRIPTION("Broadcomm wlan driver");
MODULE_LICENSE("GPL");

