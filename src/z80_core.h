/* Elwro 800 Junior emulator v0.3
 * Copyright (C) 2006 Krzysztof Komarnicki
 * Copyright (C) 2026 Artur Wojcik
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

/*
    z80_core.h
    
    Definicje wartości parametrów tablicy dekodera rozkazów, oraz jej struktury.
    Realizacja emulacji rozkazów.

    Autor: Krzysztof Komarnicki
    Ver: 0.1
    Data: 19.08.2006
    Licencja: GPL v2
*/

#pragma once

#include "z80_cpu.h"

/* struktura tablicy dekodera rozkazów */
struct z80dt {
    void (*code)(struct cpu*); /* kod operacji              - OC */
    /* reg2 = reg0 OPERATOR reg2  */
    unsigned char reg0; /* rejest wyniku  - D  */
    unsigned char reg1; /* rejest param 0 - R0 */
    unsigned char reg2; /* rejest param 1 - R1 */

    unsigned char bytes;  /* długość rozkazu w bajtach - LC */
    unsigned char cycles; /* ilość cykli maszynowych   - CC */
    unsigned char ticks;  /* ilość taktów              - TC */

    unsigned char f_mask_and; /* maska zerowanych znaczników     - AM */
    unsigned char f_mask_or;  /* maska ustawianych znaczników    - OM */
    unsigned char f_mask_chg; /* maska modyfikowanych znaczników - CM */
};

enum {
    RA = 0,
    RF = 1,
    RB = 2,
    RC = 3,
    RD = 4,
    RE = 5,
    RH = 6,
    RL = 7,
    RXH = 8,
    RXL = 9,
    RYH = 10,
    RYL = 11,
    R_I = 12,
    R_R = 13,
    MHL = 14,
    MIX = 15,
    MIY = 16,
    MSP = 17,
    MBC = 18,
    MDE = 19,
    /* specjalne */
    MCDW = 32, /* zmienna adresowana stałą */
    CDB = 33, /* stała bajtowa */
    CDW = 34, /* stała słowo   */
    RNR = 35, /* bez rejestru  */
    /* rejestry bezwzgl 16 bitowe */
    RAF = 0,
    RBC = 1,
    RDE = 2,
    RHL = 3,
    RIX = 4,
    RIY = 5,
    RSP = 6,
    RPC = 7,
    R0AF = 8,
    R1AF = 9,
};

/*
    Kodowanie znaczników
*/
enum {
    FS = 128,
    FZ = 64,
    F5 = 32,
    FH = 16,
    F3 = 8,
    FP = 4,
    FN = 2,
    FC = 1
};

/*************************************************************************************************/

/* podklasa dla arytmometru, ARYTMO,X */
enum { INC, DEC, ADD, SUB, ADC, SBC, CP };

/* podklasa dla operacji blokowych */
enum { PIR, PDR, PI, PD };

/* warunki */
enum { NOC = 0, NZ, Z, NC, C, PO, PE, P, M };

#define NOP	z80_nop
#define LDB	z80_ldb
#define LDBRM	z80_ldbrm
#define LDBMR	z80_ldbmr
#define LDBMN	z80_ldbmn
#define LDBMNR	z80_ldbmnr
#define LDBRMN	z80_ldbrmn
#define LDBN	z80_ldbn
#define LDW	z80_ldw
#define LDWN	z80_ldwn
#define LDWRMN	z80_ldwrmn
#define LDWMNR	z80_ldwmnr
#define EX	z80_ex
#define DJNZ	z80_djnz
#define JR	z80_jr
#define DAA	z80_daa
#define CPL	z80_cpl
#define SCF	z80_scf
#define CCF	z80_ccf
#define HALT	z80_halt
#define AND	z80_and
#define ANDN	z80_andn
#define ANDM	z80_andm
#define OR	z80_or
#define ORN	z80_orn
#define ORM	z80_orm
#define XOR	z80_xor
#define XORN	z80_xorn
#define XORM	z80_xorm
#define RET	z80_ret
#define POP	z80_pop
#define JP	z80_jp
#define CALL	z80_call
#define PUSH	z80_push
#define RST	z80_rst
#define OUTA	z80_outa
#define EXX	z80_exx
#define INA	z80_ina
#define PREF_DD	z80_pref_dd
#define PREF_ED	z80_pref_ed
#define PREF_FD	z80_pref_fd
#define PREF_CB	z80_pref_cb
#define DI	z80_di
#define EI	z80_ei
#define IM	z80_im
#define NEG	z80_neg
#define RETI	z80_reti
#define RETN	z80_retn
#define BIT	z80_bit
#define BITM	z80_bitm
#define RES	z80_res
#define RESM	z80_resm
#define SET	z80_set
#define SETM	z80_setm
#define LDB_    z80_ldb_
#define LDBL    z80_ldbl
#define CPBL    z80_cpbl
#define INBL    z80_inbl
#define OUTBL	z80_outbl
#define INBC	z80_inbc
#define OUTBC	z80_outbc
#define RRD	z80_rrd
#define RLD	z80_rld
#define RLC	z80_rlc
#define RLCM	z80_rlcm
#define RRC	z80_rrc
#define RRCM	z80_rrcm
#define RLS	z80_rls
#define RLSM	z80_rlsm
#define RR	z80_rr
#define RRM	z80_rrm
#define SLA	z80_sla
#define SLAM	z80_slam
#define SRA	z80_sra
#define SRAM	z80_sram
#define SLI	z80_sli
#define SLIM	z80_slim
#define SRL	z80_srl
#define SRLM	z80_srlm
#define ADD8	z80_add8
#define ADDN8	z80_addn8
#define ADDM8	z80_addm8
#define ADC8	z80_adc8
#define ADCN8	z80_adcn8
#define ADCM8	z80_adcm8
#define SUB8	z80_sub8
#define SUBN8	z80_subn8
#define SUBM8	z80_subm8
#define SBC8	z80_sbc8
#define SBCN8	z80_sbcn8
#define SBCM8	z80_sbcm8
#define CP8	z80_cp
#define CPN8	z80_cpn
#define CPM8	z80_cpm
#define DEC8	z80_dec8
#define DECM8	z80_decm8
#define INC8	z80_inc8
#define INCM8	z80_incm8
#define INCX	z80_incx
#define DECX	z80_decx
#define ADDX	z80_addx
#define ADCX	z80_adcx
#define SBCX	z80_sbcx
#define JPHL	z80_jphl

