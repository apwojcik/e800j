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

#include "z80.h"
#include "z80_cpu.h"
#include "z80_instr.h"
#include "z80_debug.h"

/* 
    wewnêtrzna struktura Z80, je¶li parametr w wywo³aniu funkcji zewnêtrznych 
    jest == NULL, to oznacza, ¿e maj± one korzystaæ z tej zmiennej
*/
Z80 *z80 = (void *)0;	

byte z80_parity_check(byte a)
{
    a ^= a >> 4;
    a ^= a >> 2;
    a ^= a >> 1;
    return !(a & 0x1);
}

void z80_reset(Z80 *zz)
{
    int freq,i,int_cnt,nmi_cnt;
    void *io, *rd, *wr, *cust, *deb, *deb_str;
    Z80 *z;

    Z80_STR(zz);
    cust = z->cust;
    io   = z->io;
    rd   = z->RD_MEM;
    wr   = z->WR_MEM;
    deb  = z->debugger;
    freq=z->lp_freq;
    deb_str = z->deb_struct;
    int_cnt = z->int_cnt;
    nmi_cnt = z->nmi_cnt;
    
    for(i=0;i < sizeof(Z80);i++) *((byte*)z+i)=0;

    z->io=io;
    z->RD_MEM=rd;
    z->WR_MEM=wr;
    z->lp_freq=z->act_freq=freq;
    z->cust = cust;
    z->debugger = deb;
    z->deb_struct = deb_str;
    z->int_cnt = int_cnt;
    z->nmi_cnt = nmi_cnt;

    SET_UNI_NRM;
    SET_AF;
#ifdef DEBUGGER    
    z80_deb_call(z,DEB_CTL_RESET);
#endif
}

void z80_init (Z80 *zz, void *cust, int freq, void *io, void *rd, void *wr)
{
    Z80 *z;

    Z80_STR(zz);
    z80 = z;
    z80_reset(z);
    z->io=io;
    z->RD_MEM=rd;
    z->WR_MEM=wr;
    z->lp_freq=z->act_freq=freq;
    z->cust=cust;    
}

Z80 *z80_get_struct(void){ return z80; }

byte z80_get_byte(Z80 *z, word addr)
{
    return z->RD_MEM(z, addr);
}

word z80_mrw(Z80 *z, word a)
{
    return (word)z80_get_byte(z,a) + (word)(z80_get_byte(z,a + 1) << 8);
}

void z80_mww(Z80 *z, word c, word d)
{
    if(z->deb_dry) return;
    z->WR_MEM(z,c,d & 0xff);
    z->WR_MEM(z,c+1,(d >> 8) & 0xff);
}

word z80_get_word(Z80 *z, word addr)
{
    word tmp;

    tmp = z80_get_byte(z,addr);
    tmp |= z80_get_byte(z,addr+1) << 8;    
    return tmp;
}

byte z80_fetch_byte(Z80 *z)
{
    byte tmp;

    if(z->INT_FETCH){
	tmp = *(z->INT_DATA + z->INT_f);
	z->INT_f++;
    }else{

	tmp = z80_get_byte(z,PC);
	PC++;

    }   
    return tmp; 
}

word z80_fetch_word(Z80 *z)
{
    word tmp;

    tmp = z80_fetch_byte(z);
    tmp |= z80_fetch_byte(z) << 8;    
    return tmp;
}

void z80_send_int(Z80 *zz, byte *data)
{ 
    Z80 *z;
    Z80_STR(zz);
    z->INT = 1; z->INT_DATA = data; 
}

void z80_send_nmi(Z80 *zz)
{ 
    Z80 *z;

    Z80_STR(zz);
    z->NMI = 1; 
}

void z80_set_run(Z80 *zz, char fl)
{ 
    Z80 *z;

    Z80_STR(zz);
    z->run=fl; 
}

void z80_exec(Z80 *zz, void (*irqm)(void), void (*irqn)(void))
{
    int i,in,nm;
    Z80 *z;

    Z80_STR(zz);

    in=nm=0;
    for(i=0; i < z->act_freq; i++){
	 if(((in++) > z->int_cnt) && irqm ) {irqm(); in = 0; }
	 if(((nm++) > z->nmi_cnt) && irqn ) {irqn(); nm=0;   }
	 z80_exe(z);	 
    }
}

byte z80_test_flag(Z80 *z, byte f)
{ 
    switch(f){
	case GNZ: return (!GET_ZF) ? 1:0;
	case GZ : return (GET_ZF) ? 1:0;	
	case GNC: return (!GET_CF) ? 1:0;
	case GC : return (GET_CF) ? 1:0;
	case GPO: return (!GET_PF) ? 1:0;
	case GPE: return (GET_PF) ? 1:0;
	case GP : return (!GET_SF) ? 1:0;
	case GM : return (GET_SF) ? 1:0;
    }
    return 0;
}

