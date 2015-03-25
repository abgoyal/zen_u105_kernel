/* File: mux_driver.c
 *
 * Portions derived from rfcomm.c, original header as follows:
 *
 * Copyright (C) 2000, 2001  Axis Communications AB
 *
 * Author: Mats Friden <mats.friden@axis.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Exceptionally, Axis Communications AB grants discretionary and
 * conditional permissions for additional use of the text contained
 * in the company's release of the AXIS OpenBT Stack under the
 * provisions set forth hereunder.
 *
 * Provided that, if you use the AXIS OpenBT Stack with other files,
 * that do not implement functionality as specified in the Bluetooth
 * System specification, to produce an executable, this does not by
 * itself cause the resulting executable to be covered by the GNU
 * General Public License. Your use of that executable is in no way
 * restricted on account of using the AXIS OpenBT Stack code with it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the provisions of the GNU
 * General Public License.
 *
 */

/*
 * Copyright (C) 2002-2004  Motorola
 * Copyright (C) 2006 Harald Welte <laforge@openezx.org>
 *  07/28/2002  Initial version
 *  11/18/2002  Second version
 *  04/21/2004  Add GPRS PROC
 *  09/28/2008  Porting to kernel 2.6.21 by Spreadtrum
*/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/major.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/sprdmux.h>

#include "ts0710.h"
#include "ts0710_mux.h"

#define TS0710MUX_GPRS_SESSION_MAX 3
#define TS0710MUX_MAJOR 250
#define TS0710MUX_MINOR_START 0
#define NR_MUXS 32

#define TS0710MUX_TIME_OUT 250	/* 2500ms, for BP UART hardware flow control AP UART  */

#define TS0710MUX_IO_DLCI_FC_ON 0x54F2
#define TS0710MUX_IO_DLCI_FC_OFF 0x54F3
#define TS0710MUX_IO_FC_ON 0x54F4
#define TS0710MUX_IO_FC_OFF 0x54F5

#define TS0710MUX_MAX_BUF_SIZE		(64*1024)
#define TS0710MUX_MAX_TOTAL_SIZE	(1024*1024)

#define TS0710MUX_SEND_BUF_OFFSET 10
#define TS0710MUX_SEND_BUF_SIZE (DEF_TS0710_MTU + TS0710MUX_SEND_BUF_OFFSET + 34)
#define TS0710MUX_RECV_BUF_SIZE TS0710MUX_SEND_BUF_SIZE

/*For BP UART problem Begin*/
#ifdef TS0710SEQ2
#define ACK_SPACE 0		/* 6 * 11(ACK frame size)  */
#else
#define ACK_SPACE 0		/* 6 * 7(ACK frame size)  */
#endif
/*For BP UART problem End*/

#define TS0710MUX_SERIAL_BUF_SIZE (DEF_TS0710_MTU + TS0710_MAX_HDR_SIZE + ACK_SPACE)	/* For BP UART problem: ACK_SPACE  */

#define TS0710MUX_MAX_TOTAL_FRAME_SIZE (DEF_TS0710_MTU + TS0710_MAX_HDR_SIZE + FLAG_SIZE)
#define TS0710MUX_MAX_CHARS_IN_BUF 65535
#define TS0710MUX_THROTTLE_THRESHOLD DEF_TS0710_MTU

#define TEST_PATTERN_SIZE 250

#define CMDTAG 0x55
#define DATATAG 0xAA

#define ACK 0x4F		/*For BP UART problem */

/*For BP UART problem Begin*/
#ifdef TS0710SEQ2
#define FIRST_BP_SEQ_OFFSET 0	/*offset from start flag */
#define SECOND_BP_SEQ_OFFSET 0	/*offset from start flag */
#define FIRST_AP_SEQ_OFFSET 0	/*offset from start flag */
#define SECOND_AP_SEQ_OFFSET 0	/*offset from start flag */
#define SLIDE_BP_SEQ_OFFSET 0	/*offset from start flag */
#define SEQ_FIELD_SIZE 0
#else
#define SLIDE_BP_SEQ_OFFSET  1	/*offset from start flag */
#define SEQ_FIELD_SIZE    0
#endif

#define ADDRESS_FIELD_OFFSET (1 + SEQ_FIELD_SIZE)	/*offset from start flag */
/*For BP UART problem End*/

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(v) (void)(v)
#endif

#define TS0710MUX_GPRS1_DLCI  3
#define TS0710MUX_GPRS2_DLCI  4
#define TS0710MUX_VT_DLCI         2

#define TS0710MUX_GPRS1_RECV_COUNT_IDX 0
#define TS0710MUX_GPRS1_SEND_COUNT_IDX 1
#define TS0710MUX_GPRS2_RECV_COUNT_IDX 2
#define TS0710MUX_GPRS2_SEND_COUNT_IDX 3
#define TS0710MUX_VT_RECV_COUNT_IDX 4
#define TS0710MUX_VT_SEND_COUNT_IDX 5

#define TS0710MUX_COUNT_MAX_IDX        5
#define TS0710MUX_COUNT_IDX_NUM (TS0710MUX_COUNT_MAX_IDX + 1)

static __u8 tty2dlci[NR_MUXS] =
    { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 ,14, 15, 16,
		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
typedef struct {
	__u8 cmdtty;
	__u8 datatty;
} dlci_tty;

static dlci_tty dlci2tty[] = {
	{0, 0},			/* DLCI 0 */
	{0, 0},			/* DLCI 1 */
	{1, 1},			/* DLCI 2 */
	{2, 2},			/* DLCI 3 */
	{3, 3},			/* DLCI 4 */
	{4, 4},			/* DLCI 5 */
	{5, 5},			/* DLCI 6 */
	{6, 6},			/* DLCI 7 */
	{7, 7},			/* DLCI 8 */
	{8, 8},			/* DLCI 9 */
	{9, 9},			/* DLCI 10 */
	{10, 10},		/* DLCI 11 */
	{11, 11},		/* DLCI 12 */
	{12, 12},		/* DLCI 13 */
	{13, 13},		/* DLCI 14 */
	{14, 14},		/* DLCI 15 */
	{15, 15},		/* DLCI 16 */
	{16, 16},		/* DLCI 17 */
	{17, 17},		/* DLCI 18 */
	{18, 18},		/* DLCI 19 */
	{19, 19},		/* DLCI 20 */
	{20, 20},		/* DLCI 21 */
	{21, 21},		/* DLCI 22 */
	{22, 22},		/* DLCI 23 */
	{23, 23},		/* DLCI 24 */
	{24, 24},		/* DLCI 25 */
	{25, 25},		/* DLCI 26 */
	{26, 26},		/* DLCI 27 */
	{27, 27},		/* DLCI 28 */
	{28, 28},		/* DLCI 29 */
	{29, 29},		/* DLCI 30 */
	{30, 30},		/* DLCI 31 */
	{31, 31},		/* DLCI 32 */
};

typedef struct {
	volatile __u8 buf[TS0710MUX_SEND_BUF_SIZE];
	volatile __u8 *frame;
	unsigned long flags;
	volatile __u16 length;
	volatile __u8 filled;
	volatile __u8 dummy;	/* Allignment to 4*n bytes */
} mux_send_struct;

/* Bit number in flags of mux_send_struct */
#define BUF_BUSY 0

struct mux_recv_packet_tag {
	__u8 *data;
	__u32 length;
	struct mux_recv_packet_tag *next;
};
typedef struct mux_recv_packet_tag mux_recv_packet;

struct mux_recv_struct_tag {
	__u8 data[TS0710MUX_RECV_BUF_SIZE];
	__u32 length;
	__u32 total;
	mux_recv_packet *mux_packet;
	struct mux_recv_struct_tag *next;
	int no_tty;
	volatile __u8 post_unthrottle;
	struct mutex recv_lock;
};
typedef struct mux_recv_struct_tag mux_recv_struct;

#define RECV_RUNNING 0
static unsigned long mux_recv_flags = 0;
int mux_exiting = 0;
static mux_send_struct *mux_send_info[NR_MUXS];
static volatile __u8 mux_send_info_flags[NR_MUXS];
static volatile __u8 mux_send_info_idx = NR_MUXS;

static mux_recv_struct *mux_recv_info[NR_MUXS];
static volatile __u8 mux_recv_info_flags[NR_MUXS];

static struct tty_driver mux_driver;

#define SPRDMUX_MAX	2
static struct sprdmux iomux[SPRDMUX_MAX];

int sprdmux_register(struct sprdmux *mux)
{
	iomux[mux->id].id = mux->id;
	iomux[mux->id].io_read = mux->io_read;
	iomux[mux->id].io_write = mux->io_write;
	iomux[mux->id].io_stop = mux->io_stop;

	return 0;
}

static struct completion send_completion;
static struct completion post_recv_completion;

static struct task_struct *mux_recv_kthread = NULL;
static struct task_struct *mux_send_kthread = NULL;
static struct task_struct *mux_post_recv_kthread = NULL;

static int mux_receive_thread(void *data);
static void receive_worker(int start);

static struct tty_struct **mux_table;
static volatile short int mux_tty[NR_MUXS];

static int mux_mode = 0;

#ifdef min
#undef min
#define min(a,b)    ( (a)<(b) ? (a):(b) )
#endif

#ifdef CONFIG_SPRD_SETH
#define TTY_INDEX_BASE	13
extern void seth_rx_data(int tty_index, __u8 *uih_data_start, __u32 uih_len);
extern void seth_restart_queue(int index);
static int is_seth_ttyidx(int tty_idx)
{
	switch(tty_idx) {
		case TTY_INDEX_BASE:
		case TTY_INDEX_BASE + 1:
			return 1;
		default:
			return 0;
	}
}
static int mux_write(struct tty_struct *tty,
		     const unsigned char *buf, int count);
int seth_mux_write(struct tty_struct *tty,
		     const unsigned char *buf, int count) {
	return mux_write(tty, buf, count);
}
#endif

static int send_ua(ts0710_con * ts0710, __u8 dlci);
static int send_dm(ts0710_con * ts0710, __u8 dlci);
static int send_sabm(ts0710_con * ts0710, __u8 dlci);
#if 0				/*Close here just for rmove building warnings */
static int send_disc(ts0710_con * ts0710, __u8 dlci);
#endif
static void queue_uih(mux_send_struct * send_info, __u16 len,
		      ts0710_con * ts0710, __u8 dlci);
static int send_pn_msg(ts0710_con * ts0710, __u8 prior, __u32 frame_size,
		       __u8 credit_flow, __u8 credits, __u8 dlci, __u8 cr);
static int send_nsc_msg(ts0710_con * ts0710, mcc_type cmd, __u8 cr);
static void set_uih_hdr(short_frame * uih_pkt, __u8 dlci, __u32 len, __u8 cr);

static __u32 crc_check(__u8 * data, __u32 length, __u8 check_sum);
static __u8 crc_calc(__u8 * data, __u32 length);
static void create_crctable(__u8 table[]);

static void mux_sched_send(void);

static __u8 crctable[256];

static ts0710_con ts0710_connection;
/*
static rpn_values rpn_val;
*/

static int valid_dlci(__u8 dlci)
{
	if ((dlci < TS0710_MAX_CHN) && (dlci > 0))
		return 1;
	else
		return 0;
}

//#define TS0710DEBUG

#define PRINT_OUTPUT_PRINTK

//#define TS0710LOG
#ifdef TS0710DEBUG

#ifdef PRINT_OUTPUT_PRINTK
#define TS0710_DEBUG(fmt, arg...) printk(KERN_INFO "MUX:%s "fmt, __FUNCTION__ , ## arg)
#else
#include "ezxlog.h"
static __u8 strDebug[256];
#define TS0710_DEBUG(fmt, arg...) ({ snprintf(strDebug, sizeof(strDebug), "MUX " __FUNCTION__ ": " fmt "\n" , ## arg); \
                                     ezxlogk("MX", strDebug, strlen(strDebug)); })
#endif /* End #ifdef PRINT_OUTPUT_PRINTK */

#else
#define TS0710_DEBUG(fmt...)
#endif /* End #ifdef TS0710DEBUG */

#ifdef TS0710LOG
static unsigned char g_tbuf[TS0710MUX_MAX_BUF_SIZE];
#ifdef PRINT_OUTPUT_PRINTK
#define TS0710_LOG(fmt, arg...) printk(fmt, ## arg)
#define TS0710_PRINTK(fmt, arg...) printk(fmt, ## arg)
#else
#include "ezxlog.h"
static __u8 strLog[256];
#define TS0710_LOG(fmt, arg...) ({ snprintf(strLog, sizeof(strLog), fmt, ## arg); \
                                     ezxlogk("MX", strLog, strlen(strLog)); })
