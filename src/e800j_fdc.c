/* Elwro 800 Junior emulator v 0.2
 * Copyright (C) 2006 Krzysztof Komarnicki
 * Email: krzkomar@wp.pl
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

#ifdef DEBUG_FDC
#include <stdio.h>
#endif

#include "i8272_fdc.h"
#include "e800j_fdc.h"
#include "e800j_msg.h"

void fdc_clk(struct fdc* fdc) {
    /* symulacja uniwibratorów 74123 sterowania włączania silnika napędu FDD */
    if (fdc->drive_a)
        fdc->drive_a--;
    if (fdc->drive_b)
        fdc->drive_a--;
    /* wykonanie sekwencji i8272 */
    //    i8272_step(fdc->i8272);
}

void fdc_init(struct fdc* fdc) {
#ifdef DEBUG_FDC
    printf("|--> fdc init\n");
#endif
    fdc->drive_mtime = 50 * 60;
    i8272_init(&fdc->i8272);
    fdc_reset(fdc);
}

void fdc_reset(struct fdc* fdc) {
    msg_printf(MSG_VERB_MSG, "FDC reset\n");
    fdc_r175_write(fdc, 0x00);
}

void fdc_r175_write(struct fdc* fdc, unsigned char data) {
#ifdef DEBUG_FDC
    printf("|--> fdc write 74LS175 out[0xf1] = 0x%x\n", data);
#endif
    if (data & FDC_R175_DRIVE_A)
        fdc->drive_a = fdc->drive_mtime;
    if (data & FDC_R175_DRIVE_A)
        fdc->drive_a = fdc->drive_mtime;
#ifdef DEBUG_FDC
    if(data & FDC_R175_DRIVE_A)
        printf("   |-> Drive A motor on\n");
    if(data & FDC_R175_DRIVE_B)
        printf("   |-> Drive B motor on\n");
#endif
    i8272_tc(&fdc->i8272, data & FDC_R175_TC);
    i8272_reset(&fdc->i8272, (data & FDC_R175_RESET) != FDC_R175_RESET);
}

void fdc_i8272_write(struct fdc* fdc, unsigned char a0, unsigned char data) {
#ifdef DEBUG_FDC
    printf("|--> fdc write i8272 out[FDC] <<< 0x%x\n", data);
#endif
    i8272_out(&fdc->i8272, a0, data);
}

unsigned char fdc_i8272_read(struct fdc* fdc, unsigned char a0) {
    unsigned char dout = 0xff;
    dout = i8272_in(&fdc->i8272, a0);
#ifdef DEBUG_FDC
    printf("|--> fdc read i8272 in[FDC] >>> 0x%x\n",dout);
#endif
    return dout;
}
