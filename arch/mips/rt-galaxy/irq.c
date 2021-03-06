/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 */

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/irq_cpu.h>
#include <asm/processor.h>
#include <asm/time.h>

#include <rt-galaxy-board.h>
#include <rt-galaxy-io.h>
#include <rt-galaxy-irq.h>

extern void rtgalaxy_sb2_setup(void);

static int rtgalaxy_internal_irq_dispatch(void)
{
	u32 pending;
	static int i;

	pending = rtgalaxy_reg_readl(RTGALAXY_MISC_ISR);

	if (!pending)
		return 0;

	while (1) {
		int to_call = i;

		i = (i + 1) & 0x1f;
		if (pending & (1 << to_call)) {
			do_IRQ(to_call + RTGALAXY_INTERNAL_IRQ_BASE);
			break;
		}
	}
	return 1;
}

void plat_irq_dispatch(void)
{
	u32 cause;

	cause = read_c0_cause() & read_c0_status() & CAUSEF_IP;

	clear_c0_status(cause);

	if (cause & CAUSEF_IP7)
		do_IRQ(7);
	if (cause & CAUSEF_IP2)
		do_IRQ(2);
	if ((cause & CAUSEF_IP3) && (!rtgalaxy_internal_irq_dispatch()))
		do_IRQ(3);
	if (cause & CAUSEF_IP4)
		do_IRQ(4);
	if (cause & CAUSEF_IP5)
		do_IRQ(5);
	if (cause & CAUSEF_IP6)
		do_IRQ(6);
}

static inline void rtgalaxy_internal_irq_mask(unsigned int irq)
{
#if 0
	u32 mask;

	irq -= RTGALAXY_INTERNAL_IRQ_BASE;
	mask = rtgalaxy_reg_readl(RTGALAXY_MISC_UMSK_ISR);
	mask &= ~(1 << irq);
	rtgalaxy_reg_writel(mask, RTGALAXY_MISC_UMSK_ISR);
#endif
}

static void rtgalaxy_internal_irq_unmask(unsigned int irq)
{
#if 0
	u32 mask;

	irq -= RTGALAXY_INTERNAL_IRQ_BASE;
	mask = rtgalaxy_reg_readl(RTGALAXY_MISC_UMSK_ISR);
	mask |= (1 << irq);
	rtgalaxy_reg_writel(mask, RTGALAXY__MISC_UMSK_ISR);
#endif
}

static void rtgalaxy_internal_irq_ack(unsigned int irq)
{
	irq -= RTGALAXY_INTERNAL_IRQ_BASE;
	rtgalaxy_reg_writel((1 << irq), RTGALAXY_MISC_ISR);
}

static void rtgalaxy_internal_irq_mask_ack(unsigned int irq)
{
	rtgalaxy_internal_irq_mask(irq);
	rtgalaxy_internal_irq_ack(irq);
}

static unsigned int rtgalaxy_internal_irq_startup(unsigned int irq)
{
	rtgalaxy_internal_irq_unmask(irq);
	return 0;
}

static struct irq_chip rtgalaxy_internal_irq_chip = {
	.name = "rt-galaxy-irq",
	.startup = rtgalaxy_internal_irq_startup,
	.shutdown = rtgalaxy_internal_irq_mask,
	.ack = rtgalaxy_internal_irq_ack,
	.mask = rtgalaxy_internal_irq_mask,
	.mask_ack = rtgalaxy_internal_irq_mask_ack,
	.unmask = rtgalaxy_internal_irq_unmask,
};

void __init arch_init_irq(void)
{
	int i;

	/* disable RTC interrupts */
	rtgalaxy_reg_writel(0x0000, RTGALAXY_RTC_CR);

	/* clear device interrupts */
	rtgalaxy_reg_writel(0x3ffc, RTGALAXY_MISC_ISR);

	mips_cpu_irq_init();

	for (i = 0; i < 32; ++i)
		set_irq_chip_and_handler(RTGALAXY_INTERNAL_IRQ_BASE + i,
					 &rtgalaxy_internal_irq_chip,
					 handle_level_irq);

	set_c0_status(1 << (RTGALAXY_IRQ_MISC + 8));

	rtgalaxy_sb2_setup();
}
