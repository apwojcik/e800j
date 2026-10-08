/* Elwro 800 Junior emulator
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "z80.h"
#include "e800j.h"
#include "e800j_gui.h"
#include "e800j_scrn.h"


byte zx_scr_map[192] = ZX_SCR_MAP;

int blink_attr(Z80 *z)
{
    ((e800j *)z->cust)->scr.blink = !((e800j *)z->cust)->scr.blink;
    return 1;
}

void draw_zx_byte(e800j *e8,byte x, byte y, byte data, byte attr)
{
    int i;
    byte fg_color;
    byte bg_color;
    
    fg_color = (attr & 0x7) | ((attr & 0x40) >> 3);
    bg_color = (attr & 0x78) >> 3;
    x <<= 3;    
    for(i=0;i < 8;i++, data <<=1 )
	gui_put_pixel(e8, (data & 0x80) ? fg_color: bg_color,x++,y);
}

byte scr_bcp_pix[6144];
byte scr_bcp_attr[6144];

void scr_rfsh(e800j *e8)
{
    int x = 0, k;
    char pix,attr;
    int y_pix=0, y_attr=0;
    byte *scr_map, *scr_attr=NULL, *scr_pix;
    static byte n=1;
    word pix_offs=0;
    word atr_offs=0;

    byte *screen = e8->scr.screen[ (MEM157_RELOK(e8->reg175)) ? 1:0];
    byte blink   = e8->scr.blink ? 0xff:0;

    if(n){
	for(k=0; k < 6144; k++) scr_bcp_pix[k] = 0;
	for(k=0; k < 1024; k++) scr_bcp_attr[k] = 0;
    }
    
    scr_map = zx_scr_map;
    scr_pix = screen;
    scr_attr = screen + 6144;
    

    for(k = 0; k < 6144; k++, pix_offs++, atr_offs++){

	x = k & 0x1f;
	if(!x){
	     y_pix    = *(scr_map++);
	     y_attr   = (y_pix << 2) & 0xe0;
	     atr_offs = ((y_pix >> 3) << 5);
	}
	attr = *(scr_attr + atr_offs);
	pix  = *(scr_pix  + pix_offs) ^ ((attr & 0x80) ? blink : 0);
	
	if( n ||
	(pix != *(scr_bcp_pix + pix_offs)) ||
	( attr != *(scr_bcp_attr + pix_offs))
	){
	
	    *(scr_bcp_pix + pix_offs)  = pix;
	    *(scr_bcp_attr + pix_offs) = attr;
	    
	    draw_zx_byte(e8, x, y_pix, pix, attr);
	}

    }

    n=0;
    gui_screen_rfsh(e8);
}

void zxscr_calc(e8_screen *scr)
{
/* obliczenie wysoko¶ci/szeroko¶ci ramki */
    if(!scr->scr_buffer) 
	free(scr->scr_buffer);
    scr->fr_spcr = scr->area_w % scr->zx_w;
    scr->fr_spcl = (scr->fr_spcr >> 1) + (scr->fr_spcr & 1);
    scr->fr_spcr = (scr->fr_spcr >> 1);
    if(!scr->fr_spcr) scr->fr_spcr = 2;
    if(!scr->fr_spcl) scr->fr_spcl = 2;
    scr->fr_spcu = scr->area_h % scr->zx_h;
    scr->fr_spcd = (scr->fr_spcu >> 1) + (scr->fr_spcu & 1);
    scr->fr_spcu = (scr->fr_spcu >> 1);
    if(!scr->fr_spcu) scr->fr_spcu = 2;
    if(!scr->fr_spcd) scr->fr_spcd = 2;
/* obliczanie szeroko¶ci/ wysokosci ekranu */
    scr->scr_ws = scr->area_w - scr->fr_spcr - scr->fr_spcl;
    scr->scr_hs = scr->area_h - scr->fr_spcu - scr->fr_spcd;
/* obliczanie ofsetów bufora dla ramki */
    scr->fr_rofs = (scr->area_w - scr->fr_spcr) * (scr->bpp/8);
    scr->fr_duos = scr->fr_spcu * scr->area_w * (scr->bpp/8);
    scr->fr_ddos = scr->fr_spcd * scr->area_w * (scr->bpp/8);
    scr->fr_dofs = (scr->fr_spcu + scr->scr_hs) * scr->area_w * (scr->bpp/8);
/* obliczanie parametrów ekranu */
    scr->scr_offs = scr->fr_spcu * scr->area_w * (scr->bpp/8);
    scr->fr_rofs += scr->scr_offs; 
    scr->scr_scale_w = ((float)scr->scr_ws / scr->zx_w) * 16;
    scr->scr_scale_w = ((float)scr->scr_ws / scr->zx_h) * 16;
    scr->scr_buffer_size = scr->area_w * scr->area_h * (scr->bpp/8);
    scr->scr_bline = scr->scr_ws * (scr->bpp/8);
    scr->scr_hf = scr->fr_spcl * (scr->bpp/8);
    
    if(!(scr->scr_buffer = (byte *)malloc(scr->scr_buffer_size))){
        printf("Video memory allocation Error\n");
        exit(-1);
    }
    memset(scr->scr_buffer, ZX_BLUE, scr->scr_buffer_size);
/* dopisaæ zwolnienie pamiêci  */
}