void z80_exe(Z80 *zz)
{
    byte tmp;
    Z80 *z;

    Z80_STR(zz);
#ifdef DEBUGGER
    if((z->debugger) && (z->deb_on)){	/* czy w trybie debuggera ? */
//	z->trap = z80_debug_test_break(z);
	z->debugger(z,DEB_CTL_DEBUG);	/* wywo³aj debuggera        */
	if(z->deb_halt) return;
    }
#endif
    if(!z->run) return;
//    TICKS_RST;
    R = (R + 1) & 0x7f; 	/* licznik od¶wierzania - zawsze pracuje, nawet przy HALT */
    if(!z->Halt_state) 		/* je¶li nie HALT dekoduj instrukcjê, inaczej wykonaj NOP */
    { 
	z->dd = 0;			/* wyzeruj ofset dla operacji indeksowanych IX/IY	  */
	DEBUGGER_CLEAN;		/* wyczyszczenie zmiennych debuggera, przed deasemblacj±  */
/* wewnetrzne wykonanie dekodowania rozkazu, wywo³ane z 3 miejsc: po DD, po FD, obs³uga INT w IM 0 */
LB_DD_FD:
	z->old_pc=PC;
	SET_M1(1);		/* pobieranie / dekodowanie instrukcji 			  */
    switch(z80_fetch_byte(z)){
	case 0x00: NOP;			DEBUG("NOP                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x01: LD(BC,NN);		DEBUG("LD        BC,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0x02: MW(BC,A);		DEBUG("LD        (BC),A    %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x03: INCW(BC);		DEBUG("INC       BC        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x04: INC(B);		DEBUG("INC       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x05: DEC(B);		DEBUG("DEC       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x06: LD(B,N);		DEBUG("LD        B,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0x07: RLCA;		DEBUG("RLCA                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x08: EX_AF;		DEBUG("EX        AF,AF'    %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x09: ADDW(HL,BC);		DEBUG("ADD       %s,BC     %s",  DEB_RT,DEB_EP,DEB_HL);break;
	case 0x0a: LD(A,MR(BC));	DEBUG("LD        A,(BC)    %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x0b: DECW(BC);		DEBUG("DEC       BC        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x0c: INC(C);		DEBUG("INC       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x0d: DEC(C);		DEBUG("DEC       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x0e: LD(C,N);		DEBUG("LD        C,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0x0f: RRCA;		DEBUG("RRCA                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x10: DJNZ(N);		DEBUG("DJNZ      0x%s         ", DEB_DIS,DEB_DFP,DEB_NRM);break;
	case 0x11: LD(DE,NN);		DEBUG("LD        DE,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0x12: MW(DE,A);		DEBUG("LD        (DE),A    %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x13: INCW(DE);		DEBUG("INC       DE        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x14: INC(D);		DEBUG("INC       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x15: DEC(D);		DEBUG("DEC       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x16: LD(D,N);		DEBUG("LD        D,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0x17: RLA;			DEBUG("RLA                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x18: JR_(N);		DEBUG("JR        0x%s         ", DEB_DIS,DEB_DFP,DEB_NRM);break;
	case 0x19: ADDW(HL,DE);		DEBUG("ADD       %s,DE     %s",  DEB_RT,DEB_EP,DEB_HL);break;
	case 0x1a: LD(A,MR(DE));	DEBUG("LD        A,(DE)    %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x1b: DECW(DE);		DEBUG("DEC       DE        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x1c: INC(E);		DEBUG("INC       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x1d: DEC(E);		DEBUG("DEC       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x1e: LD(E,N);		DEBUG("LD        E,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0x1f: RRA;			DEBUG("RRA                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x20: JR(GNZ,N);		DEBUG("JR        NZ,0x%s      ", DEB_DIS,DEB_DFP,DEB_NRM);break;
	case 0x21: LD(HL,NN);		DEBUG("LD        %s,0x%s     ",  DEB_RT,DEB_HEX16,DEB_HL);break;
	case 0x22: MWW(NN,HL);		DEBUG("LD        (0x%s),%s   ",  DEB_HEX16,DEB_RT,DEB_HL);break;
	case 0x23: INCW(HL);		DEBUG("INC       %s        %s",  DEB_RT,DEB_EP,DEB_HL);break;
	case 0x24: INC(XH);		DEBUG("INC       %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x25: DEC(XH);		DEBUG("DEC       %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x26: LD(XH,N);		DEBUG("LD        %s,0x%s       ",DEB_DXH,DEB_NS,DEB_NRM);break;
	case 0x27: DAA;			DEBUG("DAA                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x28: JR(GZ,N);		DEBUG("JR        Z,0x%s       ", DEB_DIS,DEB_DFP,DEB_NRM);break;
	case 0x29: ADDW(HL,HL);		DEBUG("ADD       %s,%s       ",  DEB_RT,DEB_RT,DEB_HL);break;
	case 0x2a: LD(HL,MRW(NN));	DEBUG("LD        %s,(0x%s)   ",  DEB_RT,DEB_HEX16,DEB_HL);break;
	case 0x2b: DECW(HL);		DEBUG("DEC       %s        %s",  DEB_RT,DEB_EP,DEB_HL);break;
	case 0x2c: INC(XL);		DEBUG("INC       %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x2d: DEC(XL);		DEBUG("DEC       %s           %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x2e: LD(XL,N);		DEBUG("LD        %s,0x%s       ", DEB_DXL,DEB_NS,DEB_NRM);break;
	case 0x2f: CPL;			DEBUG("CPL                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x30: JR(GNC,N);		DEBUG("JR        NC,0x%s      ", DEB_DIS,DEB_DFP,DEB_NRM);break;
	case 0x31: LD(SP,NN);		DEBUG("LD        SP,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0x32: MW(NN,A);		DEBUG("LD        (0x%s%s),A    ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0x33: INCW(SP);		DEBUG("INC       SP        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x34: INCM;		DEBUG("INC       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
	case 0x35: DECM;		DEBUG("DEC       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
	case 0x36: GET_DD;MW(HL+DD,N);	DEBUG("LD        (%s),0x%s  ",   DEB_RTI,DEB_NS,DEB_M);break;
	case 0x37: SCF;			DEBUG("SCF                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x38: JR(GC,N);		DEBUG("JR        C,0x%s       ", DEB_DIS,DEB_DFP,DEB_NRM);break;
	case 0x39: ADDW(HL,SP);		DEBUG("ADD       %s,SP     %s",  DEB_RT,DEB_EP,DEB_HL);break;
	case 0x3a: LD(A,MR(NN));	DEBUG("LD        A,(0x%s%s)    ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0x3b: DECW(SP);		DEBUG("DEC       SP        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x3c: INC(A);		DEBUG("INC       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x3d: DEC(A);		DEBUG("DEC       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x3e: LD(A,N);		DEBUG("LD        A,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0x3f: CCF;			DEBUG("CCF                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x40: LD(B,B);		DEBUG("LD        B,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x41: LD(B,C);		DEBUG("LD        B,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x42: LD(B,D);		DEBUG("LD        B,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x43: LD(B,E);		DEBUG("LD        B,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x44: LD(B,XH);		DEBUG("LD        B,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x45: LD(B,XL);		DEBUG("LD        B,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x46: GET_DD;LD(B,MR(HL+DD));
					DEBUG("LD        B,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x47: LD(B,A);		DEBUG("LD        B,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x48: LD(C,B);		DEBUG("LD        C,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x49: LD(C,C);		DEBUG("LD        C,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x4a: LD(C,D);		DEBUG("LD        C,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x4b: LD(C,E);		DEBUG("LD        C,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x4c: LD(C,XH);		DEBUG("LD        C,%s        %s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x4d: LD(C,XL);		DEBUG("LD        C,%s      %s%s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x4e: GET_DD;LD(C,MR(HL+DD));
					DEBUG("LD        C,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x4f: LD(C,A);		DEBUG("LD        C,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x50: LD(D,B);		DEBUG("LD        D,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x51: LD(D,C);		DEBUG("LD        D,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x52: LD(D,D);		DEBUG("LD        D,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x53: LD(D,E);		DEBUG("LD        D,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x54: LD(D,XH);		DEBUG("LD        D,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x55: LD(D,XL);		DEBUG("LD        D,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x56: GET_DD;LD(D,MR(HL+DD));
					DEBUG("LD        D,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x57: LD(D,A);		DEBUG("LD        D,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x58: LD(E,B);		DEBUG("LD        E,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x59: LD(E,C);		DEBUG("LD        E,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x5a: LD(E,D);		DEBUG("LD        E,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x5b: LD(E,E);		DEBUG("LD        E,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x5c: LD(E,XH);		DEBUG("LD        E,%s        %s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x5d: LD(E,XL);		DEBUG("LD        E,%s        %s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x5e: GET_DD;LD(E,MR(HL+DD));
					DEBUG("LD        E,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x5f: LD(E,A);		DEBUG("LD        E,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x60: LD(XH,B);		DEBUG("LD        %s,B        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x61: LD(XH,C);		DEBUG("LD        %s,C        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x62: LD(XH,D);		DEBUG("LD        %s,D        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x63: LD(XH,E);		DEBUG("LD        %s,E        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x64: LD(XH,XH);		DEBUG("LD        %s,%s         ",DEB_DXH,DEB_DXH,DEB_NRM);break;
	case 0x65: LD(XH,XL);		DEBUG("LD        %s,%s         ",DEB_DXH,DEB_DXL,DEB_NRM);break;
	case 0x66: GET_DD;LD(H,MR(HL+DD));
					DEBUG("LD        H,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x67: LD(XH,A);		DEBUG("LD        %s,A         %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x68: LD(XL,B);		DEBUG("LD        %s,B         %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x69: LD(XL,C);		DEBUG("LD        %s,C         %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x6a: LD(XL,D);		DEBUG("LD        %s,D         %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x6b: LD(XL,E);		DEBUG("LD        %s,E         %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x6c: LD(XL,XH);		DEBUG("LD        %s,%s          ",DEB_DXL,DEB_DXH,DEB_NRM);break;
	case 0x6d: LD(XL,XL);		DEBUG("LD        %s,%s          ",DEB_DXL,DEB_DXL,DEB_NRM);break;
	case 0x6e: GET_DD;LD(L,MR(HL+DD));
					DEBUG("LD        L,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x6f: LD(XL,A);		DEBUG("LD        %s,A         %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x70: GET_DD;MW(HL+DD,B);	DEBUG("LD        (%s%s),B ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x71: GET_DD;MW(HL+DD,C);	DEBUG("LD        (%s%s),C ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x72: GET_DD;MW(HL+DD,D);	DEBUG("LD        (%s%s),D ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x73: GET_DD;MW(HL+DD,E);	DEBUG("LD        (%s%s),E ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x74: GET_DD;MW(HL+DD,H);	DEBUG("LD        (%s%s),H ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x75: GET_DD;MW(HL+DD,L);	DEBUG("LD        (%s%s),L ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x76: HALT;		DEBUG("HALT                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x77: GET_DD;MW(HL+DD,A);	DEBUG("LD        (%s%s),A ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x78: LD(A,B);		DEBUG("LD        A,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x79: LD(A,C);		DEBUG("LD        A,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x7a: LD(A,D);		DEBUG("LD        A,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x7b: LD(A,E);		DEBUG("LD        A,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x7c: LD(A,XH);		DEBUG("LD        A,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x7d: LD(A,XL);		DEBUG("LD        A,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x7e: GET_DD;LD(A,MR(HL+DD));
					DEBUG("LD        A,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x7f: LD(A,A);		DEBUG("LD        A,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x80: ADD(A,B);		DEBUG("ADD       A,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x81: ADD(A,C);		DEBUG("ADD       A,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x82: ADD(A,D);		DEBUG("ADD       A,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x83: ADD(A,E);		DEBUG("ADD       A,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x84: ADD(A,XH);		DEBUG("ADD       A,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x85: ADD(A,XL);		DEBUG("ADD       A,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x86: GET_DD;ADC(A,MR(HL+DD));
					DEBUG("ADD       A,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x87: ADD(A,A);		DEBUG("ADD       A,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x88: ADC(A,B);		DEBUG("ADC       A,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x89: ADC(A,C);		DEBUG("ADC       A,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x8a: ADC(A,D);		DEBUG("ADC       A,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x8b: ADC(A,E);		DEBUG("ADC       A,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x8c: ADC(A,XH);		DEBUG("ADC       A,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x8d: ADC(A,XL);		DEBUG("ADC       A,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x8e: GET_DD;ADC(A,MR(HL+DD));	
					DEBUG("ADC       A,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x8f: ADC(A,A);		DEBUG("ADC       A,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x90: SUB(A,B);		DEBUG("SUB       A,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x91: SUB(A,C);		DEBUG("SUB       A,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x92: SUB(A,D);		DEBUG("SUB       A,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x93: SUB(A,E);		DEBUG("SUB       A,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x94: SUB(A,XH);		DEBUG("SUB       A,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x95: SUB(A,XL);		DEBUG("SUB       A,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x96: GET_DD;SUB(A,MR(HL+DD));
					DEBUG("SUB       A,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x97: SUB(A,A);		DEBUG("SUB       A,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x98: SBC(A,B);		DEBUG("SBC       A,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x99: SBC(A,C);		DEBUG("SBC       A,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x9a: SBC(A,D);		DEBUG("SBC       A,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x9b: SBC(A,E);		DEBUG("SBC       A,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x9c: SBC(A,XH);		DEBUG("SBC       A,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0x9d: SBC(A,XL);		DEBUG("SBC       A,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0x9e: GET_DD;SBC(A,MR(HL+DD));
					DEBUG("SBC       A,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x9f: SBC(A,A);		DEBUG("SBC       A,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa0: AND(B);		DEBUG("AND       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa1: AND(C);		DEBUG("AND       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa2: AND(D);		DEBUG("AND       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa3: AND(E);		DEBUG("AND       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa4: AND(XH);		DEBUG("AND       %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0xa5: AND(XL);		DEBUG("AND       %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0xa6: GET_DD;GET_DD;AND(MR(HL+DD));	
					DEBUG("AND       (%s%s)   ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0xa7: AND(A);		DEBUG("AND       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa8: XOR(B);		DEBUG("XOR       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa9: XOR(C);		DEBUG("XOR       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xaa: XOR(D);		DEBUG("XOR       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xab: XOR(E);		DEBUG("XOR       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xac: XOR(XH);		DEBUG("XOR       %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0xad: XOR(XL);		DEBUG("XOR       %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0xae: GET_DD;XOR(MR(HL+DD));	
					DEBUG("XOR       (%s%s)   ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0xaf: XOR(A);		DEBUG("XOR       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb0: OR(B);		DEBUG("OR        B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb1: OR(C);		DEBUG("OR        C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb2: OR(D);		DEBUG("OR        D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb3: OR(E);		DEBUG("OR        E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb4: OR(XH);		DEBUG("OR        %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0xb5: OR(XL);		DEBUG("OR        %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0xb6: GET_DD;OR(MR(HL+DD));	
					DEBUG("OR        (%s%s)   ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0xb7: OR(A);		DEBUG("OR        A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb8: CP(B);		DEBUG("CP        B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb9: CP(C);		DEBUG("CP        C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xba: CP(D);		DEBUG("CP        D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xbb: CP(E);		DEBUG("CP        E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xbc: CP(XH);		DEBUG("CP        %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
	case 0xbd: CP(XL);		DEBUG("CP        %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
	case 0xbe: GET_DD;CP(MR(HL+DD));DEBUG("CP        (%s%s)   ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0xbf: CP(A);		DEBUG("CP        A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xc0: RET(GNZ);		DEBUG("RET       NZ        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xc1: POP(BC);		DEBUG("POP       BC        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xc2: JP(GNZ,NN);		DEBUG("JP        NZ,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xc3: JP_(NN);		DEBUG("JP        0x%s%s        ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xc4: CALL(GNZ,NN);	DEBUG("CALL      NZ,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xc5: PUSH(BC);		DEBUG("PUSH      BC        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xc6: ADD(A,N);		DEBUG("ADD       A,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0xc7: RST(0);		DEBUG("RST       #0        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xc8: RET(GZ);		DEBUG("RET       Z         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xc9: RET_;		DEBUG("RET                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xca: JP(GZ,NN);		DEBUG("JP        Z,0x%s%s      ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xcb: GET_DD; switch(z80_fetch_byte(z)){
		case 0x00: RLC(B);		DEBUG("RLC       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x01: RLC(C);		DEBUG("RLC       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x02: RLC(D);		DEBUG("RLC       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x03: RLC(E);		DEBUG("RLC       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x04: RLC(XH);		DEBUG("RLC       %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x05: RLC(XL);		DEBUG("RLC       %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x06: MEM(RLC(tmp));	DEBUG("RLC       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x07: RLC(A);		DEBUG("RLC       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x08: RRC(B);		DEBUG("RRC       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x09: RRC(C);		DEBUG("RRC       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0a: RRC(D);		DEBUG("RRC       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0b: RRC(E);		DEBUG("RRC       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0c: RRC(XH);		DEBUG("RRC       %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x0d: RRC(XL);		DEBUG("RRC       %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x0e: MEM(RRC(tmp));	DEBUG("RRC       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x0f: RRC(A);		DEBUG("RRC       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x10: RL(B); 		DEBUG("RL        B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x11: RL(C); 		DEBUG("RL        C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x12: RL(D); 		DEBUG("RL        D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x13: RL(E); 		DEBUG("RL        E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x14: RL(XH); 		DEBUG("RL        %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x15: RL(XL); 		DEBUG("RL        %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x16: MEM(RL(tmp));	DEBUG("RL        (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x17: RL(A); 		DEBUG("RL        A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x18: RR(B); 		DEBUG("RR        B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x19: RR(C); 		DEBUG("RR        C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1a: RR(D); 		DEBUG("RR        D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1b: RR(E); 		DEBUG("RR        E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1c: RR(XH); 		DEBUG("RR        %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x1d: RR(XL); 		DEBUG("RR        %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x1e: MEM(RR(tmp));	DEBUG("RR        (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x1f: RR(A); 		DEBUG("RR        A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x20: SLA(B);		DEBUG("SLA       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x21: SLA(C);		DEBUG("SLA       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x22: SLA(D);		DEBUG("SLA       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x23: SLA(E);		DEBUG("SLA       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x24: SLA(XH);		DEBUG("SLA       %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x25: SLA(XL);		DEBUG("SLA       %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x26: MEM(SLA(tmp));	DEBUG("SLA       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x27: SLA(A);		DEBUG("SLA       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x28: SRA(B);		DEBUG("SRA       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x29: SRA(C);		DEBUG("SRA       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2a: SRA(D);		DEBUG("SRA       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2b: SRA(E);		DEBUG("SRA       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2c: SRA(XH);		DEBUG("SRA       %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x2d: SRA(XL);		DEBUG("SRA       %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x2e: MEM(SRA(tmp));	DEBUG("SRA       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x2f: SRA(A);		DEBUG("SRA       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x30: SLI(B);		DEBUG("*SLI      B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x31: SLI(C);		DEBUG("*SLI      C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x32: SLI(D);		DEBUG("*SLI      D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x33: SLI(E);		DEBUG("*SLI      E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x34: SLI(XH);		DEBUG("*SLI      %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x35: SLI(XL);		DEBUG("*SLI      %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x36: MEM(SLI(tmp));	DEBUG("*SLI      (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x37: SLI(A);		DEBUG("*SLI      A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x38: SRL(B);		DEBUG("SRL       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x39: SRL(C);		DEBUG("SRL       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3a: SRL(D);		DEBUG("SRL       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3b: SRL(E);		DEBUG("SRL       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3c: SRL(XH);		DEBUG("SRL       %s          %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x3d: SRL(XL);		DEBUG("SRL       %s          %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x3e: MEM(SRL(tmp));	DEBUG("SRL       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x3f: SRL(A);		DEBUG("SRL       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x40: BIT(0,B);		DEBUG("BIT       0,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x41: BIT(0,C);		DEBUG("BIT       0,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x42: BIT(0,D);		DEBUG("BIT       0,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x43: BIT(0,E);		DEBUG("BIT       0,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x44: BIT(0,XH);		DEBUG("BIT       0,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x45: BIT(0,XL);		DEBUG("BIT       0,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x46: BIT(0,MR(HL+DD));	DEBUG("BIT       0,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x47: BIT(0,A);		DEBUG("BIT       0,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x48: BIT(1,B);		DEBUG("BIT       1,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x49: BIT(1,C);		DEBUG("BIT       1,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4a: BIT(1,D);		DEBUG("BIT       1,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4b: BIT(1,E);		DEBUG("BIT       1,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4c: BIT(1,XH);		DEBUG("BIT       1,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x4d: BIT(1,XL);		DEBUG("BIT       1,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x4e: BIT(1,MR(HL+DD));	DEBUG("BIT       1,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x4f: BIT(1,A);		DEBUG("BIT       1,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x50: BIT(2,B);		DEBUG("BIT       2,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x51: BIT(2,C);		DEBUG("BIT       2,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x52: BIT(2,D);		DEBUG("BIT       2,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x53: BIT(2,E);		DEBUG("BIT       2,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x54: BIT(2,XH);		DEBUG("BIT       2,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x55: BIT(2,XL);		DEBUG("BIT       2,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x56: BIT(2,MR(HL+DD));	DEBUG("BIT       2,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x57: BIT(2,A);		DEBUG("BIT       2,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x58: BIT(3,B);		DEBUG("BIT       3,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x59: BIT(3,C);		DEBUG("BIT       3,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5a: BIT(3,D);		DEBUG("BIT       3,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5b: BIT(3,E);		DEBUG("BIT       3,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5c: BIT(3,XH);		DEBUG("BIT       3,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x5d: BIT(3,XL);		DEBUG("BIT       3,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x5e: BIT(3,MR(HL+DD));	DEBUG("BIT       3,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x5f: BIT(3,A);		DEBUG("BIT       3,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x60: BIT(4,B);		DEBUG("BIT       4,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x61: BIT(4,C);		DEBUG("BIT       4,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x62: BIT(4,D);		DEBUG("BIT       4,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x63: BIT(4,E);		DEBUG("BIT       4,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x64: BIT(4,XH);		DEBUG("BIT       4,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x65: BIT(4,XL);		DEBUG("BIT       4,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x66: BIT(4,MR(HL+DD));	DEBUG("BIT       4,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x67: BIT(4,A);		DEBUG("BIT       4,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x68: BIT(5,B);		DEBUG("BIT       5,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x69: BIT(5,C);		DEBUG("BIT       5,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6a: BIT(5,D);		DEBUG("BIT       5,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6b: BIT(5,E);		DEBUG("BIT       5,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6c: BIT(5,XH);		DEBUG("BIT       5,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x6d: BIT(5,XL);		DEBUG("BIT       5,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x6e: BIT(5,MR(HL+DD));	DEBUG("BIT       5,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x6f: BIT(5,A);		DEBUG("BIT       5,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x70: BIT(6,B);		DEBUG("BIT       6,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x71: BIT(6,C);		DEBUG("BIT       6,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x72: BIT(6,D);		DEBUG("BIT       6,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x73: BIT(6,E);		DEBUG("BIT       6,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x74: BIT(6,XH);		DEBUG("BIT       6,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x75: BIT(6,XL);		DEBUG("BIT       6,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x76: BIT(6,MR(HL+DD));	DEBUG("BIT       6,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x77: BIT(6,A);		DEBUG("BIT       6,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x78: BIT(7,B);		DEBUG("BIT       7,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x79: BIT(7,C);		DEBUG("BIT       7,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7a: BIT(7,D);		DEBUG("BIT       7,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7b: BIT(7,E);		DEBUG("BIT       7,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7c: BIT(7,XH);		DEBUG("BIT       7,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x7d: BIT(7,XL);		DEBUG("BIT       7,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x7e: BIT(7,MR(HL+DD));	DEBUG("BIT       7,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x7f: BIT(7,A);		DEBUG("BIT       7,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x80: RES(0,B);		DEBUG("RES       0,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x81: RES(0,C);		DEBUG("RES       0,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x82: RES(0,D);		DEBUG("RES       0,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x83: RES(0,E);		DEBUG("RES       0,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x84: RES(0,XH);		DEBUG("RES       0,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x85: RES(0,XL);		DEBUG("RES       0,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x86: MEM(RES(0,tmp));	DEBUG("RES       0,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x87: RES(0,A);		DEBUG("RES       0,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x88: RES(1,B);		DEBUG("RES       1,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x89: RES(1,C);		DEBUG("RES       1,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8a: RES(1,D);		DEBUG("RES       1,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8b: RES(1,E);		DEBUG("RES       1,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8c: RES(1,XH);		DEBUG("RES       1,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x8d: RES(1,XL);		DEBUG("RES       1,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x8e: MEM(RES(1,tmp));	DEBUG("RES       1,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x8f: RES(1,A);		DEBUG("RES       1,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x90: RES(2,B);		DEBUG("RES       2,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x91: RES(2,C);		DEBUG("RES       2,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x92: RES(2,D);		DEBUG("RES       2,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x93: RES(2,E);		DEBUG("RES       2,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x94: RES(2,XH);		DEBUG("RES       2,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x95: RES(2,XL);		DEBUG("RES       2,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x96: MEM(RES(2,tmp));	DEBUG("RES       2,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x97: RES(2,A);		DEBUG("RES       2,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x98: RES(3,B);		DEBUG("RES       3,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x99: RES(3,C);		DEBUG("RES       3,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9a: RES(3,D);		DEBUG("RES       3,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9b: RES(3,E);		DEBUG("RES       3,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9c: RES(3,XH);		DEBUG("RES       3,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0x9d: RES(3,XL);		DEBUG("RES       3,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x9e: MEM(RES(3,tmp));	DEBUG("RES       3,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x9f: RES(3,A);		DEBUG("RES       3,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa0: RES(4,B);		DEBUG("RES       4,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa1: RES(4,C);		DEBUG("RES       4,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa2: RES(4,D);		DEBUG("RES       4,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa3: RES(4,E);		DEBUG("RES       4,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa4: RES(4,XH);		DEBUG("RES       4,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xa5: RES(4,XL);		DEBUG("RES       4,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xa6: MEM(RES(4,tmp));	DEBUG("RES       4,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xa7: RES(4,A);		DEBUG("RES       4,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa8: RES(5,B);		DEBUG("RES       5,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa9: RES(5,C);		DEBUG("RES       5,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xaa: RES(5,D);		DEBUG("RES       5,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xab: RES(5,E);		DEBUG("RES       5,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xac: RES(5,XH);		DEBUG("RES       5,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xad: RES(5,XL);		DEBUG("RES       5,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xae: MEM(RES(5,tmp));	DEBUG("RES       5,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xaf: RES(5,A);		DEBUG("RES       5,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb0: RES(6,B);		DEBUG("RES       6,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb1: RES(6,C);		DEBUG("RES       6,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb2: RES(6,D);		DEBUG("RES       6,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb3: RES(6,E);		DEBUG("RES       6,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb4: RES(6,XH);		DEBUG("RES       6,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xb5: RES(6,XL);		DEBUG("RES       6,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xb6: MEM(RES(6,tmp));	DEBUG("RES       6,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xb7: RES(6,A);		DEBUG("RES       6,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb8: RES(7,B);		DEBUG("RES       7,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb9: RES(7,C);		DEBUG("RES       7,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xba: RES(7,D);		DEBUG("RES       7,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbb: RES(7,E);		DEBUG("RES       7,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbc: RES(7,XH);		DEBUG("RES       7,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xbd: RES(7,XL);		DEBUG("RES       7,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xbe: MEM(RES(7,tmp));	DEBUG("RES       7,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xbf: RES(7,A);		DEBUG("RES       7,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc0: SET(0,B);		DEBUG("SET       0,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc1: SET(0,C);		DEBUG("SET       0,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc2: SET(0,D);		DEBUG("SET       0,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc3: SET(0,E);		DEBUG("SET       0,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc4: SET(0,XH);		DEBUG("SET       0,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xc5: SET(0,XL);		DEBUG("SET       0,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xc6: MEM(SET(0,tmp));	DEBUG("SET       0,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xc7: SET(0,A);		DEBUG("SET       0,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc8: SET(1,B);		DEBUG("SET       1,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc9: SET(1,C);		DEBUG("SET       1,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xca: SET(1,D);		DEBUG("SET       1,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcb: SET(1,E);		DEBUG("SET       1,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcc: SET(1,XH);		DEBUG("SET       1,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xcd: SET(1,XL);		DEBUG("SET       1,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xce: MEM(SET(1,tmp));	DEBUG("SET       1,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xcf: SET(1,A);		DEBUG("SET       1,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd0: SET(2,B);		DEBUG("SET       2,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd1: SET(2,C);		DEBUG("SET       2,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd2: SET(2,D);		DEBUG("SET       2,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd3: SET(2,E);		DEBUG("SET       2,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd4: SET(2,XH);		DEBUG("SET       2,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xd5: SET(2,XL);		DEBUG("SET       2,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xd6: MEM(SET(2,tmp));	DEBUG("SET       2,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xd7: SET(2,A);		DEBUG("SET       2,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd8: SET(3,B);		DEBUG("SET       3,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd9: SET(3,C);		DEBUG("SET       3,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xda: SET(3,D);		DEBUG("SET       3,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdb: SET(3,E);		DEBUG("SET       3,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdc: SET(3,XH);		DEBUG("SET       3,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xdd: SET(3,XL);		DEBUG("SET       3,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xde: MEM(SET(3,tmp));	DEBUG("SET       3,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xdf: SET(3,A);		DEBUG("SET       3,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe0: SET(4,B);		DEBUG("SET       4,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe1: SET(4,C);		DEBUG("SET       4,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe2: SET(4,D);		DEBUG("SET       4,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe3: SET(4,E);		DEBUG("SET       4,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe4: SET(4,XH);		DEBUG("SET       4,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xe5: SET(4,XL);		DEBUG("SET       4,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xe6: MEM(SET(4,tmp));	DEBUG("SET       4,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xe7: SET(4,A);		DEBUG("SET       4,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe8: SET(5,B);		DEBUG("SET       5,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe9: SET(5,C);		DEBUG("SET       5,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xea: SET(5,D);		DEBUG("SET       5,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xeb: SET(5,E);		DEBUG("SET       5,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xec: SET(5,XH);		DEBUG("SET       5,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xed: SET(5,XL);		DEBUG("SET       5,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xee: MEM(SET(5,tmp));	DEBUG("SET       5,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xef: SET(5,A);		DEBUG("SET       5,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf0: SET(6,B);		DEBUG("SET       6,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf1: SET(6,C);		DEBUG("SET       6,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf2: SET(6,D);		DEBUG("SET       6,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf3: SET(6,E);		DEBUG("SET       6,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf4: SET(6,XH);		DEBUG("SET       6,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xf5: SET(6,XL);		DEBUG("SET       6,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xf6: MEM(SET(6,tmp));	DEBUG("SET       6,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xf7: SET(6,A);		DEBUG("SET       6,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf8: SET(7,B);		DEBUG("SET       7,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf9: SET(7,C);		DEBUG("SET       7,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfa: SET(7,D);		DEBUG("SET       7,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfb: SET(7,E);		DEBUG("SET       7,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfc: SET(7,XH);		DEBUG("SET       7,%s        %s",DEB_DXH,DEB_DFP,DEB_NRM);break;
		case 0xfd: SET(7,XL);		DEBUG("SET       7,%s        %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0xfe: MEM(SET(7,tmp));	DEBUG("SET       7,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xff: SET(7,A);		DEBUG("SET       7,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	    };break;
	case 0xcc: CALL(GZ,NN);		DEBUG("CALL      Z,0x%s%s      ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xcd: CALL_(NN);		DEBUG("CALL      0x%s%s        ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xce: ADC(A,N);		DEBUG("ADC       A,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0xcf: RST(8);		DEBUG("RST       8         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xd0: RET(GNC);		DEBUG("RET       NC        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xd1: POP(DE);		DEBUG("POP       DE        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xd2: JP(GNC,NN);		DEBUG("JP        NC,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xd3: OUT(N,A);		DEBUG("OUT       0x%s,A       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0xd4: CALL(GNC,NN);	DEBUG("CALL      NC,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xd5: PUSH(DE);		DEBUG("PUSH      DE        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xd6: SUB(A,N);		DEBUG("SUB       A,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0xd7: RST(16);		DEBUG("RST       16        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xd8: RET(GC);		DEBUG("RET       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xd9: EXX;			DEBUG("EXX                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xda: JP(GC,NN);		DEBUG("JP        C,0x%s%s      ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xdb: IN(A,N);		DEBUG("IN        A,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0xdc: CALL(GC,NN);		DEBUG("CALL      C,0x%s%s      ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xdd: if(!z->DD_FD){ SET_IX; goto LB_DD_FD; }; break;
	case 0xde: SBC(A,N);		DEBUG("SBC       A,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0xdf: RST(24);		DEBUG("RST       24        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xe0: RET(GPO);		DEBUG("RET       PO        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xe1: POP(HL);		DEBUG("POP       %s        %s",  DEB_RT,DEB_EP,DEB_HL);break;
	case 0xe2: JP(GPO,NN);		DEBUG("JP        PO,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xe3: EX_SP;		DEBUG("EX (SP),HL          %s%s",DEB_DFP,DEB_DFP,DEB_HL);break;
	case 0xe4: CALL(GPO,NN);	DEBUG("CALL      PO,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xe5: PUSH(HL);		DEBUG("PUSH      %s        %s",  DEB_RT,DEB_EP,DEB_HL);break;
	case 0xe6: AND(N);		DEBUG("AND       0x%s         ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0xe7: RST(32);		DEBUG("RST       32        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xe8: RET(GPE);		DEBUG("RET       PE        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xe9: JP_(HL);		DEBUG("JP        (%s)      %s",  DEB_RT,DEB_EP,DEB_HL);break;
	case 0xea: JP(GPE,NN);		DEBUG("JP        PE,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xeb: z->DD_FD=0;EX(DE,HL);
					DEBUG("EX        DE,HL     %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xec: CALL(GPE,NN);	DEBUG("CALL      PE,0x%s%s     ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xed: switch(z80_fetch_byte(z)){
		case 0x00: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x01: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x02: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x03: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x04: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x05: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x06: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x07: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x08: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x09: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0a: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0b: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0c: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0d: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0e: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0f: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x10: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x11: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x12: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x13: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x14: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x15: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x16: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x17: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x18: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x19: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1a: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1b: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1c: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1d: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1e: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1f: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x20: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x21: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x22: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x23: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x24: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x25: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x26: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x27: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x28: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x29: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2a: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2b: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2c: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2d: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2e: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2f: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x30: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x31: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x32: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x33: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x34: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x35: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x36: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x37: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x38: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x39: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3a: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3b: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3c: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3d: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3e: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3f: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x40: INBC(B,BC);		DEBUG("IN        B,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x41: OUTBC(BC,B);		DEBUG("OUT       BC,B      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x42: SBCW(HL,BC);		DEBUG("SBC       %s,BC     %s",  DEB_RT,DEB_EP,DEB_HL);break;
		case 0x43: MWW(NN,BC);		DEBUG("LD        (0x%s%s),BC   ",DEB_NNH,DEB_NNL,DEB_NRM);break;
		case 0x44: NEG;			DEBUG("NEG                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x45: RETN;		DEBUG("RETN                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x46: IM(0);		DEBUG("IM        0         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x47: LD(I,A);		DEBUG("LD        I,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x48: INBC(C,BC);		DEBUG("IN        C,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x49: OUTBC(BC,C);		DEBUG("OUT       BC,C      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4a: ADCW(HL,BC);		DEBUG("ADC       %s,BC     %s",  DEB_RT,DEB_EP,DEB_HL);break;
		case 0x4b: LD(BC,MRW(NN));	DEBUG("LD        BC,(0x%s%s)   ",DEB_NNH,DEB_NNL,DEB_NRM);break;
		case 0x4c: NEG;			DEBUG("*NEG                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4d: RETI;		DEBUG("RETI                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4e: IM(0);		DEBUG("*IM0                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4f: LD(R,A);		DEBUG("LD        R,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x50: INBC(D,BC);		DEBUG("IN        D,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x51: OUTBC(BC,D);		DEBUG("OUT       BC,D      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x52: SBCW(HL,DE);		DEBUG("SBC       %s,DE     %s",  DEB_RT,DEB_EP,DEB_HL);break;
		case 0x53: MWW(NN,DE);		DEBUG("LD        (0x%s%s),DE   ",DEB_NNH,DEB_NNL,DEB_NRM);break;
		case 0x54: NEG;			DEBUG("*NEG                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x55: RETN;		DEBUG("*RETN               %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x56: IM(1);		DEBUG("IM        1         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x57: LD(A,I);LDRIF;	DEBUG("LD        A,I       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x58: INBC(E,BC);		DEBUG("IN        E,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x59: OUTBC(BC,E);		DEBUG("OUT       BC,E      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5a: ADCW(HL,DE);		DEBUG("ADC       %s,DE     %s",  DEB_RT,DEB_EP,DEB_HL);break;
		case 0x5b: LD(DE,MRW(NN));	DEBUG("LD        DE,(0x%s%s)   ",DEB_NNH,DEB_NNL,DEB_NRM);break;
		case 0x5c: NEG;			DEBUG("*NEG                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5d: RETN;		DEBUG("*RETN               %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5e: IM(2);		DEBUG("IM        2         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5f: LD(A,R);LDRIF;	DEBUG("LD        A,R       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x60: INBC(XH,BC);		DEBUG("IN        %s,BC       %s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x61: OUTBC(BC,XH);	DEBUG("OUT       BC,%s       %s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x62: SBCW(HL,HL);		DEBUG("SBC       %s,%s     ",    DEB_RT,DEB_RT,DEB_HL);break;
		case 0x63: MWW(NN,HL);		DEBUG("LD        (0x%s),%s   ",  DEB_HEX16,DEB_RT,DEB_HL);break;
		case 0x64: NEG;			DEBUG("*NEG                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x65: RETN;		DEBUG("*RETN               %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x66: IM(0);		DEBUG("*IM0                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x67: RRD;			DEBUG("RRD                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x68: INBC(XL,BC);		DEBUG("IN        %s,BC       %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x69: OUTBC(BC,XL);	DEBUG("OUT       BC,%s       %s",DEB_DXL,DEB_DFP,DEB_NRM);break;
		case 0x6a: ADCW(HL,HL);		DEBUG("ADC       %s,%s     ",    DEB_RT,DEB_RT,DEB_HL);break;
		case 0x6b: LD(HL,MRW(NN));	DEBUG("LD        %s,(0x%s%s)   ",DEB_RT,DEB_HEX16,DEB_HL);break;
		case 0x6c: NEG;			DEBUG("*NEG                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6d: RETN;		DEBUG("*RETN               %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6e: IM(0);		DEBUG("*IM0                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6f: RLD;			DEBUG("RLD                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x70: INBCF(F,BC);		DEBUG("IN        F,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x71: OUTBC(BC,0);		DEBUG("*OUT      BC,0x00   %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x72: SBCW(HL,SP);		DEBUG("SBC       %s,SP     %s",  DEB_RT,DEB_EP,DEB_HL);break;
		case 0x73: MWW(NN,SP);		DEBUG("LD       (0x%s%s),SP    ",DEB_NNH,DEB_NNL,DEB_NRM);break;
		case 0x74: NEG;			DEBUG("*NEG                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x75: RETN;		DEBUG("*RETN               %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x76: IM(1);		DEBUG("*IM1                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x77: NOP;			DEBUG("*NOP                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x78: INBC(A,BC);		DEBUG("IN        A,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x79: OUTBC(BC,A);		DEBUG("OUT       BC,A      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7a: ADCW(HL,SP);		DEBUG("ADC       %s,SP     %s",  DEB_RT,DEB_EP,DEB_HL);break;
		case 0x7b: LD(SP,MRW(NN));	DEBUG("LD        SP,(0x%s%s)   ",DEB_NNH,DEB_NNL,DEB_NRM);break;
		case 0x7c: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7d: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7e: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7f: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x80: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x81: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x82: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x83: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x84: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x85: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x86: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x87: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x88: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x89: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8a: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8b: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8c: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8d: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8e: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8f: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x90: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x91: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x92: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x93: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x94: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x95: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x96: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x97: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x98: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x99: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9a: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9b: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9c: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9d: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9e: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9f: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa0: LDI;    		DEBUG("LDI                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa1: CPI;	   		DEBUG("CPI                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa2: INI;    		DEBUG("INI                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa3: OUTI;   		DEBUG("OUTI                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa4: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa5: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa6: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa7: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa8: LDD;    		DEBUG("LDD                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa9: CPD;    		DEBUG("CPD                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xaa: IND;    		DEBUG("IND                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xab: OUTD;   		DEBUG("OUTD                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xac: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xad: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xae: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xaf: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb0: LDIR;		DEBUG("LDIR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb1: CDIR;		DEBUG("CDIR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb2: INIR;		DEBUG("INIR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb3: OTIR;		DEBUG("OTIR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb4: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb5: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb6: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb7: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb8: LDDR;		DEBUG("LDDR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb9: CPDR;		DEBUG("CPDR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xba: INDR;		DEBUG("INDR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbb: OTDR;		DEBUG("OTDR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbc: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbd: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbe: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbf: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc0: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc1: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc2: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc3: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc4: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc5: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc6: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc7: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc8: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc9: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xca: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcb: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcc: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcd: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xce: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcf: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd0: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd1: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd2: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd3: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd4: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd5: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd6: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd7: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd8: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd9: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xda: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdb: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdc: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdd: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xde: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdf: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe0: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe1: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe2: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe3: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe4: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe5: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe6: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe7: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe8: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe9: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xea: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xeb: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xec: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xed: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xee: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xef: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf0: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf1: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf2: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf3: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf4: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf5: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf6: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf7: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf8: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf9: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfa: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfb: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfc: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfd: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfe: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xff: EDILLEGAL;		DEBUG("EDILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	    }; break;
	case 0xee: XOR(N);		DEBUG("XOR       0x%s         ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0xef: RST(40);		DEBUG("RST       40        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xf0: RET(GP);		DEBUG("RET       P         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xf1: POP(AF);		DEBUG("POP       AF        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xf2: JP(GP,NN);		DEBUG("JP        P,0x%s%s      ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xf3: DI;			DEBUG("DI                  %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xf4: CALL(GP,NN);		DEBUG("CALL      P,0x%s%s      ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xf5: PUSH(AF);		DEBUG("PUSH      AF        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xf6: OR(N);		DEBUG("OR        0x%s         ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0xf7: RST(48);		DEBUG("RST       48        %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xf8: RET(GM);		DEBUG("RET       M         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xf9: LD(SP,HL);		DEBUG("LD        SP,%s     %s",  DEB_RT,DEB_EP,DEB_HL);break;
	case 0xfa: JP(GM,NN);		DEBUG("JP        M,0x%s%s      ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xfb: EI;			DEBUG("EI                  %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xfc: CALL(GM,NN);		DEBUG("CALL      M,0x%s%s      ",DEB_NNH,DEB_NNL,DEB_NRM);break;
	case 0xfd: if(!z->DD_FD){ SET_IY; goto LB_DD_FD; }; ILLEGAL;		
					DEBUG("ILLEGAL            %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xfe: CP(N);		DEBUG("CP        0x%s         ",DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0xff: RST(56);		DEBUG("RST       56       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
    }

#ifdef DEBUGGER
	if((z->debugger) && (z->deb_on)){	/* czy w trybie debuggera ? */
	    z->debugger(z,DEB_CTL_DEASM);	/* wywo³aj debuggera i deasembluj wykonan± instrukcjê */
	    z->INT_FETCH=0; 			/* instrukcje pobieramy z pamiêci */
	    z->DD_FD = 0;    
	    if(z->Alt){
		SET_HL_;	
	    }else{
		SET_HL;
	    }
	    if(z->debug_int)  return; /* pomiñ obs³ugê IRQ (ale po instrukcji HALT wejd¼ w ni± ) */
	}
#endif
	z->INT_FETCH=0; /* instrukcje pobieramy z pamiêci */
	z->DD_FD = 0;    

    } else { /* if( !z->Halt_state ) */
	NOP;	/* wykonuje operacjê NOP */
    }

    if(z->Alt){
	SET_HL_;	
    }else{
	SET_HL;
    }

/* Obsluga przyjecia przerwania po zakonczeniu dekodowania/wykonania instrukcji */    
    if(z->INT){
	z->Halt_state=0;
	if(!IFF1) return;    
	z->INT = 0;
	z->INT_f=0;
	IFF1 = IFF2 = 0;
	switch(z->IMr){
	    case 0: z->INT_FETCH = 1; goto LB_DD_FD;
	    case 1: z80_call(z,0x38,1); break;
	    case 2: z80_call(z,z80_get_word(z,(word)(*z->INT_DATA + (word)(I << 8))),1); break;
	}
    }

    if(z->NMI){
	z->Halt_state=0;
	z->NMI = 0;
	IFF2 = IFF1;
	IFF1 = 0;
	z80_call(z,0x66,1);
    }
}


