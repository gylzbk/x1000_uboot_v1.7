/*
 * Copyright 2009-2011 Freescale Semiconductor, Inc.
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

#include <common.h>
#include <asm/fsl_serdes.h>
#include <asm/processor.h>
#include <asm/io.h>
#include "fsl_corenet_serdes.h"

/*
 * Note: For P5040, the fourth SerDes bank (with two lanes) is on SerDes2, but
 * U-boot only supports one SerDes controller.  Therefore, we ignore bank 4 in
 * this table.  This works because most of the SerDes code is for errata
 * work-arounds, and there are no P5040 errata that effect bank 4.
 */

static u8 serdes_cfg_tbl[][SRDS_MAX_LANES] = {
	[0x00] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE2, PCIE2, PCIE2, PCIE2,
		SGMII_FM2_DTSEC3, SGMII_FM2_DTSEC4, SGMII_FM1_DTSEC1,
		SGMII_FM1_DTSEC2, SGMII_FM1_DTSEC3, SGMII_FM1_DTSEC4,
		XAUI_FM2, XAUI_FM2, XAUI_FM2, XAUI_FM2, /* SATA1, SATA2, */ },
	[0x01] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE2, PCIE2, PCIE2, PCIE2,
		SGMII_FM1_DTSEC3, SGMII_FM2_DTSEC4, XAUI_FM1, XAUI_FM1,
		XAUI_FM1, XAUI_FM1, XAUI_FM2, XAUI_FM2, XAUI_FM2,
		XAUI_FM2, /* SATA1, SATA2 */ },
	[0x02] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE2, PCIE2, SGMII_FM1_DTSEC3,
		SGMII_FM1_DTSEC4, SGMII_FM2_DTSEC3, SGMII_FM2_DTSEC4,
		XAUI_FM1, XAUI_FM1, XAUI_FM1, XAUI_FM1, XAUI_FM2, XAUI_FM2,
		XAUI_FM2, XAUI_FM2, /* SATA1, SATA2 */ },
	[0x03] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE2, PCIE2, SGMII_FM2_DTSEC1,
		SGMII_FM2_DTSEC2, SGMII_FM2_DTSEC3, SGMII_FM2_DTSEC4,
		SGMII_FM1_DTSEC1, SGMII_FM1_DTSEC2, SGMII_FM1_DTSEC3,
		SGMII_FM1_DTSEC4, XAUI_FM2, XAUI_FM2, XAUI_FM2, XAUI_FM2,
		/* SATA1, SATA2 */ },
	[0x04] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE2, PCIE3, SGMII_FM2_DTSEC1,
		SGMII_FM2_DTSEC2, SGMII_FM2_DTSEC3, SGMII_FM2_DTSEC4,
		SGMII_FM1_DTSEC1, SGMII_FM1_DTSEC2, SGMII_FM1_DTSEC3,
		SGMII_FM1_DTSEC4, XAUI_FM2, XAUI_FM2, XAUI_FM2, XAUI_FM2,
		/* SATA1, SATA2 */ },
	[0x05] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE2, PCIE3, SGMII_FM1_DTSEC3,
		SGMII_FM1_DTSEC4, SGMII_FM2_DTSEC3, SGMII_FM2_DTSEC4,
		XAUI_FM1, XAUI_FM1, XAUI_FM1, XAUI_FM1, XAUI_FM2, XAUI_FM2,
		XAUI_FM2, XAUI_FM2, /* SATA1, SATA2 */ },
	[0x06] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE1, PCIE1, PCIE1, PCIE1,
		SGMII_FM2_DTSEC3, SGMII_FM2_DTSEC4, SGMII_FM1_DTSEC1,
		SGMII_FM1_DTSEC2, SGMII_FM1_DTSEC3, SGMII_FM1_DTSEC4,
		XAUI_FM2, XAUI_FM2, XAUI_FM2, XAUI_FM2, /* SATA1, SATA2 */ },
	[0x07] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE1, PCIE1, PCIE1, PCIE1,
		SGMII_FM1_DTSEC3, SGMII_FM2_DTSEC4, XAUI_FM1, XAUI_FM1,
		XAUI_FM1, XAUI_FM1, XAUI_FM2, XAUI_FM2, XAUI_FM2,
		XAUI_FM2, /* SATA1, SATA2 */ },
	[0x11] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE2, PCIE2, PCIE2, PCIE2,
		AURORA, AURORA, SGMII_FM1_DTSEC1, SGMII_FM1_DTSEC2,
		SGMII_FM1_DTSEC3, SGMII_FM1_DTSEC4, NONE, NONE, SATA1, SATA2,
		/* NONE, NONE */ },
	[0x15] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE2, PCIE2, PCIE2, PCIE2,
		AURORA, AURORA, XAUI_FM1, XAUI_FM1, XAUI_FM1, XAUI_FM1,
		NONE, NONE, SATA1, SATA2, /* NONE, NONE */ },
	[0x2a] = {PCIE1, PCIE1, PCIE3, PCIE3, PCIE2, PCIE2, PCIE2, PCIE2,
		AURORA, AURORA, SGMII_FM1_DTSEC1, SGMII_FM1_DTSEC2,
		SGMII_FM1_DTSEC3, SGMII_FM1_DTSEC4, XAUI_FM2, XAUI_FM2,
		XAUI_FM2, XAUI_FM2, /* NONE, NONE */ },
	[0x34] = {PCIE1, PCIE1, PCIE1, PCIE1, SGMII_FM1_DTSEC1,
		SGMII_FM1_DTSEC2, SGMII_FM1_DTSEC3, SGMII_FM1_DTSEC4, AURORA,
		AURORA, XAUI_FM1, XAUI_FM1, XAUI_FM1, XAUI_FM1, NONE,
		NONE, SATA1, SATA2, /* NONE, NONE */ },
	[0x35] = {PCIE1, PCIE1, PCIE1, PCIE1, PCIE2, PCIE2,
		SGMII_FM1_DTSEC3, SGMII_FM1_DTSEC4, AURORA, AURORA, XAUI_FM1,
		XAUI_FM1, XAUI_FM1, XAUI_FM1, NONE, NONE, SATA1, SATA2,
		/* NONE, NONE */ },
	[0x36] = {PCIE1, PCIE1, PCIE3, PCIE3, SGMII_FM1_DTSEC1,
		SGMII_FM1_DTSEC2, SGMII_FM1_DTSEC3, SGMII_FM1_DTSEC4, AURORA,
		AURORA, XAUI_FM1, XAUI_FM1, XAUI_FM1, XAUI_FM1, NONE,
		NONE, SATA1, SATA2, /* NONE, NONE */ },
};

enum srds_prtcl serdes_get_prtcl(int cfg, int lane)
{
	if (!serdes_lane_enabled(lane))
		return NONE;

	return serdes_cfg_tbl[cfg][lane];
}

int is_serdes_prtcl_valid(u32 prtcl)
{
	int i;

	if (prtcl >= ARRAY_SIZE(serdes_cfg_tbl))
		return 0;

	for (i = 0; i < SRDS_MAX_LANES; i++) {
		if (serdes_cfg_tbl[prtcl][i] != NONE)
			return 1;
	}

	return 0;
}
