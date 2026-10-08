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
#include "z80_debug.h"

void z80_debugger_deasm(Z80 *z, int lines, int pc);

/* rozwija skrót ³±ñcucha (je¶li taki jest), podany jako parametr w makrze DEBUG przy dekodowaniu intsrukcji */
char *z80_debugger_decode_string(Z80 *z, char *str, char *tmpstr)
{
    if(!str) return NULL;
    switch(*str){
	case 'U': return (z->DD_FD ? (z->xy ? "IY":"IX"):"HL");
	case 'V': return (z->DD_FD ? z80_hexU(DD,tmpstr):"  ");
	case 'W': return z80_hex8((z->NNp >> 8) & 0xff,tmpstr);
	case 'K': return z80_hex8(z->NNp & 0xff,tmpstr);
	case 'O': return z80_hex8(z->Np,tmpstr);
	case 'Q': return z80_hex16(PC + (char)z->Np,tmpstr);
	case 'G': return z80_hex16(z->NNp,tmpstr);
	case 'T': return z->DD_FD  ? z80_hexRTI(DD,DEB_RT,tmpstr) : "HL   ";
    }

    return str;
}

void z80_debug(Z80 *z, char *str, char *a, char *b, char class);

void z80_set_debug(Z80 *z,char fl){ z->deb_on = fl; }

void z80_debug_reset(Z80 *z)
{
    void *in,*out,*ex;

/*return;*/
    if(!z->debugger) return;
    ex = DEB(z)->exit;
    out = DEB(z)->output;
    in = DEB(z)->input;
    memset(z->deb_struct, 0, sizeof(Z80_DEB));
    z->deb_par_0 = z->deb_par_1 = z->deb_par_2 = NULL;
    z->deb_par_3 = z->deb_class = z->NNp = z->Np = 0;
    z->deb_dry   = z->debug_int = z->deb_halt = 0;
    z->deb_on = 0;
    DEB(z)->input = in;
    DEB(z)->output = out;
    DEB(z)->exit = ex;    
    DEB(z)->init=1;
    z->act_freq=z->lp_freq;
}

void z80_deb_call(void *zz, char call)
{
    Z80 *z=zz;

    if(!z->deb_struct) return;

    switch(call){
	case 0: z80_debug_reset(z); break;
	case 1: z80_debug(z,z->deb_par_0, z->deb_par_1, z->deb_par_2, z->deb_par_3); break;
	case 2: z->deb_class=0xff;
		z->old_pc = PC;
		z80_debugger_call(z);
		break;
    }

}

void z80_debugger_init(Z80 *z, Z80_DEB *deb, void (*deb_in)(char *), void (*deb_out)(char *), void (*exit_f)(void))
{
    if(!z) z = z80_get_struct();
    z->deb_struct = deb;
    if(deb){    
	deb->input  = deb_in;
	deb->output = deb_out;	
	deb->exit   = exit_f;
	z80_debug_reset(z);
    } else printf("Wrn: deb_struct == NULL !");
    z->debugger = z80_deb_call;
}

char z80_debug_test_break(Z80 *z)
{
    if(!DEB(z)) return 0;
/*
    return ((z->PCp == DEB(z)->addr_brk)||
	    (z->PCp >= DEB(z)->addr_ghe)||
	    (z->PCp <= DEB(z)->addr_gle)
           ) && (DEB(z)->fbreak);
*/
    return ((z->old_pc == DEB(z)->addr_brk)||
	    (z->old_pc >= DEB(z)->addr_ghe)||
	    (z->old_pc <= DEB(z)->addr_gle)
           ) && (DEB(z)->fbreak);

}

