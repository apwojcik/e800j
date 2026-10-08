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

void z80_daa(Z80 *z)
{
    byte tmp;
    
    tmp = A > 0x99;
    if( GET_CF || tmp ) A += GET_NF ? -0x60 : 0x60;
    SET_CF( !GET_CF && tmp );

    tmp = (A & 0x0f) > 0x09;
    if( GET_HF || tmp ) A += GET_NF ? -0x06 : 0x06;
    SET_HF( !GET_HF && tmp );
}

void z80_bit_alu(Z80 *z, byte reg, byte type)
{
    SET_M1(0);
    DRY_RUN_RET;    
    switch(type){
	case ALU_AND: A &= reg; BIN_ALU_FL(1); break;
	case ALU_OR : A |= reg; BIN_ALU_FL(0); break;
	case ALU_XOR: A ^= reg; BIN_ALU_FL(0); break;
	case ALU_CPL: A = ~A; SET_HF(1); SET_NF(1); break;
	case ALU_NEG: A = z80_alu_add(0,A,1,0,z); break;
	case ALU_SCF: SET_CF(1); SET_NF(0); SET_HF(0); break;
	case ALU_CCF: SET_CF(!GET_CF); SET_NF(0); break;
    }
}


byte z80_alu_add(word a, word b, byte sub, byte cy, Z80 *z)
{
    byte  si, c4, c7, c8;
    word tmp;

    SET_M1(0);

    b += cy & 1;

    if(sub & 0x01) b = -b;

    tmp = a + b;
    si  = tmp & 0xff;

    c8  = (tmp & 0x100) != 0;
    c7  = (((a & 0x7f) + (b & 0x7f)) & 0x80) != 0;
    c4  = (((a & 0x0f) + (b & 0x0f)) & 0x10) != 0;

    SET_PF( c7 ^ c8); SET_HF(c4); SET_TEST_SF(si); SET_TEST_ZF(si); SET_NF(sub); 

    if(sub & 0x04) return si; /* dla operacji INC/DEC */

    SET_CF(c8);

    return si;
}

word z80_alu_add16(dword a, dword b, byte sub, byte cy, Z80 *z)
{
    word  si;
    byte  c12,c15,c16;
    dword tmp;

    SET_M1(0);

    b += cy & 1;

    if(sub) b = -b;

    tmp = a + b;
    si  = tmp & 0x0ffff;

    c16  = (tmp & 0x10000) != 0;
    c15  = (((a & 0x7fff) + (b & 0x7fff)) & 0x8000) != 0;
    c12  = (((a & 0x0fff) + (b & 0x0fff)) & 0x1000) != 0;

    SET_HF(c12); SET_NF(sub); SET_CF(c16);

    /* jesli ADD HL,xx pomija ustawianie flag SZV */
    if(cy & 0x02 ) return si;

    SET_PF( c15 ^ c16); SET_ZF(!si); SET_SF(si & 0x8000);
    return si;
}

void z80_push(Z80 *z, word reg)
{
    SET_M1(0);
    DRY_RUN_RET;
    SP -= 2;
    MWW(SP,reg);
}

void z80_pop(Z80 *z, word *reg)
{
    SET_M1(0);
    DRY_RUN_RET;
    word tmp = MRW(SP);
    *((byte *)reg + 0) = tmp & 0xff;
    *((byte *)reg + 1) = (tmp >> 8) & 0xff;
    SP += 2;
}

/* nie modyfikuje PC - wa¿ne dla RST */
void z80_call(Z80 *z, word addr, byte condition)
{
    SET_M1(0);
    DRY_RUN_RET;
    if(!condition) return;
    PUSH(PC);
    PC = addr;
}

void z80_ret(Z80 *z, byte condition, byte type)
{
    SET_M1(0);
    DRY_RUN_RET;
    if(type == 2) IFF1 = IFF2;
    if(!condition) return;
    POP(PC);
}

void z80_brs(Z80 *z, byte bit, byte rg, byte *reg, byte type)
{
    byte mask = 0x01 << bit;

    SET_M1(0);
    DRY_RUN_RET;    
    switch(type){
	case BIT_TST: SET_TEST_ZF(rg & mask); 
		      SET_HF(1); SET_NF(0);
		      break;
	case BIT_RES: *reg &= ~mask; break;
	case BIT_SET: *reg |=  mask; break;
    }
}

