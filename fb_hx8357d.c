/*
 * FB driver for the HX8347D LCD Controller
 *
 * Copyright (C) 2013 Christian Vogelgsang
 *
 * Based on driver code found here: https://github.com/watterott/r61505u-Adapter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "fbtft.h"
#include "fb_hx8357d.h"

#define DRVNAME		"fb_hx8357d"
#define WIDTH		320
#define HEIGHT		480
#define DEFAULT_GAMMA	"0 0 0 0 0 0 0 0 0 0 0 0 0 0\n" \
			"0 0 0 0 0 0 0 0 0 0 0 0 0 0"


static u16 color565(u8 r, u8 g, u8 b) {
	  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye);

static int init_display(struct fbtft_par *par)
{
	u16 *fb;
	int i;

	fbtft_par_dbg(DEBUG_INIT_DISPLAY, par, "%s()\n", __func__);

	par->fbtftops.reset(par);

	/* setextc */
	write_reg(par, HX8357D_SETC, 0xFF, 0x83, 0x57);
	mdelay(300);

	/* setRGB which also enables SDO */
	write_reg(par, HX8357_SETRGB, 0x80, 0x0, 0x06, 0x06);

	/* -1.52V */
	write_reg(par, HX8357D_SETCOM, 0x25);

	/* Normal mode 70Hz, Idle mode 55 Hz */
	write_reg(par, HX8357_SETOSC, 0x68);

	/* Set Panel - BGR, Gate direction swapped */
	write_reg(par, HX8357_SETPANEL, 0x05);

	write_reg(par, HX8357_SETPWR1,
		0x00,  // Not deep standby
		0x15,  //BT
		0x1C,  //VSPR
		0x1C,  //VSNR
		0x83,  //AP
		0xAA);  //FS

	write_reg(par, HX8357D_SETSTBA,
		0x50,  //OPON normal
		0x50,  //OPON idle
		0x01,  //STBA
		0x3C,  //STBA
		0x1E,  //STBA
		0x08);  //GEN

	write_reg(par, HX8357D_SETCYC,
		0x02,  //NW 0x02
		0x40,  //RTN
		0x00,  //DIV
		0x2A,  //DUM
		0x2A,  //DUM
		0x0D,  //GDON
		0x78);  //GDOFF

	write_reg(par, HX8357D_SETGAMMA,
		0x02,
		0x0A,
		0x11,
		0x1d,
		0x23,
		0x35,
		0x41,
		0x4b,
		0x4b,
		0x42,
		0x3A,
		0x27,
		0x1B,
		0x08,
		0x09,
		0x03,
		0x02,
		0x0A,
		0x11,
		0x1d,
		0x23,
		0x35,
		0x41,
		0x4b,
		0x4b,
		0x42,
		0x3A,
		0x27,
		0x1B,
		0x08,
		0x09,
		0x03,
		0x00,
		0x01);

	/* 16 bit */
	write_reg(par, HX8357_COLMOD, 0x55);

	write_reg(par, HX8357_MADCTL, 0xC0);

	/* TE off */
	write_reg(par, HX8357_TEON, 0x00);

	/* tear line */
	write_reg(par, HX8357_TEARLINE, 0x00, 0x02);

	/* Exit Sleep */
	write_reg(par, HX8357_SLPOUT);
	mdelay(150);

	/* display on */
	write_reg(par, HX8357_DISPON);
	mdelay(50);

	mdelay(250);
	write_reg(par, HX8357_INVON);
	mdelay(250);
	write_reg(par, HX8357_INVOFF);
	mdelay(250);

	set_addr_win(par, 0, 0, WIDTH - 1, HEIGHT - 1);
	fb = (u16 *)par->info->screen_base;
	for (i = 0; i < WIDTH * HEIGHT; i++)
		fb[i] = color565(0, 255, 0);
	fbtft_write_vmem16_bus8(par, 0, WIDTH * HEIGHT);

	return 0;
}

static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
	fbtft_par_dbg(DEBUG_SET_ADDR_WIN, par,
		"%s(xs=%d, ys=%d, xe=%d, ye=%d)\n", __func__, xs, ys, xe, ye);

	/* Column addr set */
	write_reg(par, HX8357_CASET,
		xs >> 8, xs & 0xff,  /* XSTART */
		xe >> 8, xe & 0xff); /* XEND */

	/* Row addr set */
	write_reg(par, HX8357_PASET,
		ys >> 8, ys & 0xff,  /* YSTART */
		ye >> 8, ye & 0xff); /* YEND */

	/* write to RAM */
	write_reg(par, HX8357_RAMWR);
}

