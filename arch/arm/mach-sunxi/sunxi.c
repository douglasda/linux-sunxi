/*
 * arch/arm/mach-sunxi/sunxi.c
 *
 * Allwinner sunxi machines common source file
 *
 * Copyright 2012 Stefan Roese <sr@denx.de>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/delay.h>

#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>

#include "common.h"

#define WATCH_DOG_CTRL_REG	0x94

/*
 * Following will create 16MB static virtual/physical mappings
 * PHYSICAL		VIRTUAL
 * 0x00000000		0xf0000000
 */
#define VA_SUNXI_ML_CPU_BASE		UL(0xf1c00000)
#define SUNXI_ML_CPU_BASE		UL(0x01c00000)

struct map_desc sunxi_io_desc[] __initdata = {
	{
		.virtual	= VA_SUNXI_ML_CPU_BASE,
		.pfn		= __phys_to_pfn(SUNXI_ML_CPU_BASE),
		.length		= SZ_1M + SZ_2M,
		.type		= MT_DEVICE
	},
};

/* This will create static memory mapping for selected devices */
void __init sunxi_map_io(void)
{
	iotable_init(sunxi_io_desc, ARRAY_SIZE(sunxi_io_desc));
}

static void sunxi_restart(char mode, const char *cmd)
{
	/* Use watchdog to reset system */
	writel(0, sunxi_timer_base + WATCH_DOG_CTRL_REG);
	udelay(100);

	if (of_machine_is_compatible("allwinner,sun4i")) {
		writel(3, sunxi_timer_base + WATCH_DOG_CTRL_REG);
		while (1)
			;
	} else {
		writel(2, sunxi_timer_base + WATCH_DOG_CTRL_REG);
		while (1) {
			udelay(100);
			writel(readl(sunxi_timer_base + WATCH_DOG_CTRL_REG) | 1,
			       sunxi_timer_base + WATCH_DOG_CTRL_REG);
		}
	}
}

static void __init sunxi_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static const char *sunxi_dt_match[] __initdata = {
	"allwinner,sun4i",
	"allwinner,sun5i",
	NULL
};

struct sys_timer sunxi_timer = {
	.init = sunxi_timer_init,
};

DT_MACHINE_START(SOCFPGA, "Allwinner sunxi (sun4i/sun5i)")
	.map_io         = sunxi_map_io,
	.init_irq	= sunxi_init_irq,
	.handle_irq     = sunxi_handle_irq,
	.timer		= &sunxi_timer,
	.init_machine	= sunxi_init,
	.restart	= sunxi_restart,
	.dt_compat	= sunxi_dt_match,
MACHINE_END
