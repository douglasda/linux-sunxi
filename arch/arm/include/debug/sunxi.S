/*
 *  Copyright (C) 1994-1999 Russell King
 *  Moved from linux/arch/arm/kernel/debug.S by Ben Dooks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define UART_SHIFT 2
#define SW_PA_UART0_IO_BASE               0x01c28000
#define SW_VA_UART0_IO_BASE               0xf1c28000

	.macro	addruart, rp, rv, tmp
	ldr	\rp, =SW_PA_UART0_IO_BASE
	ldr	\rv, =SW_VA_UART0_IO_BASE
	.endm

#include "8250_32.S"