#if 0
/*
  Gamma string format:
    VRP0 VRP1 VRP2 VRP3 VRP4 VRP5 PRP0 PRP1 PKP0 PKP1 PKP2 PKP3 PKP4 CGM
    VRN0 VRN1 VRN2 VRN3 VRN4 VRN5 PRN0 PRN1 PKN0 PKN1 PKN2 PKN3 PKN4 CGM
*/
#define CURVE(num, idx)  curves[num*par->gamma.num_values + idx]
static int set_gamma(struct fbtft_par *par, unsigned long *curves)
{
	unsigned long mask[] = {
		0b111111, 0b111111, 0b111111, 0b111111, 0b111111, 0b111111,
		0b1111111, 0b1111111,
		0b11111, 0b11111, 0b11111, 0b11111, 0b11111,
		0b1111};
	int i, j;
	int acc = 0;

	fbtft_par_dbg(DEBUG_INIT_DISPLAY, par, "%s()\n", __func__);

	/* apply mask */
	for (i = 0; i < par->gamma.num_curves; i++)
		for (j = 0; j < par->gamma.num_values; j++) {
			acc += CURVE(i, j);
			CURVE(i, j) &= mask[j];
		}

	if (acc == 0) /* skip if all values are zero */
		return 0;

	for (i = 0; i < par->gamma.num_curves; i++) {
		write_reg(par, 0x40 + (i * 0x10), CURVE(i, 0));
		write_reg(par, 0x41 + (i * 0x10), CURVE(i, 1));
		write_reg(par, 0x42 + (i * 0x10), CURVE(i, 2));
		write_reg(par, 0x43 + (i * 0x10), CURVE(i, 3));
		write_reg(par, 0x44 + (i * 0x10), CURVE(i, 4));
		write_reg(par, 0x45 + (i * 0x10), CURVE(i, 5));
		write_reg(par, 0x46 + (i * 0x10), CURVE(i, 6));
		write_reg(par, 0x47 + (i * 0x10), CURVE(i, 7));
		write_reg(par, 0x48 + (i * 0x10), CURVE(i, 8));
		write_reg(par, 0x49 + (i * 0x10), CURVE(i, 9));
		write_reg(par, 0x4A + (i * 0x10), CURVE(i, 10));
		write_reg(par, 0x4B + (i * 0x10), CURVE(i, 11));
		write_reg(par, 0x4C + (i * 0x10), CURVE(i, 12));
	}
	write_reg(par, 0x5D, (CURVE(1, 0) << 4) | CURVE(0, 0));

	return 0;
}
#undef CURVE
#endif

#define HX8357D_MADCTL_MY  0x80
#define HX8357D_MADCTL_MX  0x40
#define HX8357D_MADCTL_MV  0x20
#define HX8357D_MADCTL_ML  0x10
#define HX8357D_MADCTL_RGB 0x00
#define HX8357D_MADCTL_BGR 0x08
#define HX8357D_MADCTL_MH  0x04
static int set_var(struct fbtft_par *par)
{
	u8 val;

	fbtft_par_dbg(DEBUG_INIT_DISPLAY, par, "%s()\n", __func__);

	switch (par->info->var.rotate) {
	case 270:
		val = HX8357D_MADCTL_MV;
		break;
	case 180:
		val = HX8357D_MADCTL_MY;
		break;
	case 90:
		val = HX8357D_MADCTL_MV | HX8357D_MADCTL_MY | HX8357D_MADCTL_MX;
		break;
	default:
		val = HX8357D_MADCTL_MX;
		break;
	}

	val |= (par->bgr ? HX8357D_MADCTL_RGB : HX8357D_MADCTL_BGR);

	/* Memory Access Control */
	write_reg(par, HX8357_MADCTL, val);

	return 0;
}

static struct fbtft_display display = {
	.regwidth = 8,
	.width = WIDTH,
	.height = HEIGHT,
	.gamma_num = 2,
	.gamma_len = 14,
	.gamma = DEFAULT_GAMMA,
	.fbtftops = {
		.init_display = init_display,
		.set_addr_win = set_addr_win,
//		.set_gamma = set_gamma,
		.set_var = set_var,
	},
};
FBTFT_REGISTER_DRIVER(DRVNAME, &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);

MODULE_DESCRIPTION("FB driver for the HX8357D LCD Controller");
MODULE_AUTHOR("Sean Cross <xobs@kosagi.com>");
MODULE_LICENSE("GPL");
