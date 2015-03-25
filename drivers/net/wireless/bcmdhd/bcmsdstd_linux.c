/*
 *  'Standard' SDIO HOST CONTROLLER driver - linux portion
 *
 * Copyright (C) 1999-2011, Broadcom Corporation
 * 
 *         Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: bcmsdstd_linux.c 291545 2011-10-24 08:02:08Z $
 */

#include <typedefs.h>
#include <pcicfg.h>
#include <bcmutils.h>
#include <sdio.h>	/* SDIO Device and Protocol Specs */
#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */
#include <sdiovar.h>	/* to get msglevel bit values */

#include <linux/sched.h>	/* request_irq() */

#include <bcmsdstd.h>

struct sdos_info {
	sdioh_info_t *sd;
	spinlock_t lock;
	wait_queue_head_t intr_wait_queue;
	struct timer_list tuning_timer;
	int tuning_timer_exp;
	int timer_enab;
	struct tasklet_struct tuning_tasklet;
};

#define SDSTD_WAITBITS_TIMEOUT		(5 * HZ)	/* seconds * HZ */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#define BLOCKABLE()	(!in_atomic())
#else
#define BLOCKABLE()	(!in_interrupt())
#endif

static void
sdstd_3_ostasklet(ulong data);
static void
sdstd_3_tuning_timer(ulong data);

/* Interrupt handler */
static irqreturn_t
sdstd_isr(int irq, void *dev_id
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20)
, struct pt_regs *ptregs
#endif
)
{
	sdioh_info_t *sd;
	struct sdos_info *sdos;
	bool ours;

	unsigned long flags;
	sd = (sdioh_info_t *)dev_id;
	sdos = (struct sdos_info *)sd->sdos_info;

	if (!sd->card_init_done) {
		sd_err(("%s: Hey Bogus intr...not even initted: irq %d\n", __FUNCTION__, irq));
		return IRQ_RETVAL(FALSE);
	} else {
		if (sdstd_3_is_retuning_int_set(sd)) {
			/* for 3.0 host, retuning request might come in this path */
			/* * disable ISR's */
			local_irq_save(flags);

			if (sdstd_3_check_and_set_retuning(sd))
				tasklet_schedule(&sdos->tuning_tasklet);

			/* * enable back ISR's */
			local_irq_restore(flags);

			/* * disable tuning isr signaling */
			sdstd_3_disable_retuning_int(sd);
			/* * note: check_client_intr() checks for  intmask also to
				wakeup. so be careful to use sd->intmask to disable
				re-tuning ISR.
				*/
		}
		ours = check_client_intr(sd);

		/* For local interrupts, wake the waiting process */
		if (ours && sd->got_hcint) {
			sd_trace(("INTR->WAKE\n"));
/* 			sdos = (struct sdos_info *)sd->sdos_info; */
			wake_up_interruptible(&sdos->intr_wait_queue);
		}
		return IRQ_RETVAL(ours);
	}
}

/* Register with Linux for interrupts */
int
sdstd_register_irq(sdioh_info_t *sd, uint irq)
{
	sd_trace(("Entering %s: irq == %d\n", __FUNCTION__, irq));
	if (request_irq(irq, sdstd_isr, IRQF_SHARED, "bcmsdstd", sd) < 0) {
		sd_err(("%s: request_irq() failed\n", __FUNCTION__));
		return ERROR;
	}
	return SUCCESS;
}

/* Free Linux irq */
void
sdstd_free_irq(uint irq, sdioh_info_t *sd)
{
	free_irq(irq, sd);
}

/* Map Host controller registers */

uint32 *
sdstd_reg_map(osl_t *osh, int32 addr, int size)
{
	return (uint32 *)REG_MAP(addr, size);
}

void
sdstd_reg_unmap(osl_t *osh, int32 addr, int size)
{
	REG_UNMAP((void*)(uintptr)addr);
}

int
sdstd_osinit(sdioh_info_t *sd)
{
	struct sdos_info *sdos;

	sdos = (struct sdos_info*)MALLOC(sd->osh, sizeof(struct sdos_info));
	sd->sdos_info = (void*)sdos;
	if (sdos == NULL)
		return BCME_NOMEM;

	sdos->sd = sd;
	spin_lock_init(&sdos->lock);
	init_waitqueue_head(&sdos->intr_wait_queue);
	return BCME_OK;
}


