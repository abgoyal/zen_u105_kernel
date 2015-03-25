/*
 * arch/arm/mm/cache-l2x0.c - L210/L220 cache controller support
 *
 * Copyright (C) 2007 ARM Limited
 * Copyright (C) 2011, Red Bend Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#include <asm/cacheflush.h>
#include <asm/hardware/cache-l2x0.h>

#if defined (CONFIG_NKERNEL) && defined (CONFIG_ARCH_SC8825)
#include <mach/arch_lock.h>
#endif

#define CACHE_LINE_SIZE		32

static void __iomem *l2x0_base;
static uint32_t aux_ctrl_save;
static uint32_t data_latency_ctrl;
static uint32_t prefetch_ctrl;
static uint32_t pwr_ctrl;

static DEFINE_SPINLOCK(l2x0_lock);
static uint32_t l2x0_way_mask;	/* Bitmask of active ways */
static uint32_t l2x0_size;
static u32 l2x0_cache_id;
static unsigned int l2x0_sets;
static unsigned int l2x0_ways;

#if defined (CONFIG_NKERNEL) && defined (CONFIG_ARCH_SC8825)
static void l2x0_mp_lock(unsigned long *flags, unsigned long *hw_flags)
{
	local_irq_save(*flags);
	*hw_flags = hw_local_irq_save();	/* prevent from OS preemption */
	spin_lock(&l2x0_lock);
	arch_hwlock_fast(HWLOCK_CACHE);
}

static void l2x0_mp_unlock(unsigned long *flags, unsigned long *hw_flags)
{
	arch_hwunlock_fast(HWLOCK_CACHE);
	spin_unlock(&l2x0_lock);
	hw_local_irq_restore(*hw_flags);
	local_irq_restore(*flags);
}

#if defined(spin_lock_irqsave)
#undef spin_lock_irqsave
#endif
#define spin_lock_irqsave(X,Y) {}
#define spin_unlock_irqrestore(X,Y) {}

#else
#define l2x0_mp_lock(X,Y) NULL
#define l2x0_mp_unlock(X,Y) NULL
#endif

static inline bool is_pl310_rev(int rev)
{
	return (l2x0_cache_id &
		(L2X0_CACHE_ID_PART_MASK | L2X0_CACHE_ID_REV_MASK)) ==
			(L2X0_CACHE_ID_PART_L310 | rev);
}

static inline void cache_wait_way(void __iomem *reg, unsigned long mask)
{
	/* wait for cache operation by line or way to complete */
	while (readl_relaxed(reg) & mask)
		;
}

#ifdef CONFIG_CACHE_PL310
static inline void cache_wait(void __iomem *reg, unsigned long mask)
{
	/* cache operations by line are atomic on PL310 */
}
#else
#define cache_wait	cache_wait_way
#endif

static inline void cache_sync(void)
{
	void __iomem *base = l2x0_base;

#ifdef CONFIG_ARM_ERRATA_753970
	/* write to an unmmapped register */
	writel_relaxed(0, base + L2X0_DUMMY_REG);
#else
	writel_relaxed(0, base + L2X0_CACHE_SYNC);
#endif
	cache_wait(base + L2X0_CACHE_SYNC, 1);
}

static inline void l2x0_clean_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_LINE_PA);
}

static inline void l2x0_inv_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_INV_LINE_PA);
}

#if defined(CONFIG_PL310_ERRATA_588369) || defined(CONFIG_PL310_ERRATA_727915)

#define debug_writel(val)	outer_cache.set_debug(val)

static void l2x0_set_debug(unsigned long val)
{
	writel_relaxed(val, l2x0_base + L2X0_DEBUG_CTRL);
}
#else
/* Optimised out for non-errata case */
static inline void debug_writel(unsigned long val)
{
}

#define l2x0_set_debug	NULL
#endif

#ifdef CONFIG_PL310_ERRATA_588369
static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;

	/* Clean by PA followed by Invalidate by PA */
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_LINE_PA);
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_INV_LINE_PA);
}
#else

static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_INV_LINE_PA);
}
#endif

