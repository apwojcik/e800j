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

#ifndef __Z80H_CPU__        
#define __Z80H_CPU__

#define Z80_STR(a)	z = a ? a : z80; /* wybór struktury */

extern char tmpstr0[];
extern char tmpstr1[];

extern char *z80_hex8(byte,char *);
extern char *z80_hex16(word,char *);
extern char *z80_hexU(byte,char *);
extern char *z80_hexRTI(byte,char*,char *);
extern void z80_debugger_call(Z80*);
extern void z80_exe(Z80 *);
extern byte z80_parity_check(byte a);
extern word z80_mrw(Z80 *z, word a);
extern void z80_mww(Z80 *z, word c, word d);
extern byte z80_fetch_byte(Z80 *z);




#ifdef DEBUGGER

#define DEBUG(a,b,c,d)		z->deb_par_0 = a, z->deb_par_1 = b, z->deb_par_2 = c, z->deb_par_3 = d;
/* parametry dla DEBUG */
#define DEB_M	0x80
#define DEB_HL	0x81
#define DEB_NRM 0x00

#define DEBUGGER_CLEAN		z->deb_par_0 = z->deb_par_1 = z->deb_par_2 = (void *)0, z->deb_par_3 = 0;

//#define DEBUGGER_CALL		z->deb_class=0xff; 
//if(z->debug){z->old_pc=PC; z80_debugger_call(z);if(z->deb_halt) return;}


#define DRY_RUN_RET	if(z->deb_dry) return;
#define DEB_DJNZ(a)	if(z->proc){z->djnz_d = a;}
#define DEB_CALL(a)	if(z->proc){z->call_d += a ? 1: -1;}
#define DEB_XY(a)	z->xy=a



#define DEB_CLASS(a)	z->deb_class=a
#define DEB_DFP	" "
#define DEB_RT  (z->DD_FD ? (z->xy ? "IY":"IX"):"HL")
#define DEB_DD	(z->DD_FD ? z80_hexU(DD,tmpstr0):"  ")
#define DEB_NNH z80_hex8((z->NNp >> 8) & 0xff,tmpstr0)
#define DEB_NNL z80_hex8(z->NNp & 0xff,tmpstr1)
#define DEB_NS	z80_hex8(z->Np,tmpstr1)
#define DEB_DIS z80_hex16(PC + (char)z->Np,tmpstr1)
#define DEB_HEX16 z80_hex16(z->NNp,tmpstr0)
#define DEB_EP	""
#define DEB_RTI z->DD_FD  ? z80_hexRTI(DD,DEB_RT,tmpstr0) : "HL   "

#else


#endif


