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

#ifdef USE_LIBSPECTRUM
#include <libspectrum.h>
#endif

#include "e800j.h"
#include "z80.h"
#include "e800j_prom.h"
#include "e800j_file.h"

void file_save_config(e800j *e8, const char *fname)
{
    FILE *f;

    if(!(f = fopen(fname,"w"))) {
	file_save_config(e8,fname); /* zapisanie domy¶lnej konfiguracji */
	return;
    }
    if(fwrite(e8->config,1,sizeof(e8_config),f) != sizeof(e8_config)){
	printf("Nie powiod³o siê utworzenie pliku konfiguracji %s\n", fname);
	fclose(f);	
	exit(-1);
    }
        
    fclose(f);
}

void file_load_config(e800j *e8, const char *fname)
{
    FILE *f;
    
    if(!(f = fopen(fname,"r"))) {
	file_save_config(e8,fname); /* zapisanie domy¶lnej konfiguracji */
	return;
    }
    
    if(fread(e8->config,1,sizeof(e8_config),f) != sizeof(e8_config)){
	printf("B³±d w pliku konfiguracji %s\n Powinienne¶ ten plik skasowaæ aby uzyskaæ domy¶ln± konfiguracjê\n",fname);
	fclose(f);	
	exit(-1);
    }
        
    fclose(f);
}


byte expand_z80(byte data, byte *dst, int *ii){
    static byte a0,a1,x;
    byte i;

    if( !dst ){
	a0 = a1 = 0;
	return 1;
    }

    if(a0){
	switch(a0){
	    case 1: if(data == 0xed)
			a0++;
		    else {
			a0 = 0; 
			dst[*ii] = 0xed; 
			if(*ii < 49152) (*ii)++;
			if(data == 0)
			    a1=1;
			else{
			    dst[*ii] = data;
			    if(*ii < 49152) (*ii)++;
			    }
		    } break;
	    case 2: x = data; 
		    a0++; 
		    break;
	    case 3: for(i = 0; i < x; i++){ 
			dst[*ii] = data;
			if(*ii < 49152) (*ii)++;
		    }
		    a0 = 0; 
		    break;
	}
	return 1;
    } 

    if(a1){
	a1 = 0;
	dst[*ii] = 0;
	if(*ii < 49152) (*ii)++; 
	if(data == 0xED)
	    a0 = 1;
	else{
	    dst[*ii] = data;
	    if(*ii < 49152) (*ii)++;
	    }
	return 1;
    }

    switch(data){
        case 0x00: a1 = 1; break;
        case 0xED: a0 = 1; break;
        default  : dst[*ii] = data; if(*ii < 49152) (*ii)++; 
    }    
    return 1;
}



int decompress_data(FILE *f,int clength,int dlength, char *memo, int addr)
{
    byte *tmp = memo;
    byte tt[65535];
    int i=0;

    expand_z80(0,NULL,&i);
    while(!feof(f)) expand_z80(fgetc(f),tt,&i);

    memcpy(tmp+addr,tt,49152);
    return 0;
}

#ifndef USE_LIBSPECTRUM
int load_z80(e800j *e8, FILE *f, const char *fname, char **msg)
{


    z80_fst_hdr hdr_1;            

*msg = "File load Error";            
    if(fread(&hdr_1,1,sizeof(hdr_1),f) != sizeof(hdr_1) ) return 1;

    if(hdr_1.Next_Version){
/* v1 */
	if(hdr_1.Compressed){
	    if(decompress_data(f,0,48*1024,e8->ram,16384)) return 1;
if(*e8->rom_bas0 == 0xf7) printf("Zmod ROM \n");
	}else{
	    if(fread(e8->ram + 16384, 48*1024,1,f) != 48*1024) return 1;
	}
    }else{
printf("|-> nie obs³ugiwana wersja  formatu (2 lub 3)\n");
//	return 0;
/* v2 lub v3 */
    }

/* wstawic zapis do rejestrów*/    
    z80_reset(e8->cpu); /* ustawiam procesor w stan pocz±tkowy*/

    ((Z80 *)e8->cpu)->PCp  = (hdr_1.PC[1] << 8) + hdr_1.PC[0];
    ((Z80 *)e8->cpu)->AFr  = (hdr_1.AF[0] << 8) + hdr_1.AF[1];
    ((Z80 *)e8->cpu)->AFr_ = (hdr_1.AFa[0] << 8) + hdr_1.AFa[1]; 
    ((Z80 *)e8->cpu)->BCr  = (hdr_1.BC[1] << 8)  + hdr_1.BC[0];
    ((Z80 *)e8->cpu)->DEr  = (hdr_1.DE[1] << 8)  + hdr_1.DE[0]; 
    ((Z80 *)e8->cpu)->HLr  = (hdr_1.HL[1] << 8)  + hdr_1.HL[0]; 
    ((Z80 *)e8->cpu)->BCr_ = (hdr_1.BCa[1] << 8) + hdr_1.BCa[0]; 
    ((Z80 *)e8->cpu)->DEr_ = (hdr_1.DEa[1] << 8) + hdr_1.DEa[0]; 
    ((Z80 *)e8->cpu)->HLr_ = (hdr_1.HLa[1] << 8) + hdr_1.HLa[0]; 
    ((Z80 *)e8->cpu)->SPp  = (hdr_1.SP[1] << 8)  + hdr_1.SP[0]; 
    ((Z80 *)e8->cpu)->Ip   = hdr_1.I;
    ((Z80 *)e8->cpu)->Rp   = hdr_1.R;
    ((Z80 *)e8->cpu)->IXp  = (hdr_1.IX[1] << 8)  + hdr_1.IX[0]; 
    ((Z80 *)e8->cpu)->IYp  = (hdr_1.IY[1] << 8)  + hdr_1.IY[0]; 
    ((Z80 *)e8->cpu)->IFF1p= hdr_1.IFF1;
    ((Z80 *)e8->cpu)->IFF2p= hdr_1.IFF2;
    ((Z80 *)e8->cpu)->IMr  = hdr_1.IM;

    e8->reg373  = hdr_1.Border;

#ifdef RUN_AT_START
    ((Z80 *)e8->cpu)->run = 1;
#endif

    return 0;    
}
#endif