static void l2x0_cache_sync(void)
{
	unsigned long flags;
#ifdef CONFIG_NKERNEL
	unsigned long hw_flags;
#endif
	spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
	l2x0_mp_lock(&flags,&hw_flags);
#endif
	cache_sync();
#ifdef CONFIG_NKERNEL
	l2x0_mp_unlock(&flags,&hw_flags);
#endif
	spin_unlock_irqrestore(&l2x0_lock, flags);
}

#ifdef CONFIG_PL310_ERRATA_727915
static void l2x0_for_each_set_way(void __iomem *reg)
{
	int set;
	int way;
	unsigned long flags;
#ifdef CONFIG_NKERNEL
	unsigned long hw_flags;
#endif

	for (way = 0; way < l2x0_ways; way++) {
		spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
		l2x0_mp_lock(&flags,&hw_flags);
#endif
		for (set = 0; set < l2x0_sets; set++)
			writel_relaxed((way << 28) | (set << 5), reg);
		cache_sync();
#ifdef CONFIG_NKERNEL
		l2x0_mp_unlock(&flags,&hw_flags);
#endif
		spin_unlock_irqrestore(&l2x0_lock, flags);
	}
}
#endif

static void __l2x0_flush_all(void)
{
	debug_writel(0x03);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_CLEAN_INV_WAY);
	cache_wait_way(l2x0_base + L2X0_CLEAN_INV_WAY, l2x0_way_mask);
	cache_sync();
	debug_writel(0x00);
}

static void l2x0_flush_all(void)
{
	unsigned long flags;
#ifdef CONFIG_NKERNEL
	unsigned long hw_flags;
#endif

#ifdef CONFIG_PL310_ERRATA_727915
	if (is_pl310_rev(REV_PL310_R2P0)) {
		l2x0_for_each_set_way(l2x0_base + L2X0_CLEAN_INV_LINE_IDX);
		return;
	}
#endif

	/* clean all ways */
	spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
	l2x0_mp_lock(&flags,&hw_flags);
#endif
	__l2x0_flush_all();

#ifdef CONFIG_NKERNEL
	l2x0_mp_unlock(&flags,&hw_flags);
#endif
	spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_clean_all(void)
{
	unsigned long flags;
#ifdef CONFIG_NKERNEL
	unsigned long hw_flags;
#endif

#ifdef CONFIG_PL310_ERRATA_727915
	if (is_pl310_rev(REV_PL310_R2P0)) {
		l2x0_for_each_set_way(l2x0_base + L2X0_CLEAN_LINE_IDX);
		return;
	}
#endif

	/* clean all ways */
	spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
	l2x0_mp_lock(&flags,&hw_flags);
#endif
	debug_writel(0x03);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_CLEAN_WAY);
	cache_wait_way(l2x0_base + L2X0_CLEAN_WAY, l2x0_way_mask);
	cache_sync();
	debug_writel(0x00);
#ifdef CONFIG_NKERNEL
	l2x0_mp_unlock(&flags,&hw_flags);
#endif
	spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_inv_all(void)
{
	unsigned long flags;
#ifdef CONFIG_NKERNEL
	unsigned long hw_flags;
#endif
	/* invalidate all ways */
	spin_lock_irqsave(&l2x0_lock, flags);
	/* Invalidating when L2 is enabled is a nono */
	BUG_ON(readl(l2x0_base + L2X0_CTRL) & 1);
#ifdef CONFIG_NKERNEL
	l2x0_mp_lock(&flags,&hw_flags);
#endif
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_INV_WAY);
	cache_wait_way(l2x0_base + L2X0_INV_WAY, l2x0_way_mask);
	cache_sync();
#ifdef CONFIG_NKERNEL
	l2x0_mp_unlock(&flags,&hw_flags);
#endif
	spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_inv_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2x0_base;
	unsigned long flags;
#ifdef CONFIG_NKERNEL
	unsigned long hw_flags;
#endif
	spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
	l2x0_mp_lock(&flags,&hw_flags);
#endif
	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		debug_writel(0x03);
		l2x0_flush_line(start);
		debug_writel(0x00);
		start += CACHE_LINE_SIZE;
	}

	if (end & (CACHE_LINE_SIZE - 1)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		debug_writel(0x03);
		l2x0_flush_line(end);
		debug_writel(0x00);
	}

	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_inv_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