#define TS0710_PRINTK(fmt, arg...) ({ printk(fmt, ## arg); \
                                      TS0710_LOG(fmt, ## arg); })
#endif /* End #ifdef PRINT_OUTPUT_PRINTK */

#else
#define TS0710_LOG(fmt...)
#define TS0710_PRINTK(fmt, arg...) printk(fmt, ## arg)
#endif /* End #ifdef TS0710LOG */

#ifdef TS0710DEBUG
static void TS0710_DEBUGHEX(__u8 * buf, int len)
{
	static unsigned char tbuf[TS0710MUX_MAX_BUF_SIZE];

	int i;
	int c;

	if (len <= 0) {
		return;
	}

	c = 0;
	for (i = 0; (i < len) && (c < (TS0710MUX_MAX_BUF_SIZE - 3)); i++) {
		sprintf(&tbuf[c], "%02x ", buf[i]);
		c += 3;
	}
	tbuf[c] = 0;

#ifdef PRINT_OUTPUT_PRINTK
	TS0710_DEBUG("%s", tbuf);
#else
	ezxlogk("MX", tbuf, c);
#endif
}

static void TS0710_DEBUGSTR(__u8 * buf, int len)
{
	static unsigned char tbuf[TS0710MUX_MAX_BUF_SIZE];

	if (len <= 0) {
		return;
	}

	if (len > (TS0710MUX_MAX_BUF_SIZE - 1)) {
		len = (TS0710MUX_MAX_BUF_SIZE - 1);
	}

	memcpy(tbuf, buf, len);
	tbuf[len] = 0;

#ifdef PRINT_OUTPUT_PRINTK
	/* 0x00 byte in the string pointed by tbuf may truncate the print result */
	TS0710_DEBUG("%s", tbuf);
#else
	ezxlogk("MX", tbuf, len);
#endif
}
#else
#define TS0710_DEBUGHEX(buf, len)
#define TS0710_DEBUGSTR(buf, len)
#endif /* End #ifdef TS0710DEBUG */

#ifdef TS0710LOG
static void TS0710_LOGSTR_FRAME(__u8 send, __u8 * data, int len)
{
	short_frame *short_pkt;
	long_frame *long_pkt;
	__u8 *uih_data_start;
	__u32 uih_len;
	__u8 dlci;
	int pos;

	if (len <= 0) {
		return;
	}

	pos = 0;
	if (send) {
		pos += sprintf(&g_tbuf[pos], "<");
		short_pkt = (short_frame *) (data + 1);
	} else {
		pos +=
		    sprintf(&g_tbuf[pos], ">%d ",
			    *(data + SLIDE_BP_SEQ_OFFSET));

#ifdef TS0710SEQ2
		pos +=
		    sprintf(&g_tbuf[pos], "%02x %02x %02x %02x ",
			    *(data + FIRST_BP_SEQ_OFFSET),
			    *(data + SECOND_BP_SEQ_OFFSET),
			    *(data + FIRST_AP_SEQ_OFFSET),
			    *(data + SECOND_AP_SEQ_OFFSET));
#endif

		short_pkt = (short_frame *) (data + ADDRESS_FIELD_OFFSET);
	}

	dlci = short_pkt->h.addr.server_chn << 1 | short_pkt->h.addr.d;
	switch (CLR_PF(short_pkt->h.control)) {
	case SABM:
		pos += sprintf(&g_tbuf[pos], "C SABM %d ::", dlci);
		break;
	case UA:
		pos += sprintf(&g_tbuf[pos], "C UA %d ::", dlci);
		break;
	case DM:
		pos += sprintf(&g_tbuf[pos], "C DM %d ::", dlci);
		break;
	case DISC:
		pos += sprintf(&g_tbuf[pos], "C DISC %d ::", dlci);
		break;

	case ACK:
		pos += sprintf(&g_tbuf[pos], "C ACK %d ", short_pkt->data[0]);

#ifdef TS0710SEQ2
		pos +=
		    sprintf(&g_tbuf[pos], "%02x %02x %02x %02x ",
			    short_pkt->data[1], short_pkt->data[2],
			    short_pkt->data[3], short_pkt->data[4]);
#endif

		pos += sprintf(&g_tbuf[pos], "::");
		break;

	case UIH:
		if (!dlci) {
			pos += sprintf(&g_tbuf[pos], "C MCC %d ::", dlci);
		} else {

			if ((short_pkt->h.length.ea) == 0) {
				long_pkt = (long_frame *) short_pkt;
				uih_len = GET_LONG_LENGTH(long_pkt->h.length);
				uih_data_start = long_pkt->h.data;
			} else {
				uih_len = short_pkt->h.length.len;
				uih_data_start = short_pkt->data;
			}
			switch (0) {
			case CMDTAG:
				pos +=
				    sprintf(&g_tbuf[pos], "I %d A %d ::", dlci,
					    uih_len);
				break;
			case DATATAG:
			default:
				pos +=
				    sprintf(&g_tbuf[pos], "I %d D %d ::", dlci,
					    uih_len);
				break;
			}

		}
		break;
	default:
		pos += sprintf(&g_tbuf[pos], "N!!! %d ::", dlci);
		break;
	}

	if (len > (sizeof(g_tbuf) - pos - 1)) {
		len = (sizeof(g_tbuf) - pos - 1);
	}

	memcpy(&g_tbuf[pos], data, len);
	pos += len;
	g_tbuf[pos] = 0;

#ifdef PRINT_OUTPUT_PRINTK
	/* 0x00 byte in the string pointed by g_tbuf may truncate the print result */
	TS0710_LOG("%s\n", g_tbuf);
#else
	ezxlogk("MX", g_tbuf, pos);
#endif
}
#else
#define TS0710_LOGSTR_FRAME(send, data, len)
#endif

#ifdef TS0710SIG
#define my_for_each_task(p) \
        for ((p) = current; ((p) = (p)->next_task) != current; )

static void TS0710_SIG2APLOGD(void)
{
	struct task_struct *p;
	static __u8 sig = 0;

	if (sig) {
		return;
	}

	read_lock(&tasklist_lock);
	my_for_each_task(p) {
		if (strncmp(p->comm, "aplogd", 6) == 0) {
			sig = 1;
			if (send_sig(SIGUSR2, p, 1) == 0) {
				TS0710_PRINTK
				    ("MUX: success to send SIGUSR2 to aplogd!\n");
			} else {
				TS0710_PRINTK
				    ("MUX: failure to send SIGUSR2 to aplogd!\n");
			}
			break;
		}
	}
	read_unlock(&tasklist_lock);

	if (!sig) {
		TS0710_PRINTK("MUX: not found aplogd!\n");
	}
}
#else
#define TS0710_SIG2APLOGD()
#endif

static int basic_write(ts0710_con * ts0710, __u8 * buf, int len)
{
	int res, send;
	UNUSED_PARAM(ts0710);

	buf[0] = TS0710_BASIC_FLAG;
	buf[len + 1] = TS0710_BASIC_FLAG;

	TS0710_LOGSTR_FRAME(1, buf, len + 2);
	TS0710_DEBUGHEX(buf, len + 2);
	send = 0;

	while (send < len + 2) {
		res = iomux[0].io_write(buf + send, len + 2 - send);
		if (res < 0)
			return -EIO;
		else if (res == 0)
			msleep(2);
		else
			send = send + res;
	}
	return len + 2;
}

/* Functions for the crc-check and calculation */

#define CRC_VALID 0xcf

static __u32 crc_check(__u8 * data, __u32 length, __u8 check_sum)
{
	__u8 fcs = 0xff;
#ifdef TS0710DEBUG
	__u32 len = length;
#endif
	length = length - 1;
	while (length--) {
		fcs = crctable[fcs ^ *data++];
	}
	fcs = crctable[fcs ^ check_sum];
	TS0710_DEBUG("fcs : %d\n", fcs);
	TS0710_DEBUG(" crc_check :len:%d check_sum:%d fcs : %d\n", len,
		     check_sum, fcs);
	if (fcs == (uint) 0xcf) {	/*CRC_VALID) */
		TS0710_DEBUG("crc_check: CRC check OK\n");
		return 0;
	} else {
		TS0710_PRINTK("MUX crc_check: CRC check failed\n");
		return 1;
	}
}

/* Calculates the checksum according to the ts0710 specification */

static __u8 crc_calc(__u8 * data, __u32 length)
{
	__u8 fcs = 0xff;

	while (length--) {
		fcs = crctable[fcs ^ *data++];
	}

	return 0xff - fcs;
}

/* Calulates a reversed CRC table for the FCS check */

static void create_crctable(__u8 table[])
{
	int i, j;

	__u8 data;
	__u8 code_word = (__u8) 0xe0;
	__u8 sr = (__u8) 0;

	for (j = 0; j < 256; j++) {
		data = (__u8) j;

		for (i = 0; i < 8; i++) {
			if ((data & 0x1) ^ (sr & 0x1)) {
				sr >>= 1;
				sr ^= code_word;
			} else {
				sr >>= 1;
			}

			data >>= 1;
			sr &= 0xff;
		}

		table[j] = sr;
		sr = 0;
	}
}

static void ts0710_reset_dlci(__u8 j)
{
	if (j >= TS0710_MAX_CHN)
		return;

	ts0710_connection.dlci[j].state = DISCONNECTED;
	ts0710_connection.dlci[j].flow_control = 0;
	ts0710_connection.dlci[j].mtu = DEF_TS0710_MTU;
	ts0710_connection.dlci[j].initiated = 0;
	ts0710_connection.dlci[j].initiator = 0;
	init_waitqueue_head(&ts0710_connection.dlci[j].open_wait);
	init_waitqueue_head(&ts0710_connection.dlci[j].close_wait);
}

static void ts0710_reset_con(void)
{
	__u8 j;

	ts0710_connection.initiator = 0;
	ts0710_connection.mtu = DEF_TS0710_MTU + TS0710_MAX_HDR_SIZE;
	ts0710_connection.be_testing = 0;
	ts0710_connection.test_errs = 0;
	init_waitqueue_head(&ts0710_connection.test_wait);

	for (j = 0; j < TS0710_MAX_CHN; j++) {
		ts0710_reset_dlci(j);
	}
}

static void ts0710_init(void)
{
	create_crctable(crctable);

	ts0710_reset_con();
}

static void ts0710_upon_disconnect(void)
{
	ts0710_con *ts0710 = &ts0710_connection;
	__u8 j;

	for (j = 0; j < TS0710_MAX_CHN; j++) {
		ts0710->dlci[j].state = DISCONNECTED;
		wake_up_interruptible(&ts0710->dlci[j].open_wait);
		wake_up_interruptible(&ts0710->dlci[j].close_wait);
	}
	ts0710->be_testing = 0;
	wake_up_interruptible(&ts0710->test_wait);
	ts0710_reset_con();
}

/* Sending packet functions */

/* Creates a UA packet and puts it at the beginning of the pkt pointer */

static int send_ua(ts0710_con * ts0710, __u8 dlci)
{
	__u8 buf[sizeof(short_frame) + FCS_SIZE + FLAG_SIZE];
	short_frame *ua;

	TS0710_DEBUG("send_ua: Creating UA packet to DLCI %d\n", dlci);

	ua = (short_frame *) (buf + 1);
	ua->h.addr.ea = 1;
	ua->h.addr.cr = ((~(ts0710->initiator)) & 0x1);
	ua->h.addr.d = (dlci) & 0x1;
	ua->h.addr.server_chn = (dlci) >> 0x1;
	ua->h.control = SET_PF(UA);
	ua->h.length.ea = 1;
	ua->h.length.len = 0;
	ua->data[0] = crc_calc((__u8 *) ua, SHORT_CRC_CHECK);

	return basic_write(ts0710, buf, sizeof(short_frame) + FCS_SIZE);
}

/* Creates a DM packet and puts it at the beginning of the pkt pointer */

static int send_dm(ts0710_con * ts0710, __u8 dlci)
{
	__u8 buf[sizeof(short_frame) + FCS_SIZE + FLAG_SIZE];
	short_frame *dm;

	TS0710_DEBUG("send_dm: Creating DM packet to DLCI %d\n", dlci);

	dm = (short_frame *) (buf + 1);
	dm->h.addr.ea = 1;
	dm->h.addr.cr = ((~(ts0710->initiator)) & 0x1);
	dm->h.addr.d = dlci & 0x1;
	dm->h.addr.server_chn = dlci >> 0x1;
	dm->h.control = SET_PF(DM);
	dm->h.length.ea = 1;
	dm->h.length.len = 0;
	dm->data[0] = crc_calc((__u8 *) dm, SHORT_CRC_CHECK);

	return basic_write(ts0710, buf, sizeof(short_frame) + FCS_SIZE);
}

static int send_sabm(ts0710_con * ts0710, __u8 dlci)
{
	__u8 buf[sizeof(short_frame) + FCS_SIZE + FLAG_SIZE];
	short_frame *sabm;

	TS0710_DEBUG("send_sabm: Creating SABM packet to DLCI %d\n", dlci);

	sabm = (short_frame *) (buf + 1);
	sabm->h.addr.ea = 1;
	sabm->h.addr.cr = ((ts0710->initiator) & 0x1);
	sabm->h.addr.d = dlci & 0x1;
	sabm->h.addr.server_chn = dlci >> 0x1;
	sabm->h.control = SET_PF(SABM);
	sabm->h.length.ea = 1;
	sabm->h.length.len = 0;
	sabm->data[0] = crc_calc((__u8 *) sabm, SHORT_CRC_CHECK);

	return basic_write(ts0710, buf, sizeof(short_frame) + FCS_SIZE);
}

#if 0				/*close here just for remove building warnings */
static int send_disc(ts0710_con * ts0710, __u8 dlci)
{
	__u8 buf[sizeof(short_frame) + FCS_SIZE + FLAG_SIZE];
	short_frame *disc;

	TS0710_DEBUG("send_disc: Creating DISC packet to DLCI %d\n", dlci);

	disc = (short_frame *) (buf + 1);
	disc->h.addr.ea = 1;
	disc->h.addr.cr = ((ts0710->initiator) & 0x1);
	disc->h.addr.d = dlci & 0x1;
	disc->h.addr.server_chn = dlci >> 0x1;
	disc->h.control = SET_PF(DISC);
	disc->h.length.ea = 1;
	disc->h.length.len = 0;
	disc->data[0] = crc_calc((__u8 *) disc, SHORT_CRC_CHECK);

	return basic_write(ts0710, buf, sizeof(short_frame) + FCS_SIZE);
}
#endif
static void queue_uih(mux_send_struct * send_info, __u16 len,
		      ts0710_con * ts0710, __u8 dlci)
{
	__u32 size;

	TS0710_DEBUG
	    ("queue_uih: Creating UIH packet with %d bytes data to DLCI %d\n",
	     len, dlci);

	if (len > SHORT_PAYLOAD_SIZE) {
		long_frame *l_pkt;

		size = sizeof(long_frame) + len + FCS_SIZE;
		l_pkt = (long_frame *) (send_info->frame - sizeof(long_frame));
		set_uih_hdr((void *)l_pkt, dlci, len, ts0710->initiator);
		l_pkt->data[len] = crc_calc((__u8 *) l_pkt, LONG_CRC_CHECK);
		send_info->frame = ((__u8 *) l_pkt) - 1;
	} else {
		short_frame *s_pkt;

		size = sizeof(short_frame) + len + FCS_SIZE;
		s_pkt =
		    (short_frame *) (send_info->frame - sizeof(short_frame));
		set_uih_hdr((void *)s_pkt, dlci, len, ts0710->initiator);
		s_pkt->data[len] = crc_calc((__u8 *) s_pkt, SHORT_CRC_CHECK);
		send_info->frame = ((__u8 *) s_pkt) - 1;
	}
	send_info->length = size;
}

/* Multiplexer command packets functions */

/* Turns on the ts0710 flow control */

static int ts0710_fcon_msg(ts0710_con * ts0710, __u8 cr)
{
	__u8 buf[30];
	mcc_short_frame *mcc_pkt;
	short_frame *uih_pkt;
	__u32 size;

	size = sizeof(short_frame) + sizeof(mcc_short_frame) + FCS_SIZE;
	uih_pkt = (short_frame *) (buf + 1);
	set_uih_hdr(uih_pkt, CTRL_CHAN, sizeof(mcc_short_frame),
		    ts0710->initiator);
	uih_pkt->data[sizeof(mcc_short_frame)] =
	    crc_calc((__u8 *) uih_pkt, SHORT_CRC_CHECK);
	mcc_pkt = (mcc_short_frame *) (uih_pkt->data);

	mcc_pkt->h.type.ea = EA;
	mcc_pkt->h.type.cr = cr;
	mcc_pkt->h.type.type = FCON;
	mcc_pkt->h.length.ea = EA;
	mcc_pkt->h.length.len = 0;

	return basic_write(ts0710, buf, size);
}

/* Turns off the ts0710 flow control */

static int ts0710_fcoff_msg(ts0710_con * ts0710, __u8 cr)
{
	__u8 buf[30];
	mcc_short_frame *mcc_pkt;
	short_frame *uih_pkt;
	__u32 size;

	size = (sizeof(short_frame) + sizeof(mcc_short_frame) + FCS_SIZE);
	uih_pkt = (short_frame *) (buf + 1);
	set_uih_hdr(uih_pkt, CTRL_CHAN, sizeof(mcc_short_frame),
		    ts0710->initiator);
	uih_pkt->data[sizeof(mcc_short_frame)] =
	    crc_calc((__u8 *) uih_pkt, SHORT_CRC_CHECK);
	mcc_pkt = (mcc_short_frame *) (uih_pkt->data);

	mcc_pkt->h.type.ea = 1;
	mcc_pkt->h.type.cr = cr;
	mcc_pkt->h.type.type = FCOFF;
	mcc_pkt->h.length.ea = 1;
	mcc_pkt->h.length.len = 0;

	return basic_write(ts0710, buf, size);
}

/* Sends an PN-messages and sets the not negotiable parameters to their
   default values in ts0710 */

static int send_pn_msg(ts0710_con * ts0710, __u8 prior, __u32 frame_size,
		       __u8 credit_flow, __u8 credits, __u8 dlci, __u8 cr)
{
	__u8 buf[30];
	pn_msg *pn_pkt;
	__u32 size;
	TS0710_DEBUG
	    ("send_pn_msg: DLCI 0x%02x, prior:0x%02x, frame_size:%d, credit_flow:%x, credits:%d, cr:%x\n",
	     dlci, prior, frame_size, credit_flow, credits, cr);

	size = sizeof(pn_msg);
	pn_pkt = (pn_msg *) (buf + 1);

	set_uih_hdr((void *)pn_pkt, CTRL_CHAN,
		    size - (sizeof(short_frame) + FCS_SIZE), ts0710->initiator);
	pn_pkt->fcs = crc_calc((__u8 *) pn_pkt, SHORT_CRC_CHECK);

	pn_pkt->mcc_s_head.type.ea = 1;
	pn_pkt->mcc_s_head.type.cr = cr;
	pn_pkt->mcc_s_head.type.type = PN;
	pn_pkt->mcc_s_head.length.ea = 1;
	pn_pkt->mcc_s_head.length.len = 8;

	pn_pkt->res1 = 0;
	pn_pkt->res2 = 0;
	pn_pkt->dlci = dlci;
	pn_pkt->frame_type = 0;
	pn_pkt->credit_flow = credit_flow;
	pn_pkt->prior = prior;
	pn_pkt->ack_timer = 0;
	SET_PN_MSG_FRAME_SIZE(pn_pkt, frame_size);
	pn_pkt->credits = credits;
	pn_pkt->max_nbrof_retrans = 0;

	return basic_write(ts0710, buf, size);
}

/* Send a Not supported command - command, which needs 3 bytes */

static int send_nsc_msg(ts0710_con * ts0710, mcc_type cmd, __u8 cr)
{
	__u8 buf[30];
	nsc_msg *nsc_pkt;
	__u32 size;

	size = sizeof(nsc_msg);
	nsc_pkt = (nsc_msg *) (buf + 1);

	set_uih_hdr((void *)nsc_pkt, CTRL_CHAN,
		    sizeof(nsc_msg) - sizeof(short_frame) - FCS_SIZE,
		    ts0710->initiator);

	nsc_pkt->fcs = crc_calc((__u8 *) nsc_pkt, SHORT_CRC_CHECK);

	nsc_pkt->mcc_s_head.type.ea = 1;
	nsc_pkt->mcc_s_head.type.cr = cr;
	nsc_pkt->mcc_s_head.type.type = NSC;
	nsc_pkt->mcc_s_head.length.ea = 1;
	nsc_pkt->mcc_s_head.length.len = 1;

	nsc_pkt->command_type.ea = 1;
	nsc_pkt->command_type.cr = cmd.cr;
	nsc_pkt->command_type.type = cmd.type;

	return basic_write(ts0710, buf, size);
}

static int ts0710_msc_msg(ts0710_con * ts0710, __u8 value, __u8 cr, __u8 dlci)
{
	__u8 buf[30];
	msc_msg *msc_pkt;
	__u32 size;

	size = sizeof(msc_msg);
	msc_pkt = (msc_msg *) (buf + 1);

	set_uih_hdr((void *)msc_pkt, CTRL_CHAN,
		    sizeof(msc_msg) - sizeof(short_frame) - FCS_SIZE,
		    ts0710->initiator);

	msc_pkt->fcs = crc_calc((__u8 *) msc_pkt, SHORT_CRC_CHECK);

	msc_pkt->mcc_s_head.type.ea = 1;
	msc_pkt->mcc_s_head.type.cr = cr;
	msc_pkt->mcc_s_head.type.type = MSC;
	msc_pkt->mcc_s_head.length.ea = 1;
	msc_pkt->mcc_s_head.length.len = 2;

	msc_pkt->dlci.ea = 1;
	msc_pkt->dlci.cr = 1;
	msc_pkt->dlci.d = dlci & 1;
	msc_pkt->dlci.server_chn = (dlci >> 1) & 0x1f;

	msc_pkt->v24_sigs = value;

	return basic_write(ts0710, buf, size);
}

static int ts0710_test_msg(ts0710_con * ts0710, __u8 * test_pattern, __u32 len,
			   __u8 cr, __u8 * f_buf)
{
	__u32 size;

	if (len > SHORT_PAYLOAD_SIZE) {
		long_frame *uih_pkt;
		mcc_long_frame *mcc_pkt;

		size =
		    (sizeof(long_frame) + sizeof(mcc_long_frame) + len +
		     FCS_SIZE);
		uih_pkt = (long_frame *) (f_buf + 1);

		set_uih_hdr((short_frame *) uih_pkt, CTRL_CHAN, len +
			    sizeof(mcc_long_frame), ts0710->initiator);
		uih_pkt->data[GET_LONG_LENGTH(uih_pkt->h.length)] =
		    crc_calc((__u8 *) uih_pkt, LONG_CRC_CHECK);
		mcc_pkt = (mcc_long_frame *) uih_pkt->data;

		mcc_pkt->h.type.ea = EA;
		/* cr tells whether it is a commmand (1) or a response (0) */
		mcc_pkt->h.type.cr = cr;
		mcc_pkt->h.type.type = TEST;
		SET_LONG_LENGTH(mcc_pkt->h.length, len);
		memcpy(mcc_pkt->value, test_pattern, len);
	} else if (len > (SHORT_PAYLOAD_SIZE - sizeof(mcc_short_frame))) {
		long_frame *uih_pkt;
		mcc_short_frame *mcc_pkt;

		/* Create long uih packet and short mcc packet */
		size =
		    (sizeof(long_frame) + sizeof(mcc_short_frame) + len +
		     FCS_SIZE);
		uih_pkt = (long_frame *) (f_buf + 1);

		set_uih_hdr((short_frame *) uih_pkt, CTRL_CHAN,
			    len + sizeof(mcc_short_frame), ts0710->initiator);
		uih_pkt->data[GET_LONG_LENGTH(uih_pkt->h.length)] =
		    crc_calc((__u8 *) uih_pkt, LONG_CRC_CHECK);
		mcc_pkt = (mcc_short_frame *) uih_pkt->data;

		mcc_pkt->h.type.ea = EA;
		mcc_pkt->h.type.cr = cr;
		mcc_pkt->h.type.type = TEST;
		mcc_pkt->h.length.ea = EA;
		mcc_pkt->h.length.len = len;
		memcpy(mcc_pkt->value, test_pattern, len);
	} else {
		short_frame *uih_pkt;
		mcc_short_frame *mcc_pkt;

		size =
		    (sizeof(short_frame) + sizeof(mcc_short_frame) + len +
		     FCS_SIZE);
		uih_pkt = (short_frame *) (f_buf + 1);

		set_uih_hdr((void *)uih_pkt, CTRL_CHAN, len
			    + sizeof(mcc_short_frame), ts0710->initiator);
		uih_pkt->data[uih_pkt->h.length.len] =
		    crc_calc((__u8 *) uih_pkt, SHORT_CRC_CHECK);
		mcc_pkt = (mcc_short_frame *) uih_pkt->data;

		mcc_pkt->h.type.ea = EA;
		mcc_pkt->h.type.cr = cr;
		mcc_pkt->h.type.type = TEST;
		mcc_pkt->h.length.ea = EA;
		mcc_pkt->h.length.len = len;
		memcpy(mcc_pkt->value, test_pattern, len);

	}
	return basic_write(ts0710, f_buf, size);
}

static void set_uih_hdr(short_frame * uih_pkt, __u8 dlci, __u32 len, __u8 cr)
{
	uih_pkt->h.addr.ea = 1;
	uih_pkt->h.addr.cr = cr;
	uih_pkt->h.addr.d = dlci & 0x1;
	uih_pkt->h.addr.server_chn = dlci >> 1;
	uih_pkt->h.control = CLR_PF(UIH);

	if (len > SHORT_PAYLOAD_SIZE) {
		SET_LONG_LENGTH(((long_frame *) uih_pkt)->h.length, len);
	} else {
		uih_pkt->h.length.ea = 1;
		uih_pkt->h.length.len = len;
	}
}

/* Parses a multiplexer control channel packet */

static void process_mcc(__u8 * data, __u32 len, ts0710_con * ts0710, int longpkt)
{
	__u8 *tbuf = NULL;
	mcc_short_frame *mcc_short_pkt;
	int j;

	if (longpkt) {
		mcc_short_pkt =
		    (mcc_short_frame *) (((long_frame *) data)->data);
	} else {
		mcc_short_pkt =
		    (mcc_short_frame *) (((short_frame *) data)->data);
	}

	switch (mcc_short_pkt->h.type.type) {
	case TEST:
		if (mcc_short_pkt->h.type.cr == MCC_RSP) {
			TS0710_DEBUG("Received test command response\n");

			if (ts0710->be_testing) {
				if ((mcc_short_pkt->h.length.ea) == 0) {
					mcc_long_frame *mcc_long_pkt;
					mcc_long_pkt =
					    (mcc_long_frame *) mcc_short_pkt;
					if (GET_LONG_LENGTH
					    (mcc_long_pkt->h.length) !=
					    TEST_PATTERN_SIZE) {
						ts0710->test_errs =
						    TEST_PATTERN_SIZE;
						TS0710_DEBUG
						    ("Err: received test pattern is %d bytes long, not expected %d\n",
						     GET_LONG_LENGTH
						     (mcc_long_pkt->h.length),
						     TEST_PATTERN_SIZE);
					} else {
						ts0710->test_errs = 0;
						for (j = 0;
						     j < TEST_PATTERN_SIZE;
						     j++) {
							if (mcc_long_pkt->value
							    [j] != (j & 0xFF)) {
								(ts0710->
								 test_errs)++;
							}
						}
					}

				} else {

#if TEST_PATTERN_SIZE < 128
					if (mcc_short_pkt->h.length.len !=
					    TEST_PATTERN_SIZE) {
#endif

						ts0710->test_errs =
						    TEST_PATTERN_SIZE;
						TS0710_DEBUG
						    ("Err: received test pattern is %d bytes long, not expected %d\n",
						     mcc_short_pkt->h.length.
						     len, TEST_PATTERN_SIZE);

#if TEST_PATTERN_SIZE < 128
					} else {
						ts0710->test_errs = 0;
						for (j = 0;
						     j < TEST_PATTERN_SIZE;
						     j++) {
							if (mcc_short_pkt->value
							    [j] != (j & 0xFF)) {
								(ts0710->
								 test_errs)++;
							}
						}
					}
#endif

				}

				ts0710->be_testing = 0;	/* Clear the flag */
				wake_up_interruptible(&ts0710->test_wait);
			} else {
				TS0710_DEBUG
				    ("Err: shouldn't or late to get test cmd response\n");
			}
		} else {
			tbuf = (__u8 *) kmalloc(len + 32, GFP_ATOMIC);
			if (!tbuf) {
				break;
			}

			if ((mcc_short_pkt->h.length.ea) == 0) {
				mcc_long_frame *mcc_long_pkt;
				mcc_long_pkt = (mcc_long_frame *) mcc_short_pkt;
				ts0710_test_msg(ts0710, mcc_long_pkt->value,
						GET_LONG_LENGTH(mcc_long_pkt->h.
								length),
						MCC_RSP, tbuf);
			} else {
				ts0710_test_msg(ts0710, mcc_short_pkt->value,
						mcc_short_pkt->h.length.len,
						MCC_RSP, tbuf);
			}

			kfree(tbuf);
		}
		break;

	case FCON:		/*Flow control on command */
		TS0710_PRINTK
		    ("MUX Received Flow control(all channels) on command\n");
		if (mcc_short_pkt->h.type.cr == MCC_CMD) {
			ts0710->dlci[0].state = CONNECTED;
			ts0710_fcon_msg(ts0710, MCC_RSP);
			mux_sched_send();
		}
		break;

	case FCOFF:		/*Flow control off command */
		TS0710_PRINTK
		    ("MUX Received Flow control(all channels) off command\n");
		if (mcc_short_pkt->h.type.cr == MCC_CMD) {
			for (j = 0; j < TS0710_MAX_CHN; j++) {
				ts0710->dlci[j].state = FLOW_STOPPED;
			}
			ts0710_fcoff_msg(ts0710, MCC_RSP);
		}
		break;

	case MSC:		/*Modem status command */
		{
			__u8 dlci;
			__u8 v24_sigs;

			dlci = (mcc_short_pkt->value[0]) >> 2;
			v24_sigs = mcc_short_pkt->value[1];

			if ((ts0710->dlci[dlci].state != CONNECTED)
			    && (ts0710->dlci[dlci].state != FLOW_STOPPED)) {
			        TS0710_DEBUG("Received Modem status dlci = %d\n",dlci);
				send_dm(ts0710, dlci);
				break;
			}
			if (mcc_short_pkt->h.type.cr == MCC_CMD) {
				TS0710_DEBUG("Received Modem status command\n");
				if (v24_sigs & 2) {
					if (ts0710->dlci[dlci].state ==
					    CONNECTED) {
						TS0710_LOG
						    ("MUX Received Flow off on dlci %d\n",
						     dlci);
						ts0710->dlci[dlci].state =
						    FLOW_STOPPED;
					}
				} else {
					if (ts0710->dlci[dlci].state ==
					    FLOW_STOPPED) {
						ts0710->dlci[dlci].state =
						    CONNECTED;
						TS0710_LOG
						    ("MUX Received Flow on on dlci %d\n",
						     dlci);
						mux_sched_send();
					}
				}

				ts0710_msc_msg(ts0710, v24_sigs, MCC_RSP, dlci);

			} else {
				TS0710_DEBUG
				    ("Received Modem status response\n");

				if (v24_sigs & 2) {
					TS0710_DEBUG("Flow stop accepted\n");
				}
			}
			break;
		}

	case PN:		/*DLC parameter negotiation */
		{
			__u8 dlci;
			__u16 frame_size;
			pn_msg *pn_pkt;

			pn_pkt = (pn_msg *) data;
			dlci = pn_pkt->dlci;
			frame_size = GET_PN_MSG_FRAME_SIZE(pn_pkt);
			TS0710_DEBUG
			    ("Received DLC parameter negotiation, PN\n");
			if (pn_pkt->mcc_s_head.type.cr == MCC_CMD) {
				TS0710_DEBUG("received PN command with:\n");
				TS0710_DEBUG("Frame size:%d\n", frame_size);

				frame_size =
				    min(frame_size, ts0710->dlci[dlci].mtu);
				send_pn_msg(ts0710, pn_pkt->prior, frame_size,
					    0, 0, dlci, MCC_RSP);
				ts0710->dlci[dlci].mtu = frame_size;
				TS0710_DEBUG("process_mcc : mtu set to %d\n",
					     ts0710->dlci[dlci].mtu);
			} else {
				TS0710_DEBUG("received PN response with:\n");
				TS0710_DEBUG("Frame size:%d\n", frame_size);

				frame_size =
				    min(frame_size, ts0710->dlci[dlci].mtu);
				ts0710->dlci[dlci].mtu = frame_size;

				TS0710_DEBUG
				    ("process_mcc : mtu set on dlci:%d to %d\n",
				     dlci, ts0710->dlci[dlci].mtu);

				if (ts0710->dlci[dlci].state == NEGOTIATING) {
					ts0710->dlci[dlci].state = CONNECTING;
					wake_up_interruptible(&ts0710->dlci
							      [dlci].open_wait);
				}
			}
			break;
		}

	case NSC:		/*Non supported command resonse */
		TS0710_LOG("MUX Received Non supported command response\n");
		break;

	default:		/*Non supported command received */
		TS0710_LOG("MUX Received a non supported command\n");
		send_nsc_msg(ts0710, mcc_short_pkt->h.type, MCC_RSP);
		break;
	}
}

static mux_recv_packet *get_mux_recv_packet(__u32 size)
{
	mux_recv_packet *recv_packet;

	TS0710_DEBUG("Enter into get_mux_recv_packet");

	recv_packet =
	    (mux_recv_packet *) kmalloc(sizeof(mux_recv_packet), GFP_ATOMIC);
	if (!recv_packet) {
		return 0;
	}

	recv_packet->data = (__u8 *) kmalloc(size, GFP_ATOMIC);
	if (!(recv_packet->data)) {
		kfree(recv_packet);
		return 0;
	}
	recv_packet->length = 0;
	recv_packet->next = 0;
	return recv_packet;
}

static void free_mux_recv_packet(mux_recv_packet * recv_packet)
{
	TS0710_DEBUG("Enter into free_mux_recv_packet");

	if (!recv_packet) {
		return;
	}

	if (recv_packet->data) {
		kfree(recv_packet->data);
	}
	kfree(recv_packet);
}

static void free_mux_recv_struct(mux_recv_struct * recv_info)
{
	mux_recv_packet *recv_packet1, *recv_packet2;

	if (!recv_info) {
		return;
	}

	recv_packet1 = recv_info->mux_packet;
	while (recv_packet1) {
		recv_packet2 = recv_packet1->next;
		free_mux_recv_packet(recv_packet1);
		recv_packet1 = recv_packet2;
	}

	kfree(recv_info);
}

static inline void add_post_recv_queue(mux_recv_struct ** head,
				       mux_recv_struct * new_item)
{
	new_item->next = *head;
	*head = new_item;
}

static void ts0710_flow_on(__u8 dlci, ts0710_con * ts0710)
{
	int i;
	__u8 cmdtty;
	__u8 datatty;
	struct tty_struct *tty;
	mux_recv_struct *recv_info;

	if ((ts0710->dlci[0].state != CONNECTED)
	    && (ts0710->dlci[0].state != FLOW_STOPPED)) {
		return;
	} else if ((ts0710->dlci[dlci].state != CONNECTED)
		   && (ts0710->dlci[dlci].state != FLOW_STOPPED)) {
		return;
	}

	if (!(ts0710->dlci[dlci].flow_control)) {
		return;
	}

	cmdtty = dlci2tty[dlci].cmdtty;
	datatty = dlci2tty[dlci].datatty;

	if (cmdtty != datatty) {
		/* Check AT cmd tty */
		tty = mux_table[cmdtty];
		if (mux_tty[cmdtty] && tty) {
			if (test_bit(TTY_THROTTLED, &tty->flags)) {
				return;
			}
		}
		recv_info = mux_recv_info[cmdtty];
		if (mux_recv_info_flags[cmdtty] && recv_info) {
			if (recv_info->total) {
				return;
			}
		}

		/* Check data tty */
		tty = mux_table[datatty];
		if (mux_tty[datatty] && tty) {
			if (test_bit(TTY_THROTTLED, &tty->flags)) {
				return;
			}
		}
		recv_info = mux_recv_info[datatty];
		if (mux_recv_info_flags[datatty] && recv_info) {
			if (recv_info->total) {
				return;
			}
		}
	}

	for (i = 0; i < 3; i++) {
		if (ts0710_msc_msg(ts0710, EA | RTC | RTR | DV, MCC_CMD, dlci) <
		    0) {
			continue;
		} else {
			TS0710_LOG("MUX send Flow on on dlci %d\n", dlci);
			ts0710->dlci[dlci].flow_control = 0;
			break;
		}
	}
}

static void ts0710_flow_off(struct tty_struct *tty, __u8 dlci,
			    ts0710_con * ts0710)
{
	int i;
	set_bit(TTY_THROTTLED, &tty->flags);

	if ((ts0710->dlci[0].state != CONNECTED)
	    && (ts0710->dlci[0].state != FLOW_STOPPED)) {
		return;
	} else if ((ts0710->dlci[dlci].state != CONNECTED)
		   && (ts0710->dlci[dlci].state != FLOW_STOPPED)) {
		return;
	}

	if (ts0710->dlci[dlci].flow_control) {
		return;
	}

	for (i = 0; i < 3; i++) {
		if (ts0710_msc_msg
		    (ts0710, EA | FC | RTC | RTR | DV, MCC_CMD, dlci) < 0) {
			continue;
		} else {
			TS0710_DEBUG("send Flow off on dlci %d\n", dlci);
			ts0710->dlci[dlci].flow_control = 1;
			break;
		}
	}
}

static int ts0710_recv_data(ts0710_con * ts0710, char *data, int len)
{
	short_frame *short_pkt;
	long_frame *long_pkt;
	__u8 *uih_data_start;
	__u32 uih_len;
	__u8 dlci;
	__u8 be_connecting;
#ifdef TS0710DEBUG
	unsigned long t;
#endif

	short_pkt = (short_frame *) data;

	dlci = short_pkt->h.addr.server_chn << 1 | short_pkt->h.addr.d;
	TS0710_DEBUG("MUX ts0710_recv_data dlci = %d\n",dlci);
	switch (CLR_PF(short_pkt->h.control)) {
	case SABM:
		TS0710_DEBUG("SABM-packet received\n");

		if (!dlci) {
			TS0710_DEBUG("server channel == 0\n");
			ts0710->dlci[0].state = CONNECTED;

			TS0710_DEBUG("sending back UA - control channel\n");
			send_ua(ts0710, dlci);
			wake_up_interruptible(&ts0710->dlci[0].open_wait);

		} else if (valid_dlci(dlci)) {

			TS0710_DEBUG("Incomming connect on channel %d\n", dlci);

			TS0710_DEBUG("sending UA, dlci %d\n", dlci);
			send_ua(ts0710, dlci);

			ts0710->dlci[dlci].state = CONNECTED;
			wake_up_interruptible(&ts0710->dlci[dlci].open_wait);

		} else {
			TS0710_DEBUG("invalid dlci %d, sending DM\n", dlci);
			send_dm(ts0710, dlci);
		}

		break;

	case UA:
		TS0710_DEBUG("UA packet received\n");

		if (!dlci) {
			TS0710_DEBUG("server channel == 0|dlci[0].state=%d\n",
				     ts0710->dlci[0].state);

			if (ts0710->dlci[0].state == CONNECTING) {
				ts0710->dlci[0].state = CONNECTED;
				wake_up_interruptible(&ts0710->dlci[0].
						      open_wait);
			} else if (ts0710->dlci[0].state == DISCONNECTING) {
				ts0710_upon_disconnect();
			} else {
				TS0710_DEBUG
				    (" Something wrong receiving UA packet\n");
			}
		} else if (valid_dlci(dlci)) {
			TS0710_DEBUG
			    ("Incomming UA on channel %d|dlci[%d].state=%d\n",
			     dlci, dlci, ts0710->dlci[dlci].state);

			if (ts0710->dlci[dlci].state == CONNECTING) {
				ts0710->dlci[dlci].state = CONNECTED;
				wake_up_interruptible(&ts0710->dlci[dlci].
						      open_wait);
			} else if (ts0710->dlci[dlci].state == DISCONNECTING) {
				ts0710->dlci[dlci].state = DISCONNECTED;
				wake_up_interruptible(&ts0710->dlci[dlci].
						      open_wait);
				wake_up_interruptible(&ts0710->dlci[dlci].
						      close_wait);
				ts0710_reset_dlci(dlci);
			} else {
				TS0710_DEBUG
				    (" Something wrong receiving UA packet\n");
			}
		} else {
			TS0710_DEBUG("invalid dlci %d\n", dlci);
		}

		break;

	case DM:
		TS0710_DEBUG("DM packet received\n");

		if (!dlci) {
			TS0710_DEBUG("server channel == 0\n");

			if (ts0710->dlci[0].state == CONNECTING) {
				be_connecting = 1;
			} else {
				be_connecting = 0;
			}
			ts0710_upon_disconnect();
			if (be_connecting) {
				ts0710->dlci[0].state = REJECTED;
			}
		} else if (valid_dlci(dlci)) {
			TS0710_DEBUG("Incomming DM on channel %d\n", dlci);

			if (ts0710->dlci[dlci].state == CONNECTING) {
				ts0710->dlci[dlci].state = REJECTED;
			} else {
				ts0710->dlci[dlci].state = DISCONNECTED;
			}
			wake_up_interruptible(&ts0710->dlci[dlci].open_wait);
			wake_up_interruptible(&ts0710->dlci[dlci].close_wait);
			ts0710_reset_dlci(dlci);
		} else {
			TS0710_DEBUG("invalid dlci %d\n", dlci);
		}

		break;

	case DISC:
		TS0710_DEBUG("DISC packet received\n");

		if (!dlci) {
			TS0710_DEBUG("server channel == 0\n");

			send_ua(ts0710, dlci);
			TS0710_DEBUG("DISC, sending back UA\n");

			ts0710_upon_disconnect();
		} else if (valid_dlci(dlci)) {
			TS0710_DEBUG("Incomming DISC on channel %d\n", dlci);

			send_ua(ts0710, dlci);
			TS0710_DEBUG("DISC, sending back UA\n");

			ts0710->dlci[dlci].state = DISCONNECTED;
			wake_up_interruptible(&ts0710->dlci[dlci].open_wait);
			wake_up_interruptible(&ts0710->dlci[dlci].close_wait);
			ts0710_reset_dlci(dlci);
		} else {
			TS0710_DEBUG("invalid dlci %d\n", dlci);
		}

		break;

	case UIH:
		TS0710_DEBUG("UIH packet received\n");

		if ((dlci >= TS0710_MAX_CHN)) {
			TS0710_DEBUG("invalid dlci %d\n", dlci);
			send_dm(ts0710, dlci);
			break;
		}

		if (GET_PF(short_pkt->h.control)) {
			TS0710_LOG
			    ("MUX Error %s: UIH packet with P/F set, discard it!\n",
			     __FUNCTION__);
			break;
		}

		if ((ts0710->dlci[dlci].state != CONNECTED)
		    && (ts0710->dlci[dlci].state != FLOW_STOPPED)) {
			TS0710_LOG
			    ("MUX Error %s: DLCI %d not connected, discard it!\n",
			     __FUNCTION__, dlci);
			send_dm(ts0710, dlci);
			break;
		}

		if ((short_pkt->h.length.ea) == 0) {
			TS0710_DEBUG("Long UIH packet received\n");
			long_pkt = (long_frame *) data;
			uih_len = GET_LONG_LENGTH(long_pkt->h.length);
			uih_data_start = long_pkt->h.data;
			TS0710_DEBUG("long packet length %d\n", uih_len);

		} else {
			TS0710_DEBUG("Short UIH pkt received\n");
			uih_len = short_pkt->h.length.len;
			uih_data_start = short_pkt->data;

		}

		if (dlci == 0) {
			TS0710_DEBUG("UIH on serv_channel 0\n");
			process_mcc(data, len, ts0710,
				    !(short_pkt->h.length.ea));
		} else if (valid_dlci(dlci)) {
			/* do tty dispatch */
			__u8 tty_idx;
			struct tty_struct *tty;
			__u8 queue_data;
			__u8 post_recv;
			__u8 flow_control;
			mux_recv_struct *recv_info;
			int recv_room;
			mux_recv_packet *recv_packet, *recv_packet2;
			TS0710_DEBUG("UIH on channel %d\n", dlci);

			if (uih_len > ts0710->dlci[dlci].mtu) {
				TS0710_PRINTK
				    ("MUX Error:  DLCI:%d, uih_len:%d is bigger than mtu:%d, discard data!\n",
				     dlci, uih_len, ts0710->dlci[dlci].mtu);
				break;
			}

			tty_idx = dlci2tty[dlci].datatty;	/* we see data and commmand as same */

#ifdef CONFIG_SPRD_SETH
			 if(is_seth_ttyidx(tty_idx)) {
				TS0710_DEBUG("MUX: seth_rx_data on mux%d\n", tty_idx);
				seth_rx_data(tty_idx, uih_data_start, uih_len);
				break;
			}
#endif

			tty = mux_table[tty_idx];
			if ((!mux_tty[tty_idx]) || (!tty)) {
				TS0710_PRINTK
				    ("MUX: No application waiting for, discard it! /dev/mux%d\n",
				      tty_idx);
			} else {	/* Begin processing received data */
				if ((!mux_recv_info_flags[tty_idx])
				    || (!mux_recv_info[tty_idx])) {
					TS0710_PRINTK
					    ("MUX Error: No mux_recv_info, discard it! /dev/mux%d\n",
					     tty_idx);
					break;
				}

				recv_info = mux_recv_info[tty_idx];
				if(!recv_info) {
					TS0710_DEBUG("recv_data : recv_info is null\n");
					break;
				}

				if (recv_info->total > TS0710MUX_MAX_TOTAL_SIZE) {
					TS0710_PRINTK
					    (KERN_INFO "MUX : discard data for tty_idx:%d, recv_info->total = %d!\n",
					     tty_idx, recv_info->total);
					break;
				}

				if (recv_info->total > TS0710MUX_MAX_TOTAL_SIZE/2) {
					ts0710_flow_off(tty, dlci, ts0710);
				}

				queue_data = 0;
				post_recv = 0;
				flow_control = 0;
				recv_room = 65535;
				if (tty->receive_room)
					recv_room = tty->receive_room;

				if (test_bit(TTY_THROTTLED, &tty->flags)) {
					queue_data = 1;
				} else {
					if (recv_info->total) {
						queue_data = 1;
						post_recv = 1;
					} else if (recv_room < uih_len) {
						queue_data = 1;
						//flow_control = 1;
					}

				}

				if (!queue_data) {
					/* Put received data into read buffer of tty */
					TS0710_DEBUG
					    ("Put received data into read buffer of /dev/mux%d",
					     tty_idx);

#ifdef TS0710DEBUG
					t = jiffies;
#endif

					(tty->ldisc->ops->receive_buf) (tty,
									uih_data_start,
									NULL,
									uih_len);

#ifdef TS0710DEBUG
					TS0710_DEBUG
					    ("tty->ldisc->ops->receive_buf:%x,  take ticks: %x\n",
					     tty->ldisc->ops->receive_buf,
					     (jiffies - t));
#endif

				} else {	/* Queue data */

					TS0710_DEBUG
					    ("Put received data into recv queue of /dev/mux%d",
					     tty_idx);
					mutex_lock(&recv_info->recv_lock);
					if (recv_info->total) {
						/* recv_info is already linked into mux_recv_queue */

						recv_packet =
						    get_mux_recv_packet
						    (uih_len);
						if (!recv_packet) {
							TS0710_PRINTK
							    ("MUX %s: no memory\n",
							     __FUNCTION__);
							mutex_unlock(&recv_info->recv_lock);
							break;
						}

						memcpy(recv_packet->data,
						       uih_data_start, uih_len);
						recv_packet->length = uih_len;
						recv_info->total += uih_len;
						recv_packet->next = NULL;

						if (!(recv_info->mux_packet)) {
							recv_info->mux_packet =
							    recv_packet;
						} else {
							recv_packet2 =
							    recv_info->
							    mux_packet;
							while (recv_packet2->
							       next) {
								recv_packet2 =
								    recv_packet2->
								    next;
							}
							recv_packet2->next =
							    recv_packet;
						}	/* End if( !(recv_info->mux_packet) ) */
					} else {	/* recv_info->total == 0 */
						if (uih_len >
						    TS0710MUX_RECV_BUF_SIZE) {
							TS0710_PRINTK
							    ("MUX Error:  tty_idx:%d, uih_len == %d is too big\n",
							     tty_idx, uih_len);
							uih_len =
							    TS0710MUX_RECV_BUF_SIZE;
						}
						memcpy(recv_info->data,
						       uih_data_start, uih_len);
						recv_info->length = uih_len;
						recv_info->total = uih_len;
					}	/* End recv_info->total == 0 */
					mutex_unlock(&recv_info->recv_lock);
				}	/* End Queue data */

				if (flow_control) {
					TS0710_DEBUG("recv_data : send flow off\n");
					/* Do something for flow control */
					ts0710_flow_off(tty, dlci, ts0710);
				}

				if (post_recv) {
					TS0710_DEBUG("recv_data : enter queue delay work\n");
					complete(&post_recv_completion);
				}
			}	/* End processing received data */
		} else {
			TS0710_DEBUG("invalid dlci %d\n", dlci);
		}

		break;

	default:
		TS0710_DEBUG("illegal packet\n");
		break;
	}
	return 0;
}

/* Close ts0710 channel */
static void ts0710_close_channel(__u8 dlci)
{
	ts0710_con *ts0710 = &ts0710_connection;
#if 0
	int try;
	unsigned long t;
#endif
	TS0710_DEBUG("ts0710_disc_command on channel %d\n", dlci);

	if ((ts0710->dlci[dlci].state == DISCONNECTED)
	    || (ts0710->dlci[dlci].state == REJECTED)) {
		return;
	} else {
		if (ts0710->dlci[dlci].state != DISCONNECTED) {
			if (dlci == 0) {	/* Control Channel */
				__u8 j;

				for (j = 0; j < TS0710_MAX_CHN; j++)
					ts0710->dlci[j].state = DISCONNECTED;
				ts0710->be_testing = 0;
				ts0710_reset_con();

			} else {	/* Other Channel */
				ts0710->dlci[dlci].state = DISCONNECTED;
				ts0710_reset_dlci(dlci);
			}
		}
	}
#if 0				/* If Open here, Plsease open send_disc() function first! */

	if ((ts0710->dlci[dlci].state == DISCONNECTED)
	    || (ts0710->dlci[dlci].state == REJECTED)) {
		return;
	} else if (ts0710->dlci[dlci].state == DISCONNECTING) {
		/* Reentry */
		return;
	} else {
		ts0710->dlci[dlci].state = DISCONNECTING;
		try = 3;
		while (try--) {
			t = jiffies;
			send_disc(ts0710, dlci);
			interruptible_sleep_on_timeout(&ts0710->dlci[dlci].
						       close_wait,
						       TS0710MUX_TIME_OUT);
			if (ts0710->dlci[dlci].state == DISCONNECTED) {
				break;
			} else if (signal_pending(current)) {
				TS0710_PRINTK
				    ("MUX DLCI %d Send DISC got signal!\n",
				     dlci);
				break;
			} else if ((jiffies - t) >= TS0710MUX_TIME_OUT) {
				TS0710_PRINTK
				    ("MUX DLCI %d Send DISC timeout!\n", dlci);
				continue;
			}
		}
		if (ts0710->dlci[dlci].state != DISCONNECTED) {
			if (dlci == 0) {	/* Control Channel */
				ts0710_upon_disconnect();
			} else {	/* Other Channel */
				ts0710->dlci[dlci].state = DISCONNECTED;
				wake_up_interruptible(&ts0710->dlci[dlci].
						      close_wait);
				ts0710_reset_dlci(dlci);
			}
		}
	}
#endif
}

static int ts0710_open_channel(__u8 dlci)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int try;
	int retval;
	unsigned long t;

	retval = -ENODEV;
	if (dlci == 0) {	/* control channel */
		if ((ts0710->dlci[0].state == CONNECTED)
		    || (ts0710->dlci[0].state == FLOW_STOPPED)) {
			return 0;
		} else if (ts0710->dlci[0].state == CONNECTING) {
			/* Reentry */
			TS0710_PRINTK
			    ("MUX DLCI: 0, reentry to open DLCI 0, pid: %d, %s !\n",
			     current->pid, current->comm);
			try = 11;
			while (try--) {
				t = jiffies;
				wait_event_interruptible_timeout(ts0710->dlci
								 [0].open_wait,
								 ts0710->dlci
								 [0].state !=
								 CONNECTING,
								 TS0710MUX_TIME_OUT);
				if ((ts0710->dlci[0].state == CONNECTED)
				    || (ts0710->dlci[0].state == FLOW_STOPPED)) {
					retval = 0;
					break;
				} else if (ts0710->dlci[0].state == REJECTED) {
					retval = -EREJECTED;
					break;
				} else if (ts0710->dlci[0].state ==
					   DISCONNECTED) {
					break;
				} else if (signal_pending(current)) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Wait for connecting got signal!\n",
					     dlci);
					retval = -EAGAIN;
					break;
				} else if ((jiffies - t) >= TS0710MUX_TIME_OUT) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Wait for connecting timeout!\n",
					     dlci);
					continue;
				} else if (ts0710->dlci[0].state == CONNECTING) {
					continue;
				}
			}

			if (ts0710->dlci[0].state == CONNECTING) {
				ts0710->dlci[0].state = DISCONNECTED;
			}
		} else if ((ts0710->dlci[0].state != DISCONNECTED)
			   && (ts0710->dlci[0].state != REJECTED)) {
			TS0710_PRINTK("MUX DLCI:%d state is invalid!\n", dlci);
			return retval;
		} else {
			ts0710->initiator = 1;
			ts0710->dlci[0].state = CONNECTING;
			ts0710->dlci[0].initiator = 1;
			try = 10;
			while (try--) {
				t = jiffies;
				send_sabm(ts0710, 0);
				wait_event_interruptible_timeout(ts0710->dlci
								 [0].open_wait,
								 ts0710->dlci
								 [0].state !=
								 CONNECTING,
								 TS0710MUX_TIME_OUT);
				if ((ts0710->dlci[0].state == CONNECTED)
				    || (ts0710->dlci[0].state == FLOW_STOPPED)) {
					retval = 0;
					break;
				} else if (ts0710->dlci[0].state == REJECTED) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Send SABM got rejected!\n",
					     dlci);
					retval = -EREJECTED;
					break;
				} else if (signal_pending(current)) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Send SABM got signal!\n",
					     dlci);
					retval = -EAGAIN;
					break;
				} else if ((jiffies - t) >= TS0710MUX_TIME_OUT) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Send SABM timeout!\n",
					     dlci);
					continue;
				}
			}

			if (ts0710->dlci[0].state == CONNECTING) {
				ts0710->dlci[0].state = DISCONNECTED;
			}
			wake_up_interruptible(&ts0710->dlci[0].open_wait);
		}
	} else {	/* other channel */
		if ((ts0710->dlci[0].state != CONNECTED)
		    && (ts0710->dlci[0].state != FLOW_STOPPED)) {
			return retval;
		} else if ((ts0710->dlci[dlci].state == CONNECTED)
			   || (ts0710->dlci[dlci].state == FLOW_STOPPED)) {
			return 0;
		} else if ((ts0710->dlci[dlci].state == NEGOTIATING)
			   || (ts0710->dlci[dlci].state == CONNECTING)) {
			/* Reentry */
			try = 8;
			while (try--) {
				t = jiffies;
				wait_event_interruptible_timeout(ts0710->dlci
								 [dlci].open_wait,
								 (ts0710->dlci
								  [dlci].state
								  != CONNECTING
								  ||
								  ts0710->dlci
								  [dlci].state
								  !=
								  NEGOTIATING),
								 TS0710MUX_TIME_OUT);
				if ((ts0710->dlci[dlci].state == CONNECTED)
				    || (ts0710->dlci[dlci].state ==
					FLOW_STOPPED)) {
					retval = 0;
					break;
				} else if (ts0710->dlci[dlci].state == REJECTED) {
					retval = -EREJECTED;
					break;
				} else if (ts0710->dlci[dlci].state ==
					   DISCONNECTED) {
					break;
				} else if (signal_pending(current)) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Wait for connecting got signal!\n",
					     dlci);
					retval = -EAGAIN;
					break;
				} else if ((jiffies - t) >= TS0710MUX_TIME_OUT) {
					TS0710_PRINTK
					    ("MUX DLCI:%d Wait for connecting timeout!\n",
					     dlci);
					continue;
				} else
				    if ((ts0710->dlci[dlci].state ==
					 NEGOTIATING)
					|| (ts0710->dlci[dlci].state ==
					    CONNECTING)) {
					continue;
				}
			}

			if ((ts0710->dlci[dlci].state == NEGOTIATING)
			    || (ts0710->dlci[dlci].state == CONNECTING)) {
				ts0710->dlci[dlci].state = DISCONNECTED;
			}
		} else if ((ts0710->dlci[dlci].state != DISCONNECTED)
			   && (ts0710->dlci[dlci].state != REJECTED)) {
			TS0710_PRINTK("MUX DLCI:%d state is invalid!\n", dlci);
			return retval;
		} else {
			ts0710->dlci[dlci].state = CONNECTING;
			ts0710->dlci[dlci].initiator = 1;
			if (ts0710->dlci[dlci].state == CONNECTING) {
				try = 3;
				while (try--) {
					t = jiffies;
					send_sabm(ts0710, dlci);
					wait_event_interruptible_timeout
					    (ts0710->dlci[dlci].open_wait,
					     ts0710->dlci[dlci].state !=
					     CONNECTING, TS0710MUX_TIME_OUT);
					if ((ts0710->dlci[dlci].state ==
					     CONNECTED)
					    || (ts0710->dlci[dlci].state ==
						FLOW_STOPPED)) {
						retval = 0;
						break;
					} else if (ts0710->dlci[dlci].state ==
						   REJECTED) {
						TS0710_PRINTK
						    ("MUX DLCI:%d Send SABM got rejected!\n",
						     dlci);
						retval = -EREJECTED;
						break;
					} else if (signal_pending(current)) {
						TS0710_PRINTK
						    ("MUX DLCI:%d Send SABM got signal!\n",
						     dlci);
						retval = -EAGAIN;
						break;
					} else if ((jiffies - t) >=
						   TS0710MUX_TIME_OUT) {
						TS0710_PRINTK
						    ("MUX DLCI:%d Send SABM timeout!\n",
						     dlci);
						continue;
					}
				}
			}

			if ((ts0710->dlci[dlci].state == NEGOTIATING)
			    || (ts0710->dlci[dlci].state == CONNECTING)) {
				ts0710->dlci[dlci].state = DISCONNECTED;
			}
			wake_up_interruptible(&ts0710->dlci[dlci].open_wait);
		}
	}
	return retval;
}

