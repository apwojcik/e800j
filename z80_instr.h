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

#ifndef __z80_instr_h__
#define __z80_instr_h__

#define BIN_ALU_FL(a)	SET_TEST_SF(A);SET_TEST_ZF(A);SET_TEST_PF(A);SET_HF(a);SET_NF(0);SET_CF(0);
#define ILLEGAL 


#define ALU_AND		0
#define ALU_OR		1
#define ALU_XOR		2
#define ALU_CPL		4
#define ALU_NEG		5
#define ALU_SCF		6
#define ALU_CCF		7
#define LDRIF		SET_NF(0);SET_PF(IFF2);SET_HF(0);SET_TEST_ZF(A);SET_TEST_SF(A);

/***********************************************************************************************/
#define NOP			SET_M1(0);DEB_CLASS(0);

#define LD(a,b)			SET_M1(0);(a) = (b);DEB_CLASS(1);
#define EX(a,b)			z80_ex(z,&(a),&(b));DEB_CLASS(2);
#define EX_SP			z80_exsp(z);DEB_CLASS(2);

#define EX_AF			SET_M1(0);if(z->AltAF) { SET_AF } else { SET_AF_ }; DEB_CLASS(3);
#define EXX			SET_M1(0);if(z->Alt) { SET_UNI_NRM; } else { SET_UNI_ALT; } DEB_CLASS(3);
#define HALT			z80_halt(z);DEB_CLASS(4);
#define DI			SET_M1(0);IFF1 = IFF2 = 0;DEB_CLASS(5);
#define EI			SET_M1(0);IFF1 = IFF2 = 1;DEB_CLASS(5);
#define IM(a)			SET_M1(0);IM_ = a;DEB_CLASS(5);

#define POP(a)			z80_pop(z,&(a));DEB_CLASS(6);
#define PUSH(a)			z80_push(z,a);DEB_CLASS(6);

#define INC(a)			z80_inc(z,&(a),0);DEB_CLASS(7);
#define DEC(a)			z80_inc(z,&(a),1);DEB_CLASS(7);
#define INCM			z80_incm(z,0);DEB_CLASS(7);
#define DECM			z80_incm(z,1);DEB_CLASS(7);
#define INCW(a)			SET_M1(0);(a)++;DEB_CLASS(7);
#define DECW(a)			SET_M1(0);(a)--;DEB_CLASS(7);

#define ADD_(a,b,sub,cy)	z80_alu_add(a,b,sub,cy,z);
#define ADD(a,b)		a = ADD_(a,b,0,0);DEB_CLASS(8);
#define ADC(a,b)		a = ADD_(a,b,0,GET_CF);DEB_CLASS(8);
#define SUB(a,b)		a = ADD_(a,b,1,0);DEB_CLASS(9);
#define SBC(a,b)		a = ADD_(a,b,1,GET_CF);DEB_CLASS(9);
#define CP(a)			ADD_(A,a,1+8,0);DEB_CLASS(10);

#define ADDW_(a,b,sub,cy) 	z80_alu_add16(a,b,sub,cy,z);
#define ADDW(a,b)		a = ADDW_(a,b,0,0); DEB_CLASS(8);
#define ADCW(a,b)		a = ADDW_(a,b,0,GET_CF);DEB_CLASS(8);
#define SBCW(a,b)		a = ADDW_(a,b,1,GET_CF);DEB_CLASS(9);

#define AND(a)			z80_bit_alu(z,a,ALU_AND);DEB_CLASS(11);
#define OR(a)			z80_bit_alu(z,a,ALU_OR);DEB_CLASS(11);
#define XOR(a)			z80_bit_alu(z,a,ALU_XOR);DEB_CLASS(11);
#define DAA			z80_daa(z);
#define CPL			z80_bit_alu(z,0,ALU_CPL);DEB_CLASS(13);
#define NEG			z80_bit_alu(z,0,ALU_NEG);DEB_CLASS(13);
#define SCF			z80_bit_alu(z,0,ALU_SCF);DEB_CLASS(14);
#define CCF			z80_bit_alu(z,0,ALU_CCF);DEB_CLASS(14);

#define RLCA			z80_shift(z,&(A),COM_RLCA);DEB_CLASS(15);
#define RRCA			z80_shift(z,&(A),COM_RRCA);DEB_CLASS(15);
#define RLA			z80_shift(z,&(A),COM_RLA);DEB_CLASS(15);
#define RRA			z80_shift(z,&(A),COM_RRA);DEB_CLASS(15);
#define RRD			z80_shift(z,&(A),COM_RRD);DEB_CLASS(15);
#define RLD			z80_shift(z,&(A),COM_RLD);DEB_CLASS(15);

#define RLC(b)			z80_shift(z,&(b),COM_RLC);DEB_CLASS(16);
#define RRC(b)			z80_shift(z,&(b),COM_RRC);DEB_CLASS(16);
#define RL(b)			z80_shift(z,&(b),COM_RL);DEB_CLASS(16);
#define RR(b)			z80_shift(z,&(b),COM_RR);DEB_CLASS(16);
#define SLA(b)			z80_shift(z,&(b),COM_SLA);DEB_CLASS(16);
#define SLI(b)			z80_shift(z,&(b),COM_SLI);DEB_CLASS(16);
#define SRA(b)			z80_shift(z,&(b),COM_SRA);DEB_CLASS(16);
#define SRL(b)			z80_shift(z,&(b),COM_SRL);DEB_CLASS(16);