void z80_cpi(Z80 *z,byte inc)
{
    byte tmp;

    SET_M1(0);
    DRY_RUN_RET;
    tmp=GET_CF;
    CP(MR(HL));
    if(inc) HL--; else HL++;
    BC--;
    SET_PF(BC);
    SET_CF(tmp);
    SET_NF(1);
}

void z80_ldi(Z80 *z,byte inc)
{
    SET_M1(0);
    DRY_RUN_RET;
    SET_PF(1);
    MW(DE,MR(HL));
    if(inc) { HL--; DE--;} else { HL++; DE++; }
    BC--;
    SET_HF(0);
    SET_NF(0);
    SET_PF(0);
}

void z80_ini(Z80 *z,byte inc)
{
    SET_M1(0);
    DRY_RUN_RET;
    MW(HL,IO(BC,0,1));
    if(inc) HL--; else HL++;
    B--;
    SET_NF(1);
    SET_ZF(!B);
}

void z80_outi(Z80 *z,byte inc)
{
    SET_M1(0);
    DRY_RUN_RET;
    IO(BC,MR(HL),0);
    if(inc) HL--; else HL++;
    B--;
    SET_NF(1);
    SET_ZF(!B);
}


void z80_rep(Z80 *z, void (op)(Z80*,byte), byte inc, byte type)
{
    DRY_RUN_RET;
    do op(z,inc); while(type ? B : BC);
}

void z80_jp(Z80 *z,byte condition,word addr)
{
    SET_M1(0);
    DRY_RUN_RET;    
    if(!condition) return;    
    PC = addr;
}

void z80_jr(Z80 *z,byte condition, byte dis)
{
    SET_M1(0);
    DRY_RUN_RET;    
    if(!condition) return;    
    PC += (char)dis;
}

void z80_djnz(Z80 *z, byte dis)
{
    SET_M1(0);
    DRY_RUN_RET;    
    z80_jr(z,--B,dis);
    DEB_DJNZ( B != 0 );	
}


void z80_out(Z80 *z,word addr, byte data)
{
    SET_M1(0);
    DRY_RUN_RET;    
    IO(addr,data,0);
}

/* type = 0 - IN A,adr ; 1- IN r,(BC); 2 - IN F,(BC) */
byte z80_in(Z80 *z,word addr,byte type)
{
    byte tmp;
    
    SET_M1(0);
    if(z->dry_run) return 0xff;    
    tmp = IO(addr,0,1);
    
    if(!type) return tmp;

    SET_TEST_SF(tmp);
    SET_TEST_ZF(tmp);
    SET_HF(0);
    SET_TEST_PF(tmp);
    SET_NF(0);
    
    return (type == 2) ? F : tmp; 
}

void z80_inc(Z80 *z,byte *reg, byte dec)
{
    SET_M1(0);
    DRY_RUN_RET;    
    REG = dec ? z80_alu_add(REG,1,5,0,z) : z80_alu_add(REG,1,4,0,z);
}

void z80_incm(Z80 *z, byte dec)
{
    byte tmp;

    GET_DD;
    SET_M1(0);    
    DRY_RUN_RET;    
    tmp = MR(HL + DD);    
    tmp = dec ? z80_alu_add(tmp,1,5,0,z) : z80_alu_add(tmp,1,4,0,z);
    MW(HL + DD, tmp);
}

void z80_ex(Z80 *z,word *reg1, word *reg2)
{
    word tmp = *reg1;

    SET_M1(0);
    DRY_RUN_RET;        
    *reg1 =* reg2;
    *reg2 = tmp;
}

void z80_exsp(Z80 *z)
{
    word tmp;

    SET_M1(0);
    DRY_RUN_RET;        
    tmp = HL;
    HL = MRW(SP);
    MWW(SP,tmp);
}

void z80_halt(Z80 *z)
{
    SET_M1(0);
    DRY_RUN_RET;    
    SET_HALT(1);
}