#ifndef BIG_ENDIAN
#	define SET_A z->Ap =(byte *) &z->AFr+1
#	define SET_F z->Fp =(byte *) &z->AFr
#	define SET_B z->Bp =(byte *) &z->BCr+1
#	define SET_C z->Cp =(byte *) &z->BCr
#	define SET_D z->Dp =(byte *) &z->DEr+1
#	define SET_E z->Ep =(byte *) &z->DEr
#	define SET_H z->Hp =(byte *) &z->HLr+1
#	define SET_L z->Lp =(byte *) &z->HLr
#	define SET_A_ z->Ap =(byte *) &z->AFr_+1
#	define SET_F_ z->Fp =(byte *) &z->AFr_
#	define SET_B_ z->Bp =(byte *) &z->BCr_+1
#	define SET_C_ z->Cp =(byte *) &z->BCr_
#	define SET_D_ z->Dp =(byte *) &z->DEr_+1
#	define SET_E_ z->Ep =(byte *) &z->DEr_
#	define SET_H_ z->Hp =(byte *) &z->HLr_+1
#	define SET_L_ z->Lp =(byte *) &z->HLr_
#	define SET_IXL z->Lp =(byte *) &z->IXp+1
#	define SET_IXH z->Hp = (byte *) &z->IXp
#	define SET_IYL z->Lp =(byte *) &z->IYp+1
#	define SET_IYH z->Hp =(byte *) &z->IYp
#else
#	define SET_A z->Ap =(byte *) &z->AFr
#	define SET_F z->Fp =(byte *) &z->AFr+1
#	define SET_B z->Bp =(byte *) &z->BCr
#	define SET_C z->Cp =(byte *) &z->BCr+1
#	define SET_D z->Dp =(byte *) &z->DEr
#	define SET_E z->Ep =(byte *) &z->DEr+1
#	define SET_H z->Hp =(byte *) &z->HLr
#	define SET_L z->Lp =(byte *) &z->HLr+1
#	define SET_A_ z->Ap =(byte *) &z->AFr_
#	define SET_F_ z->Fp =(byte *) &z->AFr_+1
#	define SET_B_ z->Bp =(byte *) &z->BCr_
#	define SET_C_ z->Cp =(byte *) &z->BCr_+1
#	define SET_D_ z->Dp =(byte *) &z->DEr_
#	define SET_E_ z->Ep =(byte *) &z->DEr_+1
#	define SET_H_ z->Hp =(byte *) &z->HLr_
#	define SET_L_ z->Lp =(byte *) &z->HLr_+1
#	define SET_IXL z->Lp =(byte *) &z->IXp
#	define SET_IXH z->Hp = (byte *) &z->IXp+1
#	define SET_IYL z->Lp =(byte *) &z->IYp
#	define SET_IYH z->Hp =(byte *) &z->IYp+1
#endif

#define SET_M1(a)	z->M1=a;
#define GET_M1		(z->M1)

#define SET_AF 		z->AFp = &z->AFr; SET_A; SET_F; z->AltAF=0;
#define SET_BC 		z->BCp = &z->BCr; SET_B; SET_C;
#define SET_DE 		z->DEp = &z->DEr; SET_D; SET_E;
#define SET_HL 		z->HLp = &z->HLr; SET_H; SET_L; 

#define SET_AF_ 	z->AFp = &z->AFr_; SET_A_; SET_F_; z->AltAF=1;
#define SET_BC_ 	z->BCp = &z->BCr_; SET_B_; SET_C_;
#define SET_DE_ 	z->DEp = &z->DEr_; SET_D_; SET_E_;
#define SET_HL_ 	z->HLp = &z->HLr_; SET_H_; SET_L_;

//#define SET_IX 	z->HLp = &z->IXp; SET_IXL; SET_IXH; z->DD_FD=1; DEB_XY(0);
//#define SET_IY 	z->HLp = &z->IYp; SET_IYL; SET_IYH; z->DD_FD=1; DEB_XY(1);

#define SET_IX 		z->HLp = &z->IXp;  z->DD_FD=1; DEB_XY(0);
#define SET_IY 		z->HLp = &z->IYp;  z->DD_FD=1; DEB_XY(1);


#define SET_UNI_NRM 	SET_BC; SET_DE;SET_HL;z->Alt=0;
#define SET_UNI_ALT 	SET_BC_; SET_DE_;SET_HL_;z->Alt=1;

#define A		(*z->Ap)
#define B		(*z->Bp)
#define C		(*z->Cp)
#define D		(*z->Dp)
#define E		(*z->Ep)
#define F		(*z->Fp)
#define L		(*z->Lp)
#define H		(*z->Hp)
#define I		(z->Ip)
#define R		(z->Rp)
#define IXH		(z->IXHp)
#define IXL		(z->IXLp)
#define IYH		(z->IYHp)
#define IYL		(z->IYLp)

#define AF		(*z->AFp)
#define BC		(*z->BCp)
#define DE		(*z->DEp)
#define HL		(*z->HLp)
#define IX		(z->IXp)
#define IY		(z->IYp)
#define PC		(z->PCp)
#define SP		(z->SPp)

