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
#include "e800j_kbrd.h"
#include "i8251.h"

#define HW_PRINT	printf

/* Ustawianie portów wyj¶ciowych */
void out_mem_74157(byte data, e800j *e8)
{ 
    HW_PRINT("OUT MEM SN74LS157: %x\n", data);
    e8->reg175 = (data & 0xf0); 
    e8->addr_prom_mask = (data & 0xc0) | ((data & 0x20) << 3);
}

void out_fe_74373(byte data, e800j *e8)
{ 
//    HW_PRINT("OUT FE SN743LS73: %x\n",data);

}

void out_par_8255(byte data, e800j *e8)
{ 
    HW_PRINT("OUT PAR 8255: %x\n",data);

}

void out_ser_8251(byte data, e800j *e8, word addr)
{ 
    HW_PRINT("OUT SER 8251: %x\n",data);
    i8251_write(e8->i8251p, data, A0(addr));
}

void out_fdc_8272(byte data, e800j *e8)
{ 
    HW_PRINT("OUT FDC 8272: %x\n",data);
    
}

void out_fdc_74157(byte data, e800j *e8)
{ 
    HW_PRINT("OUT FDC SN74LS157: %x\n",data);

}

/* Czytanie z portów wej¶cia */
byte in_kbrd(word addr, e800j *e8)
{
    byte t = key_matrix(addr) & (e8->tape_in ? 0xff:0xbf);
//printf("IN %x=%x\n",addr,t);
    return t;
}

byte in_8255(word addr, e800j *e8)
{
    return 0xff;
}

byte in_8251(word addr, e800j *e8)
{
    return i8251_read(e8->i8251p,addr & 0x01);
}

byte in_8272(word addr, e800j *e8)
{
    return 0xff;
}


unsigned char io(word addr,byte data, byte type, Z80 *z)
{
    e800j *e8 = (e800j *)z->cust;
    byte  ctrl = *(e8->prom_jio + (addr & 0x1ff));

    if(!type){
/* IOW */
	if(IO_CF7(ctrl))   { e8->wr175 = 1; out_mem_74157(data,e8); return 0; }
	if(IO_CFE(ctrl))   { e8->wr373 = 1; out_fe_74373(data,e8);  return 0; }
	if(IO_CS55(ctrl))  { e8->wr55  = 1; out_par_8255(data,e8);  return 0; } 
	if(IO_CS51(ctrl))  { e8->wr51  = 1; out_ser_8251(data,e8,addr);  return 0; } 
	if(IO_CSFDC(ctrl)) { e8->wr72  = 1; out_fdc_8272(data,e8);  return 0; }  	
	if(IO_CF1(ctrl))   { e8->wrFDC = 1; out_fdc_74157(data,e8); return 0; }  		
    } else {
/* IOR */
	if(IO_CFE(ctrl))   return in_kbrd(addr,e8);
	if(IO_CS55(ctrl))  return in_8255(addr,e8);
	if(IO_CS51(ctrl))  return in_8251(addr,e8);
	if(IO_CSFDC(ctrl)) return in_8272(addr,e8);	
    }
    return 0xff;
}