static int ts0710_exec_test_cmd(void)
{
	ts0710_con *ts0710 = &ts0710_connection;
	__u8 *f_buf;		/* Frame buffer */
	__u8 *d_buf;		/* Data buffer */
	int retval = -EFAULT;
	int j;
	unsigned long t;

	if (ts0710->be_testing) {
		/* Reentry */
		t = jiffies;
		interruptible_sleep_on_timeout(&ts0710->test_wait,
					       3 * TS0710MUX_TIME_OUT);
		if (ts0710->be_testing == 0) {
			if (ts0710->test_errs == 0) {
				retval = 0;
			} else {
				retval = -EFAULT;
			}
		} else if (signal_pending(current)) {
			TS0710_DEBUG
			    ("Wait for Test_cmd response got signal!\n");
			retval = -EAGAIN;
		} else if ((jiffies - t) >= 3 * TS0710MUX_TIME_OUT) {
			TS0710_DEBUG("Wait for Test_cmd response timeout!\n");
			retval = -EFAULT;
		}
	} else {
		ts0710->be_testing = 1;	/* Set the flag */

		f_buf = (__u8 *) kmalloc(TEST_PATTERN_SIZE + 32, GFP_KERNEL);
		d_buf = (__u8 *) kmalloc(TEST_PATTERN_SIZE + 32, GFP_KERNEL);
		if ((!f_buf) || (!d_buf)) {
			if (f_buf) {
				kfree(f_buf);
			}
			if (d_buf) {
				kfree(d_buf);
			}

			ts0710->be_testing = 0;	/* Clear the flag */
			ts0710->test_errs = TEST_PATTERN_SIZE;
			wake_up_interruptible(&ts0710->test_wait);
			return -ENOMEM;
		}

		for (j = 0; j < TEST_PATTERN_SIZE; j++) {
			d_buf[j] = j & 0xFF;
		}

		t = jiffies;
		ts0710_test_msg(ts0710, d_buf, TEST_PATTERN_SIZE, MCC_CMD,
				f_buf);
		interruptible_sleep_on_timeout(&ts0710->test_wait,
					       2 * TS0710MUX_TIME_OUT);
		if (ts0710->be_testing == 0) {
			if (ts0710->test_errs == 0) {
				retval = 0;
			} else {
				retval = -EFAULT;
			}
		} else if (signal_pending(current)) {
			TS0710_DEBUG("Send Test_cmd got signal!\n");
			retval = -EAGAIN;
		} else if ((jiffies - t) >= 2 * TS0710MUX_TIME_OUT) {
			TS0710_DEBUG("Send Test_cmd timeout!\n");
			ts0710->test_errs = TEST_PATTERN_SIZE;
			retval = -EFAULT;
		}

		ts0710->be_testing = 0;	/* Clear the flag */
		wake_up_interruptible(&ts0710->test_wait);

		/* Release buffer */
		if (f_buf) {
			kfree(f_buf);
		}
		if (d_buf) {
			kfree(d_buf);
		}
	}

	return retval;
}

