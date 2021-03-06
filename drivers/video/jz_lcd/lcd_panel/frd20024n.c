/*
 * JZ LCD PANEL DATA
 *
 * Copyright (c) 2014 Ingenic Semiconductor Co.,Ltd
 * Author: Huddy <hyli@ingenic.cn>
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

#include <config.h>
#include <serial.h>
#include <common.h>
#include <lcd.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <asm/types.h>
#include <asm/arch-m200/tcu.h>
#include <asm/arch-m200/lcdc.h>
#include <asm/arch-m200/gpio.h>
#include <asm/arch-x1000/tcu.h>
#include <asm/arch-x1000/lcdc.h>
#include <asm/arch-x1000/gpio.h>
#include <regulator.h>
#include <jz_lcd/frd240a3602b.h>

struct frd240a3602b_data frd240a3602b_pdata;

vidinfo_t panel_info = { 176, 220, LCD_BPP, };

void panel_pin_init(void)
{
	printf("--------panel_pin_init--------\n");
	
	int ret = 0;
	ret = gpio_request(frd240a3602b_pdata.gpio_lcd_cs, "lcd_cs");
	if(ret){
		printf("canot request gpio lcd_cs\n");
	}

	ret = gpio_request(frd240a3602b_pdata.gpio_lcd_rd, "lcd_rd");
	if(ret){
		printf("canot request gpio lcd_rd\n");
	}

        ret = gpio_request(frd240a3602b_pdata.gpio_lcd_rst, "lcd_rst");
	if(ret){
		printf("canot request gpio lcd_rst\n");
	}
	ret = gpio_request(frd240a3602b_pdata.gpio_lcd_bl, "lcd_bl");
	if(ret){
		printf("canot request gpio lcd_bl\n");
	}
	serial_puts("frd20024n panel display pin init\n");
}

void panel_power_on(void)
{
	printf("--------panel_power_on--------\n");
	
	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_cs, 1);
	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_rd, 1);
	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_rst, 1);
	mdelay(10);
	/*power reset*/
	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_rst, 0);
	mdelay(500);
	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_rst, 1);
	mdelay(520);

	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_cs, 0);
	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_bl, 0);

	/*serial_puts("frd240a3602b panel display on\n");*/
}

void panel_power_off(void)
{
	printf("--------panel_power_off--------\n");
	
	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_cs, 0);
	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_rd, 0);
	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_rst, 0);
	gpio_direction_output(frd240a3602b_pdata.gpio_lcd_bl, 0);
	/*serial_puts("frd240a3602b panel display off\n");*/
}

struct fb_videomode jzfb1_videomode = {
	.name = "176x220",
	.refresh = 60,
	.xres = 176,
	.yres = 220,
	.pixclock = KHZ2PICOS(5760),
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len = 0,
	.vsync_len = 0,
	.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};
