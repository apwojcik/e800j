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
#include "e800j_prom.h"
#include "e800j_file.h"

int decompress_data(FILE *f,int clength,int dlength, char *memo, int addr)
{
    int clen, dlen,rep=0;
    byte data;
    byte state;

    clen = dlen = state = 0;
    while(!feof(f)){
	if((clen >= clength) && (clength)) break;
	if((state==3)&&(rep==0)) break;
	data = fgetc(f);
	clen++;
	switch(state){
	    case 1: 
	    case 0: if(data == 0xed){
			state++;
			break;
		    }
		    dlen++;
		    *(memo + addr++) = data;
		    state = 0;
		    break;
	    case 2: rep = data;
		    state = 3;
		    break;
	    case 3: for(;rep > 0; rep--, dlen++) *(memo + addr++) = data;
		    state = 0;
	}
    }

    return 0;    
}

int load_z80(Z80 *z, FILE *f)
{
    e800j *e8 = (e800j *)z->cust;

    z80_fst_hdr hdr_1;            

    if(fread(&hdr_1,1,sizeof(hdr_1),f) != sizeof(hdr_1) ) return 1;

        
    if(hdr_1.Next_Version){
/* v1 */
	if(hdr_1.Compressed){
	    if(decompress_data(f,0,48*1024,e8->ram,16384)) return 1;
	}else{
	    if(fread(e8->ram + 16384, 48*1024,1,f) != 48*1024) return 1;
	}
    }else{
/* v2 lub v3 */
    }

/* wstawic zapis do rejestrów*/    

    z->PCp  = (hdr_1.PC[1] << 8) + hdr_1.PC[0];
printf("|->%x\n",z->PCp);
    z->AFr  = (hdr_1.AF[0] << 8) + hdr_1.AF[1];
    z->AFr_ = (hdr_1.AFa[0] << 8) + hdr_1.AFa[1]; 
    
    z->BCr  = (hdr_1.BC[1] << 8)  + hdr_1.BC[0];
    z->DEr  = (hdr_1.DE[1] << 8)  + hdr_1.DE[0]; 
    z->HLr  = (hdr_1.HL[1] << 8)  + hdr_1.HL[0]; 
    z->BCr_ = (hdr_1.BCa[1] << 8) + hdr_1.BCa[0]; 
    z->DEr_ = (hdr_1.DEa[1] << 8) + hdr_1.DEa[0]; 
    z->HLr_ = (hdr_1.HLa[1] << 8) + hdr_1.HLa[0]; 
    z->SPp  = (hdr_1.SP[1] << 8)  + hdr_1.SP[0]; 
    z->Ip   = hdr_1.I;
    z->Rp   = hdr_1.R;
    z->IXp  = (hdr_1.IX[1] << 8)  + hdr_1.IX[0]; 
    z->IYp  = (hdr_1.IY[1] << 8)  + hdr_1.IY[0]; 
    z->IFF1p= hdr_1.IFF1;
    z->IFF2p= hdr_1.IFF2;
    z->IMr  = hdr_1.IM;
/*
    border(z80,zx, h.border); 
*/

    return 0;    
}

byte load_file_block(char *fname,void *buffer, int buf_size)
{
    FILE *f;
    
    if(!(f = fopen(fname,"r"))) return 1;
    if((fread(buffer,1,buf_size,f)) != buf_size ){
	fclose(f);
	return 1;
    }
    fclose(f);
    return 0;
}

byte load_rom(e800j *e8, char *fname_bas0, char *fname_bas1, char *fname_boot)
{
    if(fname_bas1){
	if(load_file_block(fname_bas0, e8->rom_bas0,0x2000)) return 1;
	if(load_file_block(fname_bas1, e8->rom_bas1,0x2000)) return 1;
    } else {
	if(load_file_block(fname_bas0, e8->rom_bas0,0x4000)) return 1;
    }
    if(fname_boot){ 
	if(load_file_block(fname_boot,e8->rom_boot,0x2000)) return 1;
    }
    return 0;
}