byte load_file_block(const char *fname,void *buffer, int buf_size)
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


#ifdef USE_LIBSPECTRUM
char copy_snapshot(e800j *e8, libspectrum_snap *snap)
{
    int i;

    Z80 *z = (Z80*)e8->cpu;
    
    libspectrum_machine machine;
    
    machine = libspectrum_snap_machine(snap);

    if(machine != LIBSPECTRUM_MACHINE_48)
	if(machine != LIBSPECTRUM_MACHINE_16){
	    printf("Nie obs³ugiwany typ komputera: %s\n",libspectrum_machine_name(machine));
//	    return 0;
	}	
    z80_reset(z);
/* ustawienie rejestrów */
    z->Ar  = libspectrum_snap_a(snap);		z->Fr  = libspectrum_snap_f(snap);
    z->Ar_ = libspectrum_snap_a_(snap);		z->Fr_ = libspectrum_snap_f_(snap);

    z->BCr  = libspectrum_snap_bc(snap);	z->BCr_  = libspectrum_snap_bc_(snap);
    z->DEr  = libspectrum_snap_de(snap);	z->DEr_  = libspectrum_snap_de_(snap);
    z->HLr  = libspectrum_snap_hl(snap);	z->HLr_  = libspectrum_snap_hl_(snap);

    z->IXp  = libspectrum_snap_ix(snap);	z->IYp  = libspectrum_snap_iy(snap);
    z->Ip   = libspectrum_snap_i(snap);		z->Rp   = libspectrum_snap_r(snap);
    z->SPp  = libspectrum_snap_sp(snap);	z->PCp  = libspectrum_snap_pc(snap);

    z->IFF1p = libspectrum_snap_iff1(snap);	z->IFF2p = libspectrum_snap_iff2(snap);
    z->IMr   = libspectrum_snap_im(snap);
    z->Halt_state = libspectrum_snap_halted(snap);
/* kolor ramki */
    e8->reg373 = libspectrum_snap_out_ula(snap);
    
#ifdef RUN_AT_START
    z->run = 1;
#endif
/* kopiowanie pamiêci */
    for(i=0; i < 16; i++)
    if(libspectrum_snap_pages(snap,i)){
	switch(i){
	    case 0: memcpy(((e800j*)(z->cust))->ram + 0xc000,libspectrum_snap_pages(snap,i),0x4000); break;
	    case 2: memcpy(((e800j*)(z->cust))->ram + 0x8000,libspectrum_snap_pages(snap,i),0x4000); break;
	    case 5: memcpy(((e800j*)(z->cust))->ram + 0x4000,libspectrum_snap_pages(snap,i),0x4000); break;
	}		
    	
    }
    return 0;
}

int load_z80(e800j *e8, FILE *f, const char *fname, char **msg)
{
    libspectrum_snap *snap;
    int error;
    char *buffer;
    long length;

*msg = "File load Error";            
    if((error=libspectrum_snap_alloc(&snap))) return error;
    fseek(f,0L,SEEK_END);
    length = ftell(f);
    fseek(f,0L,SEEK_SET);
    error=1;
    if((buffer = (char *) malloc(length))){
        if(!(error = load_file_block(fname,buffer,length))){
	    if(!(error = libspectrum_snap_read(snap,buffer,length,LIBSPECTRUM_ID_UNKNOWN, fname))){
	       error = copy_snapshot(e8, snap);	    
	    }
	}
	free(buffer);
    }
    libspectrum_snap_free(snap);    
    return error;
}
#endif











