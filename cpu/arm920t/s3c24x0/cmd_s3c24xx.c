/*
 * (C) Copyright 2006 by OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>		/* for print_IPaddr */
#include <s3c2410.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_CMD_BDI)

#define ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))
#define MHZ	1000000

static void print_cpu_speed(void)
{
	printf("FCLK = %u MHz, HCLK = %u MHz, PCLK = %u MHz, UCLK = %u MHz\n",
		get_FCLK()/MHZ, get_HCLK()/MHZ, get_PCLK()/MHZ, get_UCLK()/MHZ);
}

struct s3c24x0_pll_speed {
	u_int16_t	mhz;
	u_int32_t	mpllcon;
	u_int32_t	clkdivn;
};

#define CLKDIVN_1_1_1	0x00
#define CLKDIVN_1_2_2	0x02
#define CLKDIVN_1_2_4	0x03
#define CLKDIVN_1_4_4	0x04

#if defined(CONFIG_S3C2410)
static const u_int32_t upllcon = ((0x78 << 12) + (0x2 << 4) + 0x3);
static const struct s3c24x0_pll_speed pll_configs[] = {
	{
		.mhz = 50,
		.mpllcon = ((0x5c << 12) + (0x4 << 4) + 0x2),
		.clkdivn = CLKDIVN_1_1_1,
	},
	{
		.mhz = 101,
		.mpllcon = ((0x7f << 12) + (0x2 << 4) + 0x2),
		.clkdivn = CLKDIVN_1_2_2,
	},
	{
		.mhz = 202,
		.mpllcon = ((0x90 << 12) + (0x7 << 4) + 0x0),
		.clkdivn = CLKDIVN_1_2_4,
	},
	{
		.mhz = 266,
		.mpllcon = ((0x7d << 12) + (0x1 << 4) + 0x1),
		.clkdivn = CLKDIVN_1_2_4,
	},
};
#else
#error "please define valid pll configurations for your cpu type"
#endif

static void list_cpu_speeds(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(pll_configs); i++)
		printf("%u MHz\n", pll_configs[i].mhz);
}

static int reconfig_mpll(u_int16_t mhz)
{
	S3C24X0_CLOCK_POWER * const clk_power = S3C24X0_GetBase_CLOCK_POWER();
	int i;

	for (i = 0; i < ARRAY_SIZE(pll_configs); i++) {
		if (pll_configs[i].mhz == mhz) {
			/* to reduce PLL lock time, adjust the LOCKTIME register */
			clk_power->LOCKTIME = 0xFFFFFF;

			/* configure MPLL */
			clk_power->MPLLCON = pll_configs[i].mpllcon;
			clk_power->UPLLCON = upllcon;
			clk_power->CLKDIVN = pll_configs[i].clkdivn;

			/* If we changed the speed, we need to re-configure
			 * the serial baud rate generator */
			serial_setbrg();
			return 0;
		}
	}
	return -1;
}

int do_s3c24xx ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (!strcmp(argv[1], "speed")) {
		if (argc < 2)
			goto out_help;
		if (!strcmp(argv[2], "get"))
			print_cpu_speed();
		else if (!strcmp(argv[2], "list"))
			list_cpu_speeds();
		else if (!strcmp(argv[2], "set")) {
			unsigned long mhz;
			if (argc < 3)
				goto out_help;

			mhz = simple_strtoul(argv[3], NULL, 10);

			if (reconfig_mpll(mhz) < 0)
				printf("error, speed %uMHz unknown\n", mhz);
			else
				print_cpu_speed();
		} else
			goto out_help;
	} else {
out_help:
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	return 0;
}

/* -------------------------------------------------------------------- */


U_BOOT_CMD(
	s3c24xx,	4,	1,	do_s3c24xx,
	"s3c24xx - SoC  specific commands\n",
	"speed get - display current PLL speed config\n"
	"s3c24xx speed list - display supporte PLL speed configs\n"
	"s3c24xx speed set - set PLL speed\n"
);

#endif
