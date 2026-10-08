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
	z->trap = z80_debug_test_break(z);
	z->debugger(z,DEB_CTL_DEBUG);	/* wywo³aj debuggera        */
	if(z->deb_halt) return;
    }
#endif
    if(!z->run) return;
//    TICKS_RST;
    R = (R + 1) & 0x7f; 	/* licznik od¶wierzania - zawsze pracuje, nawet przy HALT */
    if(!z->Halt_state) 		/* je¶li nie HALT dekoduj instrukcjê, inaczej wykonaj NOP */
    { 
	DD = 0;			/* wyzeruj ofset dla operacji indeksowanych IX/IY	  */
	DEBUGGER_CLEAN;		/* wyczyszczenie zmiennych debuggera, przed deasemblacj±  */
/* wewnetrzne wykonanie dekodowania rozkazu, wywo³ane z 3 miejsc: po DD, po FD, obs³uga INT w IM 0 */
LB_DD_FD:
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
	case 0x24: INC(H);		DEBUG("INC       H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x25: DEC(H);		DEBUG("DEC       H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x26: LD(H,N);		DEBUG("LD        H,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
	case 0x27: DAA;			DEBUG("DAA                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x28: JR(GZ,N);		DEBUG("JR        Z,0x%s       ", DEB_DIS,DEB_DFP,DEB_NRM);break;
	case 0x29: ADDW(HL,HL);		DEBUG("ADD       %s,%s       ",  DEB_RT,DEB_RT,DEB_HL);break;
	case 0x2a: LD(HL,MRW(NN));	DEBUG("LD        %s,(0x%s)   ",  DEB_RT,DEB_HEX16,DEB_HL);break;
	case 0x2b: DECW(HL);		DEBUG("DEC       %s        %s",  DEB_RT,DEB_EP,DEB_HL);break;
	case 0x2c: INC(L);		DEBUG("INC       L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x2d: DEC(L);		DEBUG("DEC       L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x2e: LD(L,N);		DEBUG("LD        L,0x%s       ", DEB_NS,DEB_DFP,DEB_NRM);break;
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
	case 0x44: LD(B,H);		DEBUG("LD        B,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x45: LD(B,L);		DEBUG("LD        B,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x46: GET_DD;LD(B,MR(HL+DD));
					DEBUG("LD        B,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x47: LD(B,A);		DEBUG("LD        B,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x48: LD(C,B);		DEBUG("LD        C,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x49: LD(C,C);		DEBUG("LD        C,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x4a: LD(C,D);		DEBUG("LD        C,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x4b: LD(C,E);		DEBUG("LD        C,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x4c: LD(C,H);		DEBUG("LD        C,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x4d: LD(C,L);		DEBUG("LD        C,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x4e: GET_DD;LD(C,MR(HL+DD));
					DEBUG("LD        C,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x4f: LD(C,A);		DEBUG("LD        C,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x50: LD(D,B);		DEBUG("LD        D,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x51: LD(D,C);		DEBUG("LD        D,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x52: LD(D,D);		DEBUG("LD        D,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x53: LD(D,E);		DEBUG("LD        D,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x54: LD(D,H);		DEBUG("LD        D,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x55: LD(D,L);		DEBUG("LD        D,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x56: GET_DD;LD(D,MR(HL+DD));
					DEBUG("LD        D,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x57: LD(D,A);		DEBUG("LD        D,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x58: LD(E,B);		DEBUG("LD        E,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x59: LD(E,C);		DEBUG("LD        E,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x5a: LD(E,D);		DEBUG("LD        E,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x5b: LD(E,E);		DEBUG("LD        E,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x5c: LD(E,H);		DEBUG("LD        E,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x5d: LD(E,L);		DEBUG("LD        E,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x5e: GET_DD;LD(E,MR(HL+DD));
					DEBUG("LD        E,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x5f: LD(E,A);		DEBUG("LD        E,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x60: LD(H,B);		DEBUG("LD        H,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x61: LD(H,C);		DEBUG("LD        H,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x62: LD(H,D);		DEBUG("LD        H,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x63: LD(H,E);		DEBUG("LD        H,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x64: LD(H,H);		DEBUG("LD        H,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x65: LD(H,L);		DEBUG("LD        H,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x66: GET_DD;LD(H,MR(HL+DD));
					DEBUG("LD        H,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x67: LD(H,A);		DEBUG("LD        H,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x68: LD(L,B);		DEBUG("LD        L,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x69: LD(L,C);		DEBUG("LD        L,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x6a: LD(L,D);		DEBUG("LD        L,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x6b: LD(L,E);		DEBUG("LD        L,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x6c: LD(L,H);		DEBUG("LD        L,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x6d: LD(L,L);		DEBUG("LD        L,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x6e: GET_DD;LD(L,MR(HL+DD));
					DEBUG("LD        L,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x6f: LD(L,A);		DEBUG("LD        L,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
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
	case 0x7c: LD(A,H);		DEBUG("LD        A,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x7d: LD(A,L);		DEBUG("LD        A,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x7e: GET_DD;LD(A,MR(HL+DD));
					DEBUG("LD        A,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x7f: LD(A,A);		DEBUG("LD        A,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x80: ADD(A,B);		DEBUG("ADD       A,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x81: ADD(A,C);		DEBUG("ADD       A,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x82: ADD(A,D);		DEBUG("ADD       A,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x83: ADD(A,E);		DEBUG("ADD       A,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x84: ADD(A,H);		DEBUG("ADD       A,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x85: ADD(A,L);		DEBUG("ADD       A,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x86: GET_DD;ADC(A,MR(HL+DD));
					DEBUG("ADD       A,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x87: ADD(A,A);		DEBUG("ADD       A,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x88: ADC(A,B);		DEBUG("ADC       A,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x89: ADC(A,C);		DEBUG("ADC       A,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x8a: ADC(A,D);		DEBUG("ADC       A,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x8b: ADC(A,E);		DEBUG("ADC       A,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x8c: ADC(A,H);		DEBUG("ADC       A,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x8d: ADC(A,L);		DEBUG("ADC       A,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x8e: GET_DD;ADC(A,MR(HL+DD));	
					DEBUG("ADC       A,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x8f: ADC(A,A);		DEBUG("ADC       A,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x90: SUB(A,B);		DEBUG("SUB       A,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x91: SUB(A,C);		DEBUG("SUB       A,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x92: SUB(A,D);		DEBUG("SUB       A,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x93: SUB(A,E);		DEBUG("SUB       A,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x94: SUB(A,H);		DEBUG("SUB       A,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x95: SUB(A,L);		DEBUG("SUB       A,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x96: GET_DD;SUB(A,MR(HL+DD));
					DEBUG("SUB       A,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x97: SUB(A,A);		DEBUG("SUB       A,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x98: SBC(A,B);		DEBUG("SBC       A,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x99: SBC(A,C);		DEBUG("SBC       A,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x9a: SBC(A,D);		DEBUG("SBC       A,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x9b: SBC(A,E);		DEBUG("SBC       A,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x9c: SBC(A,H);		DEBUG("SBC       A,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x9d: SBC(A,L);		DEBUG("SBC       A,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0x9e: GET_DD;SBC(A,MR(HL+DD));
					DEBUG("SBC       A,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0x9f: SBC(A,A);		DEBUG("SBC       A,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa0: AND(B);		DEBUG("AND       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa1: AND(C);		DEBUG("AND       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa2: AND(D);		DEBUG("AND       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa3: AND(E);		DEBUG("AND       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa4: AND(H);		DEBUG("AND       H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa5: AND(L);		DEBUG("AND       L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa6: GET_DD;GET_DD;AND(MR(HL+DD));	
					DEBUG("AND       (%s%s)   ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0xa7: AND(A);		DEBUG("AND       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa8: XOR(B);		DEBUG("XOR       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xa9: XOR(C);		DEBUG("XOR       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xaa: XOR(D);		DEBUG("XOR       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xab: XOR(E);		DEBUG("XOR       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xac: XOR(H);		DEBUG("XOR       H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xad: XOR(L);		DEBUG("XOR       L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xae: GET_DD;XOR(MR(HL+DD));	
					DEBUG("XOR       (%s%s)   ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0xaf: XOR(A);		DEBUG("XOR       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb0: OR(B);		DEBUG("OR        B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb1: OR(C);		DEBUG("OR        C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb2: OR(D);		DEBUG("OR        D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb3: OR(E);		DEBUG("OR        E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb4: OR(H);		DEBUG("OR        H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb5: OR(L);		DEBUG("OR        L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb6: GET_DD;OR(MR(HL+DD));	
					DEBUG("OR        (%s%s)   ",     DEB_RT,DEB_DD,DEB_M);break;
	case 0xb7: OR(A);		DEBUG("OR        A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb8: CP(B);		DEBUG("CP        B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xb9: CP(C);		DEBUG("CP        C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xba: CP(D);		DEBUG("CP        D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xbb: CP(E);		DEBUG("CP        E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xbc: CP(H);		DEBUG("CP        H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
	case 0xbd: CP(L);		DEBUG("CP        L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
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
		case 0x04: RLC(H);		DEBUG("RLC       H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x05: RLC(L);		DEBUG("RLC       L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x06: MEM(RLC(tmp));	DEBUG("RLC       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x07: RLC(A);		DEBUG("RLC       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x08: RRC(B);		DEBUG("RRC       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x09: RRC(C);		DEBUG("RRC       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0a: RRC(D);		DEBUG("RRC       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0b: RRC(E);		DEBUG("RRC       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0c: RRC(H);		DEBUG("RRC       H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0d: RRC(L);		DEBUG("RRC       L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0e: MEM(RRC(tmp));	DEBUG("RRC       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x0f: RRC(A);		DEBUG("RRC       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x10: RL(B); 		DEBUG("RL        B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x11: RL(C); 		DEBUG("RL        C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x12: RL(D); 		DEBUG("RL        D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x13: RL(E); 		DEBUG("RL        E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x14: RL(H); 		DEBUG("RL        H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x15: RL(L); 		DEBUG("RL        L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x16: MEM(RL(tmp));	DEBUG("RL        (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x17: RL(A); 		DEBUG("RL        A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x18: RR(B); 		DEBUG("RR        B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x19: RR(C); 		DEBUG("RR        C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1a: RR(D); 		DEBUG("RR        D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1b: RR(E); 		DEBUG("RR        E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1c: RR(H); 		DEBUG("RR        H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1d: RR(L); 		DEBUG("RR        L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1e: MEM(RR(tmp));	DEBUG("RR        (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x1f: RR(A); 		DEBUG("RR        A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x20: SLA(B);		DEBUG("SLA       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x21: SLA(C);		DEBUG("SLA       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x22: SLA(D);		DEBUG("SLA       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x23: SLA(E);		DEBUG("SLA       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x24: SLA(H);		DEBUG("SLA       H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x25: SLA(L);		DEBUG("SLA       L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x26: MEM(SLA(tmp));	DEBUG("SLA       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x27: SLA(A);		DEBUG("SLA       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x28: SRA(B);		DEBUG("SRA       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x29: SRA(C);		DEBUG("SRA       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2a: SRA(D);		DEBUG("SRA       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2b: SRA(E);		DEBUG("SRA       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2c: SRA(H);		DEBUG("SRA       H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2d: SRA(L);		DEBUG("SRA       L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2e: MEM(SRA(tmp));	DEBUG("SRA       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x2f: SRA(A);		DEBUG("SRA       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x30: SLI(B);		DEBUG("SLI       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x31: SLI(C);		DEBUG("SLI       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x32: SLI(D);		DEBUG("SLI       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x33: SLI(E);		DEBUG("SLI       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x34: SLI(H);		DEBUG("SLI       H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x35: SLI(L);		DEBUG("SLI       L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x36: MEM(SLI(tmp));	DEBUG("SLI       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x37: SLI(A);		DEBUG("SLI       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x38: SRL(B);		DEBUG("SRL       B         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x39: SRL(C);		DEBUG("SRL       C         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3a: SRL(D);		DEBUG("SRL       D         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3b: SRL(E);		DEBUG("SRL       E         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3c: SRL(H);		DEBUG("SRL       H         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3d: SRL(L);		DEBUG("SRL       L         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3e: MEM(SRL(tmp));	DEBUG("SRL       (%s%s)   ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x3f: SRL(A);		DEBUG("SRL       A         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x40: BIT(0,B);		DEBUG("BIT       0,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x41: BIT(0,C);		DEBUG("BIT       0,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x42: BIT(0,D);		DEBUG("BIT       0,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x43: BIT(0,E);		DEBUG("BIT       0,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x44: BIT(0,H);		DEBUG("BIT       0,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x45: BIT(0,L);		DEBUG("BIT       0,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x46: BIT(0,MR(HL+DD));	DEBUG("BIT       0,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x47: BIT(0,A);		DEBUG("BIT       0,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x48: BIT(1,B);		DEBUG("BIT       1,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x49: BIT(1,C);		DEBUG("BIT       1,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4a: BIT(1,D);		DEBUG("BIT       1,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4b: BIT(1,E);		DEBUG("BIT       1,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4c: BIT(1,H);		DEBUG("BIT       1,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4d: BIT(1,L);		DEBUG("BIT       1,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4e: BIT(1,MR(HL+DD));	DEBUG("BIT       1,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x4f: BIT(1,A);		DEBUG("BIT       1,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x50: BIT(2,B);		DEBUG("BIT       2,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x51: BIT(2,C);		DEBUG("BIT       2,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x52: BIT(2,D);		DEBUG("BIT       2,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x53: BIT(2,E);		DEBUG("BIT       2,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x54: BIT(2,H);		DEBUG("BIT       2,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x55: BIT(2,L);		DEBUG("BIT       2,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x56: BIT(2,MR(HL+DD));	DEBUG("BIT       2,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x57: BIT(2,A);		DEBUG("BIT       2,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x58: BIT(3,B);		DEBUG("BIT       3,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x59: BIT(3,C);		DEBUG("BIT       3,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5a: BIT(3,D);		DEBUG("BIT       3,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5b: BIT(3,E);		DEBUG("BIT       3,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5c: BIT(3,H);		DEBUG("BIT       3,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5d: BIT(3,L);		DEBUG("BIT       3,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5e: BIT(3,MR(HL+DD));	DEBUG("BIT       3,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x5f: BIT(3,A);		DEBUG("BIT       3,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x60: BIT(4,B);		DEBUG("BIT       4,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x61: BIT(4,C);		DEBUG("BIT       4,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x62: BIT(4,D);		DEBUG("BIT       4,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x63: BIT(4,E);		DEBUG("BIT       4,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x64: BIT(4,H);		DEBUG("BIT       4,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x65: BIT(4,L);		DEBUG("BIT       4,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x66: BIT(4,MR(HL+DD));	DEBUG("BIT       4,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x67: BIT(4,A);		DEBUG("BIT       4,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x68: BIT(5,B);		DEBUG("BIT       5,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x69: BIT(5,C);		DEBUG("BIT       5,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6a: BIT(5,D);		DEBUG("BIT       5,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6b: BIT(5,E);		DEBUG("BIT       5,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6c: BIT(5,H);		DEBUG("BIT       5,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6d: BIT(5,L);		DEBUG("BIT       5,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6e: BIT(5,MR(HL+DD));	DEBUG("BIT       5,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x6f: BIT(5,A);		DEBUG("BIT       5,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x70: BIT(6,B);		DEBUG("BIT       6,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x71: BIT(6,C);		DEBUG("BIT       6,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x72: BIT(6,D);		DEBUG("BIT       6,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x73: BIT(6,E);		DEBUG("BIT       6,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x74: BIT(6,H);		DEBUG("BIT       6,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x75: BIT(6,L);		DEBUG("BIT       6,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x76: BIT(6,MR(HL+DD));	DEBUG("BIT       6,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x77: BIT(6,A);		DEBUG("BIT       6,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x78: BIT(7,B);		DEBUG("BIT       7,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x79: BIT(7,C);		DEBUG("BIT       7,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7a: BIT(7,D);		DEBUG("BIT       7,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7b: BIT(7,E);		DEBUG("BIT       7,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7c: BIT(7,H);		DEBUG("BIT       7,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7d: BIT(7,L);		DEBUG("BIT       7,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7e: BIT(7,MR(HL+DD));	DEBUG("BIT       7,(%s%s) ",     DEB_RT,DEB_DD,DEB_M);break;
		case 0x7f: BIT(7,A);		DEBUG("BIT       7,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x80: RES(0,B);		DEBUG("RES       0,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x81: RES(0,C);		DEBUG("RES       0,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x82: RES(0,D);		DEBUG("RES       0,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x83: RES(0,E);		DEBUG("RES       0,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x84: RES(0,H);		DEBUG("RES       0,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x85: RES(0,L);		DEBUG("RES       0,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x86: MEM(RES(0,tmp));	DEBUG("RES       0,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x87: RES(0,A);		DEBUG("RES       0,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x88: RES(1,B);		DEBUG("RES       1,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x89: RES(1,C);		DEBUG("RES       1,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8a: RES(1,D);		DEBUG("RES       1,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8b: RES(1,E);		DEBUG("RES       1,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8c: RES(1,H);		DEBUG("RES       1,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8d: RES(1,L);		DEBUG("RES       1,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8e: MEM(RES(1,tmp));	DEBUG("RES       1,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x8f: RES(1,A);		DEBUG("RES       1,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x90: RES(2,B);		DEBUG("RES       2,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x91: RES(2,C);		DEBUG("RES       2,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x92: RES(2,D);		DEBUG("RES       2,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x93: RES(2,E);		DEBUG("RES       2,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x94: RES(2,H);		DEBUG("RES       2,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x95: RES(2,L);		DEBUG("RES       2,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x96: MEM(RES(2,tmp));	DEBUG("RES       2,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x97: RES(2,A);		DEBUG("RES       2,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x98: RES(3,B);		DEBUG("RES       3,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x99: RES(3,C);		DEBUG("RES       3,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9a: RES(3,D);		DEBUG("RES       3,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9b: RES(3,E);		DEBUG("RES       3,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9c: RES(3,H);		DEBUG("RES       3,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9d: RES(3,L);		DEBUG("RES       3,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9e: MEM(RES(3,tmp));	DEBUG("RES       3,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0x9f: RES(3,A);		DEBUG("RES       3,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa0: RES(4,B);		DEBUG("RES       4,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa1: RES(4,C);		DEBUG("RES       4,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa2: RES(4,D);		DEBUG("RES       4,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa3: RES(4,E);		DEBUG("RES       4,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa4: RES(4,H);		DEBUG("RES       4,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa5: RES(4,L);		DEBUG("RES       4,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa6: MEM(RES(4,tmp));	DEBUG("RES       4,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xa7: RES(4,A);		DEBUG("RES       4,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa8: RES(5,B);		DEBUG("RES       5,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa9: RES(5,C);		DEBUG("RES       5,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xaa: RES(5,D);		DEBUG("RES       5,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xab: RES(5,E);		DEBUG("RES       5,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xac: RES(5,H);		DEBUG("RES       5,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xad: RES(5,L);		DEBUG("RES       5,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xae: MEM(RES(5,tmp));	DEBUG("RES       5,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xaf: RES(5,A);		DEBUG("RES       5,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb0: RES(6,B);		DEBUG("RES       6,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb1: RES(6,C);		DEBUG("RES       6,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb2: RES(6,D);		DEBUG("RES       6,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb3: RES(6,E);		DEBUG("RES       6,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb4: RES(6,H);		DEBUG("RES       6,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb5: RES(6,L);		DEBUG("RES       6,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb6: MEM(RES(6,tmp));	DEBUG("RES       6,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xb7: RES(6,A);		DEBUG("RES       6,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb8: RES(7,B);		DEBUG("RES       7,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb9: RES(7,C);		DEBUG("RES       7,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xba: RES(7,D);		DEBUG("RES       7,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbb: RES(7,E);		DEBUG("RES       7,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbc: RES(7,H);		DEBUG("RES       7,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbd: RES(7,L);		DEBUG("RES       7,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbe: MEM(RES(7,tmp));	DEBUG("RES       7,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xbf: RES(7,A);		DEBUG("RES       7,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc0: SET(0,B);		DEBUG("SET       0,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc1: SET(0,C);		DEBUG("SET       0,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc2: SET(0,D);		DEBUG("SET       0,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc3: SET(0,E);		DEBUG("SET       0,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc4: SET(0,H);		DEBUG("SET       0,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc5: SET(0,L);		DEBUG("SET       0,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc6: MEM(SET(0,tmp));	DEBUG("SET       0,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xc7: SET(0,A);		DEBUG("SET       0,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc8: SET(1,B);		DEBUG("SET       1,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc9: SET(1,C);		DEBUG("SET       1,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xca: SET(1,D);		DEBUG("SET       1,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcb: SET(1,E);		DEBUG("SET       1,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcc: SET(1,H);		DEBUG("SET       1,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcd: SET(1,L);		DEBUG("SET       1,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xce: MEM(SET(1,tmp));	DEBUG("SET       1,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xcf: SET(1,A);		DEBUG("SET       1,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd0: SET(2,B);		DEBUG("SET       2,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd1: SET(2,C);		DEBUG("SET       2,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd2: SET(2,D);		DEBUG("SET       2,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd3: SET(2,E);		DEBUG("SET       2,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd4: SET(2,H);		DEBUG("SET       2,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd5: SET(2,L);		DEBUG("SET       2,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd6: MEM(SET(2,tmp));	DEBUG("SET       2,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xd7: SET(2,A);		DEBUG("SET       2,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd8: SET(3,B);		DEBUG("SET       3,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd9: SET(3,C);		DEBUG("SET       3,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xda: SET(3,D);		DEBUG("SET       3,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdb: SET(3,E);		DEBUG("SET       3,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdc: SET(3,H);		DEBUG("SET       3,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdd: SET(3,L);		DEBUG("SET       3,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xde: MEM(SET(3,tmp));	DEBUG("SET       3,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xdf: SET(3,A);		DEBUG("SET       3,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe0: SET(4,B);		DEBUG("SET       4,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe1: SET(4,C);		DEBUG("SET       4,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe2: SET(4,D);		DEBUG("SET       4,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe3: SET(4,E);		DEBUG("SET       4,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe4: SET(4,H);		DEBUG("SET       4,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe5: SET(4,L);		DEBUG("SET       4,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe6: MEM(SET(4,tmp));	DEBUG("SET       4,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xe7: SET(4,A);		DEBUG("SET       4,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe8: SET(5,B);		DEBUG("SET       5,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe9: SET(5,C);		DEBUG("SET       5,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xea: SET(5,D);		DEBUG("SET       5,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xeb: SET(5,E);		DEBUG("SET       5,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xec: SET(5,H);		DEBUG("SET       5,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xed: SET(5,L);		DEBUG("SET       5,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xee: MEM(SET(5,tmp));	DEBUG("SET       5,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xef: SET(5,A);		DEBUG("SET       5,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf0: SET(6,B);		DEBUG("SET       6,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf1: SET(6,C);		DEBUG("SET       6,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf2: SET(6,D);		DEBUG("SET       6,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf3: SET(6,E);		DEBUG("SET       6,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf4: SET(6,H);		DEBUG("SET       6,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf5: SET(6,L);		DEBUG("SET       6,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf6: MEM(SET(6,tmp));	DEBUG("SET       6,(%s%s) ",     DEB_RT,DEB_DD,DEB_NRM);break;
		case 0xf7: SET(6,A);		DEBUG("SET       6,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf8: SET(7,B);		DEBUG("SET       7,B       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf9: SET(7,C);		DEBUG("SET       7,C       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfa: SET(7,D);		DEBUG("SET       7,D       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfb: SET(7,E);		DEBUG("SET       7,E       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfc: SET(7,H);		DEBUG("SET       7,H       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfd: SET(7,L);		DEBUG("SET       7,L       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
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
	case 0xdd: if(!z->DD_FD){ SET_IX; goto LB_DD_FD; }; break;ILLEGAL;
					DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
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
		case 0x00: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x01: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x02: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x03: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x04: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x05: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x06: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x07: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x08: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x09: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0a: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0b: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0c: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0d: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0e: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x0f: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x10: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x11: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x12: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x13: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x14: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x15: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x16: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x17: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x18: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x19: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1a: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1b: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1c: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1d: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1e: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x1f: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x20: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x21: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x22: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x23: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x24: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x25: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x26: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x27: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x28: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x29: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2a: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2b: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2c: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2d: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2e: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x2f: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x30: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x31: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x32: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x33: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x34: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x35: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x36: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x37: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x38: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x39: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3a: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3b: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3c: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3d: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3e: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x3f: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
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
		case 0x4c: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4d: RETI;		DEBUG("RETI                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4e: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x4f: LD(R,A);		DEBUG("LD        R,A       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x50: INBC(D,BC);		DEBUG("IN        D,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x51: OUTBC(BC,D);		DEBUG("OUT       BC,D      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x52: SBCW(HL,DE);		DEBUG("SBC       %s,DE     %s",  DEB_RT,DEB_EP,DEB_HL);break;
		case 0x53: MWW(NN,DE);		DEBUG("LD        (0x%s%s),DE   ",DEB_NNH,DEB_NNL,DEB_NRM);break;
		case 0x54: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x55: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x56: IM(1);		DEBUG("IM        1         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x57: LD(A,I);LDRIF;	DEBUG("LD        A,I       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x58: INBC(E,BC);		DEBUG("IN        E,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x59: OUTBC(BC,E);		DEBUG("OUT       BC,E      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5a: ADCW(HL,DE);		DEBUG("ADC       %s,DE     %s",  DEB_RT,DEB_EP,DEB_HL);break;
		case 0x5b: LD(DE,MRW(NN));	DEBUG("LD        DE,(0x%s%s)   ",DEB_NNH,DEB_NNL,DEB_NRM);break;
		case 0x5c: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5d: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5e: IM(2);		DEBUG("IM        2         %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x5f: LD(A,R);LDRIF;	DEBUG("LD        A,R       %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x60: INBC(H,BC);		DEBUG("IN        H,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x61: OUTBC(BC,H);		DEBUG("OUT       BC,H      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x62: SBCW(HL,HL);		DEBUG("SBC       %s,%s     ",    DEB_RT,DEB_RT,DEB_HL);break;
		case 0x63: MWW(NN,HL);		DEBUG("LD        (0x%s),%s   ",  DEB_HEX16,DEB_RT,DEB_HL);break;
		case 0x64: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x65: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x66: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x67: RRD;			DEBUG("RRD                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x68: INBC(L,BC);		DEBUG("IN        L,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x69: OUTBC(BC,L);		DEBUG("OUT       BC,L      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6a: ADCW(HL,HL);		DEBUG("ADC       %s,%s     ",    DEB_RT,DEB_RT,DEB_HL);break;
		case 0x6b: LD(HL,MRW(NN));	DEBUG("LD        %s,(0x%s%s)   ",DEB_RT,DEB_HEX16,DEB_HL);break;
		case 0x6c: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6d: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6e: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x6f: RLD;			DEBUG("RLD                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x70: INBCF(F,BC);		DEBUG("IN        F,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x71: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x72: SBCW(HL,SP);		DEBUG("SBC       %s,SP     %s",  DEB_RT,DEB_EP,DEB_HL);break;
		case 0x73: MWW(NN,SP);		DEBUG("LD       (0x%s%s),SP    ",DEB_NNH,DEB_NNL,DEB_NRM);break;
		case 0x74: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x75: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x76: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x77: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x78: INBC(A,BC);		DEBUG("IN        A,BC      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x79: OUTBC(BC,A);		DEBUG("OUT       BC,A      %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7a: ADCW(HL,SP);		DEBUG("ADC       %s,SP     %s",  DEB_RT,DEB_EP,DEB_HL);break;
		case 0x7b: LD(SP,MRW(NN));	DEBUG("LD        SP,(0x%s%s)   ",DEB_NNH,DEB_NNL,DEB_NRM);break;
		case 0x7c: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7d: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7e: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x7f: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x80: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x81: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x82: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x83: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x84: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x85: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x86: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x87: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x88: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x89: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8a: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8b: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8c: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8d: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8e: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x8f: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x90: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x91: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x92: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x93: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x94: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x95: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x96: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x97: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x98: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x99: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9a: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9b: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9c: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9d: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9e: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0x9f: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa0: LDI;    		DEBUG("LDI                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa1: CPI;	   		DEBUG("CPI                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa2: INI;    		DEBUG("INI                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa3: OUTI;   		DEBUG("OUTI                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa4: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa5: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa6: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa7: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa8: LDD;    		DEBUG("LDD                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xa9: CPD;    		DEBUG("CPD                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xaa: IND;    		DEBUG("IND                 %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xab: OUTD;   		DEBUG("OUTD                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xac: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xad: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xae: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xaf: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb0: LDIR;		DEBUG("LDIR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb1: CDIR;		DEBUG("CDIR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb2: INIR;		DEBUG("INIR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb3: OTIR;		DEBUG("OTIR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb4: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb5: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb6: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb7: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb8: LDDR;		DEBUG("LDDR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xb9: CPDR;		DEBUG("CPDR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xba: INDR;		DEBUG("INDR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbb: OTDR;		DEBUG("OTDR                %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbc: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbd: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbe: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xbf: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc0: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc1: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc2: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc3: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc4: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc5: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc6: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc7: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc8: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xc9: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xca: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcb: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcc: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcd: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xce: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xcf: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd0: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd1: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd2: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd3: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd4: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd5: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd6: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd7: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd8: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xd9: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xda: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdb: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdc: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdd: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xde: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xdf: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe0: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe1: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe2: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe3: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe4: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe5: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe6: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe7: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe8: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xe9: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xea: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xeb: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xec: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xed: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xee: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xef: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf0: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf1: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf2: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf3: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf4: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf5: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf6: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf7: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf8: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xf9: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfa: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfb: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfc: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfd: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xfe: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
		case 0xff: ILLEGAL;		DEBUG("ILLEGAL             %s%s",DEB_DFP,DEB_DFP,DEB_NRM);break;
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


