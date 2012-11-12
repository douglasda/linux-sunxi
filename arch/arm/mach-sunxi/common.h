/*
 * arch/arm/mach-sunxi/common.h
 *
 * Copyright 2012 Stefan Roese <sr@denx.de>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ARCH_ARM_MACH_SUNXI_COMMON_H
#define __ARCH_ARM_MACH_SUNXI_COMMON_H

extern void __iomem *sunxi_timer_base;

void __init sunxi_timer_init(void);
void __init sunxi_init_irq(void);
void sunxi_handle_irq(struct pt_regs *regs);

#endif