#define JP_(a)			z80_jp(z,1,a);DEB_CLASS(17);
#define JR_(a)			z80_jr(z,1,a);DEB_CLASS(18);
#define JR(a,b)			z80_jr(z,FLAG_TEST(a),b);DEB_CLASS(18);
#define JP(a,b)			z80_jp(z,FLAG_TEST(a),b);DEB_CLASS(17);
#define DJNZ(a)			z80_djnz(z,a);DEB_CLASS(19);

#define CALL_(a)		z80_call(z,a,1);DEB_CLASS(20);
#define CALL(a,b)		z80_call(z,b,FLAG_TEST(a));DEB_CLASS(20);
#define RST(a)			z80_call(z,a,1);DEB_CLASS(21);
#define RET_			z80_ret(z,1,0);DEB_CLASS(22);
#define RET(a)			z80_ret(z,FLAG_TEST(a),0);DEB_CLASS(22);
#define RETI			z80_ret(z,1,1);DEB_CLASS(22);
#define RETN			z80_ret(z,1,2);DEB_CLASS(22);

#define BIT(a,b)		z80_brs(z,a,b,(void *)0,BIT_TST);DEB_CLASS(23);
#define RES(a,b)		z80_brs(z,a,0,&(b),BIT_RES); DEB_CLASS(23);
#define SET(a,b)		z80_brs(z,a,0,&(b),BIT_SET); DEB_CLASS(23);

#define OUT(a,b)		z80_out(z,a,b);DEB_CLASS(24);
#define IN(a,b)			a = z80_in(z,b,0);DEB_CLASS(24);
#define INBC(a,b)		a = z80_in(z,b,1);DEB_CLASS(24);
#define INBCF(a,b)		a = z80_in(z,b,2);DEB_CLASS(24);

#define LDI			z80_ldi(z,0);DEB_CLASS(25);
#define LDD			z80_ldi(z,1);DEB_CLASS(25);
#define LDIR			z80_rep(z,z80_ldi,0,0);DEB_CLASS(25);
#define LDDR			z80_rep(z,z80_ldi,1,0);DEB_CLASS(25);

#define CPI			z80_cpi(z,0);DEB_CLASS(25);
#define CPD			z80_cpi(z,1);DEB_CLASS(25);
#define CDIR			z80_rep(z,z80_cpi,0,0);DEB_CLASS(25);
#define CPDR			z80_rep(z,z80_cpi,1,0);DEB_CLASS(25);

#define INI			z80_ini(z,0);DEB_CLASS(25);
#define IND			z80_ini(z,1);DEB_CLASS(25);
#define INIR			z80_rep(z,z80_ini,0,1);DEB_CLASS(25);
#define INDR			z80_rep(z,z80_ini,1,1);DEB_CLASS(25);

#define OUTI			z80_outi(z,0);DEB_CLASS(25);
#define OUTD			z80_outi(z,1);DEB_CLASS(25);
#define OTIR			z80_rep(z,z80_outi,0,1);DEB_CLASS(25);
#define OTDR			z80_rep(z,z80_outi,1,1);DEB_CLASS(25);

extern byte z80_alu_add(word a, word b, byte sub, byte cy, Z80 *z);
extern word z80_alu_add16(dword a, dword b, byte sub, byte cy, Z80 *z);
extern void z80_push(Z80 *z, word reg);
extern void z80_pop(Z80 *z, word *reg);
extern void z80_call(Z80 *z, word addr, byte condition);
extern void z80_ret(Z80 *z, byte condition, byte type);
extern void z80_brs(Z80 *z, byte bit, byte rg, byte *reg, byte type);
extern void z80_cpi(Z80 *z,byte inc);
extern void z80_ldi(Z80 *z,byte inc);
extern void z80_ini(Z80 *z,byte inc);
extern void z80_outi(Z80 *z,byte inc);
extern void z80_rep(Z80 *z, void (op)(Z80*,byte), byte inc, byte type);
extern void z80_jp(Z80 *z,byte condition,word addr);
extern void z80_jr(Z80 *z,byte condition, byte dis);
extern void z80_djnz(Z80 *z, byte dis);
extern void z80_shift(Z80 *z, byte *reg, byte type);
extern void z80_out(Z80 *z,word addr, byte data);
extern byte z80_in(Z80 *z,word addr, byte type);
extern void z80_inc(Z80 *z,byte *reg, byte type);
extern void z80_incm(Z80 *z, byte type);
extern void z80_daa(Z80 *z);
extern void z80_bit_alu(Z80 *z, byte reg, byte type);
extern void z80_ex(Z80 *z,word *reg1, word *reg2);
extern void z80_exsp(Z80 *z);
extern void z80_halt(Z80 *z);
#endif
