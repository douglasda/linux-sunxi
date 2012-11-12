/*
 * Allwinner sunxi SoC IRQ handling
 *
 * Copyright 2012 Stefan Roese <sr@denx.de>
 *
 * Loosly based on the sunxi source code provided by Allwinner:
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Benn Huang <benn@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>

#include <asm/mach/arch.h>
#include <asm/exception.h>

/* Interrupt Controller Registers Map */

#define SW_INT_VECTOR_REG		0x00
#define SW_INT_BASE_ADR_REG		0x04
#define SW_INT_PROTECTION_REG		0x08
#define SW_INT_NMI_CTRL_REG		0x0c
#define SW_INT_IRQ_PENDING_REG0		0x10
#define SW_INT_IRQ_PENDING_REG1		0x14
#define SW_INT_IRQ_PENDING_REG2		0x18

#define SW_INT_SELECT_REG0		0x30
#define SW_INT_SELECT_REG1		0x34
#define SW_INT_SELECT_REG2		0x38

#define SW_INT_SELECT_REG0		0x30
#define SW_INT_SELECT_REG1		0x34
#define SW_INT_SELECT_REG2		0x38

#define SW_INT_ENABLE_REG0		0x40
#define SW_INT_ENABLE_REG1		0x44
#define SW_INT_ENABLE_REG2		0x48

#define SW_INT_MASK_REG0		0x50
#define SW_INT_MASK_REG1		0x54
#define SW_INT_MASK_REG2		0x58

#define SW_INT_IRQNO_ENMI		0

static void __iomem *int_base;
static struct irq_domain *sunxi_vic_domain;

static void sunxi_irq_ack(struct irq_data *d)
{
	unsigned int irq = irqd_to_hwirq(d);

	if (irq < 32) {
		writel(readl(int_base + SW_INT_IRQ_PENDING_REG0) | (1 << irq),
		       int_base + SW_INT_IRQ_PENDING_REG0);
	} else if (irq < 64) {
		irq -= 32;
		writel(readl(int_base + SW_INT_IRQ_PENDING_REG1) | (1 << irq),
		       int_base + SW_INT_IRQ_PENDING_REG1);
	} else if (irq < 96) {
		irq -= 64;
		writel(readl(int_base + SW_INT_IRQ_PENDING_REG2) | (1 << irq),
		       int_base + SW_INT_IRQ_PENDING_REG2);
	}
}

void sunxi_irq_mask(struct irq_data *d)
{
	unsigned int irq = irqd_to_hwirq(d);

	if (irq < 32) {
		writel(readl(int_base + SW_INT_ENABLE_REG0) & ~(1 << irq),
		       int_base + SW_INT_ENABLE_REG0);
	} else if (irq < 64) {
		irq -= 32;
		writel(readl(int_base + SW_INT_ENABLE_REG1) & ~(1 << irq),
		       int_base + SW_INT_ENABLE_REG1);
	} else if (irq < 96) {
		irq -= 64;
		writel(readl(int_base + SW_INT_ENABLE_REG2) & ~(1 << irq),
		       int_base + SW_INT_ENABLE_REG2);
	}
}

static void sunxi_irq_unmask(struct irq_data *d)
{
	unsigned int irq = irqd_to_hwirq(d);

	if (irq < 32) {
		writel(readl(int_base + SW_INT_ENABLE_REG0) | (1 << irq),
		       int_base + SW_INT_ENABLE_REG0);

		/* Must clear pending bit when enabled */
		if (irq == SW_INT_IRQNO_ENMI)
			writel((1 << SW_INT_IRQNO_ENMI),
			       int_base + SW_INT_IRQ_PENDING_REG0);
	} else if (irq < 64) {
		irq -= 32;
		writel(readl(int_base + SW_INT_ENABLE_REG1) | (1 << irq),
		       int_base + SW_INT_ENABLE_REG1);
	} else if (irq < 96) {
		irq -= 64;
		writel(readl(int_base + SW_INT_ENABLE_REG2) | (1 << irq),
		       int_base + SW_INT_ENABLE_REG2);
	}
}

static struct irq_chip sunxi_irq_chip = {
	.name		= "sunxi_vic",
	.irq_ack        = sunxi_irq_ack,
	.irq_mask       = sunxi_irq_mask,
	.irq_unmask     = sunxi_irq_unmask,
};

static int sunxi_vic_irq_map(struct irq_domain *h,
			     unsigned int virq, irq_hw_number_t hw)
{
	irq_set_chip_and_handler(virq, &sunxi_irq_chip,
				 handle_level_irq);
	set_irq_flags(virq, IRQF_VALID | IRQF_PROBE);

	return 0;
}

static struct irq_domain_ops sunxi_vic_irq_ops = {
	.map = sunxi_vic_irq_map,
	.xlate = irq_domain_xlate_onecell,
};

static int __init sunxi_vic_of_init(struct device_node *node,
				    struct device_node *parent)
{
	int_base = of_iomap(node, 0);

	BUG_ON(!int_base);

	/* Disable all interrupts */
	writel(0, int_base + SW_INT_ENABLE_REG0);
	writel(0, int_base + SW_INT_ENABLE_REG1);
	writel(0, int_base + SW_INT_ENABLE_REG2);

	/*
	 * Unmask all interrupts now.
	 * Masking and unmasking is done later by toggling
	 * the bits in the enable register.
	 */
	writel(0, int_base + SW_INT_MASK_REG0);
	writel(0, int_base + SW_INT_MASK_REG1);
	writel(0, int_base + SW_INT_MASK_REG2);

	/* Clear/ack all interrupts */
	writel(0xffffffff, int_base + SW_INT_IRQ_PENDING_REG0);
	writel(0xffffffff, int_base + SW_INT_IRQ_PENDING_REG1);
	writel(0xffffffff, int_base + SW_INT_IRQ_PENDING_REG2);

	/* Enable protection mode */
	writel(0x01, int_base + SW_INT_PROTECTION_REG);
	/* Config the external interrupt source type */
	writel(0x00, int_base + SW_INT_NMI_CTRL_REG);

	sunxi_vic_domain = irq_domain_add_linear(node, 3 * 32,
						 &sunxi_vic_irq_ops, NULL);
	if (!sunxi_vic_domain)
		panic("Unable to add sunxi VIC irq domain (DT)\n");

	irq_set_default_host(sunxi_vic_domain);

	return 0;
}

asmlinkage void __exception_irq_entry sunxi_handle_irq(struct pt_regs *regs)
{
	u32 irq;

	irq = readl(int_base + SW_INT_VECTOR_REG) >> 2;
	irq = irq_find_mapping(sunxi_vic_domain, irq);
	handle_IRQ(irq, regs);
}
static const struct of_device_id sunxi_vic_of_match[] __initconst = {
	{ .compatible = "allwinner,sunxi-vic", .data = sunxi_vic_of_init },
	{ },
};

void __init sunxi_init_irq(void)
{
	of_irq_init(sunxi_vic_of_match);
}