/* initilize tuning related OS structures */
void
sdstd_3_osinit_tuning(sdioh_info_t *sd)
{
	struct sdos_info *sdos = (struct sdos_info *)sd->sdos_info;

	sd_trace(("%s Enter\n", __FUNCTION__));
	/* initialize timer and tasklet for tuning */
	init_timer(&sdos->tuning_timer);
	sdos->tuning_timer.data = (ulong)sdos;
	sdos->tuning_timer.function = sdstd_3_tuning_timer;
	sdos->tuning_timer_exp = 2 * (sdstd_3_get_tuning_exp(sdos->sd));

	sdos->timer_enab = TRUE;

	tasklet_init(&sdos->tuning_tasklet, sdstd_3_ostasklet, (ulong)sdos);
	if (sdos->tuning_timer_exp) {
		sdos->tuning_timer.expires = jiffies +	sdos->tuning_timer_exp * HZ;
		add_timer(&sdos->tuning_timer);
	}
}

/* finalize tuning related OS structures */
void
sdstd_3_osclean_tuning(sdioh_info_t *sd)
{
	struct sdos_info *sdos = (struct sdos_info *)sd->sdos_info;

	printf("%s Enter\n", __FUNCTION__);
	if (sdos->timer_enab == TRUE) {
		/* disable timer if it was running */
		del_timer_sync(&sdos->tuning_timer);
		sdos->timer_enab = FALSE;
	}

	tasklet_kill(&sdos->tuning_tasklet);
}

static void
sdstd_3_ostasklet(ulong data)
{
	struct sdos_info *sdos = (struct sdos_info *)data;
	int tune_state = sdstd_3_get_tune_state(sdos->sd);
	int data_state = sdstd_3_get_data_state(sdos->sd);
	if ((tune_state == TUNING_START) || (tune_state == TUNING_ONGOING) ||
		(tune_state == TUNING_START_AFTER_DAT)) {
		return;
	}
	else if (data_state == DATA_TRANSFER_IDLE)
		sdstd_3_set_tune_state(sdos->sd, TUNING_START);
	else if (data_state == DATA_TRANSFER_ONGOING)
		sdstd_3_set_tune_state(sdos->sd, TUNING_START_AFTER_DAT);
}

static void
sdstd_3_tuning_timer(ulong data)
{
	struct sdos_info *sdos = (struct sdos_info *)data;
/* 	uint8 timeout = 0; */
	unsigned long int_flags;

	sd_trace(("%s: enter\n", __FUNCTION__));
	/* schedule tasklet */
	/* * disable ISR's */
	local_irq_save(int_flags);
	if (sdstd_3_check_and_set_retuning(sdos->sd))
		tasklet_schedule(&sdos->tuning_tasklet);

	/* * enable back ISR's */
	local_irq_restore(int_flags);
}

void sdstd_3_start_tuning(sdioh_info_t *sd)
{
	int tune_state;
	unsigned long int_flags;
	struct sdos_info *sdos = (struct sdos_info *)sd->sdos_info;

	sd_trace(("%s: enter\n", __FUNCTION__));
	/* * disable ISR's */
	local_irq_save(int_flags);

	/* disable timer if it was running */
	if (sdos->timer_enab == TRUE) {
			del_timer_sync(&sdos->tuning_timer);
	}

	tune_state = sdstd_3_get_tune_state(sd);

	if (tune_state == TUNING_ONGOING) {
		/* do nothing */
		local_irq_restore(int_flags);
		goto exit;
	}
	/* change state */
	sdstd_3_set_tune_state(sd, TUNING_ONGOING);
	/* * enable ISR's */
	local_irq_restore(int_flags);
	sdstd_3_clk_tuning(sd, sdstd_3_get_uhsi_clkmode(sd));
	/* * disable ISR's */
	local_irq_save(int_flags);
	sdstd_3_set_tune_state(sd, TUNING_IDLE);
	/* * enable ISR's */
	local_irq_restore(int_flags);

	/* enable retuning intrrupt */
	sdstd_3_enable_retuning_int(sd);

	/* start retuning timer if enabled */
	if ((sdos->tuning_timer_exp) && (sdos->timer_enab)) {
		if (sd->sd3_tuning_reqd) {
			sdos->tuning_timer.expires = jiffies +  sdos->tuning_timer_exp * HZ;
			add_timer(&sdos->tuning_timer);
		}
	}
exit:
	return;

}

void
sdstd_osfree(sdioh_info_t *sd)
{
	struct sdos_info *sdos;
	ASSERT(sd && sd->sdos_info);

	sdos = (struct sdos_info *)sd->sdos_info;
	MFREE(sd->osh, sdos, sizeof(struct sdos_info));
}

