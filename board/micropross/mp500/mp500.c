// SPDX-License-Identifier: GPL-2.0+
/*
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <miiphy.h>
#include <dm.h>
#include <i2c_eeprom.h>

DECLARE_GLOBAL_DATA_PTR;

#define FEATURE_COMPAT_EXTEND		(1<<0) /* bit is reserved for additional bit fields */
#define FEATURE_COMPAT_HWREV		(1<<1)
#define FEATURE_COMPAT_MAC		(1<<2)
#define FEATURE_COMPAT_SERIAL_NUM	(1<<3)
#define FEATURE_COMPAT_VARIANT_KEY	(1<<4)
#define FEATURE_COMPAT_BOOT_COUNT	(1<<5)

struct eeprom_data
{
	u32	feature_incompat;
	u32	feature_compat;

	u8	hw_rev;
	u8	mac_addr[6];
	u8	variant_key[9];
	u8	serial_num[10];
	u8	reserved1[6];

	u32	checksum;
	u32	reserved_crc_alignment;

	u32	boot_count;
	u32	reserved_remain_unsafe_page;
};

static struct eeprom_data _eeprom;
static struct eeprom_data *eeprom;

int dram_init(void)
{
	gd->ram_size = imx_ddr_size();
	return 0;
}

/*
 * Do not overwrite the console
 * Use always serial for U-Boot console
 */
int overwrite_console(void)
{
	return 1;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	return 0;
}

static int load_eeprom_data(void)
{
	struct udevice *dev;
	int ret;

	ret = uclass_first_device_err(UCLASS_I2C_EEPROM, &dev);
	if (ret)
		return ret;

	ret = i2c_eeprom_read(dev, 0, (u8*)&_eeprom, sizeof(_eeprom));
	if (ret)
		return ret;

	eeprom = &_eeprom;

	return 0;
}

int get_hw_revision(void)
{
	if(!eeprom)
		return -1;

	if(FEATURE_COMPAT_HWREV & eeprom->feature_compat)
		return eeprom->hw_rev;

	return -1;
}

const char* get_mac_addr(void)
{
	u8 *p;
	static char buf[32];

	if(!eeprom)
		return NULL;

	if(!(FEATURE_COMPAT_MAC & eeprom->feature_compat))
		return NULL;

	p = (u8*)&eeprom->mac_addr;
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
		*p, *(p+1), *(p+2), *(p+3), *(p+4), *(p+5));

	return (const char*)&buf;
}

const char* get_variant_key(void)
{
	static char buf[16];

	if(!eeprom)
		return NULL;

	if(!(FEATURE_COMPAT_VARIANT_KEY & eeprom->feature_compat))
		return NULL;

	memset(&buf, 0, sizeof(buf));
	memcpy(&buf, &eeprom->variant_key, 9);
	return (const char*)&buf;
}

const char* get_serial_number(void)
{
	static char buf[10];

	if(!eeprom)
		return NULL;

	if(!(FEATURE_COMPAT_SERIAL_NUM & eeprom->feature_compat))
		return NULL;

	strncpy((char*)&buf, (char*)&eeprom->serial_num, 10);
	return (const char*)&buf;
}

int increment_boot_count(void)
{
	struct udevice *dev;
	int ret = -1 ;

	if(!eeprom)
		return ret;

	ret = uclass_first_device_err(UCLASS_I2C_EEPROM, &dev);
	if (ret)
		return ret;


	if(!(eeprom->feature_compat & FEATURE_COMPAT_BOOT_COUNT))
	{
		eeprom->boot_count = 0;
		eeprom->feature_compat |= FEATURE_COMPAT_BOOT_COUNT;
	}

	eeprom->boot_count++;

	ret = i2c_eeprom_write(dev, offsetof(struct eeprom_data, boot_count),
			       (u8*)&eeprom->boot_count, sizeof(eeprom->boot_count));
	return ret;
}

int get_boot_count(void)
{
	if(!eeprom)
		return -1;

	if(eeprom->feature_compat & FEATURE_COMPAT_BOOT_COUNT)
		return eeprom->boot_count;
	return -1;
}

u32 get_board_rev(void)
{
	u32 cpurev = get_cpu_rev();
	u32 type = ((cpurev >> 12) & 0xff);
	if (type == MXC_CPU_MX6SOLO)
		cpurev = (MXC_CPU_MX6DL) << 12 | (get_cpu_rev() & 0xFFF);

	return cpurev;
}

int misc_init_r(void)
{
	env_set_hex("reset_cause", get_imx_reset_cause());
	if (load_eeprom_data())
		printf("Failed to read eeprom\n");
	else {
		printf("Serial Number:\t%s\n", get_serial_number());
		printf("HW Revision:\t%d\n", get_hw_revision());
		printf("Ethernet MAC:\t%s\n",  get_mac_addr());
		printf("Variant Key:\t%s\n",  get_variant_key());

		if(env_get_yesno("boot_count_en"))
		{
			printf("Boot count:\t%d\n", get_boot_count());
			increment_boot_count();
		}
	       env_set("serial#", get_serial_number());
	       env_set_hex("revision#", get_board_rev());
	       if (!env_get("ethaddr"))
		       env_set("ethaddr", get_mac_addr());
	}
	return 0;
}

int mx6_rgmii_rework(struct phy_device *phy)
{
#define PAD_SKEW(d3, d2, d1, d0)  (((d3)<<12)|((d2)<<8)|((d1)<<4)|(d0))
#define CLK_SKEW(tx, rx)          (((tx)<<5) | (rx))

	/* TX Data Pad Skew: de-skew D2 and D3 */
	phy_write(phy, 0, 0x0d, 0x0002);
	phy_write(phy, 0, 0x0e, 0x0006);
	phy_write(phy, 0, 0x0d, 0x4002);
	phy_write(phy, 0, 0x0e, PAD_SKEW(0,0,7,7));

	/* Clock Pad Skew: max delay for TX and RX clock */
	phy_write(phy, 0, 0x0d, 0x0002);
	phy_write(phy, 0, 0x0e, 0x0008);
	phy_write(phy, 0, 0x0d, 0x4002);
	phy_write(phy, 0, 0x0e, CLK_SKEW(0x1f, 0x1f));

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	mx6_rgmii_rework(phydev);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