static void mux_sched_send(void)
{
	complete(&send_completion);
}

/*
 * Returns 1 if found, 0 otherwise. needle must be null-terminated.
 * strstr might not work because WebBox sends garbage before the first OKread
 *
 */
int findInBuf(unsigned char *buf, int len, char *needle)
{
	int i;
	int needleMatchedPos = 0;

	if (needle[0] == '\0') {
		return 1;
	}

	for (i = 0; i < len; i++) {
		if (needle[needleMatchedPos] == buf[i]) {
			needleMatchedPos++;
			if (needle[needleMatchedPos] == '\0') {
				/* Entire needle was found */
				return 1;
			}
		} else {
			needleMatchedPos = 0;
		}
	}
	return 0;
}

static int cmux_mode = 0;
int is_cmux_mode(void)
{
	return cmux_mode;
}

/****************************
 * TTY driver routines
*****************************/

static void mux_close(struct tty_struct *tty, struct file *filp)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int line;
	__u8 dlci;
	__u8 cmdtty;
	__u8 datatty;

	UNUSED_PARAM(filp);

	if (!tty) {
		return;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		return;
	}
	if (mux_tty[line] > 0)
		mux_tty[line]--;

	dlci = tty2dlci[line];
	cmdtty = dlci2tty[dlci].cmdtty;
	datatty = dlci2tty[dlci].datatty;
	if ((mux_tty[cmdtty] == 0) && (mux_tty[datatty] == 0)) {
		if (dlci == 1) {
			ts0710_close_channel(0);
			TS0710_PRINTK
			    ("MUX mux_close: tapisrv might be down!!! Close DLCI 1\n");
			TS0710_SIG2APLOGD();
			cmux_mode = 0;
			/* destroy the receive thread*/
			iomux[0].io_stop(SPRDMUX_READ);
			if(mux_recv_kthread) {
				kthread_stop(mux_recv_kthread);
				mux_recv_kthread = NULL;
			}
		}
		ts0710_close_channel(dlci);
	}

	if (mux_tty[line] == 0) {
		if ((mux_send_info_flags[line])
		    && (mux_send_info[line])
		    /*&& (mux_send_info[line]->filled == 0) */
		    ) {
			mux_send_info_flags[line] = 0;
			kfree(mux_send_info[line]);
			mux_send_info[line] = 0;
			TS0710_DEBUG("Free mux_send_info for /dev/mux%d\n", line);
		}

		if ((mux_recv_info_flags[line])
		    && (mux_recv_info[line])
		    && (mux_recv_info[line]->total == 0)) {
			mux_recv_info_flags[line] = 0;
			free_mux_recv_struct(mux_recv_info[line]);
			mux_recv_info[line] = 0;
			TS0710_DEBUG("Free mux_recv_info for /dev/mux%d\n", line);
		}

		ts0710_flow_on(dlci, ts0710);
		complete(&post_recv_completion);
		wake_up_interruptible(&tty->read_wait);
		wake_up_interruptible(&tty->write_wait);
		tty->packet = 0;

	}
}

