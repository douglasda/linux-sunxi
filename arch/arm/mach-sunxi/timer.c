/*
 * arch/arm/mach-sunxi/timer.c
 *
 * Copyright 2012 Stefan Roese <sr@denx.de>
 *
 * Loosly based on the sunxi source code provided by Allwinner:
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Benn Huang <benn@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <asm/mach/time.h>

#define SYS_TIMER_SCAL		16	/* timer clock source pre-divsion */
#define SYS_TIMER_CLKSRC	24000000	/* timer clock source */
#define TMR_INTER_VAL		(SYS_TIMER_CLKSRC / (SYS_TIMER_SCAL * HZ))

#define SW_TIMER_INT_CTL_REG	0x00
#define SW_TIMER_INT_STA_REG	0x04
#define SW_TIMER0_CTL_REG	0x10
#define SW_TIMER0_INTVAL_REG	0x14
#define SW_TIMER0_CNTVAL_REG	0x18

void __iomem *sunxi_timer_base;

static void timer_set_mode(enum clock_event_mode mode,
			   struct clock_event_device *clk)
{
	u32 ctrl;

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		pr_info("timer0: Periodic Mode\n");
		/* Interval (999 + 1) */
		writel(TMR_INTER_VAL, sunxi_timer_base + SW_TIMER0_INTVAL_REG);
		ctrl = readl(sunxi_timer_base + SW_TIMER0_CTL_REG);
		ctrl &= ~(1 << 7);	/* Continuous mode */
		ctrl |= 1;		/* Enable this timer */
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		pr_info("timer0: Oneshot Mode\n");
		ctrl = readl(sunxi_timer_base + SW_TIMER0_CTL_REG);
		ctrl |= (1 << 7);	/* Single mode */
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		ctrl = readl(sunxi_timer_base + SW_TIMER0_CTL_REG);
		ctrl &= ~(1 << 0);	/* Disable timer0 */
		break;
	}

	writel(ctrl, sunxi_timer_base + SW_TIMER0_CTL_REG);
}

/* Useless when periodic mode */
static int timer_set_next_event(unsigned long evt,
				struct clock_event_device *unused)
{
	u32 ctrl;

	/* Clear any pending before continue */
	ctrl = readl(sunxi_timer_base + SW_TIMER0_CTL_REG);
	writel(evt, sunxi_timer_base + SW_TIMER0_CNTVAL_REG);
	ctrl |= (1 << 1);
	writel(ctrl, sunxi_timer_base + SW_TIMER0_CTL_REG);
	writel(ctrl | 0x1, sunxi_timer_base + SW_TIMER0_CTL_REG);

	return 0;
}

static struct clock_event_device timer0_clockevent = {
	.name = "sunxi_timer0",
	.shift = 32,
	.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_mode = timer_set_mode,
	.set_next_event = timer_set_next_event,
};


static irqreturn_t sunxi_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = (struct clock_event_device *)dev_id;

	/* Acknowlege the interrupt */
	writel(0x1, sunxi_timer_base + SW_TIMER_INT_STA_REG);

	/*
	 * timer_set_next_event will be called only in ONESHOT mode
	 */
	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction sunxi_timer_irq = {
	.name = "sunxi_timer0",
	.flags = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler = sunxi_timer_interrupt,
	.dev_id = &timer0_clockevent,
};

static struct of_device_id sunxi_timer_ids[] = {
	{ .compatible = "allwinner,sunxi-timer" },
	{ }
};

void __init sunxi_timer_init(void)
{
	struct device_node *np;
	int ret;
	u32 val = 0;
	int timer_irq;

	np = of_find_matching_node(NULL, sunxi_timer_ids);
	if (!np) {
		pr_err("%s: Timer description missing from Device Tree\n",
		       __func__);
		return;
	}

	sunxi_timer_base = of_iomap(np, 0);
	if (!sunxi_timer_base) {
		pr_err("%s: Missing iobase description in Device Tree\n",
		       __func__);
		of_node_put(np);
		return;
	}

	timer_irq = irq_of_parse_and_map(np, 0);
	if (!timer_irq) {
		pr_err("%s: Missing irq description in Device Tree\n",
		       __func__);
		of_node_put(np);
		return;
	}

	writel(TMR_INTER_VAL, sunxi_timer_base + SW_TIMER0_INTVAL_REG);

	/* Set clock sourch to HOSC, 16 pre-division */
	val = readl(sunxi_timer_base + SW_TIMER0_CTL_REG);
	val &= ~(0x07 << 4);
	val &= ~(0x03 << 2);
	val |= (4 << 4) | (1 << 2);
	writel(val, sunxi_timer_base + SW_TIMER0_CTL_REG);

	/* Set mode to auto reload */
	val = readl(sunxi_timer_base + SW_TIMER0_CTL_REG);
	val |= (1 << 1);
	writel(val, sunxi_timer_base + SW_TIMER0_CTL_REG);

	ret = setup_irq(timer_irq, &sunxi_timer_irq);
	if (ret)
		pr_warn("Failed to setup irq %d\n", timer_irq);

	/* Enable time0 interrupt */
	val = readl(sunxi_timer_base + SW_TIMER_INT_CTL_REG);
	val |= (1 << 0);
	writel(val, sunxi_timer_base + SW_TIMER_INT_CTL_REG);

	timer0_clockevent.mult = div_sc(SYS_TIMER_CLKSRC / SYS_TIMER_SCAL,
					NSEC_PER_SEC, timer0_clockevent.shift);
	timer0_clockevent.max_delta_ns =
		clockevent_delta2ns(0xff, &timer0_clockevent);
	timer0_clockevent.min_delta_ns =
		clockevent_delta2ns(0x1, &timer0_clockevent);
	timer0_clockevent.cpumask = cpumask_of(0);

	if (of_machine_is_compatible("allwinner,sun4i"))
		timer0_clockevent.rating = 100;
	else
		timer0_clockevent.rating = 300;

	clockevents_register_device(&timer0_clockevent);
}