/* Interrupt enable/disable */
SDIOH_API_RC
sdioh_interrupt_set(sdioh_info_t *sd, bool enable)
{
	ulong flags;
	struct sdos_info *sdos;

	sd_trace(("%s: %s\n", __FUNCTION__, enable ? "Enabling" : "Disabling"));

	sdos = (struct sdos_info *)sd->sdos_info;
	ASSERT(sdos);

	if (!(sd->host_init_done && sd->card_init_done)) {
		sd_err(("%s: Card & Host are not initted - bailing\n", __FUNCTION__));
		return SDIOH_API_RC_FAIL;
	}

	if (enable && !(sd->intr_handler && sd->intr_handler_arg)) {
		sd_err(("%s: no handler registered, will not enable\n", __FUNCTION__));
		return SDIOH_API_RC_FAIL;
	}

	/* Ensure atomicity for enable/disable calls */
	spin_lock_irqsave(&sdos->lock, flags);

	sd->client_intr_enabled = enable;
	if (enable && !sd->lockcount)
		sdstd_devintr_on(sd);
	else
		sdstd_devintr_off(sd);

	spin_unlock_irqrestore(&sdos->lock, flags);

	return SDIOH_API_RC_SUCCESS;
}

/* Protect against reentrancy (disable device interrupts while executing) */
void
sdstd_lock(sdioh_info_t *sd)
{
	ulong flags;
	struct sdos_info *sdos;
	int    wait_count = 0;

	sdos = (struct sdos_info *)sd->sdos_info;
	ASSERT(sdos);

	sd_trace(("%s: %d\n", __FUNCTION__, sd->lockcount));

	spin_lock_irqsave(&sdos->lock, flags);
	while (sd->lockcount)
	{
	    spin_unlock_irqrestore(&sdos->lock, flags);
	    yield();
		spin_lock_irqsave(&sdos->lock, flags);
		if (++wait_count == 25000) {
		    if (!(sd->lockcount == 0)) {
			sd_err(("%s: ERROR: sd->lockcount == 0\n", __FUNCTION__));
		    }
		}
	}
	if (wait_count)
		printk("sdstd_lock: wait count = %d\n", wait_count);
	sdstd_devintr_off(sd);
	sd->lockcount++;
	spin_unlock_irqrestore(&sdos->lock, flags);
}

/* Enable client interrupt */
void
sdstd_unlock(sdioh_info_t *sd)
{
	ulong flags;
	struct sdos_info *sdos;

	sd_trace(("%s: %d, %d\n", __FUNCTION__, sd->lockcount, sd->client_intr_enabled));
	ASSERT(sd->lockcount > 0);

	sdos = (struct sdos_info *)sd->sdos_info;
	ASSERT(sdos);

	spin_lock_irqsave(&sdos->lock, flags);
	if (--sd->lockcount == 0 && sd->client_intr_enabled) {
		sdstd_devintr_on(sd);
	}
	spin_unlock_irqrestore(&sdos->lock, flags);
}

void
sdstd_waitlockfree(sdioh_info_t *sd)
{
	if (sd->lockcount) {
		printk("wait lock free\n");
		while (sd->lockcount)
		{
		    yield();
		}
	}
}


/* Returns 0 for success, -1 for interrupted, -2 for timeout */
int
sdstd_waitbits(sdioh_info_t *sd, uint16 norm, uint16 err, bool yield, uint16 *bits)
{
	struct sdos_info *sdos;
	int rc = 0;

	sdos = (struct sdos_info *)sd->sdos_info;

#ifndef BCMSDYIELD
	ASSERT(!yield);
#endif
	sd_trace(("%s: int 0x%02x err 0x%02x yield %d canblock %d\n",
	          __FUNCTION__, norm, err, yield, BLOCKABLE()));

	/* Clear the "interrupt happened" flag and last intrstatus */
	sd->got_hcint = FALSE;
	sd->last_intrstatus = 0;

#ifdef BCMSDYIELD
	if (yield && BLOCKABLE()) {
		/* Enable interrupts, wait for the indication, then disable */
		sdstd_intrs_on(sd, norm, err);
		rc = wait_event_interruptible_timeout(sdos->intr_wait_queue,
		                                      (sd->got_hcint),
		                                      SDSTD_WAITBITS_TIMEOUT);
		if (rc < 0)
			rc = -1;	/* interrupted */
		else if (rc == 0)
			rc = -2;	/* timeout */
		else
			rc = 0;		/* success */
		sdstd_intrs_off(sd, norm, err);
	} else
#endif /* BCMSDYIELD */
	{
		sdstd_spinbits(sd, norm, err);
	}

	sd_trace(("%s: last_intrstatus 0x%04x\n", __FUNCTION__, sd->last_intrstatus));

	*bits = sd->last_intrstatus;

	return rc;
}