static void mux_throttle(struct tty_struct *tty)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int line;
	int i;
	__u8 dlci;
	mux_recv_struct *recv_info;

	if (!tty) {
		return;
	}

	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		return;
	}

	TS0710_DEBUG("Enter into %s, minor number is: %d\n", __FUNCTION__,
		     line);

	dlci = tty2dlci[line];
	if ((ts0710->dlci[0].state != CONNECTED)
	    && (ts0710->dlci[0].state != FLOW_STOPPED)) {
		return;
	} else if ((ts0710->dlci[dlci].state != CONNECTED)
		   && (ts0710->dlci[dlci].state != FLOW_STOPPED)) {
		return;
	}

	if (ts0710->dlci[dlci].flow_control) {
		TS0710_DEBUG("mux_throttle : just return\n");
		return;
	}

	recv_info = mux_recv_info[line];
	if(recv_info->total < TS0710MUX_MAX_TOTAL_SIZE/2)
		return;

	for (i = 0; i < 3; i++) {
		if (ts0710_msc_msg
		    (ts0710, EA | FC | RTC | RTR | DV, MCC_CMD, dlci) < 0) {
			continue;
		} else {
			TS0710_DEBUG("MUX Send Flow off on dlci %d\n", dlci);
			ts0710->dlci[dlci].flow_control = 1;
			break;
		}
	}
}

