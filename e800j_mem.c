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

#include "e800j_mem.h"
#include "z80_cpu.h"
#include "e800j.h"

/* tablica mapowania pamięci - obraz pamięci PROM 'MEM' Elwro 800 junior */
unsigned char prom_mem[512] = PROM_MEM;


/*********************************************************************************************/
/*                                   ZX SPECTRUM 48K                                         */
/*                                         PAMIĘĆ                                            */
/*********************************************************************************************/

uchar mem_zx48_read(void *z, ushort addr)
{
    if(addr < 16384) return ((elwro*)((z80 *)z)->ptr)->rom[ addr ];
    return ((elwro*)((z80 *)z)->ptr)->ram[ addr ];
}

void mem_zx48_write(void *z, ushort addr, uchar data)
{
    ((elwro*)((z80 *)z)->ptr)->ram[ addr] = data;
}

/*********************************************************************************************/
/*                                   ELWRO 800 JUNIOR                                        */
/*                                         PAMIĘĆ                                            */
/*********************************************************************************************/

uchar mem_e800j_read(void *z, ushort addr)
{
    elwro *el = (elwro*)((z80 *)z)->ptr;
    uchar bank = ~prom_mem[(el->mem_mask | (addr >> 10)) & 0x1ff];
    ushort romaddr = addr & 0x1fff;

//    if(bank & 0x20); /* RST 48 */
    if(bank & 0x01) return el->rom[ 0x0000 + romaddr ];  /* BAS0 */
    if(bank & 0x02) return el->rom[ 0x2000 + romaddr ];  /* BAS1 */
    if(bank & 0x10) return el->rom[ 0x4000 + romaddr ];  /* BOOT */
    if(bank & 0x08) return el->ram[ addr ]; // pamięć RAM
    if(bank & 0x04) return el->ram[ addr ]; // pamięć atrybutów

    return 0xff;
}

void mem_e800j_write(void *z, ushort addr, uchar data)
{
    ((elwro*)((z80 *)z)->ptr)->ram[ addr] = data;
}

/*********************************************************************************************/
/*                                   ZX SPECTRUM 128K                                        */
/*                                         PAMIĘĆ                                            */
/*********************************************************************************************/

uchar mem_zx128_read(void *z, ushort addr)
{
//    if(addr < 16384) return ((elwro*)((z80 *)z)->ptr)->rom[ addr ];
//    return ((elwro*)((z80 *)z)->ptr)->ram[ addr ];
    return 0xff;
}

void mem_zx128_write(void *z, ushort addr, uchar data)
{
//    ((elwro*)((z80 *)z)->ptr)->ram[ addr] = data;
}

