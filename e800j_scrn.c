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

void draw_zx_byte(byte x, byte y, byte data, byte attr)
{
    int i;
    byte fg_color;
    byte bg_color;
    
    fg_color = (attr & 0x7) | ((attr & 0x40) >> 3);
    bg_color = (attr & 0x78) >> 3;
    x <<= 3;    
    for(i=0;i < 8;i++, data <<=1 )
	gui_put_pixel((data & 0x80) ? fg_color: bg_color,x++,y);
}

byte scr_bcp_pix[6144];
byte scr_bcp_attr[6144];

void scr_rfsh(Z80 *z)
{
    int x = 0, k;
    char pix,attr;
    int y_pix=0, y_attr=0;
    byte *scr_map, *scr_attr=NULL, *scr_pix;
    static byte n=1;
    word pix_offs=0;
    word atr_offs=0;

    byte *screen = ((e800j*)z->cust)->scr.screen[ (MEM157_RELOK(((e800j*)z->cust)->reg175)) ? 1:0];
    byte blink   = ((e800j*)z->cust)->scr.blink ? 0xff:0;

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
	    
	    draw_zx_byte(x, y_pix, pix, attr);
	}

    }

    n=0;
    gui_screen_rfsh();
}
