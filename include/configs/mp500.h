/* SPDX-License-Identifier: GPL-2.0+ */
/*
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include "mx6_common.h"

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(10 * 1024 * 1024)

#undef CONFIG_MXC_GPT_HCLK

#define CONFIG_EXTRA_ENV_SETTINGS \
	"console=ttymxc0\0" \
	"fdt_addr=0x28000000\0" \
	"kernel_addr=0x20800000\0" \
	"fdt_high=0x3c000000\0" \
	"initrd_high=0xffffffff\0" \
	"update_uboot=dhcp u-boot.bin; sf probe;" \
	"sf update ${loadaddr} 0x150000 0xc0000\0"

/* FLASH and environment organization */
#define CONFIG_ENV_SIZE			SZ_64K
#define CONFIG_ENV_OFFSET		(9 * SZ_64K)
#define CONFIG_ENV_SECT_SIZE		SZ_64K

/* Miscellaneous configurable options */
#define CONFIG_SYS_MEMTEST_START       0x10000000
#define CONFIG_SYS_MEMTEST_END	       0x10010000
#define CONFIG_SYS_MEMTEST_SCRATCH     0x10800000

/* Physical Memory Map */
#define PHYS_SDRAM		       MMDC0_ARB_BASE_ADDR

#define CONFIG_SYS_SDRAM_BASE	       PHYS_SDRAM
#define CONFIG_SYS_INIT_RAM_ADDR       IRAM_BASE_ADDR
#define CONFIG_SYS_INIT_RAM_SIZE       IRAM_SIZE

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#endif	       /* __CONFIG_H */
