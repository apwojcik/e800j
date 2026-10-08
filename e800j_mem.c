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

void mem_wr(Z80 *z, word addr, byte data)
{
//    static long cnt=0;
    if(addr < 16384){
//	 printf("%4i Proba zapisu do ROM pod adres 0x%x, adres instr:%x\n",cnt++,addr, z->PCp);
//exit(1);
//	 return;
    }
    *(((e800j *)z->cust)->ram+addr) = data;
}

byte mem_rd(Z80 *z, word addr)
{
    byte ctrl, data=0;
    e800j *e8 = (e800j *)z->cust;
    byte *m = e8->ram;
    word rom_addr  = ROM_ADDR(addr);
    word ctrl_addr = (addr >> 10) & 0x3f;

    ctrl_addr |= e8->addr_prom_mask;
    ctrl = *(e8->prom_mem + ctrl_addr );

/*
    if(MEM_IAHR(ctrl)) 
	if(z->M1 && !TEST_A9(addr) && IO_IAL(*(e8->prom_jio + A0_A8(addr))) )
	{
#ifdef DEBUG_HW_MSG
	    printf("|-->HW Elwro RST #18\n");
#endif
	    return RST_18;
	}
*/
    if(MEM_ROM1(ctrl)) data = *(e8->rom_bas0 + rom_addr);
    if(MEM_ROM2(ctrl)) data = *(e8->rom_boot + rom_addr);
    if(MEM_ROM3(ctrl)) data = *(e8->rom_bas1 + rom_addr);
    if(MEM_DRAM(ctrl)) data = *(m + addr);

    return data;    
}