void z80_debug(Z80 *z, char *str, char *a, char *b, char class)
{
    char par_a_str[16];
    char par_b_str[16];    
    
    char tmpstr0[256];
    char tmpstr1[80];
    char tmpstr2[256];
    char *hdr="Adres         Instrukcja         AF | BC | DE | HL | AF'| BC'| DE'| HL'| IX | IY | SP |SZHPNC| I| R\n";
    char *tmpstr3=hdr;

    /* zdekodowanie parametrów - je¶li potrzeba */
    a = z80_debugger_decode_string(z, a, par_a_str);
    b = z80_debugger_decode_string(z, b, par_b_str);

    memset(tmpstr0,0,sizeof(tmpstr0));
    memset(tmpstr1,0,sizeof(tmpstr1));
    memset(tmpstr2,0,sizeof(tmpstr2));

    if(z->DD_FD) z->old_pc--;
    if((DEB(z)->cnt >= DEB(z)->instr)&&(DEB(z)->fcnt)){ z->deb_halt=1; DEB(z)->debug_print=1;}
    
//z->trap = z80_debug_test_break(z);

    if( z->trap ){
	 z->deb_halt=1; 
	 DEB(z)->debug_print=1;
	 DEB(z)->addr_brk = DEB(z)->addr_ghe = DEB(z)->addr_gle = 0x0000;
    }
    if(DEB(z)->debug_print){
	if(!DEB(z)->hdr) tmpstr3="\0";
	DEB(z)->hdr=0;
	sprintf(tmpstr0,str,a,b);
	sprintf(tmpstr1,"%4x %s",z->old_pc,tmpstr0);
	if(!z->deb_dry){
	    sprintf(tmpstr2,"\t%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%i%i%i%i%i%i|%2x|%2x",
		z->AFr,z->BCr,z->DEr,z->HLr, z->AFr_,z->BCr_,z->DEr_,z->HLr_, IX,IY,SP,
		GET_SF,GET_ZF,GET_HF,GET_PF,GET_NF,GET_CF,I,R);
	    DEB(z)->deasm = z->old_pc;
	    z->debug_int=1;
	}
	sprintf(tmpstr0,"%s%s%s\n",tmpstr3,tmpstr1,tmpstr2);
	DEB(z)->output(tmpstr0);
    }
    DEB(z)->debug_print=0;
    if((DEB(z)->fprint) && (DEB(z)->fbreak)) DEB(z)->debug_print=1;
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

    for(i=0; i < DEB(z)->lines; i++){
	tmp = DEB(z)->daddr;
	for(j=0;j < 16;j++){
	    sprintf(tmps0+j*3,"%2x ",MR(DEB(z)->daddr+j+i*16) & 0xffff);
	}
	for(j=0;j < 16;j++){
	    sprintf(tmps1+j,"%c",f_z_8(MR(DEB(z)->daddr+j+i*16) & 0xffff));
	}
	DEB(z)->daddr+=16;
	sprintf(tmps2,"%4x %s%s\n",tmp,tmps0,tmps1); 
	DEB(z)->output(tmps2);
    }    
}

void dummy_wrmem(void *ptr,word addr, byte data){}