/* obrót w lewo - bit 7 po przesuniêciu wchodzi na bit 0 */
byte z80_rl(byte a)
{
    return ((a << 1) & 0xfe) | (( a >> 7) & 0x01);
}

/* przesuniecie w lewo - bit 7 jest tracony, na bit 0 wchodzi bit b */
byte z80_slb(byte a, byte b)
{
    return ((a << 1) & 0xfe) | (b & 0x01);
}

/* obrót w prawo - bit 0 po przesuniêciu wchodzi na bit 7 */
byte z80_rr(byte a)
{
    return ((a >> 1) & 0x7f) | (( a << 7) & 0x80);
}

/* przesuniecie w prawo - bit 0 jest tracony, na bit 7 wchodzi bit b */
byte z80_srb(byte a, byte b)
{
    return ((a >> 1) & 0x7f) | ((b << 7) & 0x80);
}

void z80_shift(Z80 *z, byte *reg, byte type)
{
    byte tmp;

    SET_M1(0);
    DRY_RUN_RET;    
    SET_HF(0); SET_NF(0);
    switch(type){
/* obroty akumulatora */
	case COM_RLCA: SET_CF(A & 0x80); A = z80_rl(A); break;
	case COM_RLA : tmp = GET_CF; SET_CF(A & 0x80); A = z80_slb(A, tmp); break;
	case COM_RRCA: SET_CF(A & 0x01); A = z80_rr(A); break;
	case COM_RRA : tmp = GET_CF; SET_CF(A & 0x01); A = z80_srb(A, tmp); break;
	case COM_RLD: tmp = (REG & 0x0f) | ((MR(HL) << 4) & 0xf0);
		      REG = (REG & 0xf0) | ((MR(HL) >> 4) & 0x0f); MW(HL,tmp);
		      SET_TEST_SF(REG); SET_TEST_ZF(REG); SET_TEST_PF(REG); 
		      break;    
	case COM_RRD: tmp = ((REG << 4) & 0xf0) | ((MR(HL) >> 4) & 0x0f);
		      REG = (REG & 0xf0) | (MR(HL) & 0xf0); MW(HL,tmp);	
		      SET_TEST_SF(REG); SET_TEST_ZF(REG); SET_TEST_PF(REG); 
		      break;
/* obroty rejestrów */
	case COM_RLC : SET_CF(REG & 0x80); REG = z80_rl(REG); 
		       SET_TEST_SF(REG); SET_TEST_ZF(REG); SET_TEST_PF(REG); 
		       break;
	case COM_RRC : SET_CF(REG & 0x01); REG = z80_rr(REG); 
		       SET_TEST_SF(REG); SET_TEST_ZF(REG); SET_TEST_PF(REG); 
		       break;
	case COM_RL  : tmp = GET_CF; SET_CF(REG & 0x80); REG = z80_slb(REG, tmp); 
		       SET_TEST_SF(REG); SET_TEST_ZF(REG); SET_TEST_PF(REG); 
		       break;
	case COM_RR  : tmp = GET_CF; SET_CF(REG & 0x01); REG = z80_srb(REG, tmp); 
		       SET_TEST_SF(REG); SET_TEST_ZF(REG); SET_TEST_PF(REG); 
		       break;
/* przesuniêcia rejestrów */		      
	case COM_SLA : SET_CF(REG & 0x80); REG = z80_slb(REG,0);
		       SET_TEST_SF(REG); SET_TEST_ZF(REG); SET_TEST_PF(REG); 
		       break;
	case COM_SLI:  SET_CF(REG & 0x80); REG = z80_slb(REG,1);
		       SET_TEST_SF(REG); SET_TEST_ZF(REG); SET_TEST_PF(REG); 
		       break;
	case COM_SRA:  SET_CF(REG & 0x01); REG = z80_srb(REG,REG >> 7);
		       SET_TEST_SF(REG); SET_TEST_ZF(REG); SET_TEST_PF(REG); 
		       break;
	case COM_SRL:  SET_CF(REG & 0x01); REG = z80_srb(REG, 0);
		       SET_TEST_SF(REG); SET_TEST_ZF(REG); SET_TEST_PF(REG); 
		       break;
    }
}

