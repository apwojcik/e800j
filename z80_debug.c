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
#include <string.h>
#include "z80.h"
#include "z80_cpu.h"
#include "z80_instr.h"

#ifdef DEBUGGER

typedef struct{
    byte lines; /* ilosc linii instrukcji jaka ma zostac wyswietlona */
    long  instr; /* ilosc instrukcji do wykonania */
    word addr_brk; /* pulapka adresu */
    byte fwcnt:1;
    byte fcnt:1;
    byte fbreak:1;
    byte fwadr:1;
    byte free:1;
    byte fprint:1;
    byte hdr:1;
    byte pout;
    byte stdout;
    word daddr;
    word deasm;
} Z80_DEB;

Z80_DEB zd;

void z80_debug(char *str, char *a, char*b,Z80 *z)
{
    char tmpstr0[256];
    char tmpstr1[80];
    char tmpstr2[256];
    char *hdr="Adres         Instrukcja         AF | BC | DE | HL | AF'| BC'| DE'| HL'| IX | IY | SP |SZHPNC| I| R\n";
    char *tmpstr3=hdr;

    memset(tmpstr0,0,sizeof(tmpstr0));
    memset(tmpstr1,0,sizeof(tmpstr1));
    memset(tmpstr2,0,sizeof(tmpstr2));

    if(z->DD_FD) z->old_pc--;
    if((z->cnt >= zd.instr)&&(zd.fcnt)){ z->deb_halt=1; z->debug_print=1;}
    if((z->old_pc == zd.addr_brk)&&(zd.fbreak)){ z->deb_halt=1; z->debug_print=1;}
    if(z->debug_print){
	if(!zd.hdr) tmpstr3="\0";
	zd.hdr=0;
	sprintf(tmpstr0,str,a,b);
	sprintf(tmpstr1,"%4x %s",z->old_pc,tmpstr0);
	if(!z->dry_run){
	    sprintf(tmpstr2,"\t%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%i%i%i%i%i%i|%2x|%2x",
		z->AFr,z->BCr,z->DEr,z->HLr, z->AFr_,z->BCr_,z->DEr_,z->HLr_, IX,IY,SP,
		GET_SF,GET_ZF,GET_HF,GET_PF,GET_NF,GET_CF,I,R);
	    zd.deasm = z->old_pc;
	    z->debug_int=1;
	}
	sprintf(tmpstr0,"%s%s%s\n",tmpstr3,tmpstr1,tmpstr2);
	z->deb_out(tmpstr0);
    }
    z->debug_print=0;
    if((zd.fprint) && (zd.fbreak)) z->debug_print=1;
}

char tmpstr0[8];
char tmpstr1[8];

char zhex(byte data)
{
    data &= 0x0f;
    return (data > 9) ? 'a' - 10 + data : '0' + data;
}

char *z80_hex8(byte data,char *tmpstr)
{
    *tmpstr = zhex((data >> 4) & 0x0f);
    *(tmpstr+1) = zhex( data & 0x0f);    
    *(tmpstr+2) = 0;
    return tmpstr;
}

char *z80_hex16(word data,char *tmpstr)
{
    *tmpstr = zhex((data >> 12) & 0x0f);
    *(tmpstr+1) = zhex((data >> 8) & 0x0f);    
    *(tmpstr+2) = zhex((data >> 4) & 0x0f);    
    *(tmpstr+3) = zhex(data & 0x0f);    
    *(tmpstr+4) = 0;
    return tmpstr;
}

char *z80_hexU(byte data,char *tmpstr)
{
    *tmpstr = '+';
    if(data & 0x80) {
	data = ~data + 1;
	*tmpstr = '-';
    }
    data &= 0x7f;
    *(tmpstr+1) = zhex((data >> 4) & 0x0f);
    *(tmpstr+2) = zhex( data  & 0x0f);    
    *(tmpstr+3) = 0;
    return tmpstr;
}

char *z80_hexRTI(byte data,char *rg,char *tmpstr)
{
    *(tmpstr + 0) = *rg;
    *(tmpstr + 1) = *(rg+1);
    *(tmpstr + 2) = '+';
    if(data & 0x80) {
	data = ~data + 1;
	*(tmpstr + 2) = '-';
    }
    data &= 0x7f;
    *(tmpstr+3) = zhex((data >> 4) & 0x0f);
    *(tmpstr+4) = zhex( data & 0x0f);    
    *(tmpstr+5) = 0;
    return tmpstr;
}

