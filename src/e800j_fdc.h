/* Elwro 800 Junior emulator v0.3
 * Copyright (C) 2006 Krzysztof Komarnicki
 * Copyright (C) 2026 Artur Wojcik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Symulacja karty kontrolera dysków elastycznych Elwro 800 Junior */

#pragma once

#include "i8272_fdc.h"

enum {
    FDC_R175_DRIVE_A = 0x01,
    FDC_R175_DRIVE_B = 0x02,
    FDC_R175_TC = 0x04,
    FDC_R175_RESET = 0x08
};

struct fdc {
    struct i8272 i8272;
    unsigned int drive_mtime;
    unsigned int drive_a;
    unsigned int drive_b;
};

void fdc_init(struct fdc* fdc);
void fdc_r175_write(struct fdc* fdc, unsigned char data);
void fdc_i8272_write(struct fdc* fdc, unsigned char a0, unsigned char data);
unsigned char fdc_i8272_read(struct fdc* fdc, unsigned char a0);
void fdc_reset(struct fdc* fdc);
void fdc_clk(struct fdc* fdc);