static void mux_unthrottle(struct tty_struct *tty)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int line;
	__u8 dlci;
	mux_recv_struct *recv_info;

	if (!tty) {
		return;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		return;
	}

	if ((!mux_recv_info_flags[line]) || (!mux_recv_info[line])) {
		return;
	}

	TS0710_DEBUG("Enter into %s, minor number is: %d\n", __FUNCTION__,
		     line);

	recv_info = mux_recv_info[line];
	dlci = tty2dlci[line];

	if (recv_info->total) {
		recv_info->post_unthrottle = 1;
		complete(&post_recv_completion);
	} else {
		if(ts0710->dlci[dlci].flow_control == 1) {
			TS0710_DEBUG("mux_unthrottle : send flow on\n");
			ts0710_flow_on(dlci, ts0710);
		}
	}
}

static int mux_chars_in_buffer(struct tty_struct *tty)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int retval;
	int line;
	__u8 dlci;
	mux_send_struct *send_info;

	retval = TS0710MUX_MAX_CHARS_IN_BUF;
	if (!tty) {
		goto out;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		goto out;
	}

	dlci = tty2dlci[line];
	if (ts0710->dlci[0].state == FLOW_STOPPED) {
		TS0710_DEBUG
		    ("Flow stopped on all channels, returning MAX chars in buffer\n");
		goto out;
	} else if (ts0710->dlci[dlci].state == FLOW_STOPPED) {
		TS0710_DEBUG("Flow stopped, returning MAX chars in buffer\n");
		goto out;
	} else if (ts0710->dlci[dlci].state != CONNECTED) {
		TS0710_DEBUG("DLCI %d not connected\n", dlci);
		goto out;
	}

	if (!(mux_send_info_flags[line])) {
		goto out;
	}
	send_info = mux_send_info[line];
	if (!send_info) {
		goto out;
	}
	if (send_info->filled) {
		goto out;
	}

	retval = 0;

out:
	return retval;
}

static int mux_write(struct tty_struct *tty,
		     const unsigned char *buf, int count)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int line;
	__u8 dlci;
	mux_send_struct *send_info;
	__u8 *d_buf;
	__u16 c;

	if (count <= 0) {
		return 0;
	}

	if (!tty) {
		return 0;
	}

	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS))
		return -ENODEV;

	dlci = tty2dlci[line];
	if (ts0710->dlci[0].state == FLOW_STOPPED) {
		TS0710_DEBUG
		    ("Flow stopped on all channels, returning zero /dev/mux%d\n",
		     line);
		return 0;
	} else if (ts0710->dlci[dlci].state == FLOW_STOPPED) {
		TS0710_DEBUG("Flow stopped, returning zero /dev/mux%d\n", line);
		return 0;
	} else if (ts0710->dlci[dlci].state == CONNECTED) {

		if (!(mux_send_info_flags[line])) {
			TS0710_PRINTK
			    ("MUX Error: mux_write: mux_send_info_flags[%d] == 0\n",
			     line);
			return -ENODEV;
		}
		send_info = mux_send_info[line];
		if (!send_info) {
			TS0710_PRINTK
			    ("MUX Error: mux_write: mux_send_info[%d] == 0\n",
			     line);
			return -ENODEV;
		}

		c = min(count, (ts0710->dlci[dlci].mtu - 1));
		if (c <= 0) {
			return 0;
		}

		if (test_and_set_bit(BUF_BUSY, &send_info->flags))
			return 0;

		if (send_info->filled) {
			TS0710_PRINTK("mux_write: send_info filled is set\n");
			clear_bit(BUF_BUSY, &send_info->flags);
			return 0;
		}

		d_buf = ((__u8 *) send_info->buf) + TS0710MUX_SEND_BUF_OFFSET;
		memcpy(&d_buf[0], buf, c);	/* no tag byte */

		TS0710_DEBUG("Prepare to send %d bytes from /dev/mux%d", c,
			     line);
		TS0710_DEBUGHEX(d_buf, c);

		send_info->frame = d_buf;
		queue_uih(send_info, c, ts0710, dlci);
		send_info->filled = 1;
		clear_bit(BUF_BUSY, &send_info->flags);

		/* Sending bottom half should be
		run after return from this function */
		mux_sched_send();

		return c;
	} else {
		TS0710_PRINTK("MUX mux_write: DLCI %d not connected\n", dlci);
		return -EDISCONNECTED;
	}
}

static int mux_write_room(struct tty_struct *tty)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int retval;
	int line;
	__u8 dlci;
	mux_send_struct *send_info;

	retval = 0;
	if (!tty) {
		goto out;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		goto out;
	}

	dlci = tty2dlci[line];
	if (ts0710->dlci[0].state == FLOW_STOPPED) {
		TS0710_DEBUG("Flow stopped on all channels, returning ZERO\n");
		goto out;
	} else if (ts0710->dlci[dlci].state == FLOW_STOPPED) {
		TS0710_DEBUG("Flow stopped, returning ZERO\n");
		goto out;
	} else if (ts0710->dlci[dlci].state != CONNECTED) {
		TS0710_DEBUG("DLCI %d not connected\n", dlci);
		goto out;
	}

	if (!(mux_send_info_flags[line])) {
		goto out;
	}
	send_info = mux_send_info[line];
	if (!send_info) {
		goto out;
	}
	if (send_info->filled) {
		goto out;
	}

	retval = ts0710->dlci[dlci].mtu - 1;

out:
	return retval;
}

static int mux_ioctl(struct tty_struct *tty, unsigned int cmd,
		     unsigned long arg)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int line;
	__u8 dlci;

	UNUSED_PARAM(arg);

	if (!tty) {
		return -EIO;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		return -ENODEV;
	}

	dlci = tty2dlci[line];
	switch (cmd) {
	case TS0710MUX_IO_MSC_HANGUP:
		if (ts0710_msc_msg(ts0710, EA | RTR | DV, MCC_CMD, dlci) < 0) {
			return -EAGAIN;
		} else {
			return 0;
		}

	case TS0710MUX_IO_TEST_CMD:
		return ts0710_exec_test_cmd();

	default:
		break;
	}
	return -ENOIOCTLCMD;
}

static void mux_flush_buffer(struct tty_struct *tty)
{
	int line;

	if (!tty) {
		return;
	}

	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		return;
	}

	TS0710_PRINTK("MUX %s: line is:%d\n", __FUNCTION__, line);

	if ((mux_send_info_flags[line])
	    && (mux_send_info[line])
	    && (mux_send_info[line]->filled)) {

		mux_send_info[line]->filled = 0;
	}

	wake_up_interruptible(&tty->write_wait);
#ifdef SERIAL_HAVE_POLL_WAIT
	wake_up_interruptible(&tty->poll_wait);
#endif
	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	    tty->ldisc->ops->write_wakeup) {
		(tty->ldisc->ops->write_wakeup) (tty);
	}

}