byte f_z_8(byte c)
{
    if( c < 32 ) c='.';
    if(c > 127 ) c='.';
    return c;
}

void z80_displ_mem(Z80 *z)
{
    int i,j,tmp;
    char tmps0[128];
    char tmps1[128];
    char tmps2[256];

    for(i=0; i < zd.lines; i++){
	tmp = zd.daddr;
	for(j=0;j < 16;j++){
	    sprintf(tmps0+j*3,"%2x ",MR(zd.daddr+j+i*16) & 0xffff);
	}
	for(j=0;j < 16;j++){
	    zd.daddr++;
	    sprintf(tmps1+j,"%c",f_z_8(MR(zd.daddr) & 0xffff));
	}
	sprintf(tmps2,"%4x %s%s\n",tmp,tmps0,tmps1); 
	z->deb_out(tmps2);
    }    
}

void dummy_wrmem(void *ptr,word addr, byte data){}

void z80_deasm_mem(Z80 *z)
{
    Z80 zz;
    int i;
    memcpy(&zz,z,sizeof(Z80));
    zz.PCp = zd.deasm;
    for(i = 0; i < zd.lines; i++){
        zz.dry_run=1;
	zz.debug_print=1;
	zz.deb_halt=0;
	zd.free=1;
	zz.WR_MEM=dummy_wrmem;
	z80_exe(&zz);
    }
    zd.free=0;
    zd.deasm=zz.PCp;
}

void split_cmd(char *src,char **a)
{
    int i=1;
    char n=1;
        
    for(i=0;i < 4;i++) *(a + i) = NULL;

    if(!src) return;

    *a=src;    

    for(i=1;*src && (i < 5);src++) 
	    if(*src == ' '){
		if(n){
		    if(i < 4) a[i++] = src + 1;
		    *src=0;
		    n=0;
		}
	    } else n=1;
}