#ifdef CONFIG_NKERNEL
			l2x0_mp_unlock(&flags,&hw_flags);
#endif
			spin_unlock_irqrestore(&l2x0_lock, flags);
			spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
			l2x0_mp_lock(&flags,&hw_flags);
#endif
		}
	}
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	cache_sync();
#ifdef CONFIG_NKERNEL
	l2x0_mp_unlock(&flags,&hw_flags);
#endif
	spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_clean_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2x0_base;
	unsigned long flags;
#ifdef CONFIG_NKERNEL
	unsigned long hw_flags;
#endif

	if ((end - start) >= l2x0_size) {
		l2x0_clean_all();
		return;
	}

	spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
	l2x0_mp_lock(&flags,&hw_flags);
#endif
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_clean_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
#ifdef CONFIG_NKERNEL
			l2x0_mp_unlock(&flags,&hw_flags);
#endif
			spin_unlock_irqrestore(&l2x0_lock, flags);
			spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
			l2x0_mp_lock(&flags,&hw_flags);
#endif
		}
	}
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	cache_sync();
#ifdef CONFIG_NKERNEL
	l2x0_mp_unlock(&flags,&hw_flags);
#endif
	spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_flush_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2x0_base;
	unsigned long flags;
#ifdef CONFIG_NKERNEL
	unsigned long hw_flags;
#endif

	if ((end - start) >= l2x0_size) {
		l2x0_flush_all();
		return;
	}

	spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
	l2x0_mp_lock(&flags,&hw_flags);
#endif
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		debug_writel(0x03);
		while (start < blk_end) {
			l2x0_flush_line(start);
			start += CACHE_LINE_SIZE;
		}
		debug_writel(0x00);

		if (blk_end < end) {
#ifdef CONFIG_NKERNEL
			l2x0_mp_unlock(&flags,&hw_flags);
#endif
			spin_unlock_irqrestore(&l2x0_lock, flags);
			spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
			l2x0_mp_lock(&flags,&hw_flags);
#endif
		}
	}
	cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
	cache_sync();
#ifdef CONFIG_NKERNEL
	l2x0_mp_unlock(&flags,&hw_flags);
#endif
	spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_disable(void)
{
	unsigned long flags;
#ifdef CONFIG_NKERNEL
	unsigned long hw_flags;
#endif
	spin_lock_irqsave(&l2x0_lock, flags);
#ifdef CONFIG_NKERNEL
	l2x0_mp_lock(&flags,&hw_flags);
#endif
	__l2x0_flush_all();
	writel_relaxed(0, l2x0_base + L2X0_CTRL);
	dsb();
#ifdef CONFIG_NKERNEL
	l2x0_mp_unlock(&flags,&hw_flags);
#endif
	spin_unlock_irqrestore(&l2x0_lock, flags);
}