void z80_debugger_deasm(Z80 *z, int lines, int pc)
{
    Z80 zz;
    Z80_DEB db;
    int i;

    if(lines == 0) return;
    
    memcpy(&zz,z,sizeof(Z80));
    memcpy(&db,z->deb_struct,sizeof(Z80_DEB));
    zz.deb_struct = &db;
    z80_reset(&zz); 		 	/* mamy teraz kopiê CPU, korzystaj±cego z tych samych zasobów */
    zz.deb_dry = 1; 		 	/* nie modyfikuje zasobów  */
    ((Z80_DEB*)(zz.cust))->free=1;	/* nie wchodzi do konsoli  */
    zz.run = 1; zz.deb_on = 1;  	/* uruchomia z debuggerem  */
    zz.PCp = DEB(z)->deasm;		/* adres deasemblacji      */
    zz.debug_int = 1;			/* ignoruj przerwania      */
    zz.WR_MEM=dummy_wrmem;    		/* nie modyfikuj pamiêci   */
    zz.PCp = pc;
    for(i = 0; i < lines; i++){
	((Z80_DEB *)(zz.deb_struct))->debug_print = 1;
	zz.deb_halt=0;
	z80_exe(&zz);
    }
    DEB(z)->free=0;
    DEB(z)->deasm=zz.PCp;
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

    DEB(z)->fprint=0;
    DEB(z)->instr=0;
    DEB(z)->addr_brk=0;
    DEB(z)->fwcnt=0;
    DEB(z)->fcnt=0;
    DEB(z)->fbreak=0;
    DEB(z)->fwadr=0;
    DEB(z)->free=0;
    DEB(z)->hdr=0;

    z->deb_halt=1;
    z->act_freq=1;
    z->trap=0;
    if(DEB(z)->fin){
	z80_debugger_deasm(z,2,PC);
	DEB(z)->fin=0;
    }
    DEB(z)->input(stin);
    if(!*stin) return;
    DEB(z)->fin = 0;
    if(DEB(z)->deb_stdout) printf(">%s\n",stin);
    sprintf(temp,">%s\n",stin);
    DEB(z)->output(temp);     
    z->debug_int=1; /* blokada przerwan */
    split_cmd(stin,arg);
    sscanf(arg[0],"%s",cmdline);
    if((!strcmp(cmdline,"quit"))||(!strcmp(cmdline,"q"))){
	z80_set_debug(z,0);
	z80_debug_reset(z);
        DEB(z)->exit(); 
    } else
    if((!strcmp(cmdline,"help"))||(!strcmp(cmdline,"h"))){
	 DEB(z)->output("Help:\nc - czysc ekran\nq.quit - wyjscie\nh,help - pomoc\n");
	 DEB(z)->output("t <xxxx> - krok wykonania instrukcji procesora (xxx - ilosc krokow)\n");
	 DEB(z)->output("p <xxxx> - wykonanie instrukcji lub ich serii \n");	 
	 DEB(z)->output("d <xxxx> - wyswietlenie zawartosci pamieci spod adresu xxxx\n");	 
	 DEB(z)->output("u <xxxx>  - deasemblacja zawartosci pamieci spod adresu xxxx\n");	 
	 DEB(z)->output("a adres data - wpisanie danej pod wskazany adres\n");	 
	 DEB(z)->output("ff adres nn dd - wypelnienie obszaru pamieci nn od adresu adres wartoscia dd\n");	 
	 DEB(z)->output("l xxxx - ustawienie xxxx linii wyswietlania \n");	 
	 DEB(z)->output("pout [0|1] - wyswietlanie instrukcji podczas wykonywania instrukcji \n");	 
	 DEB(z)->output("stdout [0|1] - kopia wyswietlania na stdout\n");	 
	 DEB(z)->output("find\n");	 
	 DEB(z)->output("in_port adres - pobiera i wyswietla wartosc postu o adresie adres\n");	 
	 DEB(z)->output("out_port adres dana - wysyla dana na port o adresie adres\n");	 
	 DEB(z)->output("set reg wartosc - ustawienie wartosci rejestru [AF,BC,DE,HL,AF',BC',DE',HL',IX,IY,SP,I,R,PC]\n");	 
	 DEB(z)->output("r - wyswietla stan rejestrow\n");	 
	 DEB(z)->output("x <adr> - wyswietla wartosc bajtu spod adresu adr\n");
	 DEB(z)->output("g addr - pulapka na adres rowny\n");	 
	 DEB(z)->output("ghe adr - pulapka na adres wiekszy lub rowny \n");	
	 DEB(z)->output("gle adr - pulapka na adres mniejszy lub rowny \n");	
    } else
    if(!strcmp(cmdline,"c")) { DEB(z)->output("\033[2J\033[1;1H"); } else
    if(!strcmp(cmdline,"t")) { 
	if(!arg[1]){
	    z->deb_halt=0; DEB(z)->fprint=DEB(z)->pout; DEB(z)->instr = DEB(z)->cnt + 1;
	    DEB(z)->fcnt=1;
	    DEB(z)->hdr=1;
	    DEB(z)->proc=0;
	} else {
	    sscanf(arg[1],"%x",&arg1);
	    z->deb_halt=0; DEB(z)->fprint=DEB(z)->pout; DEB(z)->instr = DEB(z)->cnt + arg1;
	    DEB(z)->fcnt=1;
	    DEB(z)->hdr=1;
	    DEB(z)->proc=0;
	    z->act_freq=z->lp_freq;
	}
	DEB(z)->fin = 1;
    }else
    if(!strcmp(cmdline,"p")) { 
	if(!arg[1]){
	    DEB(z)->proc=1;
	} else {
	    sscanf(arg[1],"%x",&arg1);
	    z->deb_halt=0; DEB(z)->fprint=DEB(z)->pout; DEB(z)->instr = DEB(z)->cnt + arg1;
	    DEB(z)->fcnt=1;
	    DEB(z)->hdr=1;
	    DEB(z)->proc=1;
	    z->act_freq=z->lp_freq;
	}
	DEB(z)->fin = 1;
    }else
    if(!strcmp(cmdline,"g")) { 
	if(!arg[1]) return;
	sscanf(arg[1],"%x",&arg1);
	z->deb_halt=0; DEB(z)->fprint=DEB(z)->pout; DEB(z)->addr_brk = arg1;

	DEB(z)->fbreak=1;
	DEB(z)->hdr=1;
	DEB(z)->proc=0;
	z->act_freq=z->lp_freq;
	z->debug_int=0;
	DEB(z)->fin = 1;
    }else
    if(!strcmp(cmdline,"ghe")) { 
	if(!arg[1]) return;
	sscanf(arg[1],"%x",&arg1);
	z->deb_halt=0; DEB(z)->fprint=DEB(z)->pout; DEB(z)->addr_ghe = arg1;
	DEB(z)->fbreak=1;
	DEB(z)->hdr=1;
	DEB(z)->proc=0;
	z->act_freq=z->lp_freq;
	z->debug_int=0;
	DEB(z)->fin = 1;
    }else
    if(!strcmp(cmdline,"gle")) { 
	if(!arg[1]) return;
	sscanf(arg[1],"%x",&arg1);
	z->deb_halt=0; DEB(z)->fprint=DEB(z)->pout; DEB(z)->addr_gle = arg1;
	DEB(z)->fbreak=1;
	DEB(z)->hdr=1;
	DEB(z)->proc=0;
	z->act_freq=z->lp_freq;
	z->debug_int=0;
	DEB(z)->fin = 1;
    }else
    if(!strcmp(cmdline,"pout")) { 
	if(!arg[1]) return;
	sscanf(arg[1],"%x",&arg1);
	DEB(z)->pout=arg1;
    }else
    if(!strcmp(cmdline,"stdout")) { 
	if(!arg[1]){
	    DEB(z)->output("Za malo danych wejsciowych\n");
	    return;
	}
	sscanf(arg[1],"%x",&arg1);
	DEB(z)->deb_stdout=arg1;
    }else
    if(!strcmp(cmdline,"d")) { 
	if(arg[1]){
	    sscanf(arg[1],"%x",&arg1);
	    DEB(z)->daddr=arg1;
	}
	z80_displ_mem(z);
    }else
    if(!strcmp(cmdline,"u")) { 
	if(arg[1]){
	    sscanf(arg[1],"%x",&arg1);
	    DEB(z)->deasm=arg1;
	}
	if(!DEB(z)->deasm)DEB(z)->deasm = PC;
	z80_debugger_deasm(z,DEB(z)->lines, DEB(z)->deasm);
    }else
    if(!strcmp(cmdline,"a")) { 
	if( !arg[1] || !arg[2]){
	    DEB(z)->output("Za malo danych wejsciowych\n");
	    return;
	 }
	sscanf(arg[1],"%x",&arg1);
	sscanf(arg[2],"%x",&arg2);	
	arg1 &= 0xffff; arg2 &= 0xff;
	MW(arg1,arg2);
    }else
    if(!strcmp(cmdline,"ff")) { 
	if( !arg[1] || !arg[2] || !arg[3]){
	    DEB(z)->output("Za malo danych wejsciowych\n");
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
	    DEB(z)->output("Za malo danych wejsciowych\n");
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
	    DEB(z)->output(temp);
	    return;
	}
    }else
    if(!strcmp(cmdline,"in_port")) { 
	if( !arg[1]){
	    DEB(z)->output("Za malo danych wejsciowych\n");
	    return;
	 }
	sscanf(arg[1],"%x",&arg1);
	sprintf(temp,"zawartoscia portu IN[%x] jest: %x\n",arg1, z->io(arg1,0,1,z));
	DEB(z)->output(temp);
    }else
    if(!strcmp(cmdline,"out_port")) { 
	if( !arg[1] || !arg[2]){
	    DEB(z)->output("Za malo danych wejsciowych\n");
	    return;
	 }
	sscanf(arg[1],"%x",&arg1);
	sscanf(arg[2],"%x",&arg2);
	z->io(arg1,arg2,1,z);
    }else
    if(!strcmp(cmdline,"l")) { 
	if( !arg[1]){
	    DEB(z)->output("Za malo danych wejsciowych\n");
	    return;
	 }
	sscanf(arg[1],"%x",&arg1);
	DEB(z)->lines=arg1;	
    }else
    if(!strcmp(cmdline,"x")) { 
	if(!arg[1]) return;
	sscanf(arg[1],"%x",&arg1);
	arg1 &= 0xffff;
	sprintf(temp,"MEM[%x]=%x\n",arg1,MR(arg1));
	DEB(z)->output(temp);
    }else

    if(!strcmp(cmdline,"r")) { 
	    sprintf(temp," AF | BC | DE | HL | AF'| BC'| DE'| HL'| IX | IY | SP | PC |SZHPNC| I|\
 R|IFF1|IFF2|IM\n%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%4x|%i%i%i%i%i%i|%2x|%2x|%4x|%4x|%2x\n",
	    z->AFr,z->BCr,z->DEr,z->HLr, z->AFr_,z->BCr_,z->DEr_,z->HLr_, IX,IY,SP,PC,
	    GET_SF,GET_ZF,GET_HF,GET_PF,GET_NF,GET_CF,I,R,z->IFF1p,z->IFF2p,z->IMr);
	    DEB(z)->output(temp);
    }else
     DEB(z)->output("Nieznane polecenie\n");
}

void z80_debugger_call(Z80 *z)
{

    if(DEB(z)->free) return;

    z->deb_on = 1;
    
//    DEB(z)->debug_print=0;
    if(DEB(z)->init){
	if(!z->deb_dry)
	    DEB(z)->output("Tryb debuggera, h - pomoc, q - wyjscie\n");
	DEB(z)->init=0;
	z->deb_halt=1;
	DEB(z)->lines=16;
    }
    if(z->deb_halt) z80_console(z);
    if(DEB(z)->fprint) DEB(z)->debug_print=1;    
    DEB(z)->cnt++;
}