static int mux_open(struct tty_struct *tty, struct file *filp)
{
	int retval, err;
	int line;
	__u8 dlci;
	__u8 cmdtty;
	__u8 datatty;
	mux_send_struct *send_info;
	mux_recv_struct *recv_info;

	UNUSED_PARAM(filp);

	retval = -ENODEV;

	if (!tty) {
		goto out;
	}
	line = tty->index;
	if ((line < 0) || (line >= NR_MUXS)) {
		goto out;
	}
	if(line > 0) {
		if(cmux_mode != 1)
			return -ENODEV;
	}

	printk(KERN_INFO "MUX: mux[%d] opened!\n", tty->index);

#ifdef TS0710SERVER
	/* do nothing as a server */
	mux_tty[line]++;
	retval = 0;
#else
	mux_tty[line]++;
	dlci = tty2dlci[line];
	if (dlci == 1) {
		if (cmux_mode == 0) {
			char buffer[256];
			char *buff = buffer;
			int count, i = 0;
			/* tty_lock may stops ptmx_open used by adbd */
			tty_unlock();
			memset(buffer, 0, 256);
			if (mux_mode == 1)
				iomux[0].io_write("AT+SMMSWAP=0\r", strlen("AT+SMMSWAP=0\r"));
			else
				iomux[0].io_write("AT\r", strlen("AT\r"));
			/*wait for response "OK \r" */
			printk(KERN_INFO "\n cmux receive:<\n");
			msleep(1000);
			while (1) {
				count = iomux[0].io_read(buff, sizeof(buffer) - (buff - buffer));
				if(count > 0) {
					buff += count;
					printk(KERN_INFO "ts mux receive %d chars\n", count);
					if (findInBuf(buffer, 256, "OK")) {
						break;
					} else if (findInBuf(buffer, 256, "ERROR")) {
						printk(KERN_INFO "\n wrong modem state !!!!\n");
						break;
					}
				} else {
					msleep(2000);
					if (mux_mode == 1)
						iomux[0].io_write("AT+SMMSWAP=0\r", strlen("AT+SMMSWAP=0\r"));
					else
						iomux[0].io_write("AT\r", strlen("AT\r"));
					i++;
				}
				if (i > 5) {
					printk(KERN_WARNING "\n wrong modem state !!!!\n");
					retval = -ENODEV;
					goto out;
				}
			}
			iomux[0].io_write("at+cmux=0\r", strlen("at+cmux=0\r"));
			iomux[0].io_read(buffer, sizeof(buffer));
			printk(KERN_INFO "\n cmux receive>\n");
			cmux_mode = 1;
			/*create receive thread*/
			mux_recv_kthread = kthread_create(mux_receive_thread, NULL, "mux_receive");
			if(IS_ERR(mux_recv_kthread)) {
				printk(KERN_ERR "Unable to create mux_receive thread\n");
				err = PTR_ERR(mux_recv_kthread);
				mux_recv_kthread = NULL;
				return err;
			}
			wake_up_process(mux_recv_kthread);
			tty_lock();
		}

		/* Open server channel 0 first */
		if ((retval = ts0710_open_channel(0)) != 0) {
			TS0710_PRINTK("MUX: Can't connect server channel 0!\n");
			ts0710_init();

			mux_tty[line]--;
			goto out;
		}
	}

	/* Allocate memory first. As soon as connection has been established, MUX may receive */
	if (mux_send_info_flags[line] == 0) {
		send_info =
		    (mux_send_struct *) kmalloc(sizeof(mux_send_struct),
						GFP_KERNEL);
		if (!send_info) {
			retval = -ENOMEM;

			mux_tty[line]--;
			goto out;
		}
		send_info->length = 0;
		send_info->flags = 0;
		send_info->filled = 0;
		mux_send_info[line] = send_info;
		mux_send_info_flags[line] = 1;
		TS0710_DEBUG("Allocate mux_send_info for /dev/mux%d", line);
	}

	if (mux_recv_info_flags[line] == 0) {
		recv_info =
		    (mux_recv_struct *) kmalloc(sizeof(mux_recv_struct),
						GFP_KERNEL);
		if (!recv_info) {
			mux_send_info_flags[line] = 0;
			kfree(mux_send_info[line]);
			mux_send_info[line] = 0;
			TS0710_DEBUG("Free mux_send_info for /dev/mux%d", line);
			retval = -ENOMEM;

			mux_tty[line]--;
			goto out;
		}
		recv_info->length = 0;
		recv_info->total = 0;
		recv_info->mux_packet = 0;
		recv_info->next = 0;
		recv_info->no_tty = line;
		recv_info->post_unthrottle = 0;
		mutex_init(&recv_info->recv_lock);
		mux_recv_info[line] = recv_info;
		mux_recv_info_flags[line] = 1;
		TS0710_DEBUG("Allocate mux_recv_info for /dev/mux%d", line);
	}

	/* Now establish DLCI connection */
	cmdtty = dlci2tty[dlci].cmdtty;
	datatty = dlci2tty[dlci].datatty;
	if ((mux_tty[cmdtty] > 0) || (mux_tty[datatty] > 0)) {
		if ((retval = ts0710_open_channel(dlci)) != 0) {
			TS0710_PRINTK("MUX: Can't connected channel %d!\n",
				      dlci);
			ts0710_reset_dlci(dlci);

			mux_send_info_flags[line] = 0;
			kfree(mux_send_info[line]);
			mux_send_info[line] = 0;
			TS0710_DEBUG("Free mux_send_info for /dev/mux%d", line);

			mux_recv_info_flags[line] = 0;
			free_mux_recv_struct(mux_recv_info[line]);
			mux_recv_info[line] = 0;
			TS0710_DEBUG("Free mux_recv_info for /dev/mux%d", line);

			mux_tty[line]--;
			goto out;
		}
	}
	retval = 0;
#endif
out:
	return retval;
}

static inline int rt_policy(int policy)
{
	if (unlikely(policy == SCHED_FIFO) || unlikely(policy == SCHED_RR))
		return 1;
	return 0;
}

static inline int task_has_rt_policy(struct task_struct *p)
{
	return rt_policy(p->policy);
}

/*For BP UART problem Begin*/
#ifdef TS0710SEQ2
static int send_ack(ts0710_con * ts0710, __u8 seq_num, __u8 bp_seq1,
		    __u8 bp_seq2)
#else
static int send_ack(ts0710_con * ts0710, __u8 seq_num)
#endif
{
	return 0;
}

static int mux_set_thread_pro(int pro)
{
	int ret;
	struct sched_param s;

	/* just for this write, set us real-time */
	if (!task_has_rt_policy(current)) {
		struct cred *new = prepare_creds();
		cap_raise(new->cap_effective, CAP_SYS_NICE);
		commit_creds(new);
		s.sched_priority = MAX_RT_PRIO - pro;
		ret = sched_setscheduler(current, SCHED_RR, &s);
		if (ret != 0)
			printk(KERN_WARNING "MUX: set priority failed!\n");
	}
	return 0;
}

/*For BP UART problem End*/

static void receive_worker(int start)
{
	int count;
	static unsigned char tbuf[TS0710MUX_MAX_BUF_SIZE];
	static unsigned char *tbuf_ptr;
	static unsigned char *start_flag;
	unsigned char *search, *to, *from;
	short_frame *short_pkt;
	long_frame *long_pkt;
	static int framelen = -1;
	/*For BP UART problem Begin */
	static __u8 expect_seq = 0;
	__u32 crc_error;
	__u8 *uih_data_start;
	__u32 uih_len;
	/*For BP UART problem End */

	if (start) {
		memset(tbuf, 0, TS0710MUX_MAX_BUF_SIZE);
		tbuf_ptr = tbuf;
		start_flag = 0;
	}

	count = iomux[0].io_read(tbuf_ptr, TS0710MUX_MAX_BUF_SIZE - (tbuf_ptr - tbuf));
	if (count <= 0) {
		return;
	}
	tbuf_ptr += count;

	if ((start_flag != 0) && (framelen != -1)) {
		if ((tbuf_ptr - start_flag) < framelen) {
			clear_bit(RECV_RUNNING, &mux_recv_flags);
			return;
		}
	}

	search = &tbuf[0];
	while (1) {
		if (start_flag == 0) {	/* Frame Start Flag not found */
			framelen = -1;
			while (search < tbuf_ptr) {
				if (*search == TS0710_BASIC_FLAG) {

					start_flag = search;
					break;
				}
#ifdef TS0710LOG
				else {
					TS0710_LOG(">S %02x %c\n", *search,
						   *search);
				}
#endif

				search++;
			}

			if (start_flag == 0) {
				tbuf_ptr = &tbuf[0];
				break;
			}
		} else {	/* Frame Start Flag found */
			/* 1 start flag + 1 address + 1 control + 1 or 2 length + lengths data + 1 FCS + 1 end flag */
			/* For BP UART problem 1 start flag + 1 seq_num + 1 address + ...... */
			/*if( (framelen == -1) && ((tbuf_ptr - start_flag) > TS0710_MAX_HDR_SIZE) ) */
			if ((framelen == -1) && ((tbuf_ptr - start_flag) > (TS0710_MAX_HDR_SIZE + SEQ_FIELD_SIZE))) {	/*For BP UART problem */
				/*short_pkt = (short_frame *) (start_flag + 1); */
				short_pkt = (short_frame *) (start_flag + ADDRESS_FIELD_OFFSET);	/*For BP UART problem */
				if (short_pkt->h.length.ea == 1) {	/* short frame */
					/*framelen = TS0710_MAX_HDR_SIZE + short_pkt->h.length.len + 1; */
					framelen = TS0710_MAX_HDR_SIZE + short_pkt->h.length.len + 1 + SEQ_FIELD_SIZE;	/*For BP UART problem */
				} else {	/* long frame */
					/*long_pkt = (long_frame *) (start_flag + 1); */
					long_pkt = (long_frame *) (start_flag + ADDRESS_FIELD_OFFSET);	/*For BP UART problem */
					/*framelen = TS0710_MAX_HDR_SIZE + GET_LONG_LENGTH( long_pkt->h.length ) + 2; */
					framelen = TS0710_MAX_HDR_SIZE + GET_LONG_LENGTH(long_pkt->h.length) + 2 + SEQ_FIELD_SIZE;	/*For BP UART problem */
				}

				/*if( framelen > TS0710MUX_MAX_TOTAL_FRAME_SIZE ) { */
				if (framelen > (TS0710MUX_MAX_TOTAL_FRAME_SIZE + SEQ_FIELD_SIZE)) {	/*For BP UART problem */
					TS0710_LOGSTR_FRAME(0, start_flag,
							    (tbuf_ptr -
							     start_flag));
					TS0710_PRINTK
					    ("MUX Error: %s: frame length:%d is bigger than Max total frame size:%d\n",
		 /*__FUNCTION__, framelen, TS0710MUX_MAX_TOTAL_FRAME_SIZE);*/
					     __FUNCTION__, framelen, (TS0710MUX_MAX_TOTAL_FRAME_SIZE + SEQ_FIELD_SIZE));	/*For BP UART problem */
					search = start_flag + 1;
					start_flag = 0;
					framelen = -1;
					continue;
				}
			}

			if ((framelen != -1)
			    && ((tbuf_ptr - start_flag) >= framelen)) {
				if (*(start_flag + framelen - 1) == TS0710_BASIC_FLAG) {	/* OK, We got one frame */

					/*For BP UART problem Begin */
					TS0710_LOGSTR_FRAME(0, start_flag,
							    framelen);
					TS0710_DEBUGHEX(start_flag, framelen);

					short_pkt =
					    (short_frame *) (start_flag +
							     ADDRESS_FIELD_OFFSET);
					if ((short_pkt->h.length.ea) == 0) {
						long_pkt =
						    (long_frame *) (start_flag +
								    ADDRESS_FIELD_OFFSET);
						uih_len =
						    GET_LONG_LENGTH(long_pkt->h.
								    length);
						uih_data_start =
						    long_pkt->h.data;

						crc_error =
						    crc_check((__u8
							       *) (start_flag +
								   SLIDE_BP_SEQ_OFFSET),
							      LONG_CRC_CHECK +
							      1,
							      *(uih_data_start +
								uih_len));
					} else {
						uih_len =
						    short_pkt->h.length.len;
						uih_data_start =
						    short_pkt->data;

						crc_error =
						    crc_check((__u8
							       *) (start_flag +
								   SLIDE_BP_SEQ_OFFSET),
							      SHORT_CRC_CHECK +
							      1,
							      *(uih_data_start +
								uih_len));
					}

					if (!crc_error) {
						if (	/*expect_seq ==
							 *(start_flag +
							 SLIDE_BP_SEQ_OFFSET)*/
							   1) {
							expect_seq =
							    *(start_flag +
							      SLIDE_BP_SEQ_OFFSET);
							expect_seq++;
							if (expect_seq >= 4) {
								expect_seq = 0;
							}
#ifdef TS0710SEQ2
							send_ack
							    (&ts0710_connection,
							     expect_seq,
							     *(start_flag +
							       FIRST_BP_SEQ_OFFSET),
							     *(start_flag +
							       SECOND_BP_SEQ_OFFSET));
#else
							send_ack
							    (&ts0710_connection,
							     expect_seq);
#endif

							ts0710_recv_data
							    (&ts0710_connection,
							     start_flag +
							     ADDRESS_FIELD_OFFSET,
							     framelen - 2 -
							     SEQ_FIELD_SIZE);
						} else {

#ifdef TS0710DEBUG
							if (*
							    (start_flag +
							     SLIDE_BP_SEQ_OFFSET)
!= 0x9F) {
#endif

								TS0710_LOG
								    ("MUX sequence number %d is not expected %d, discard data!\n",
								     *
								     (start_flag
								      +
								      SLIDE_BP_SEQ_OFFSET),
								     expect_seq);

#ifdef TS0710SEQ2
								send_ack
								    (&ts0710_connection,
								     expect_seq,
								     *
								     (start_flag
								      +
								      FIRST_BP_SEQ_OFFSET),
								     *
								     (start_flag
								      +
								      SECOND_BP_SEQ_OFFSET));
#else
								send_ack
								    (&ts0710_connection,
								     expect_seq);
#endif

#ifdef TS0710DEBUG
							} else {
								*(uih_data_start
								  + uih_len) =
						     0;
								TS0710_PRINTK
								    ("MUX bp log: %s\n",
								     uih_data_start);
							}
#endif

						}
					} else {	/* crc_error */
						search = start_flag + 1;
						start_flag = 0;
						framelen = -1;
						continue;
					}	/*End if(!crc_error) */

					search = start_flag + framelen;
				} else {
					TS0710_LOGSTR_FRAME(0, start_flag,
							    framelen);
					TS0710_DEBUGHEX(start_flag, framelen);
					TS0710_PRINTK
					    ("MUX: Lost synchronization!\n");
					search = start_flag + 1;
				}

				start_flag = 0;
				framelen = -1;
				continue;
			}

			if (start_flag != &tbuf[0]) {

				to = tbuf;
				from = start_flag;
				count = tbuf_ptr - start_flag;
				while (count--) {
					*to++ = *from++;
				}

				tbuf_ptr -= (start_flag - tbuf);
				start_flag = tbuf;

			}
			break;
		}		/* End Frame Start Flag found */
	}			/* End while(1) */

	clear_bit(RECV_RUNNING, &mux_recv_flags);
}