#define IFF1		(z->IFF1p)
#define IFF2		(z->IFF2p)
#define IM_		z->IMr
#define SET_HALT(a)	z->Halt_state=a


#define BIT_TST	0
#define BIT_RES	1
#define BIT_SET	2

/* cykle maszynowe, takty, i po dd lub fd */
#define TICKS(mach, ticks, mach_dd, ticks_dd) if(z->DD_FD) { z->mach += mach_dd;z->ticks+=ticks_dd } \
						else { z->mach += mach;z->tics+=ticks } 
#define TICKS_RST	z->mach = z->ticks= 0;

#define IO(a,b,c) 	z->io(a,b,c,z)

#define MASK_CF		1
#define MASK_NF 	2
#define MASK_PF 	4
#define MASK_HF 	16
#define MASK_ZF 	64
#define MASK_SF 	128

/* ustawia znacznik */
#define SET_CF(a) 	F = (a) ? (F | MASK_CF) : (F & ~MASK_CF);
#define SET_NF(a) 	F = (a) ? (F | MASK_NF) : (F & ~MASK_NF);
#define SET_PF(a) 	F = (a) ? (F | MASK_PF) : (F & ~MASK_PF);
#define SET_HF(a) 	F = (a) ? (F | MASK_HF) : (F & ~MASK_HF);
#define SET_ZF(a) 	F = (a) ? (F | MASK_ZF) : (F & ~MASK_ZF);
#define SET_SF(a) 	F = (a) ? (F | MASK_SF) : (F & ~MASK_SF);
#define SET_53(a)
/* pobieranie wartosci znacznika, wynikiem jest 1 jesli ustawiony lub 0 */
#define GET_CF		((F & MASK_CF) ? 1:0)
#define GET_NF		((F & MASK_NF) ? 1:0)
#define GET_PF		((F & MASK_PF) ? 1:0)
#define GET_HF		((F & MASK_HF) ? 1:0)
#define GET_ZF		((F & MASK_ZF) ? 1:0)
#define GET_SF		((F & MASK_SF) ? 1:0)

#define GNZ		0
#define GZ		1
#define GNC		2
#define GC		3
#define GPO		4
#define GPE		5
#define GP		6
#define GM		7

/* sprawdza stan znaczników, zwraca 1 lub 0 */
#define FLAG_TEST(a) 	z80_test_flag(z,a)

/* ustawia flagi zgodnie z wartosci± parametru a*/
#define SET_TEST_SF(a)	SET_SF((a) & MASK_SF);
#define SET_TEST_ZF(a)	SET_ZF(!(a));
#define SET_TEST_PF(a)	SET_PF(z80_parity_check(a) ? MASK_PF:0);

#ifdef DEBUGGER
#define N		(z->Np=z80_fetch_byte(z))
#define NN		(z->NNp=z80_fetch_word(z))
#else
#define N		(z80_fetch_byte(z))
#define NN		(z80_fetch_word(z))
#endif

#define DD		(z->dd)
#define GET_DD 		{if(z->DD_FD) z->dd=z80_fetch_byte(z);}

#define MR(a) 		(z->RD_MEM(z,(a)))
#define MW(a,b)		(z->WR_MEM(z,(a),(b)))
#define MRW(a)		z80_mrw(z,a)
#define MWW(a,b)	z80_mww(z,a,b)

#define MEM(a) 		{tmp=MR(HL+DD); a; MW(HL+DD,tmp);}

#define COM_RLCA	0
#define COM_RRCA	1
#define COM_RLA		2
#define COM_RRA		3
#define COM_RLC		4
#define COM_RRC		5
#define COM_RL		6
#define COM_RR		7
#define COM_SLA		8
#define COM_SLI		9
#define COM_SRA		10
#define COM_SRL		11
#define COM_RRD		12
#define COM_RLD		13

#define REG		(*reg)

#endif

