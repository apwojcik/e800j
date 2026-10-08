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

int vid_addr( int pix_nb)
{
    pix_nb >>= 3;
    return (pix_nb & 0x181f) | ((pix_nb & 0x700) >> 3) | ((pix_nb & 0xe0) << 3);
}

int blink_attr(Z80 *z)
{
    ((e800j *)z->cust)->scr.blink = !((e800j *)z->cust)->scr.blink;
    return 1;
}

void scr_rfsh(Z80 *z)
{
    int x,y,pix_nb,i,k;
    int fg_color, bg_color;
    char pix,attr;
    byte *screen = ((e800j*)z->cust)->scr.screen[ (MEM157_RELOK(((e800j*)z->cust)->reg175)) ? 1:0];
    byte blink   = ((e800j*)z->cust)->scr.blink ? 0xff:0;

    fg_color = pix = pix_nb = bg_color = 0;

    for(y=0;y < 192; y++)
	for(k=x=0;k < 32; k++)
	{	
	    attr = *(screen + 6144 + k + 32*(y/8));	
	
	    fg_color = (attr & 0x7) | ((attr & 0x40) >> 3);
	    bg_color = (attr & 0x78) >> 3;
	    pix = *(screen + vid_addr(pix_nb));
	    pix ^= (attr & 0x80) ? blink:0;
	    for(i=0;i < 8;i++, pix_nb++, pix <<=1 )
		gui_put_pixel((pix & 0x80) ? fg_color: bg_color,x++,y);
    }	
    gui_screen_rfsh();
}