static int mux_receive_thread(void *data)
{
	int start = 1;

	mux_set_thread_pro(95);
	while (!kthread_should_stop()) {
		if (mux_exiting == 1) {
			mux_exiting = 2;
			return 0;
		}
		receive_worker(start);
		start = 0;
	}
	return 0;
}

static int mux_post_recv_thread(void *private_)
{
	ts0710_con *ts0710 = &ts0710_connection;
	int tty_idx;
	struct tty_struct *tty;
	__u8 flow_control;
	__u8 dlci;
	mux_recv_struct *recv_info;
	int recv_room;
	mux_recv_packet *recv_packet;

	UNUSED_PARAM(private_);
	mux_set_thread_pro(90);

	TS0710_DEBUG("Enter into post_recv_thread");
	while (!kthread_should_stop()) {

		wait_for_completion_interruptible(&post_recv_completion);

		if (test_and_set_bit(RECV_RUNNING, &mux_recv_flags)) {
			return 0;
		}

		for(tty_idx = 0; tty_idx < NR_MUXS; tty_idx++) {
			recv_info = mux_recv_info[tty_idx];
			if(!recv_info) {
				continue;
			}
			mutex_lock(&recv_info->recv_lock);
			if (!(recv_info->total)) {
				TS0710_DEBUG
					("MUX Error: %s: Should not get here, recv_info->total == 0 \n",
					__FUNCTION__);
				mutex_unlock(&recv_info->recv_lock);
				continue;
			}
			dlci = tty2dlci[tty_idx];
			tty = mux_table[tty_idx];
			if ((!mux_tty[tty_idx]) || (!tty)) {
				TS0710_PRINTK
					("MUX: No application waiting for, free recv_info! tty_idx:%d\n",
					tty_idx);
				mux_recv_info_flags[tty_idx] = 0;
				free_mux_recv_struct(mux_recv_info[tty_idx]);
				mux_recv_info[tty_idx] = 0;
				ts0710_flow_on(dlci, ts0710);
				mutex_unlock(&recv_info->recv_lock);
				continue;
			}
			if (test_bit(TTY_THROTTLED, &tty->flags)) {
				TS0710_DEBUG("post_recv_thread : TTY_THROTTLED is set\n");
				mutex_unlock(&recv_info->recv_lock);
				continue;
			}
			flow_control = 0;
			while (recv_info->total) {
				recv_room = 65535;
				if (tty->receive_room)
					recv_room = tty->receive_room;

				if (recv_info->length) {
					if (recv_room < recv_info->length) {
						if(recv_info->total >= TS0710MUX_MAX_TOTAL_SIZE/2)
							flow_control = 1;
						break;
					}

					/* Put queued data into read buffer of tty */
					TS0710_DEBUG
						("Put queued recv data into read buffer of /dev/mux%d\n",
						tty_idx);
					TS0710_DEBUGHEX(recv_info->data,
							recv_info->length);
					(tty->ldisc->ops->receive_buf) (tty,
									recv_info->data,
									NULL,
									recv_info->length);
					recv_info->total -= recv_info->length;
					recv_info->length = 0;
				} else {	/* recv_info->length == 0 */
					if ((recv_packet = recv_info->mux_packet)) {

						if (recv_room < recv_packet->length) {
							if(recv_info->total >= TS0710MUX_MAX_TOTAL_SIZE/2)
								flow_control = 1;
							break;
						}

						/* Put queued data into read buffer of tty */
						TS0710_DEBUG
							("Put queued recv data into read buffer2 of /dev/mux%d \n",
							tty_idx);
						TS0710_DEBUGHEX(recv_packet->data,
								recv_packet->length);
						(tty->ldisc->ops->receive_buf) (tty,
										recv_packet->
										data,
										NULL,
										recv_packet->
										length);
						recv_info->total -= recv_packet->length;
						recv_info->mux_packet = recv_packet->next;
						free_mux_recv_packet(recv_packet);
					} else {
						TS0710_PRINTK
							("MUX Error: %s: Should not get here, recv_info->total is:%u \n",
							__FUNCTION__, recv_info->total);
					}
				}	/* End recv_info->length == 0 */
			}		/* End while( recv_info->total ) */
			mutex_unlock(&recv_info->recv_lock);

			if (!(recv_info->total)) {
				/* Important clear */
				recv_info->mux_packet = 0;

				if (recv_info->post_unthrottle) {
					/* Do something for post_unthrottle */
					if(ts0710->dlci[dlci].flow_control == 1) {
						ts0710_flow_on(dlci, ts0710);
						TS0710_DEBUG("post_recv_thread : recv_info is empty & send flow on\n");
					}
					recv_info->post_unthrottle = 0;
				}
			} else {
				if (flow_control) {
					ts0710_flow_off(tty, dlci, ts0710);
				} else {
					/* this will trigger up layer(tty_unthrottle) call mux unthrottle */
					set_bit(TTY_THROTTLED, &tty->flags);
				}
			}
		}			/* End while( (recv_info = recv_info2) ) */
		clear_bit(RECV_RUNNING, &mux_recv_flags);
	}
	return 0;
}

static int mux_send_thread(void *private_)
{
	ts0710_con *ts0710 = &ts0710_connection;
	__u8 j;
	mux_send_struct *send_info;
	struct tty_struct *tty;
	__u8 dlci;

	UNUSED_PARAM(private_);

	TS0710_DEBUG("Enter into mux_send_thread\n");

	mux_send_info_idx = NR_MUXS;

	if (ts0710->dlci[0].state == FLOW_STOPPED) {
		TS0710_DEBUG("Flow stopped on all channels\n");
		return 0;
	}

	mux_set_thread_pro(80);

	while (!kthread_should_stop()) {

		wait_for_completion_interruptible(&send_completion);

		for (j = 0; j < NR_MUXS; j++) {
			if (!(mux_send_info_flags[j])) {
				continue;
			}

			send_info = mux_send_info[j];
			if (!send_info) {
				continue;
			}

			if (!(send_info->filled)) {
				continue;
			}

			dlci = tty2dlci[j];
			if (ts0710->dlci[dlci].state == FLOW_STOPPED) {
				TS0710_DEBUG("Flow stopped on channel DLCI: %d\n",
					dlci);
				continue;
			} else if (ts0710->dlci[dlci].state != CONNECTED) {
				TS0710_DEBUG("DLCI %d not connected\n", dlci);
				send_info->filled = 0;
				continue;
			}

			if (send_info->length <= TS0710MUX_SERIAL_BUF_SIZE) {
				TS0710_DEBUG("Send queued UIH for /dev/mux%d", j);
				basic_write(ts0710, (__u8 *) send_info->frame,
					send_info->length);
				send_info->length = 0;
				send_info->filled = 0;
#ifdef CONFIG_SPRD_SETH
				if(is_seth_ttyidx(j)) {
					TS0710_DEBUG("seth_restart on mux%d\n", j);
					seth_restart_queue(j);
				}
#endif
			} else {
				mux_send_info_idx = j;
				break;
			}
		}			/* End for() loop */

		/* Queue UIH data to be transmitted */
		for (j = 0; j < NR_MUXS; j++) {
			if (!(mux_send_info_flags[j])) {
				continue;
			}

			send_info = mux_send_info[j];
			if (!send_info) {
				continue;
			}

			if (send_info->filled) {
				continue;
			}

			/* Now queue UIH data to send_info->buf */
			if (!mux_tty[j]) {
				continue;
			}

			tty = mux_table[j];
			if (!tty) {
				continue;
			}

			dlci = tty2dlci[j];
			if (ts0710->dlci[dlci].state == FLOW_STOPPED) {
				TS0710_DEBUG("Flow stopped on channel DLCI: %d\n",
					dlci);
				continue;
			} else if (ts0710->dlci[dlci].state != CONNECTED) {
				TS0710_DEBUG("DLCI %d not connected\n", dlci);
				continue;
			}

			if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP))
				&& tty->ldisc->ops->write_wakeup) {
				(tty->ldisc->ops->write_wakeup) (tty);
			}
			wake_up_interruptible(&tty->write_wait);

#ifdef SERIAL_HAVE_POLL_WAIT
			wake_up_interruptible(&tty->poll_wait);
#endif

			if (send_info->filled) {
				if (j < mux_send_info_idx) {
					mux_send_info_idx = j;
				}
			}
		}			/* End for() loop */
	}
	return 0;
}

static int mux_proc_read(char *page, char **start, off_t off, int count,
			 int *eof, void *data)
{
	int len;
	if (off > 0) {
		*eof = 1;
		return 0;
	}
	len = sprintf(page, "%d\n", mux_mode);
	return len;
}

static int mux_proc_write(struct file *filp, const char __user * buf,
			  unsigned long len, void *data)
{
	char mux_buf[len + 1];
	int val;

	memset(mux_buf, 0, len + 1);
	if (len > 0) {
		if (copy_from_user(mux_buf, buf, len))
			return -EFAULT;
		val = simple_strtoul(mux_buf, NULL, 10);
		mux_mode = val;
	}
	return len;
}

static int mux_create_proc(void)
{
	struct proc_dir_entry *mux_entry;

	mux_entry = create_proc_entry("mux_mode", 0666, NULL);	/* creat /proc/mux_mode */
	if (!mux_entry) {
		printk(KERN_INFO "Can not create mux proc entry\n");
		return -ENOMEM;
	}
	mux_entry->read_proc = mux_proc_read;
	mux_entry->write_proc = mux_proc_write;
	return 0;
}

static void mux_remove_proc(void)
{
	remove_proc_entry("mux_mode", NULL);	/* remove /proc/mux_mode */
}

static const struct tty_operations tty_ops = {
	.open = mux_open,
	.close = mux_close,
	.write = mux_write,
	.write_room = mux_write_room,
	.flush_buffer = mux_flush_buffer,
	.chars_in_buffer = mux_chars_in_buffer,
	.throttle = mux_throttle,
	.unthrottle = mux_unthrottle,
	.ioctl = mux_ioctl,
};

static int __init mux_init(void)
{
	unsigned int j;
	int err;

	ts0710_init();

	for (j = 0; j < NR_MUXS; j++) {
		mux_send_info_flags[j] = 0;
		mux_send_info[j] = 0;
		mux_recv_info_flags[j] = 0;
		mux_recv_info[j] = 0;
	}
	mux_send_info_idx = NR_MUXS;
	mux_recv_flags = 0;

	init_completion(&send_completion);
	init_completion(&post_recv_completion);

	/*create mux_send thread*/
	mux_send_kthread = kthread_create(mux_send_thread, NULL, "mux_send");
	if(IS_ERR(mux_send_kthread)) {
		printk(KERN_ERR "Unable to create mux_send thread\n");
		err = PTR_ERR(mux_send_kthread);
		mux_send_kthread = NULL;
		return err;
	}
	wake_up_process(mux_send_kthread);

	/*create mux_post_recv thread*/
	mux_post_recv_kthread = kthread_create(mux_post_recv_thread, NULL, "mux_post_recv");
	if(IS_ERR(mux_post_recv_kthread)) {
		printk(KERN_ERR "Unable to create mux_post_recv thread\n");
		err = PTR_ERR(mux_post_recv_kthread);
		mux_post_recv_kthread = NULL;
		return err;
	}
	wake_up_process(mux_post_recv_kthread);

	memset(&mux_driver, 0, sizeof(struct tty_driver));
	memset(&mux_tty, 0, sizeof(mux_tty));
	kref_init(&mux_driver.kref);
	mux_driver.magic = TTY_DRIVER_MAGIC;
	mux_driver.driver_name = "ts0710mux";
	mux_driver.name = "ts0710mux";
	mux_driver.major = TS0710MUX_MAJOR;
	mux_driver.minor_start = TS0710MUX_MINOR_START;
	mux_driver.num = NR_MUXS;
	mux_driver.type = TTY_DRIVER_TYPE_SERIAL;
	mux_driver.subtype = SERIAL_TYPE_NORMAL;
	mux_driver.init_termios = tty_std_termios;
	mux_driver.init_termios.c_iflag = 0;
	mux_driver.init_termios.c_oflag = 0;
	mux_driver.init_termios.c_cflag = B38400 | CS8 | CREAD;
	mux_driver.init_termios.c_lflag = 0;
	mux_driver.flags = TTY_DRIVER_RESET_TERMIOS | TTY_DRIVER_REAL_RAW;
	mux_driver.other = NULL;
	mux_driver.owner = THIS_MODULE;

	tty_set_operations(&mux_driver, &tty_ops);

	if (tty_register_driver(&mux_driver))
		panic("Couldn't register mux driver");
	mux_table = mux_driver.ttys;

	if (mux_create_proc())
		printk(KERN_WARNING "create mux proc interface failed!\n");

	return 0;
}

static void __exit mux_exit(void)
{
	int j;

	mux_send_info_idx = NR_MUXS;
	for (j = 0; j < NR_MUXS; j++) {
		if ((mux_send_info_flags[j]) && (mux_send_info[j])) {
			kfree(mux_send_info[j]);
		}
		mux_send_info_flags[j] = 0;
		mux_send_info[j] = 0;

		if ((mux_recv_info_flags[j]) && (mux_recv_info[j])) {
			free_mux_recv_struct(mux_recv_info[j]);
		}
		mux_recv_info_flags[j] = 0;
		mux_recv_info[j] = 0;
	}
	mux_exiting = 1;

	while (mux_exiting == 1) {
		msleep(10);
	}

	if(mux_recv_kthread) {
		kthread_stop(mux_recv_kthread);
		mux_recv_kthread = NULL;
	}

	if(mux_send_kthread) {
		kthread_stop(mux_send_kthread);
		mux_send_kthread = NULL;
	}

	if(mux_post_recv_kthread) {
		kthread_stop(mux_post_recv_kthread);
		mux_post_recv_kthread = NULL;
	}

	if (tty_unregister_driver(&mux_driver))
		panic("Couldn't unregister mux driver");

	mux_remove_proc();
}

module_init(mux_init);
module_exit(mux_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harald Welte <laforge@openezx.org>");
MODULE_DESCRIPTION("GSM TS 07.10 Multiplexer");
