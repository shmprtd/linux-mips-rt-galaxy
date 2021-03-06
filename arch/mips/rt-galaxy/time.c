/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timex.h>

#include <asm/mipsregs.h>
#include <asm/mipsmtregs.h>
#include <asm/hardirq.h>
#include <asm/irq.h>
#include <asm/div64.h>
#include <asm/cpu.h>
#include <asm/time.h>

#include <rt-galaxy-io.h>
#include <rt-galaxy-irq.h>
#include <rt-galaxy-board.h>

/*
 * Estimate CPU frequency.  Sets mips_hpt_frequency as a side-effect
 */
static unsigned int __init estimate_cpu_frequency(void)
{
	unsigned long flags;
	unsigned int count;
	unsigned int cause;

	local_irq_save(flags);

	cause = read_c0_cause();
	write_c0_cause(cause & ~0x08000000);
	rtgalaxy_reg_writel(0, RTGALAXY_TIMR_TC2CR);
	rtgalaxy_reg_writel(0x80000000, RTGALAXY_TIMR_TC2CR);

	write_c0_count(0);

	while (rtgalaxy_reg_readl(RTGALAXY_TIMR_TC2CVR) <
	       (rtgalaxy_info.board->ext_freq / HZ)) ;

	count = read_c0_count();

	write_c0_cause(cause);
	rtgalaxy_reg_writel(0, RTGALAXY_TIMR_TC2CR);

	local_irq_restore(flags);

	count *= HZ;
	count *= 2;
	count += 5000;		/* round */
	count -= count % 10000;

	return count;
}

unsigned int __init get_c0_compare_int(void)
{
	return RTGALAXY_IRQ_TIMER;
}

void __init plat_time_init(void)
{
	unsigned int est_freq;

	est_freq = estimate_cpu_frequency();

	printk("CPU frequency %d.%02d MHz\n", est_freq / 1000000,
	       (est_freq % 1000000) * 100 / 1000000);

	write_c0_count(0);
	write_c0_compare(0xffff);
	write_c0_cause(read_c0_cause() & ~0x08000000);

	mips_hpt_frequency = est_freq;
}