void z80_ldbrmn(struct cpu*);
void z80_addx(struct cpu*);
void z80_adcx(struct cpu*);
void z80_sbcx(struct cpu*);
void z80_add8(struct cpu*);
void z80_addn8(struct cpu*);
void z80_addm8(struct cpu*);
void z80_adc8(struct cpu*);
void z80_adcn8(struct cpu*);
void z80_adcm8(struct cpu*);
void z80_sub8(struct cpu*);
void z80_subn8(struct cpu*);
void z80_subm8(struct cpu*);
void z80_sbc8(struct cpu*);
void z80_sbcn8(struct cpu*);
void z80_sbcm8(struct cpu*);
void z80_cp(struct cpu*);
void z80_cpn(struct cpu*);
void z80_cpm(struct cpu*);
void z80_inc8(struct cpu*);
void z80_incm8(struct cpu*);
void z80_dec8(struct cpu*);
void z80_decm8(struct cpu*);
void z80_nop(struct cpu*);
void z80_ldb(struct cpu*);
void z80_ldbn(struct cpu*);
void z80_ldbrm(struct cpu*);
void z80_ldbmr(struct cpu*);
void z80_ldbmn(struct cpu*);
void z80_ldbmnr(struct cpu*);
void z80_ldw(struct cpu*);
void z80_ldwn(struct cpu*);
void z80_ldwmnr(struct cpu*);
void z80_ldwrmn(struct cpu*);
void z80_ex(struct cpu*);
void z80_djnz(struct cpu*);
void z80_jr(struct cpu*);
void z80_daa(struct cpu*);
void z80_cpl(struct cpu*);
void z80_scf(struct cpu*);
void z80_ccf(struct cpu*);
void z80_halt(struct cpu*);
void z80_and(struct cpu*);
void z80_andn(struct cpu*);
void z80_andm(struct cpu*);
void z80_or(struct cpu*);
void z80_orn(struct cpu*);
void z80_orm(struct cpu*);
void z80_xor(struct cpu*);
void z80_xorn(struct cpu*);
void z80_xorm(struct cpu*);
void z80_ret(struct cpu*);
void z80_pop(struct cpu*);
void z80_jp(struct cpu*);
void z80_call(struct cpu*);
void z80_push(struct cpu*);
void z80_rst(struct cpu*);
void z80_outa(struct cpu*);
void z80_exx(struct cpu*);
void z80_ina(struct cpu*);
void z80_pref_dd(struct cpu*);
void z80_pref_ed(struct cpu*);
void z80_pref_fd(struct cpu*);
void z80_pref_cb(struct cpu*);
void z80_di(struct cpu*);
void z80_ei(struct cpu*);
void z80_im(struct cpu*);
void z80_neg(struct cpu*);
void z80_reti(struct cpu*);
void z80_retn(struct cpu*);
void z80_bit(struct cpu*);
void z80_bitm(struct cpu*);
void z80_res(struct cpu*);
void z80_resm(struct cpu*);
void z80_set(struct cpu*);
void z80_setm(struct cpu*);
void z80_ldb_(struct cpu*);
void z80_ldbl(struct cpu*);
void z80_cpbl(struct cpu*);
void z80_inbl(struct cpu*);
void z80_outbl(struct cpu*);
void z80_inbc(struct cpu*);
void z80_outbc(struct cpu*);
void z80_rrd(struct cpu*);
void z80_rld(struct cpu*);
void z80_rlc(struct cpu*);
void z80_rlcm(struct cpu*);
void z80_rrc(struct cpu*);
void z80_rrcm(struct cpu*);
void z80_rls(struct cpu*);
void z80_rlsm(struct cpu*);
void z80_rr(struct cpu*);
void z80_rrm(struct cpu*);
void z80_sla(struct cpu*);
void z80_slam(struct cpu*);
void z80_sra(struct cpu*);
void z80_sram(struct cpu*);
void z80_sli(struct cpu*);
void z80_slim(struct cpu*);
void z80_srl(struct cpu*);
void z80_srlm(struct cpu*);
void z80_incx(struct cpu*);
void z80_decx(struct cpu*);
void z80_jphl(struct cpu*);

#define AFr	(*(z->AF_r))
#define BCr	(*(z->BC_r))
#define DEr	(*(z->DE_r))
#define HLr	(*(z->HL_r))
#define IXr	(*(z->IX_r))
#define IYr	(*(z->IY_r))
#define PCr	(*(z->PC_r))
#define SPr	(*(z->SP_r))
#define Ar	(*(z->A_r))
#define Br	(*(z->B_r))
#define Cr	(*(z->C_r))
#define Dr	(*(z->D_r))
#define Er	(*(z->E_r))
#define Hr	(*(z->H_r))
#define Lr	(*(z->L_r))
#define Fr	(*(z->F_r))
#define XLr	(*(z->XL_r))
#define XHr	(*(z->XH_r))
#define YLr	(*(z->YL_r))
#define YHr	(*(z->YH_r))
#define Ir	(*(z->I_r))
#define Rr	(*(z->R_r))