void __init l2x0_init(void __iomem *base, __u32 aux_val, __u32 aux_mask)
{
	__u32 aux;
	__u32 way_size = 0;
	const char *type;

	l2x0_base = base;

	l2x0_cache_id = readl_relaxed(l2x0_base + L2X0_CACHE_ID);
	aux = readl_relaxed(l2x0_base + L2X0_AUX_CTRL);

	aux &= aux_mask;
	aux |= aux_val;

	/* Determine the number of ways */
	switch (l2x0_cache_id & L2X0_CACHE_ID_PART_MASK) {
	case L2X0_CACHE_ID_PART_L310:
		if (aux & (1 << 16))
			l2x0_ways = 16;
		else
			l2x0_ways = 8;
		type = "L310";
		break;
	case L2X0_CACHE_ID_PART_L210:
		l2x0_ways = (aux >> 13) & 0xf;
		type = "L210";
		break;
	default:
		/* Assume unknown chips have 8 ways */
		l2x0_ways = 8;
		type = "L2x0 series";
		break;
	}

	l2x0_way_mask = (1 << l2x0_ways) - 1;

	/*
	 * L2 cache Size =  Way size * Number of ways
	 */
	way_size = (aux & L2X0_AUX_CTRL_WAY_SIZE_MASK) >> 17;
	way_size = SZ_1K << (way_size + 3);
	l2x0_size = l2x0_ways * way_size;
	l2x0_sets = way_size / CACHE_LINE_SIZE;

	/*
	 * Check if l2x0 controller is already enabled.
	 * If you are booting from non-secure mode
	 * accessing the below registers will fault.
	 */
	if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & 1)) {

/* FIXME: why is this BUG() needed? */
#if 0
#ifdef CONFIG_NKERNEL
		printk(KERN_CRIT "l2x0 controller should be enabled\n");
		BUG();
#endif
#endif
		/* l2x0 controller is disabled */
		writel_relaxed(aux, l2x0_base + L2X0_AUX_CTRL);

		l2x0_inv_all();

		/* enable L2X0 */
		writel_relaxed(1, l2x0_base + L2X0_CTRL);
	}

	outer_cache.inv_range = l2x0_inv_range;
	outer_cache.clean_range = l2x0_clean_range;
	outer_cache.flush_range = l2x0_flush_range;
	outer_cache.sync = l2x0_cache_sync;
	outer_cache.flush_all = l2x0_flush_all;
	outer_cache.inv_all = l2x0_inv_all;
	outer_cache.disable = l2x0_disable;
	outer_cache.set_debug = l2x0_set_debug;

	printk(KERN_INFO "%s cache controller enabled\n", type);
	printk(KERN_INFO "l2x0: %d ways, CACHE_ID 0x%08x, AUX_CTRL 0x%08x, Cache size: %d B\n",
			l2x0_ways, l2x0_cache_id, aux, l2x0_size);
}

/* add for powermanager control l2 cache*/
void l2x0_suspend(void)
{
	u32 l2x0_revision = readl_relaxed(l2x0_base + L2X0_CACHE_ID) & 0x3f;
#ifdef CONFIG_ARCH_SC8825
	u32 enabled = readl_relaxed(l2x0_base + L2X0_CTRL) & 0x1;
	if(!enabled)
		return;
#endif
	/* Save aux control register value */
	aux_ctrl_save = readl_relaxed(l2x0_base + L2X0_AUX_CTRL);
	data_latency_ctrl = readl_relaxed(l2x0_base + L2X0_DATA_LATENCY_CTRL);
	if (l2x0_revision >= 0x4) {
		prefetch_ctrl = readl_relaxed(l2x0_base + L2X0_PREFETCH_CTRL);
		if (l2x0_revision >= 0x5)
			pwr_ctrl = readl_relaxed(l2x0_base + L2X0_POWER_CTRL);
	}
#ifdef CONFIG_ARCH_SC8825
	/* Disable the cache */
	writel_relaxed(0, l2x0_base + L2X0_CTRL);
	/* Flush all cache */
	__l2x0_flush_all();
#else
	/* Flush all cache */
	 __l2x0_flush_all();
	/* Disable the cache */
	writel_relaxed(0, l2x0_base + L2X0_CTRL);
#endif
	/* Memory barrier */
	dsb();
}

void l2x0_resume(int collapsed)
{
	if (collapsed) {
		u32 l2x0_revision = readl_relaxed(l2x0_base + L2X0_CACHE_ID) & 0x3f;

		/* Disable the cache */
		writel_relaxed(0, l2x0_base + L2X0_CTRL);

		if (l2x0_revision >= 0x4) {
			writel_relaxed(prefetch_ctrl,l2x0_base + L2X0_PREFETCH_CTRL);
			if (l2x0_revision >= 0x5)
				writel_relaxed(pwr_ctrl,l2x0_base + L2X0_POWER_CTRL);
		}

		writel_relaxed(data_latency_ctrl, l2x0_base +
				L2X0_DATA_LATENCY_CTRL);

		/* Restore aux control register value */
		writel_relaxed(aux_ctrl_save, l2x0_base + L2X0_AUX_CTRL);
	}

	/* Invalidate the cache */
	l2x0_inv_all();
	/* Enable the cache */
	writel_relaxed(1, l2x0_base + L2X0_CTRL);

	mb();
}