void z80_console(Z80 *z)
{
    char cmdline[256];
    char temp[256];
    char stin[256];   
    char *arg[4];
    int arg1,arg2,arg3,i;

    zd.fprint=0;
    zd.instr=0;
    zd.addr_brk=0;
    zd.fwcnt=0;
    zd.fcnt=0;
    zd.fbreak=0;
    zd.fwadr=0;
    zd.free=0;
    zd.hdr=0;

    z->deb_halt=1;
    z->act_freq=1;

    z->deb_in(stin);    
    if(!*stin) return;
    if(z->deb_stdout) printf(">%s\n",stin);
    sprintf(temp,">%s\n",stin);
    z->deb_out(temp);     
    z->debug_int=1; /* blokada przerwan */
    split_cmd(stin,arg);
    sscanf(arg[0],"%s",cmdline);
    if((!strcmp(cmdline,"quit"))||(!strcmp(cmdline,"q"))){
	z80_set_debug(z,0);
        z->exit(); 
    } else
    if((!strcmp(cmdline,"help"))||(!strcmp(cmdline,"h"))){
	 z->deb_out("Help:\nc - czysc ekran\nq.quit - wyjscie\nh,help - pomoc\n");
	 z->deb_out("t <xxxx> - krok wykonania instrukcji procesora (xxx - ilosc krokow)\n");
	 z->deb_out("p <xxxx> - wykonanie instrukcji lub ich serii \n");	 
	 z->deb_out("g xxxx - wykonanie do adresu xxxx\n");	 
	 z->deb_out("d <xxxx> - wyswietlenie zawartosci pamieci spod adresu xxxx\n");	 
	 z->deb_out("u <xxxx>  - deasemblacja zawartosci pamieci spod adresu xxxx\n");	 
	 z->deb_out("a adres data - wpisanie danej pod wskazany adres\n");	 
	 z->deb_out("ff adres nn dd - wypelnienie obszaru pamieci nn od adresu adres wartoscia dd\n");	 
	 z->deb_out("l xxxx - ustawienie xxxx linii wyswietlania \n");	 
	 z->deb_out("pout [0|1] - wyswietlanie instrukcji podczas wykonywania instrukcji \n");	 
	 z->deb_out("stdout [0|1] - kopia wyswietlania na stdout\n");	 
	 z->deb_out("find\n");	 
	 z->deb_out("in_port adres - pobiera i wyswietla wartosc postu o adresie adres\n");	 
	 z->deb_out("out_port adres dana - wysyla dana na port o adresie adres\n");	 
	 z->deb_out("set reg wartosc - ustawienie wartosci rejestru [AF,BC,DE,HL,AF',BC',DE',HL',IX,IY,SP,I,R,PC]\n");	 
	 z->deb_out("r - wyswietla stan rejestrow\n");	 
	 z->deb_out("x <adr> - wyswietla wartosc bajtu spod adresu adr\n");
    } else
    if(!strcmp(cmdline,"c")) { z->deb_out("\033[2J\033[1;1H"); } else
    if(!strcmp(cmdline,"t")) { 
	if(!arg[1]){
	    z->deb_halt=0; zd.fprint=zd.pout; zd.instr = z->cnt + 1;
	    zd.fcnt=1;
	    zd.hdr=1;
	    z->proc=0;
	} else {
	    sscanf(arg[1],"%x",&arg1);
	    z->deb_halt=0; zd.fprint=zd.pout; zd.instr = z->cnt + arg1;
	    zd.fcnt=1;
	    zd.hdr=1;
	    z->proc=0;
	    z->act_freq=z->lp_freq;
	}
    }else
    if(!strcmp(cmdline,"p")) { 
	if(!arg[1]){
	    z->deb_halt=0; zd.fprint=zd.pout; zd.instr = z->cnt + 1;
	    zd.fcnt=1;
	    zd.hdr=1;
	    z->proc=1;
	} else {
	    sscanf(arg[1],"%x",&arg1);
	    z->deb_halt=0; zd.fprint=zd.pout; zd.instr = z->cnt + arg1;
	    zd.fcnt=1;
	    zd.hdr=1;
	    z->proc=1;
	    z->act_freq=z->lp_freq;
	}
    }else
    if(!strcmp(cmdline,"g")) { 
	if(!arg[1]) return;
	sscanf(arg[1],"%x",&arg1);
	z->deb_halt=0; zd.fprint=zd.pout; zd.addr_brk = arg1;
	zd.fbreak=1;
	zd.hdr=1;
	z->proc=0;
	z->act_freq=z->lp_freq;
	z->debug_int=0;
    }else
    if(!strcmp(cmdline,"pout")) { 
	if(!arg[1]) return;
	sscanf(arg[1],"%x",&arg1);
	zd.pout=arg1;
    }else
    if(!strcmp(cmdline,"stdout")) { 
	if(!arg[1]){
	    z->deb_out("Za malo danych wejsciowych\n");
	    return;
	}
	sscanf(arg[1],"%x",&arg1);
	z->deb_stdout=arg1;
    }else
    if(!strcmp(cmdline,"d")) { 
	if(arg[1]){
	    sscanf(arg[1],"%x",&arg1);
	    zd.daddr=arg1;
	}
	z80_displ_mem(z);
    }else
    if(!strcmp(cmdline,"u")) { 
	if(arg[1]){
	    sscanf(arg[1],"%x",&arg1);
	    zd.deasm=arg1;
	}
	z80_deasm_mem(z);
    }else
    if(!strcmp(cmdline,"a")) { 
	if( !arg[1] || !arg[2]){
	    z->deb_out("Za malo danych wejsciowych\n");
	    return;
	 }
	sscanf(arg[1],"%x",&arg1);
	sscanf(arg[2],"%x",&arg2);	
	arg1 &= 0xffff; arg2 &= 0xff;
	MW(arg1,arg2);
    }else
    if(!strcmp(cmdline,"ff")) { 
	if( !arg[1] || !arg[2] || !arg[3]){
	    z->deb_out("Za malo danych wejsciowych\n");
	    return;
	 }
	sscanf(arg[1],"%x",&arg1);
	sscanf(arg[2],"%x",&arg2);	
	sscanf(arg[3],"%x",&arg3);	
	for(i=0;i < arg2; i++){
	    arg1 &= 0xffff;
	    MW(arg1,arg2);
	    arg1++;
	}
    }else
//    if(!strcmp(cmdline,"find")) { 
//
//    }else
    if(!strcmp(cmdline,"set")) { 
	if( !arg[1] || !arg[2]){
	    z->deb_out("Za malo danych wejsciowych\n");
	    return;
	 }
	sscanf(arg[1],"%s",cmdline);	
	sscanf(arg[2],"%x",&arg1);
	if(!strcmp(cmdline,"AF"))  {z->AFr=arg1;} else
	if(!strcmp(cmdline,"BC"))  {z->BCr=arg1;} else
	if(!strcmp(cmdline,"DE"))  {z->DEr=arg1;} else
	if(!strcmp(cmdline,"HL"))  {z->HLr=arg1;} else
	if(!strcmp(cmdline,"AF'")) {z->AFr_=arg1;} else
	if(!strcmp(cmdline,"BC'")) {z->BCr_=arg1;} else
	if(!strcmp(cmdline,"DE'")) {z->DEr_=arg1;} else
	if(!strcmp(cmdline,"HL'")) {z->HLr_=arg1;} else
	if(!strcmp(cmdline,"IX"))  {z->IXp=arg1;} else
	if(!strcmp(cmdline,"IY"))  {z->IYp=arg1;} else
	if(!strcmp(cmdline,"SP"))  {z->SPp=arg1;} else
	if(!strcmp(cmdline,"PC"))  {z->PCp=arg1;} else
	if(!strcmp(cmdline,"I"))   {z->Ip=arg1;} else
	if(!strcmp(cmdline,"R"))   {z->Rp=arg1;} 
	else{
	    sprintf(temp,"Nieznany rejestr %s\n",cmdline);
	    z->deb_out(temp);
	    return;
	}
    }else
    if(!strcmp(cmdline,"in_port")) { 
	if( !arg[1]){
	    z->deb_out("Za malo danych wejsciowych\n");
	    return;
	 }
	sscanf(arg[1],"%x",&arg1);
	sprintf(temp,"zawartoscia portu IN[%x] jest: %x\n",arg1, z->io(arg1,0,1,z));
	z->deb_out(temp);
    }else
    if(!strcmp(cmdline,"out_port")) { 
	if( !arg[1] || !arg[2]){
	    z->deb_out("Za malo danych wejsciowych\n");
	    return;
	 }
	sscanf(arg[1],"%x",&arg1);
	sscanf(arg[2],"%x",&arg2);
	z->io(arg1,arg2,1,z);
    }else
    if(!strcmp(cmdline,"l")) { 
	if( !arg[1]){
	    z->deb_out("Za malo danych wejsciowych\n");
	    return;
	 }
	sscanf(arg[1],"%x",&arg1);
	zd.lines=arg1;	
    }else
    if(!strcmp(cmdline,"x")) { 
	if(!arg[1]) return;
	sscanf(arg[1],"%x",&arg1);
	arg1 &= 0xffff;
	sprintf(temp,"MEM[%x]=%x\n",arg1,MR(arg1));
	z->deb_out(temp);
    }else

    if(!strcmp(cmdline,"r")) { 
	    sprintf(temp," AF | BC | DE | HL | AF'| BC'| DE'| HL'| IX | IY | SP | PC |SZHPNC| I|\
 R|IFF1|IFF2|IM\n%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%i%i%i%i%i%i|%2x|%2x|%4x|%4x|%2x\n",
	    z->AFr,z->BCr,z->DEr,z->HLr, z->AFr_,z->BCr_,z->DEr_,z->HLr_, IX,IY,SP,PC,
	    GET_SF,GET_ZF,GET_HF,GET_PF,GET_NF,GET_CF,I,R,z->IFF1p,z->IFF2p,z->IMr);
	    z->deb_out(temp);
    }else
     z->deb_out("Nieznane polecenie\n");
}

void z80_debugger_call(Z80 *z)
{
    static char fr=1;

    if(fr){
	memset(&zd,0,sizeof(zd));
	zd.free=0;
    }
    if(zd.free) return;
    z->debug_print=0;
    if(fr){
	z->deb_out("Tryb debuggera, h - pomoc, q - wyjscie\n");
	fr=0;
	z->deb_halt=1;
	zd.lines=16;
    }
    if(z->deb_halt) z80_console(z);
    if(zd.fprint) z->debug_print=1;    
    z->cnt++;
}

#endif
