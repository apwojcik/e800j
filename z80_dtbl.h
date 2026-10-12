/* Elwro 800 Junior emulator v 0.2
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

/*
    z80dectabl.h

    Tablice stałych do dekodowania instrukcji, obliczania znaczników i detektora warunku


    Z80_DECODE_TABLE - tablica dekodująca instrukcje CPU Z80, składa się z 9 bloków po 256 struktur,
    każdy blok dekoduje instrukcje wg kombinacji wszystkich prefiksów:
        CB ED DD FD
    0 - 0  0  0  0
    1 - 0  0  1  0
    2 - 0  0  0  1
    3 - 1  0  0  0
    4 - 0  0  1  0
    5 - 0  0  0  1
    6 - 0  1  0  0
    7 - 0  0  1  0
    8 - 0  0  0  1

    Na każdą strukturę przypada 10 danych w kolejności:
    0 - wskaźnik do funkcji obsługi instrukcji
    1,2,3 - parametry instrukcji (rejestry, ofsety itp)
    4 - ilość bajtów / instrukcję
    5 - cykle
    6 - takty
    7 - maska AND (pozytywna, tzn 1 na pozycji zeruje bit)
    8 - maska OR
    9 - maska zezwolenia modyfikacji


    Z80_PARITY_TABLE - tablica zwraca informację o parzystości liczby
    Z80_CARRY4_TABLE - przeniesienia przy dodawaniu argumentów 4 bitowych (5x4 bity)
    Z80_CARRY3_TABLE - przeniesienia przy dodawaniu argumentów 3 bitowych (4x3 bity)
    Z80_COND_MASK - detektor warunku, maska bitowa
    Z80_COND_PLRT - detektor warunku, polaryzacja


    Wystąpienie warunku jest liczone w następujący sposób:

    war = (cond_mask[cond] & (Fr | F5)) ^ cond_plrt[cond]
    gdzie:
    war - warunek, typu boolean
    cond - numer warunku
    Fr - rejestr F
    F5 - bit 5, ustawiany jest przy teście dla warunku 0, aby zawsze był spełniony

*/

#ifndef __Z80DECTABL_H__
#define __Z80DECTABL_H__

/****************************** Przeslania 8-bit ******************************************/
/*         		   OC,     D,  R0,  R1,    LC,   CC,     TC,   AM,   OM,   CM */
#define _NOP 		{ NOP,     RNR,   RNR, RNR,  0x01, 0x01, 0x04,   0x00, 0x00, 0x00 }
#define _LDR_W(r,w)	{ LDB,    R##r,  R##w, RNR,  0x01, 0x01, 0x04,   0x00, 0x00, 0x00 }
#define _LDR_X(r,w)	{ LDB,    R##r,  R##w, RNR,  0x02, 0x01, 0x04,   0x00, 0x00, 0x00 }
#define _LDA_B		_LDR_W(A,B)
#define _LDA_C		_LDR_W(A,C)
#define _LDA_D		_LDR_W(A,D)
#define _LDA_E		_LDR_W(A,E)
#define _LDA_H		_LDR_W(A,H)
#define _LDA_L		_LDR_W(A,L)
#define _LDA_A		_LDR_W(A,A)
#define _LDB_B		_LDR_W(B,B)
#define _LDB_C		_LDR_W(B,C)
#define _LDB_D		_LDR_W(B,D)
#define _LDB_E		_LDR_W(B,E)
#define _LDB_H		_LDR_W(B,H)
#define _LDB_L		_LDR_W(B,L)
#define _LDB_A		_LDR_W(B,A)
#define _LDC_B		_LDR_W(C,B)
#define _LDC_C		_LDR_W(C,C)
#define _LDC_D		_LDR_W(C,D)
#define _LDC_E		_LDR_W(C,E)
#define _LDC_H		_LDR_W(C,H)
#define _LDC_L		_LDR_W(C,L)
#define _LDC_A		_LDR_W(C,A)
#define _LDD_B		_LDR_W(D,B)
#define _LDD_C		_LDR_W(D,C)
#define _LDD_D		_LDR_W(D,D)
#define _LDD_E		_LDR_W(D,E)
#define _LDD_H		_LDR_W(D,H)
#define _LDD_L		_LDR_W(D,L)
#define _LDD_A		_LDR_W(D,A)
#define _LDE_B		_LDR_W(E,B)
#define _LDE_C		_LDR_W(E,C)
#define _LDE_D		_LDR_W(E,D)
#define _LDE_E		_LDR_W(E,E)
#define _LDE_H		_LDR_W(E,H)
#define _LDE_L		_LDR_W(E,L)
#define _LDE_A		_LDR_W(E,A)
#define _LDH_B		_LDR_W(H,B)
#define _LDH_C		_LDR_W(H,C)
#define _LDH_D		_LDR_W(H,D)
#define _LDH_E		_LDR_W(H,E)
#define _LDH_H		_LDR_W(H,H)
#define _LDH_L		_LDR_W(H,L)
#define _LDH_A		_LDR_W(H,A)
#define _LDL_B		_LDR_W(L,B)
#define _LDL_C		_LDR_W(L,C)
#define _LDL_D		_LDR_W(L,D)
#define _LDL_E		_LDR_W(L,E)
#define _LDL_H		_LDR_W(L,H)
#define _LDL_L		_LDR_W(L,L)
#define _LDL_A		_LDR_W(L,A)
/* nieoficjalne przesłania */
#define _LDXL_B		_LDR_X(XL,B)
#define _LDXL_C		_LDR_X(XL,C)
#define _LDXL_D		_LDR_X(XL,D)
#define _LDXL_E		_LDR_X(XL,E)
#define _LDXL_H		_LDR_X(XL,H)
#define _LDXL_L		_LDR_X(XL,L)
#define _LDXL_A		_LDR_X(XL,A)
#define _LDXH_B		_LDR_X(XH,B)
#define _LDXH_C		_LDR_X(XH,C)
#define _LDXH_D		_LDR_X(XH,D)
#define _LDXH_E		_LDR_X(XH,E)
#define _LDXH_H		_LDR_X(XH,H)
#define _LDXH_L		_LDR_X(XH,L)
#define _LDXH_A		_LDR_X(XH,A)
#define _LDYL_B		_LDR_X(YL,B)
#define _LDYL_C		_LDR_X(YL,C)
#define _LDYL_D		_LDR_X(YL,D)
#define _LDYL_E		_LDR_X(YL,E)
#define _LDYL_H		_LDR_X(YL,H)
#define _LDYL_L		_LDR_X(YL,L)
#define _LDYL_A		_LDR_X(YL,A)
#define _LDYH_B		_LDR_X(YH,B)
#define _LDYH_C		_LDR_X(YH,C)
#define _LDYH_D		_LDR_X(YH,D)
#define _LDYH_E		_LDR_X(YH,E)
#define _LDYH_H		_LDR_X(YH,H)
#define _LDYH_L		_LDR_X(YH,L)
#define _LDYH_A		_LDR_X(YH,A)
#define _LDB_XH		_LDR_X(B,XH)
#define _LDC_XH		_LDR_X(C,XH)
#define _LDD_XH		_LDR_X(D,XH)
#define _LDE_XH		_LDR_X(E,XH)
#define _LDH_XH		_LDR_X(H,XH)
#define _LDL_XH		_LDR_X(L,XH)
#define _LDA_XH		_LDR_X(A,XH)
#define _LDB_XL		_LDR_X(B,XL)
#define _LDC_XL		_LDR_X(C,XL)
#define _LDD_XL		_LDR_X(D,XL)
#define _LDE_XL		_LDR_X(E,XL)
#define _LDH_XL		_LDR_X(H,XL)
#define _LDL_XL		_LDR_X(L,XL)
#define _LDA_XL		_LDR_X(A,XL)
#define _LDB_YL		_LDR_X(B,YL)
#define _LDC_YL		_LDR_X(C,YL)
#define _LDD_YL		_LDR_X(D,YL)
#define _LDE_YL		_LDR_X(E,YL)
#define _LDH_YL		_LDR_X(H,YL)
#define _LDL_YL		_LDR_X(L,YL)
#define _LDA_YL		_LDR_X(A,YL)
#define _LDB_YH		_LDR_X(B,YH)
#define _LDC_YH		_LDR_X(C,YH)
#define _LDD_YH		_LDR_X(D,YH)
#define _LDE_YH		_LDR_X(E,YH)
#define _LDH_YH		_LDR_X(H,YH)
#define _LDL_YH		_LDR_X(L,YH)
#define _LDA_YH		_LDR_X(A,YH)
#define _LDR_N(r)	{ LDBN,  R##r, CDB, RNR,  0x02, 0x02, 0x07,   0x00, 0x00, 0x00 }
#define _LDR_N_x(r)	{ LDBN,  R##r, CDB, RNR,  0x03, 0x02, 0x07,   0x00, 0x00, 0x00 }
#define _LDA_N		_LDR_N(A)
#define _LDB_N		_LDR_N(B)
#define _LDC_N		_LDR_N(C)
#define _LDD_N		_LDR_N(D)
#define _LDE_N		_LDR_N(E)
#define _LDH_N		_LDR_N(H)
#define _LDL_N		_LDR_N(L)
#define _LDXH_N		_LDR_N(XH)
#define _LDXL_N		_LDR_N(XL)
#define _LDYH_N		_LDR_N(YH)
#define _LDYL_N		_LDR_N(YL)
#define _LDR_MHL(r)	{ LDBRM,  R##r, RHL, RNR,  0x01, 0x02, 0x07,   0x00, 0x00, 0x00 }
#define _LDR_MIX(r)	{ LDBRM,  R##r, RIX, CDB,  0x03, 0x05, 0x13,   0x00, 0x00, 0x00 }
#define _LDR_MIY(r)	{ LDBRM,  R##r, RIY, CDB,  0x03, 0x05, 0x13,   0x00, 0x00, 0x00 }
#define _LDA_MHL	_LDR_MHL(A)
#define _LDB_MHL	_LDR_MHL(B)
#define _LDC_MHL	_LDR_MHL(C)
#define _LDD_MHL	_LDR_MHL(D)
#define _LDE_MHL	_LDR_MHL(E)
#define _LDH_MHL	_LDR_MHL(H)
#define _LDL_MHL	_LDR_MHL(L)
#define _LDA_MIX	_LDR_MIX(A)
#define _LDB_MIX	_LDR_MIX(B)
#define _LDC_MIX	_LDR_MIX(C)
#define _LDD_MIX	_LDR_MIX(D)
#define _LDE_MIX	_LDR_MIX(E)
#define _LDH_MIX	_LDR_MIX(H)
#define _LDL_MIX	_LDR_MIX(L)
#define _LDA_MIY	_LDR_MIY(A)
#define _LDB_MIY	_LDR_MIY(B)
#define _LDC_MIY	_LDR_MIY(C)
#define _LDD_MIY	_LDR_MIY(D)
#define _LDE_MIY	_LDR_MIY(E)
#define _LDH_MIY	_LDR_MIY(H)
#define _LDL_MIY	_LDR_MIY(L)
#define _LDMHL_R(r)	{ LDBMR,  RHL, R##r, RNR,  0x01, 0x02, 0x07,   0x00, 0x00, 0x00 }
#define _LDMIX_R(r)	{ LDBMR,  RIX, R##r, CDB,  0x03, 0x05, 0x13,   0x00, 0x00, 0x00 }
#define _LDMIY_R(r)	{ LDBMR,  RIY, R##r, CDB,  0x03, 0x05, 0x13,   0x00, 0x00, 0x00 }
#define _LDMHL_A	_LDMHL_R(A)
#define _LDMHL_B	_LDMHL_R(B)
#define _LDMHL_C	_LDMHL_R(C)
#define _LDMHL_D	_LDMHL_R(D)
#define _LDMHL_E	_LDMHL_R(E)
#define _LDMHL_H	_LDMHL_R(H)
#define _LDMHL_L	_LDMHL_R(L)
#define _LDMIX_A	_LDMIX_R(A)
#define _LDMIX_B	_LDMIX_R(B)
#define _LDMIX_C	_LDMIX_R(C)
#define _LDMIX_D	_LDMIX_R(D)
#define _LDMIX_E	_LDMIX_R(E)
#define _LDMIX_H	_LDMIX_R(H)
#define _LDMIX_L	_LDMIX_R(L)
#define _LDMIY_A	_LDMIY_R(A)
#define _LDMIY_B	_LDMIY_R(B)
#define _LDMIY_C	_LDMIY_R(C)
#define _LDMIY_D	_LDMIY_R(D)
#define _LDMIY_E	_LDMIY_R(E)
#define _LDMIY_H	_LDMIY_R(H)
#define _LDMIY_L	_LDMIY_R(L)
#define _LDMHL_N	{ LDBMN,  RHL, CDB, RNR,  0x02, 0x03, 0x0a,   0x00, 0x00, 0x00 }
#define _LDMIX_N	{ LDBMN,  RIX, CDB, CDB,  0x04, 0x05, 0x13,   0x00, 0x00, 0x00 }
#define _LDMIY_N	{ LDBMN,  RIY, CDB, CDB,  0x04, 0x05, 0x13,   0x00, 0x00, 0x00 }
#define _LDA_MBC	{ LDBRM,  RA, RBC,  RNR,  0x01, 0x02, 0x07,   0x00, 0x00, 0x00 }
#define _LDA_MDE	{ LDBRM,  RA, RDE,  RNR,  0x01, 0x02, 0x07,   0x00, 0x00, 0x00 }
#define _LDA_MCDW	{ LDBRMN,  RA, MCDW, RNR,  0x03, 0x04, 0x0d,   0x00, 0x00, 0x00 }
#define _LDMBC_A	{ LDBMR,  RBC, RA,  RNR,  0x01, 0x02, 0x07,   0x00, 0x00, 0x00 }
#define _LDMDE_A	{ LDBMR,  RDE, RA,  RNR,  0x01, 0x02, 0x07,   0x00, 0x00, 0x00 }
#define _LDMCDW_A	{ LDBMNR, MCDW, RA,  RNR,  0x03, 0x04, 0x0d,   0x00, 0x00, 0x00 }
#define _LDA_I		{ LDB_, RA, R_I,  RNR,  0x02, 0x02, 0x09,   FH+FN, 0x00, ~FC }
#define _LDA_R		{ LDB_, RA, R_R,  RNR,  0x02, 0x02, 0x09,   FH+FN, 0x00, ~FC }
#define _LDI_A		{ LDB, R_I,  RA,  RNR,  0x02, 0x02, 0x09,   0x00, 0x00, 0x00 }
#define _LDR_A		{ LDB, R_R,  RA,  RNR,  0x02, 0x02, 0x09,   0x00, 0x00, 0x00 }

/****************************** Przeslania 16-bit ******************************************/
/*         		   OC,   D,  R0,   R1,    LC,   CC,   TC,     AM,   OM,   CM */
#define _LDDD_NN(r)	{ LDWN,  R##r, CDW,  RNR,  0x03, 0x03, 0x0a,   0x00, 0x00, 0x00 }
#define _LDDD_NN_x(r)	{ LDWN,  R##r, CDW,  RNR,  0x04, 0x04, 0x0e,   0x00, 0x00, 0x00 }
#define _LDBC_NN	_LDDD_NN(BC)
#define _LDDE_NN	_LDDD_NN(DE)
#define _LDHL_NN	_LDDD_NN(HL)
#define _LDSP_NN	_LDDD_NN(SP)
#define _LDIX_NN	_LDDD_NN_x(IX)
#define _LDIY_NN	_LDDD_NN_x(IY)
#define _LDDD_MCDW(r)	{ LDWRMN, R##r, MCDW,  RNR,  0x04, 0x06, 0x14,   0x00, 0x00, 0x00 }
#define _LDBC_MCDW	_LDDD_MCDW(BC)
#define _LDDE_MCDW	_LDDD_MCDW(DE)
#define _LDHL_MCDW	{ LDWRMN,  RHL, MCDW,  RNR,  0x03, 0x05, 0x10,   0x00, 0x00, 0x00 }
#define _LDSP_MCDW	_LDDD_MCDW(SP)
#define _LDIX_MCDW	{ LDWRMN,  RIX, MCDW,  RNR,  0x04, 0x06, 0x14,   0x00, 0x00, 0x00 }
#define _LDIY_MCDW	{ LDWRMN,  RIY, MCDW,  RNR,  0x04, 0x06, 0x14,   0x00, 0x00, 0x00 }
#define _LDMCDW_DD(r)	{ LDWMNR, MCDW, R##r,  RNR,  0x04, 0x06, 0x14,   0x00, 0x00, 0x00 }
#define _LDMCDW_BC	_LDMCDW_DD(BC)
#define _LDMCDW_DE	_LDMCDW_DD(DE)
#define _LDMCDW_HL	{ LDWMNR,  MCDW, RHL,  RNR,  0x03, 0x05, 0x10,   0x00, 0x00, 0x00 }
#define _LDMCDW_SP	_LDMCDW_DD(SP)
#define _LDMCDW_IX	_LDMCDW_DD(IX)
#define _LDMCDW_IY	_LDMCDW_DD(IY)
#define _LDSP_HL	{ LDW,   RSP, RHL,  RNR,  0x01, 0x01, 0x06,   0x00, 0x00, 0x00 }
#define _LDSP_IX	{ LDW,   RSP, RIX,  RNR,  0x02, 0x02, 0x0a,   0x00, 0x00, 0x00 }
#define _LDSP_IY	{ LDW,   RSP, RIY,  RNR,  0x02, 0x02, 0x0a,   0x00, 0x00, 0x00 }
#define _PUSH_DD(r)	{ PUSH, R##r, RNR,  RNR,  0x01, 0x03, 0x0b,   0x00, 0x00, 0x00 }
#define _PUSHBC		_PUSH_DD(BC)
#define _PUSHDE		_PUSH_DD(DE)
#define _PUSHHL		_PUSH_DD(HL)
#define _PUSHAF		_PUSH_DD(AF)
#define _PUSHIX		{ PUSH,   RIX, RNR,  RNR,  0x02, 0x04, 0x0f,   0x00, 0x00, 0x00 }
#define _PUSHIY		{ PUSH,   RIY, RNR,  RNR,  0x02, 0x04, 0x0f,   0x00, 0x00, 0x00 }
#define _POP_DD(r)	{ POP,   R##r, RNR,  RNR,  0x01, 0x03, 0x0a,   0x00, 0x00, 0x00 }
#define _POPBC		_POP_DD(BC)
#define _POPDE		_POP_DD(DE)
#define _POPHL		_POP_DD(HL)
#define _POPAF		_POP_DD(AF)
#define _POPIX		{ POP,   RIX, RNR,  RNR,  0x02, 0x04, 0x0d,   0x00, 0x00, 0x00 }
#define _POPIY		{ POP,   RIY, RNR,  RNR,  0x02, 0x04, 0x0d,   0x00, 0x00, 0x00 }

/******************************* Instrukcje zamian **************************************/
/*         		   OC,   D,  R0,   R1,    LC,   CC,   TC,     AM,   OM,   CM */
#define _EXDE_HL	{  EX, RDE, RHL,  RNR,  0x01, 0x01, 0x04,   0x00, 0x00, 0x00 }
#define _EXAF_AF	{  EX,R0AF,R1AF,  RNR,  0x01, 0x01, 0x04,   0x00, 0x00, 0x00 }
#define _EXX		{  EXX,RNR, RNR,  RNR,  0x01, 0x01, 0x04,   0x00, 0x00, 0x00 }
#define _EXMSP_HL	{  EX, MSP, RHL,  RNR,  0x01, 0x05, 0x13,   0x00, 0x00, 0x00 }
#define _EXMSP_IX	{  EX, MSP, RIX,  RNR,  0x01, 0x05, 0x17,   0x00, 0x00, 0x00 }
#define _EXMSP_IY	{  EX, MSP, RIY,  RNR,  0x01, 0x05, 0x17,   0x00, 0x00, 0x00 }

/********************************* Rozkazy  blokowe i I/O **********************************/
/*         		   OC,   D,  R0,   R1,    LC,   CC,   TC,     AM,   OM,   CM */
#define _LDI		{  LDBL, PI, RNR,  RNR,  0x02, 0x04, 0x10, FH+FN, 0x00, F5+FH+F3+FP+FN }
#define _LDIR		{  LDBL,PIR, RNR,  RNR,  0x02, 0x05, 0x15, FH+FN+FP, 0x00, F5+FH+F3+FP+FN }
#define _LDD		{  LDBL, PD, RNR,  RNR,  0x02, 0x04, 0x10, FH+FN, 0x00, F5+FH+F3+FP+FN }
#define _LDDR		{  LDBL,PDR, RNR,  RNR,  0x02, 0x05, 0x15, FH+FN+FP, 0x00, F5+FH+F3+FP+FN }
#define _CPI		{  CPBL, PI, RNR,  RNR,  0x02, 0x04, 0x10,   0x00,   FN, ~FC  }
#define _CPIR		{  CPBL,PIR, RNR,  RNR,  0x02, 0x05, 0x15,   0x00,   FN, ~FC  }
#define _CPD		{  CPBL, PD, RNR,  RNR,  0x02, 0x04, 0x10,   0x00,   FN, ~FC  }
#define _CPDR		{  CPBL,PDR, RNR,  RNR,  0x02, 0x05, 0x15,   0x00,   FN, ~FC  }

#define _INA		{  INA,   RA,  CDB,  RNR,  0x02, 0x03, 0x0b,   0x00, 0x00, 0x00 }
#define _IN_B		{  INBC,  RB,  RNR,  RNR,  0x02, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_C		{  INBC,  RC,  RNR,  RNR,  0x02, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_D		{  INBC,  RD,  RNR,  RNR,  0x02, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_E		{  INBC,  RE,  RNR,  RNR,  0x02, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_H		{  INBC,  RH,  RNR,  RNR,  0x02, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_L		{  INBC,  RL,  RNR,  RNR,  0x02, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_A		{  INBC,  RA,  RNR,  RNR,  0x02, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_F		{  INBC,  RF,  RNR,  RNR,  0x02, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_XH		{  INBC,  RXH, RNR,  RNR,  0x03, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_XL		{  INBC,  RXL, RNR,  RNR,  0x03, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_YH		{  INBC,  RYH, RNR,  RNR,  0x03, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _IN_YL		{  INBC,  RYL, RNR,  RNR,  0x03, 0x03, 0x0c,   FN,   0x00, 0xfe }
#define _INI		{  INBL, PI, RNR,  RNR,  0x02, 0x04, 0x10,   0x00,   FN, 0xff }
#define _INIR		{  INBL,PIR, RNR,  RNR,  0x02, 0x05, 0x15,   0x00,FZ+FN, 0xff }
#define _IND		{  INBL, PD, RNR,  RNR,  0x02, 0x04, 0x10,   0x00,   FN, 0xff }
#define _INDR		{  INBL,PDR, RNR,  RNR,  0x02, 0x05, 0x15,   0x00,FZ+FN, 0xff }

#define _OUTA		{  OUTA,  CDB, RA,  RNR,  0x02, 0x03, 0x0b,  0x00,   0x00, 0x00 }
#define _OUT_B		{  OUTBC, RNR, RB,  RNR,  0x02, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUT_C		{  OUTBC, RNR, RC,  RNR,  0x02, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUT_D		{  OUTBC, RNR, RD,  RNR,  0x02, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUT_E		{  OUTBC, RNR, RE,  RNR,  0x02, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUT_H		{  OUTBC, RNR, RH,  RNR,  0x02, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUT_L		{  OUTBC, RNR, RL,  RNR,  0x02, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUT_A		{  OUTBC, RNR, RA,  RNR,  0x02, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUT_XH		{  OUTBC, RNR, RXH, RNR,  0x03, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUT_XL		{  OUTBC, RNR, RXL, RNR,  0x03, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUT_YH		{  OUTBC, RNR, RYH, RNR,  0x03, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUT_YL		{  OUTBC, RNR, RYL, RNR,  0x03, 0x03, 0x0c,  0x00,   0x00, 0x00 }
#define _OUTI		{  OUTBL, PI, RNR,  RNR,  0x02, 0x04, 0x10,  0x00,   FN, 0xff }
#define _OTIR		{  OUTBL,PIR, RNR,  RNR,  0x02, 0x05, 0x15,  0x00,FZ+FN, 0xff }
#define _OUTD		{  OUTBL, PD, RNR,  RNR,  0x02, 0x04, 0x10,  0x00,   FN, 0xff }
#define _OTDR		{  OUTBL,PDR, RNR,  RNR,  0x02, 0x05, 0x15,  0x00,FZ+FN, 0xff }

/*********************** Rozkazy arytmetyczno-logiczne 8 bit *************************/
/*         		      OC,   D,  R0,   R1,    LC,   CC,   TC,     AM,   OM,   CM */
/* arytmetyczne */
#define _ADDA_R(r)	{ ADD8, RNR, R##r,  ADD,  0x01, 0x01, 0x04,     FN, 0x00, 0xff }
#define _ADDA_R_x(r)	{ ADD8, RNR, R##r,  ADD,  0x02, 0x01, 0x04,     FN, 0x00, 0xff }
#define _ADDA_B		_ADDA_R(B)
#define _ADDA_C		_ADDA_R(C)
#define _ADDA_D		_ADDA_R(D)
#define _ADDA_E		_ADDA_R(E)
#define _ADDA_H		_ADDA_R(H)
#define _ADDA_L		_ADDA_R(L)
#define _ADDA_A		_ADDA_R(A)
#define _ADDA_XH	_ADDA_R_x(XH)
#define _ADDA_XL	_ADDA_R_x(XL)
#define _ADDA_YH	_ADDA_R_x(YH)
#define _ADDA_YL	_ADDA_R_x(YL)
#define _ADDA_N		{ ADDN8, RNR, CDB,  ADD,  0x02, 0x02, 0x07,     FN, 0x00, 0xff }
#define _ADDA_MHL	{ ADDM8, RNR, RHL,  ADD,  0x01, 0x02, 0x07,     FN, 0x00, 0xff }
#define _ADDA_MIX	{ ADDM8, CDB, RIX,  ADD,  0x01, 0x02, 0x07,     FN, 0x00, 0xff }
#define _ADDA_MIY	{ ADDM8, CDB, RIY,  ADD,  0x01, 0x02, 0x07,     FN, 0x00, 0xff }
#define _ADCA_R(r)	{ ADC8, RNR, R##r,  ADC,  0x01, 0x01, 0x04,     FN, 0x00, 0xff }
#define _ADCA_R_x(r)	{ ADC8, RNR, R##r,  ADC,  0x02, 0x01, 0x04,     FN, 0x00, 0xff }
#define _ADCA_B		_ADCA_R(B)
#define _ADCA_C		_ADCA_R(C)
#define _ADCA_D		_ADCA_R(D)
#define _ADCA_E		_ADCA_R(E)
#define _ADCA_H		_ADCA_R(H)
#define _ADCA_L		_ADCA_R(L)
#define _ADCA_A		_ADCA_R(A)
#define _ADCA_XH	_ADCA_R_x(XH)
#define _ADCA_XL	_ADCA_R_x(XL)
#define _ADCA_YH	_ADCA_R_x(YH)
#define _ADCA_YL	_ADCA_R_x(YL)
#define _ADCA_N		{ ADCN8, RNR, CDB,  ADC,  0x02, 0x02, 0x07,     FN, 0x00, 0xff }
#define _ADCA_MHL	{ ADCM8, RNR, RHL,  ADC,  0x01, 0x02, 0x07,     FN, 0x00, 0xff }
#define _ADCA_MIX	{ ADCM8, CDB, RIX,  ADC,  0x01, 0x02, 0x07,     FN, 0x00, 0xff }
#define _ADCA_MIY	{ ADCM8, CDB, RIY,  ADC,  0x01, 0x02, 0x07,     FN, 0x00, 0xff }
#define _SUBA_R(r)	{ SUB8, RNR, R##r,  SUB,  0x01, 0x01, 0x04,     0x00, FN, 0xff }
#define _SUBA_R_x(r)	{ SUB8, RNR, R##r,  SUB,  0x02, 0x01, 0x04,     0x00, FN, 0xff }
#define _SUBA_B		_SUBA_R(B)
#define _SUBA_C		_SUBA_R(C)
#define _SUBA_D		_SUBA_R(D)
#define _SUBA_E		_SUBA_R(E)
#define _SUBA_H		_SUBA_R(H)
#define _SUBA_L		_SUBA_R(L)
#define _SUBA_A		_SUBA_R(A)
#define _SUBA_XH	_SUBA_R_x(XH)
#define _SUBA_XL	_SUBA_R_x(XL)
#define _SUBA_YH	_SUBA_R_x(YH)
#define _SUBA_YL	_SUBA_R_x(YL)
#define _SUBA_N		{ SUBN8, RNR, CDB,  SUB,  0x02, 0x02, 0x07,     0x00, FN, 0xff }
#define _SUBA_MHL	{ SUBM8, RNR, RHL,  SUB,  0x01, 0x02, 0x07,     0x00, FN, 0xff }
#define _SUBA_MIX	{ SUBM8, CDB, RIX,  SUB,  0x01, 0x02, 0x07,     0x00, FN, 0xff }
#define _SUBA_MIY	{ SUBM8, CDB, RIY,  SUB,  0x01, 0x02, 0x07,     0x00, FN, 0xff }
#define _SBCA_R(r)	{ SBC8, RNR, R##r,  SBC,  0x01, 0x01, 0x04,     0x00, FN, 0xff }
#define _SBCA_R_x(r)	{ SBC8, RNR, R##r,  SBC,  0x02, 0x01, 0x04,     0x00, FN, 0xff }
#define _SBCA_B		_SBCA_R(B)
#define _SBCA_C		_SBCA_R(C)
#define _SBCA_D		_SBCA_R(D)
#define _SBCA_E		_SBCA_R(E)
#define _SBCA_H		_SBCA_R(H)
#define _SBCA_L		_SBCA_R(L)
#define _SBCA_A		_SBCA_R(A)
#define _SBCA_XH	_SBCA_R_x(XH)
#define _SBCA_XL	_SBCA_R_x(XL)
#define _SBCA_YH	_SBCA_R_x(YH)
#define _SBCA_YL	_SBCA_R_x(YL)
#define _SBCA_N		{ SBCN8, RNR, CDB,  SBC,  0x02, 0x02, 0x07,     0x00, FN, 0xff }
#define _SBCA_MHL	{ SBCM8, RNR, RHL,  SBC,  0x01, 0x02, 0x07,     0x00, FN, 0xff }
#define _SBCA_MIX	{ SBCM8, CDB, RIX,  SBC,  0x01, 0x02, 0x07,     0x00, FN, 0xff }
#define _SBCA_MIY	{ SBCM8, CDB, RIY,  SBC,  0x01, 0x02, 0x07,     0x00, FN, 0xff }
#define _CPA_R(r)	{ CP8, RNR, R##r,  CP,  0x01, 0x01, 0x04,     0x00, FN, 0xff }
#define _CPA_R_x(r)	{ CP8, RNR, R##r,  CP,  0x02, 0x01, 0x04,     0x00, FN, 0xff }
#define _CPA_B		_CPA_R(B)
#define _CPA_C		_CPA_R(C)
#define _CPA_D		_CPA_R(D)
#define _CPA_E		_CPA_R(E)
#define _CPA_H		_CPA_R(H)
#define _CPA_L		_CPA_R(L)
#define _CPA_A		_CPA_R(A)
#define _CPA_XH		_CPA_R_x(XH)
#define _CPA_XL		_CPA_R_x(XL)
#define _CPA_YH		_CPA_R_x(YH)
#define _CPA_YL		_CPA_R_x(YL)
#define _CPA_N		{ CPN8, RNR, CDB,  CP,  0x02, 0x02, 0x07,     0x00, FN, 0xff }
#define _CPA_MHL	{ CPM8, RNR, RHL,  CP,  0x01, 0x02, 0x07,     0x00, FN, 0xff }
#define _CPA_MIX	{ CPM8, CDB, RIX,  CP,  0x01, 0x02, 0x07,     0x00, FN, 0xff }
#define _CPA_MIY	{ CPM8, CDB, RIY,  CP,  0x01, 0x02, 0x07,     0x00, FN, 0xff }
/* logiczne */
#define _ANDA_R(r)	{ AND, RNR, R##r,  RNR,  0x01, 0x01, 0x04,     FC+FN, FH, 0xff }
#define _ANDA_R_x(r)	{ AND, RNR, R##r,  RNR,  0x02, 0x01, 0x04,     FC+FN, FH, 0xff }
#define _ANDA_B		_ANDA_R(B)
#define _ANDA_C		_ANDA_R(C)
#define _ANDA_D		_ANDA_R(D)
#define _ANDA_E		_ANDA_R(E)
#define _ANDA_H		_ANDA_R(H)
#define _ANDA_L		_ANDA_R(L)
#define _ANDA_A		_ANDA_R(A)
#define _ANDA_XH	_ANDA_R_x(XH)
#define _ANDA_XL	_ANDA_R_x(XL)
#define _ANDA_YH	_ANDA_R_x(YH)
#define _ANDA_YL	_ANDA_R_x(YL)
#define _ANDA_N		{ ANDN, RNR, CDB, RNR,  0x02, 0x02, 0x07,     FC+FN, FH, 0xff }
#define _ANDA_MHL	{ ANDM, RNR, RHL, RNR,  0x01, 0x02, 0x07,     FC+FN, FH, 0xff }
#define _ANDA_MIX	{ ANDM, CDB, RIX, RNR,  0x01, 0x02, 0x07,     FC+FN, FH, 0xff }
#define _ANDA_MIY	{ ANDM, CDB, RIY, RNR,  0x01, 0x02, 0x07,     FC+FN, FH, 0xff }
#define _ORA_R(r)	{ OR, RNR, R##r,  RNR,  0x01, 0x01, 0x04,      FC+FN+FH, 0x00, 0xff }
#define _ORA_R_x(r)	{ OR, RNR, R##r,  RNR,  0x02, 0x01, 0x04,      FC+FN+FH, 0x00, 0xff }
#define _ORA_B		_ORA_R(B)
#define _ORA_C		_ORA_R(C)
#define _ORA_D		_ORA_R(D)
#define _ORA_E		_ORA_R(E)
#define _ORA_H		_ORA_R(H)
#define _ORA_L		_ORA_R(L)
#define _ORA_A		_ORA_R(A)
#define _ORA_XH		_ORA_R_x(XH)
#define _ORA_XL		_ORA_R_x(XL)
#define _ORA_YH		_ORA_R_x(YH)
#define _ORA_YL		_ORA_R_x(YL)
#define _ORA_N		{ ORN, RNR, CDB,   RNR,  0x02, 0x02, 0x07,     FC+FN+FH, 0x00, 0xff }
#define _ORA_MHL	{ ORM, RNR, RHL,   RNR,  0x01, 0x02, 0x07,     FC+FN+FH, 0x00, 0xff }
#define _ORA_MIX	{ ORM, CDB, RIX,   RNR,  0x01, 0x02, 0x07,     FC+FN+FH, 0x00, 0xff }
#define _ORA_MIY	{ ORM, CDB, RIY,   RNR,  0x01, 0x02, 0x07,     FC+FN+FH, 0x00, 0xff }
#define _XORA_R(r)	{ XOR, RNR, R##r,  RNR,  0x01, 0x01, 0x04,     FC+FN+FH, 0x00, 0xff }
#define _XORA_R_x(r)	{ XOR, RNR, R##r,  RNR,  0x02, 0x01, 0x04,     FC+FN+FH, 0x00, 0xff }
#define _XORA_B		_XORA_R(B)
#define _XORA_C		_XORA_R(C)
#define _XORA_D		_XORA_R(D)
#define _XORA_E		_XORA_R(E)
#define _XORA_H		_XORA_R(H)
#define _XORA_L		_XORA_R(L)
#define _XORA_A		_XORA_R(A)
#define _XORA_XH	_XORA_R_x(XH)
#define _XORA_XL	_XORA_R_x(XL)
#define _XORA_YH	_XORA_R_x(YH)
#define _XORA_YL	_XORA_R_x(YL)
#define _XORA_N		{ XORN, RNR, CDB,  RNR,  0x02, 0x02, 0x07,     FC+FN+FH, 0x00, 0xff }
#define _XORA_MHL	{ XORM, RNR, RHL,  RNR,  0x01, 0x02, 0x07,     FC+FN+FH, 0x00, 0xff }
#define _XORA_MIX	{ XORM, CDB, RIX,  RNR,  0x01, 0x02, 0x07,     FC+FN+FH, 0x00, 0xff }
#define _XORA_MIY	{ XORM, CDB, RIY,  RNR,  0x01, 0x02, 0x07,     FC+FN+FH, 0x00, 0xff }
#define _INCR(r)	{ INC8, RNR, R##r, INC,  0x01, 0x01, 0x04,     FN, 0x00, ~FC }
#define _INCR_x(r)	{ INC8, RNR, R##r, INC,  0x02, 0x01, 0x04,     FN, 0x00, ~FC }
#define _INCB		_INCR(B)
#define _INCC		_INCR(C)
#define _INCD		_INCR(D)
#define _INCE		_INCR(E)
#define _INCH		_INCR(H)
#define _INCL		_INCR(L)
#define _INCA		_INCR(A)
#define _INCXH		_INCR_x(XH)
#define _INCXL		_INCR_x(XL)
#define _INCYH		_INCR_x(YH)
#define _INCYL		_INCR_x(YL)
#define _INCMHL		{ INCM8, RNR, RHL,  INC,  0x01, 0x03, 0x0b,     FN, 0x00, ~FC }
#define _INCMIX		{ INCM8, CDB, RIX,  INC,  0x03, 0x06, 0x17,     FN, 0x00, ~FC }
#define _INCMIY		{ INCM8, CDB, RIY,  INC,  0x03, 0x06, 0x17,     FN, 0x00, ~FC }

#define _DECR(r)	{ DEC8, RNR, R##r, DEC,  0x01, 0x01, 0x04,     0x00, FN, ~FC }
#define _DECR_x(r)	{ DEC8, RNR, R##r, DEC,  0x02, 0x01, 0x04,     0x00, FN, ~FC }
#define _DECB		_DECR(B)
#define _DECC		_DECR(C)
#define _DECD		_DECR(D)
#define _DECE		_DECR(E)
#define _DECH		_DECR(H)
#define _DECL		_DECR(L)
#define _DECA		_DECR(A)
#define _DECXH		_DECR_x(XH)
#define _DECXL		_DECR_x(XL)
#define _DECYH		_DECR_x(YH)
#define _DECYL		_DECR_x(YL)
#define _DECMHL		{ DECM8, RNR, RHL,  DEC,  0x01, 0x03, 0x0b,     0x00, FN, ~FC }
#define _DECMIX		{ DECM8, CDB, RIX,  DEC,  0x03, 0x06, 0x17,     0x00, FN, ~FC }
#define _DECMIY		{ DECM8, CDB, RIY,  DEC,  0x03, 0x06, 0x17,     0x00, FN, ~FC }

/*********************** Pomocnicze Rozkazy arytmetyczne 8 bit *************************/
/*         		   OC,   D,  R0,   R1,    LC,   CC,   TC,     AM,   OM,   CM   */
#define _DAA		{ DAA, RNR, RNR,  RNR,  0x01, 0x01, 0x04,     0x00, 0x00, ~FN }
#define _CPL		{ CPL, RNR, RNR,  RNR,  0x01, 0x01, 0x04,     0x00, FH+FN, F5+FH+F3+FN }
#define _NEG		{ NEG, RNR, RNR,  RNR,  0x02, 0x02, 0x08,     0x00, FN, 0xff }
#define _CCF		{ CCF, RNR, RNR,  RNR,  0x01, 0x01, 0x04,     FN, FN, F5+FH+F3+FN+FC }
#define _SCF		{ SCF, RNR, RNR,  RNR,  0x01, 0x01, 0x04,     FN+FH, FC, F5+FH+F3+FN+FC }
#define _HALT		{HALT, RNR, RNR,  RNR,  0x01, 0x01, 0x04,     0x00, 0x00, F5+F3 }
#define _LDMHL_MHL	_HALT
#define _DI		{  DI, RNR, RNR,  RNR,  0x01, 0x01, 0x04,     0x00, 0x00, F5+F3 }
#define _EI		{  EI, RNR, RNR,  RNR,  0x01, 0x01, 0x04,     0x00, 0x00, F5+F3 }
#define _IM0		{  IM,   0, RNR,  RNR,  0x02, 0x02, 0x08,     0x00, 0x00, F5+F3 }
#define _IM1		{  IM,   1, RNR,  RNR,  0x02, 0x02, 0x08,     0x00, 0x00, F5+F3 }
#define _IM2		{  IM,   2, RNR,  RNR,  0x02, 0x02, 0x08,     0x00, 0x00, F5+F3 }

/*********************** Rozkazy arytmetyki 16 bitowej **********************************/
/*         		   OC,   D,  R0,   R1,    LC,   CC,   TC,     AM,   OM,   CM    */
#define _ADDHL_R(r)	{ ADDX,RHL,R##r,  ADD,  0x01, 0x03, 0x0b,     FN, 0x00, F5+FH+F3+FN+FC }
#define _ADDHL_BC	_ADDHL_R(BC)
#define _ADDHL_DE	_ADDHL_R(DE)
#define _ADDHL_HL	_ADDHL_R(HL)
#define _ADDHL_SP	_ADDHL_R(SP)
#define _ADCHL_R(r)	{ ADCX,RHL,R##r,  ADC,  0x02, 0x04, 0x0f,     FN, 0x00, 0xff }
#define _ADCHL_BC	_ADCHL_R(BC)
#define _ADCHL_DE	_ADCHL_R(DE)
#define _ADCHL_HL	_ADCHL_R(HL)
#define _ADCHL_SP	_ADCHL_R(SP)
#define _SBCHL_R(r)	{ SBCX,RHL,R##r,  SBC,  0x02, 0x04, 0x0f,     0x00, FN, 0xff }
#define _SBCHL_BC	_SBCHL_R(BC)
#define _SBCHL_DE	_SBCHL_R(DE)
#define _SBCHL_HL	_SBCHL_R(HL)
#define _SBCHL_SP	_SBCHL_R(SP)
#define _ADDIX_R(r)	{ ADDX,RIX,R##r,  ADD,  0x02, 0x04, 0x0f,     FN, 0x00, F5+FH+F3+FN+FC }
#define _ADDIX_BC	_ADDIX_R(BC)
#define _ADDIX_DE	_ADDIX_R(DE)
#define _ADDIX_IX	_ADDIX_R(IX)
#define _ADDIX_SP	_ADDIX_R(SP)
#define _ADDIY_R(r)	{ ADDX,RIY,R##r,  ADD,  0x02, 0x04, 0x0f,     FN, 0x00, F5+FH+F3+FN+FC }
#define _ADDIY_BC	_ADDIY_R(BC)
#define _ADDIY_DE	_ADDIY_R(DE)
#define _ADDIY_IY	_ADDIY_R(IY)
#define _ADDIY_SP	_ADDIY_R(SP)
#define _ADCIX_R(r)	{ ADCX,RIX,R##r,  ADC,  0x02, 0x04, 0x0f,     FN, 0x00, F5+FH+F3+FN+FC }
#define _ADCIX_BC	_ADCIX_R(BC)
#define _ADCIX_DE	_ADCIX_R(DE)
#define _ADCIX_IX	_ADCIX_R(IX)
#define _ADCIX_SP	_ADCIX_R(SP)
#define _ADCIY_R(r)	{ ADCX,RIY,R##r,  ADC,  0x02, 0x04, 0x0f,     FN, 0x00, F5+FH+F3+FN+FC }
#define _ADCIY_BC	_ADCIY_R(BC)
#define _ADCIY_DE	_ADCIY_R(DE)
#define _ADCIY_IY	_ADCIY_R(IY)
#define _ADCIY_SP	_ADCIY_R(SP)
#define _SBCIX_R(r)	{ SBCX,RIX,R##r,  SBC,  0x02, 0x04, 0x0f,     FN, 0x00, F5+FH+F3+FN+FC }
#define _SBCIX_BC	_SBCIX_R(BC)
#define _SBCIX_DE	_SBCIX_R(DE)
#define _SBCIX_IX	_SBCIX_R(IX)
#define _SBCIX_SP	_SBCIX_R(SP)
#define _SBCIY_R(r)	{ SBCX,RIY,R##r,  SBC,  0x02, 0x04, 0x0f,     FN, 0x00, F5+FH+F3+FN+FC }
#define _SBCIY_BC	_SBCIY_R(BC)
#define _SBCIY_DE	_SBCIY_R(DE)
#define _SBCIY_IY	_SBCIY_R(IY)
#define _SBCIY_SP	_SBCIY_R(SP)
#define _INCBC		{ INCX, RBC, RNR,  INC,  0x01, 0x01, 0x06,     0x00, 0x00, 0x00 }
#define _INCDE		{ INCX, RDE, RNR,  INC,  0x01, 0x01, 0x06,     0x00, 0x00, 0x00 }
#define _INCHL		{ INCX, RHL, RNR,  INC,  0x01, 0x01, 0x06,     0x00, 0x00, 0x00 }
#define _INCSP		{ INCX, RSP, RNR,  INC,  0x01, 0x01, 0x06,     0x00, 0x00, 0x00 }
#define _INCIX		{ INCX, RIX, RNR,  INC,  0x02, 0x02, 0x0a,     0x00, 0x00, 0x00 }
#define _INCIY		{ INCX, RIY, RNR,  INC,  0x02, 0x02, 0x0a,     0x00, 0x00, 0x00 }
#define _DECBC		{ DECX, RBC, RNR,  DEC,  0x01, 0x01, 0x06,     0x00, 0x00, 0x00 }
#define _DECDE		{ DECX, RDE, RNR,  DEC,  0x01, 0x01, 0x06,     0x00, 0x00, 0x00 }
#define _DECHL		{ DECX, RHL, RNR,  DEC,  0x01, 0x01, 0x06,     0x00, 0x00, 0x00 }
#define _DECSP		{ DECX, RSP, RNR,  DEC,  0x01, 0x01, 0x06,     0x00, 0x00, 0x00 }
#define _DECIX		{ DECX, RIX, RNR,  DEC,  0x02, 0x02, 0x0a,     0x00, 0x00, 0x00 }
#define _DECIY		{ DECX, RIY, RNR,  DEC,  0x02, 0x02, 0x0a,     0x00, 0x00, 0x00 }

/***********************   Rozkazy działań na bitach   **********************************/
/*         		    OC,   D,  R0,   R1,    LC,   CC,   TC,     AM,   OM,   CM    */
#define _BITR(bit,r)	{ BIT,1 << bit,R##r,  bit,  0x02, 0x02, 0x08,     FN, FH, FH+FN+FZ+FP }
#define _BITR_x(bit,r)	{ BIT,1 << bit,R##r,  bit,  0x03, 0x03, 0x0b,     FN, FH, FH+FN+FZ+FP }
#define _BITR_y(bit,r)	{ BIT,1 << bit,R##r,  bit,  0x03, 0x03, 0x0b,     FN, FH, FH+FN+FZ+FP }
#define _BITR_m(bit,r)	{ BITM,1 << bit,R##r,  bit,  0x02, 0x02, 0x08,    FN, FH, FH+FN+FZ+FP }
#define _BITR_mx(bit,r)	{ BITM,1 << bit,R##r,  bit,  0x03, 0x03, 0x0b,    FN, FH, FH+FN+FZ+FP }
#define _BITR_my(bit,r)	{ BITM,1 << bit,R##r,  bit,  0x03, 0x03, 0x0b,    FN, FH, FH+FN+FZ+FP }

#define _BIT0_A		_BITR(0,A)
#define _BIT1_A		_BITR(1,A)
#define _BIT2_A		_BITR(2,A)
#define _BIT3_A		_BITR(3,A)
#define _BIT4_A		_BITR(4,A)
#define _BIT5_A		_BITR(5,A)
#define _BIT6_A		_BITR(6,A)
#define _BIT7_A		_BITR(7,A)
#define _BIT0_B		_BITR(0,B)
#define _BIT1_B		_BITR(1,B)
#define _BIT2_B		_BITR(2,B)
#define _BIT3_B		_BITR(3,B)
#define _BIT4_B		_BITR(4,B)
#define _BIT5_B		_BITR(5,B)
#define _BIT6_B		_BITR(6,B)
#define _BIT7_B		_BITR(7,B)
#define _BIT0_C		_BITR(0,C)
#define _BIT1_C		_BITR(1,C)
#define _BIT2_C		_BITR(2,C)
#define _BIT3_C		_BITR(3,C)
#define _BIT4_C		_BITR(4,C)
#define _BIT5_C		_BITR(5,C)
#define _BIT6_C		_BITR(6,C)
#define _BIT7_C		_BITR(7,C)
#define _BIT0_D		_BITR(0,D)
#define _BIT1_D		_BITR(1,D)
#define _BIT2_D		_BITR(2,D)
#define _BIT3_D		_BITR(3,D)
#define _BIT4_D		_BITR(4,D)
#define _BIT5_D		_BITR(5,D)
#define _BIT6_D		_BITR(6,D)
#define _BIT7_D		_BITR(7,D)
#define _BIT0_E		_BITR(0,E)
#define _BIT1_E		_BITR(1,E)
#define _BIT2_E		_BITR(2,E)
#define _BIT3_E		_BITR(3,E)
#define _BIT4_E		_BITR(4,E)
#define _BIT5_E		_BITR(5,E)
#define _BIT6_E		_BITR(6,E)
#define _BIT7_E		_BITR(7,E)
#define _BIT0_H		_BITR(0,H)
#define _BIT1_H		_BITR(1,H)
#define _BIT2_H		_BITR(2,H)
#define _BIT3_H		_BITR(3,H)
#define _BIT4_H		_BITR(4,H)
#define _BIT5_H		_BITR(5,H)
#define _BIT6_H		_BITR(6,H)
#define _BIT7_H		_BITR(7,H)
#define _BIT0_L		_BITR(0,L)
#define _BIT1_L		_BITR(1,L)
#define _BIT2_L		_BITR(2,L)
#define _BIT3_L		_BITR(3,L)
#define _BIT4_L		_BITR(4,L)
#define _BIT5_L		_BITR(5,L)
#define _BIT6_L		_BITR(6,L)
#define _BIT7_L		_BITR(7,L)
#define _BIT0_XH	_BITR_x(0,XH)
#define _BIT1_XH	_BITR_x(1,XH)
#define _BIT2_XH	_BITR_x(2,XH)
#define _BIT3_XH	_BITR_x(3,XH)
#define _BIT4_XH	_BITR_x(4,XH)
#define _BIT5_XH	_BITR_x(5,XH)
#define _BIT6_XH	_BITR_x(6,XH)
#define _BIT7_XH	_BITR_x(7,XH)
#define _BIT0_XL	_BITR_x(0,XL)
#define _BIT1_XL	_BITR_x(1,XL)
#define _BIT2_XL	_BITR_x(2,XL)
#define _BIT3_XL	_BITR_x(3,XL)
#define _BIT4_XL	_BITR_x(4,XL)
#define _BIT5_XL	_BITR_x(5,XL)
#define _BIT6_XL	_BITR_x(6,XL)
#define _BIT7_XL	_BITR_x(7,XL)
#define _BIT0_YH	_BITR_x(0,YH)
#define _BIT1_YH	_BITR_y(1,YH)
#define _BIT2_YH	_BITR_y(2,YH)
#define _BIT3_YH	_BITR_y(3,YH)
#define _BIT4_YH	_BITR_y(4,YH)
#define _BIT5_YH	_BITR_y(5,YH)
#define _BIT6_YH	_BITR_y(6,YH)
#define _BIT7_YH	_BITR_y(7,YH)
#define _BIT0_YL	_BITR_y(0,YL)
#define _BIT1_YL	_BITR_y(1,YL)
#define _BIT2_YL	_BITR_y(2,YL)
#define _BIT3_YL	_BITR_y(3,YL)
#define _BIT4_YL	_BITR_y(4,YL)
#define _BIT5_YL	_BITR_y(5,YL)
#define _BIT6_YL	_BITR_y(6,YL)
#define _BIT7_YL	_BITR_y(7,YL)
#define _BIT0_MHL	_BITR_m(0,HL)
#define _BIT1_MHL	_BITR_m(1,HL)
#define _BIT2_MHL	_BITR_m(2,HL)
#define _BIT3_MHL	_BITR_m(3,HL)
#define _BIT4_MHL	_BITR_m(4,HL)
#define _BIT5_MHL	_BITR_m(5,HL)
#define _BIT6_MHL	_BITR_m(6,HL)
#define _BIT7_MHL	_BITR_m(7,HL)
#define _BIT0_MIX	_BITR_mx(0,IX)
#define _BIT1_MIX	_BITR_mx(1,IX)
#define _BIT2_MIX	_BITR_mx(2,IX)
#define _BIT3_MIX	_BITR_mx(3,IX)
#define _BIT4_MIX	_BITR_mx(4,IX)
#define _BIT5_MIX	_BITR_mx(5,IX)
#define _BIT6_MIX	_BITR_mx(6,IX)
#define _BIT7_MIX	_BITR_mx(7,IX)
#define _BIT0_MIY	_BITR_my(0,IY)
#define _BIT1_MIY	_BITR_my(1,IY)
#define _BIT2_MIY	_BITR_my(2,IY)
#define _BIT3_MIY	_BITR_my(3,IY)
#define _BIT4_MIY	_BITR_my(4,IY)
#define _BIT5_MIY	_BITR_my(5,IY)
#define _BIT6_MIY	_BITR_my(6,IY)
#define _BIT7_MIY	_BITR_my(7,IY)

#define _SETR(bit,r)	{ SET,1 << bit,R##r,  bit,  0x02, 0x02, 0x08,     0x00, 0x00, 0x00 }
#define _SETR_x(bit,r)	{ SET,1 << bit,R##r,  bit,  0x03, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _SETR_y(bit,r)	{ SET,1 << bit,R##r,  bit,  0x03, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _SETR_m(bit,r)	{ SETM,1 << bit,R##r, bit,  0x02, 0x02, 0x08,     0x00, 0x00, 0x00 }
#define _SETR_mx(bit,r)	{ SETM,1 << bit,R##r, bit,  0x03, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _SETR_my(bit,r)	{ SETM,1 << bit,R##r, bit,  0x03, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _SET0_A		_SETR(0,A)
#define _SET1_A		_SETR(1,A)
#define _SET2_A		_SETR(2,A)
#define _SET3_A		_SETR(3,A)
#define _SET4_A		_SETR(4,A)
#define _SET5_A		_SETR(5,A)
#define _SET6_A		_SETR(6,A)
#define _SET7_A		_SETR(7,A)
#define _SET0_B		_SETR(0,B)
#define _SET1_B		_SETR(1,B)
#define _SET2_B		_SETR(2,B)
#define _SET3_B		_SETR(3,B)
#define _SET4_B		_SETR(4,B)
#define _SET5_B		_SETR(5,B)
#define _SET6_B		_SETR(6,B)
#define _SET7_B		_SETR(7,B)
#define _SET0_C		_SETR(0,C)
#define _SET1_C		_SETR(1,C)
#define _SET2_C		_SETR(2,C)
#define _SET3_C		_SETR(3,C)
#define _SET4_C		_SETR(4,C)
#define _SET5_C		_SETR(5,C)
#define _SET6_C		_SETR(6,C)
#define _SET7_C		_SETR(7,C)
#define _SET0_D		_SETR(0,D)
#define _SET1_D		_SETR(1,D)
#define _SET2_D		_SETR(2,D)
#define _SET3_D		_SETR(3,D)
#define _SET4_D		_SETR(4,D)
#define _SET5_D		_SETR(5,D)
#define _SET6_D		_SETR(6,D)
#define _SET7_D		_SETR(7,D)
#define _SET0_E		_SETR(0,E)
#define _SET1_E		_SETR(1,E)
#define _SET2_E		_SETR(2,E)
#define _SET3_E		_SETR(3,E)
#define _SET4_E		_SETR(4,E)
#define _SET5_E		_SETR(5,E)
#define _SET6_E		_SETR(6,E)
#define _SET7_E		_SETR(7,E)
#define _SET0_H		_SETR(0,H)
#define _SET1_H		_SETR(1,H)
#define _SET2_H		_SETR(2,H)
#define _SET3_H		_SETR(3,H)
#define _SET4_H		_SETR(4,H)
#define _SET5_H		_SETR(5,H)
#define _SET6_H		_SETR(6,H)
#define _SET7_H		_SETR(7,H)
#define _SET0_L		_SETR(0,L)
#define _SET1_L		_SETR(1,L)
#define _SET2_L		_SETR(2,L)
#define _SET3_L		_SETR(3,L)
#define _SET4_L		_SETR(4,L)
#define _SET5_L		_SETR(5,L)
#define _SET6_L		_SETR(6,L)
#define _SET7_L		_SETR(7,L)
#define _SET0_XH	_SETR_x(0,XH)
#define _SET1_XH	_SETR_x(1,XH)
#define _SET2_XH	_SETR_x(2,XH)
#define _SET3_XH	_SETR_x(3,XH)
#define _SET4_XH	_SETR_x(4,XH)
#define _SET5_XH	_SETR_x(5,XH)
#define _SET6_XH	_SETR_x(6,XH)
#define _SET7_XH	_SETR_x(7,XH)
#define _SET0_XL	_SETR_x(0,XL)
#define _SET1_XL	_SETR_x(1,XL)
#define _SET2_XL	_SETR_x(2,XL)
#define _SET3_XL	_SETR_x(3,XL)
#define _SET4_XL	_SETR_x(4,XL)
#define _SET5_XL	_SETR_x(5,XL)
#define _SET6_XL	_SETR_x(6,XL)
#define _SET7_XL	_SETR_x(7,XL)
#define _SET0_YH	_SETR_y(0,YH)
#define _SET1_YH	_SETR_y(1,YH)
#define _SET2_YH	_SETR_y(2,YH)
#define _SET3_YH	_SETR_y(3,YH)
#define _SET4_YH	_SETR_y(4,YH)
#define _SET5_YH	_SETR_y(5,YH)
#define _SET6_YH	_SETR_y(6,YH)
#define _SET7_YH	_SETR_y(7,YH)
#define _SET0_YL	_SETR_y(0,YL)
#define _SET1_YL	_SETR_y(1,YL)
#define _SET2_YL	_SETR_y(2,YL)
#define _SET3_YL	_SETR_y(3,YL)
#define _SET4_YL	_SETR_y(4,YL)
#define _SET5_YL	_SETR_y(5,YL)
#define _SET6_YL	_SETR_y(6,YL)
#define _SET7_YL	_SETR_y(7,YL)
#define _SET0_MHL	_SETR_m(0,HL)
#define _SET1_MHL	_SETR_m(1,HL)
#define _SET2_MHL	_SETR_m(2,HL)
#define _SET3_MHL	_SETR_m(3,HL)
#define _SET4_MHL	_SETR_m(4,HL)
#define _SET5_MHL	_SETR_m(5,HL)
#define _SET6_MHL	_SETR_m(6,HL)
#define _SET7_MHL	_SETR_m(7,HL)
#define _SET0_MIX	_SETR_mx(0,IX)
#define _SET1_MIX	_SETR_mx(1,IX)
#define _SET2_MIX	_SETR_mx(2,IX)
#define _SET3_MIX	_SETR_mx(3,IX)
#define _SET4_MIX	_SETR_mx(4,IX)
#define _SET5_MIX	_SETR_mx(5,IX)
#define _SET6_MIX	_SETR_mx(6,IX)
#define _SET7_MIX	_SETR_mx(7,IX)
#define _SET0_MIY	_SETR_my(0,IY)
#define _SET1_MIY	_SETR_my(1,IY)
#define _SET2_MIY	_SETR_my(2,IY)
#define _SET3_MIY	_SETR_my(3,IY)
#define _SET4_MIY	_SETR_my(4,IY)
#define _SET5_MIY	_SETR_my(5,IY)
#define _SET6_MIY	_SETR_my(6,IY)
#define _SET7_MIY	_SETR_my(7,IY)

#define _RESR(bit,r)	{ RES,(uchar)~(1 << bit),R##r,  bit,  0x02, 0x02, 0x08,     0x00, 0x00, 0x00 }
#define _RESR_x(bit,r)	{ RES,(uchar)~(1 << bit),R##r,  bit,  0x03, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RESR_y(bit,r)	{ RES,(uchar)~(1 << bit),R##r,  bit,  0x03, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RESR_m(bit,r)	{ RESM,(uchar)~(1 << bit),R##r, bit,  0x02, 0x02, 0x08,     0x00, 0x00, 0x00 }
#define _RESR_mx(bit,r)	{ RESM,(uchar)~(1 << bit),R##r, bit,  0x03, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RESR_my(bit,r) { RESM,(uchar)~(1 << bit),R##r, bit,  0x03, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RES0_A		_RESR(0,A)
#define _RES1_A		_RESR(1,A)
#define _RES2_A		_RESR(2,A)
#define _RES3_A		_RESR(3,A)
#define _RES4_A		_RESR(4,A)
#define _RES5_A		_RESR(5,A)
#define _RES6_A		_RESR(6,A)
#define _RES7_A		_RESR(7,A)
#define _RES0_B		_RESR(0,B)
#define _RES1_B		_RESR(1,B)
#define _RES2_B		_RESR(2,B)
#define _RES3_B		_RESR(3,B)
#define _RES4_B		_RESR(4,B)
#define _RES5_B		_RESR(5,B)
#define _RES6_B		_RESR(6,B)
#define _RES7_B		_RESR(7,B)
#define _RES0_C		_RESR(0,C)
#define _RES1_C		_RESR(1,C)
#define _RES2_C		_RESR(2,C)
#define _RES3_C		_RESR(3,C)
#define _RES4_C		_RESR(4,C)
#define _RES5_C		_RESR(5,C)
#define _RES6_C		_RESR(6,C)
#define _RES7_C		_RESR(7,C)
#define _RES0_D		_RESR(0,D)
#define _RES1_D		_RESR(1,D)
#define _RES2_D		_RESR(2,D)
#define _RES3_D		_RESR(3,D)
#define _RES4_D		_RESR(4,D)
#define _RES5_D		_RESR(5,D)
#define _RES6_D		_RESR(6,D)
#define _RES7_D		_RESR(7,D)
#define _RES0_E		_RESR(0,E)
#define _RES1_E		_RESR(1,E)
#define _RES2_E		_RESR(2,E)
#define _RES3_E		_RESR(3,E)
#define _RES4_E		_RESR(4,E)
#define _RES5_E		_RESR(5,E)
#define _RES6_E		_RESR(6,E)
#define _RES7_E		_RESR(7,E)
#define _RES0_H		_RESR(0,H)
#define _RES1_H		_RESR(1,H)
#define _RES2_H		_RESR(2,H)
#define _RES3_H		_RESR(3,H)
#define _RES4_H		_RESR(4,H)
#define _RES5_H		_RESR(5,H)
#define _RES6_H		_RESR(6,H)
#define _RES7_H		_RESR(7,H)
#define _RES0_L		_RESR(0,L)
#define _RES1_L		_RESR(1,L)
#define _RES2_L		_RESR(2,L)
#define _RES3_L		_RESR(3,L)
#define _RES4_L		_RESR(4,L)
#define _RES5_L		_RESR(5,L)
#define _RES6_L		_RESR(6,L)
#define _RES7_L		_RESR(7,L)
#define _RES0_XH	_RESR_x(0,XH)
#define _RES1_XH	_RESR_x(1,XH)
#define _RES2_XH	_RESR_x(2,XH)
#define _RES3_XH	_RESR_x(3,XH)
#define _RES4_XH	_RESR_x(4,XH)
#define _RES5_XH	_RESR_x(5,XH)
#define _RES6_XH	_RESR_x(6,XH)
#define _RES7_XH	_RESR_x(7,XH)
#define _RES0_XL	_RESR_x(0,XL)
#define _RES1_XL	_RESR_x(1,XL)
#define _RES2_XL	_RESR_x(2,XL)
#define _RES3_XL	_RESR_x(3,XL)
#define _RES4_XL	_RESR_x(4,XL)
#define _RES5_XL	_RESR_x(5,XL)
#define _RES6_XL	_RESR_x(6,XL)
#define _RES7_XL	_RESR_x(7,XL)
#define _RES0_YH	_RESR_y(0,YH)
#define _RES1_YH	_RESR_y(1,YH)
#define _RES2_YH	_RESR_y(2,YH)
#define _RES3_YH	_RESR_y(3,YH)
#define _RES4_YH	_RESR_y(4,YH)
#define _RES5_YH	_RESR_y(5,YH)
#define _RES6_YH	_RESR_y(6,YH)
#define _RES7_YH	_RESR_y(7,YH)
#define _RES0_YL	_RESR_y(0,YL)
#define _RES1_YL	_RESR_y(1,YL)
#define _RES2_YL	_RESR_y(2,YL)
#define _RES3_YL	_RESR_y(3,YL)
#define _RES4_YL	_RESR_y(4,YL)
#define _RES5_YL	_RESR_y(5,YL)
#define _RES6_YL	_RESR_y(6,YL)
#define _RES7_YL	_RESR_y(7,YL)
#define _RES0_MHL	_RESR_m(0,HL)
#define _RES1_MHL	_RESR_m(1,HL)
#define _RES2_MHL	_RESR_m(2,HL)
#define _RES3_MHL	_RESR_m(3,HL)
#define _RES4_MHL	_RESR_m(4,HL)
#define _RES5_MHL	_RESR_m(5,HL)
#define _RES6_MHL	_RESR_m(6,HL)
#define _RES7_MHL	_RESR_m(7,HL)
#define _RES0_MIX	_RESR_mx(0,IX)
#define _RES1_MIX	_RESR_mx(1,IX)
#define _RES2_MIX	_RESR_mx(2,IX)
#define _RES3_MIX	_RESR_mx(3,IX)
#define _RES4_MIX	_RESR_mx(4,IX)
#define _RES5_MIX	_RESR_mx(5,IX)
#define _RES6_MIX	_RESR_mx(6,IX)
#define _RES7_MIX	_RESR_mx(7,IX)
#define _RES0_MIY	_RESR_my(0,IY)
#define _RES1_MIY	_RESR_my(1,IY)
#define _RES2_MIY	_RESR_my(2,IY)
#define _RES3_MIY	_RESR_my(3,IY)
#define _RES4_MIY	_RESR_my(4,IY)
#define _RES5_MIY	_RESR_my(5,IY)
#define _RES6_MIY	_RESR_my(6,IY)
#define _RES7_MIY	_RESR_my(7,IY)

/************************ Rozkazy arytmetyczno-logiczne 8 bit *************************/
/*                  OC,   D,  R0,   R1,    LC,   CC,   TC,     AM,   OM,   CM */
#define _RLCA	{ RLC,  RA,  0,  RNR,  0x01, 0x01, 0x04,     FN+FH, 0x00, FN+FH+FC+F5+F3 }
#define _RLA	{ RLS,  RA,  0,  RNR,  0x01, 0x01, 0x04,     FN+FH, 0x00, FN+FH+FC+F5+F3 }
#define _RRCA	{ RRC,  RA,  0,  RNR,  0x01, 0x01, 0x04,     FN+FH, 0x00, FN+FH+FC+F5+F3 }
#define _RRA	{ RR,   RA,  0,  RNR,  0x01, 0x01, 0x04,     FN+FH, 0x00, FN+FH+FC+F5+F3 }

#define _RLC_(r)  { RLC,  R##r,  R##r,  RNR,  0x02, 0x02, 0x08,     FN+FH, 0x00, 0xff }
#define _RLC_x(r) { RLC,  R##r,  R##r,  RNR,  0x03, 0x03, 0x0c,     FN+FH, 0x00, 0xff }
#define _RLC_B	_RLC_(B)
#define _RLC_C	_RLC_(C)
#define _RLC_D	_RLC_(D)
#define _RLC_E	_RLC_(E)
#define _RLC_H	_RLC_(H)
#define _RLC_L	_RLC_(L)
#define _RLC_A	_RLC_(A)
#define _RLC_XH	_RLC_(XH)
#define _RLC_XL	_RLC_(XL)
#define _RLC_YH	_RLC_(YH)
#define _RLC_YL	_RLC_(YL)
#define _RLC_MHL { RLCM,  RHL,  MHL,  RNR,  0x02, 0x04, 0x0f,     FN+FH, 0x00, 0xff }
#define _RLC_MIX { RLCM,  RIX,  MIX,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }
#define _RLC_MIY { RLCM,  RIY,  MIY,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }

#define _RL_(r)  { RLS,  R##r,  R##r,  RNR,  0x02, 0x02, 0x08,     FN+FH, 0x00, 0xff }
#define _RL_x(r) { RLS,  R##r,  R##r,  RNR,  0x03, 0x03, 0x0c,     FN+FH, 0x00, 0xff }
#define _RL_B	_RL_(B)
#define _RL_C	_RL_(C)
#define _RL_D	_RL_(D)
#define _RL_E	_RL_(E)
#define _RL_H	_RL_(H)
#define _RL_L	_RL_(L)
#define _RL_A	_RL_(A)
#define _RL_XH	_RL_(XH)
#define _RL_XL	_RL_(XL)
#define _RL_YH	_RL_(YH)
#define _RL_YL	_RL_(YL)
#define _RL_MHL { RLSM,  RHL,  MHL,  RNR,  0x02, 0x04, 0x0f,     FN+FH, 0x00, 0xff }
#define _RL_MIX { RLSM,  RIX,  MIX,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }
#define _RL_MIY { RLSM,  RIY,  MIY,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }

#define _RRC_(r)  { RRC,  R##r,  R##r,  RNR,  0x02, 0x02, 0x08,     FN+FH, 0x00, 0xff }
#define _RRC_x(r) { RRC,  R##r,  R##r,  RNR,  0x03, 0x03, 0x0c,     FN+FH, 0x00, 0xff }
#define _RRC_B	_RRC_(B)
#define _RRC_C	_RRC_(C)
#define _RRC_D	_RRC_(D)
#define _RRC_E	_RRC_(E)
#define _RRC_H	_RRC_(H)
#define _RRC_L	_RRC_(L)
#define _RRC_A	_RRC_(A)
#define _RRC_XH	_RRC_(XH)
#define _RRC_XL	_RRC_(XL)
#define _RRC_YH	_RRC_(YH)
#define _RRC_YL	_RRC_(YL)
#define _RRC_MHL { RRCM,  RHL,  MHL,  RNR,  0x02, 0x04, 0x0f,     FN+FH, 0x00, 0xff }
#define _RRC_MIX { RRCM,  RIX,  MIX,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }
#define _RRC_MIY { RRCM,  RIY,  MIY,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }

#define _RR_(r)  { RR,  R##r,  R##r,  RNR,  0x02, 0x02, 0x08,     FN+FH, 0x00, 0xff }
#define _RR_x(r) { RR,  R##r,  R##r,  RNR,  0x03, 0x03, 0x0c,     FN+FH, 0x00, 0xff }
#define _RR_B	_RR_(B)
#define _RR_C	_RR_(C)
#define _RR_D	_RR_(D)
#define _RR_E	_RR_(E)
#define _RR_H	_RR_(H)
#define _RR_L	_RR_(L)
#define _RR_A	_RR_(A)
#define _RR_XH	_RR_(XH)
#define _RR_XL	_RR_(XL)
#define _RR_YH	_RR_(YH)
#define _RR_YL	_RR_(YL)
#define _RR_MHL { RRM,  RHL,  MHL,  RNR,  0x02, 0x04, 0x0f,     FN+FH, 0x00, 0xff }
#define _RR_MIX { RRM,  RIX,  MIX,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }
#define _RR_MIY { RRM,  RIY,  MIY,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }

#define _SLA_(r)  { SLA,  R##r,  R##r,  RNR,  0x02, 0x02, 0x08,     FN+FH, 0x00, 0xff }
#define _SLA_x(r) { SLA,  R##r,  R##r,  RNR,  0x03, 0x03, 0x0c,     FN+FH, 0x00, 0xff }
#define _SLA_B	_SLA_(B)
#define _SLA_C	_SLA_(C)
#define _SLA_D	_SLA_(D)
#define _SLA_E	_SLA_(E)
#define _SLA_H	_SLA_(H)
#define _SLA_L	_SLA_(L)
#define _SLA_A	_SLA_(A)
#define _SLA_XH	_SLA_(XH)
#define _SLA_XL	_SLA_(XL)
#define _SLA_YH	_SLA_(YH)
#define _SLA_YL	_SLA_(YL)
#define _SLA_MHL { SLAM,  RHL,  MHL,  RNR,  0x02, 0x04, 0x0f,     FN+FH, 0x00, 0xff }
#define _SLA_MIX { SLAM,  RIX,  MIX,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }
#define _SLA_MIY { SLAM,  RIY,  MIY,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }

#define _SRA_(r)  { SRA,  R##r,  R##r,  RNR,  0x02, 0x02, 0x08,     FN+FH, 0x00, 0xff }
#define _SRA_x(r) { SRA,  R##r,  R##r,  RNR,  0x03, 0x03, 0x0c,     FN+FH, 0x00, 0xff }
#define _SRA_B	_SRA_(B)
#define _SRA_C	_SRA_(C)
#define _SRA_D	_SRA_(D)
#define _SRA_E	_SRA_(E)
#define _SRA_H	_SRA_(H)
#define _SRA_L	_SRA_(L)
#define _SRA_A	_SRA_(A)
#define _SRA_XH	_SRA_(XH)
#define _SRA_XL	_SRA_(XL)
#define _SRA_YH	_SRA_(YH)
#define _SRA_YL	_SRA_(YL)
#define _SRA_MHL { SRAM,  RHL,  MHL,  RNR,  0x02, 0x04, 0x0f,     FN+FH, 0x00, 0xff }
#define _SRA_MIX { SRAM,  RIX,  MIX,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }
#define _SRA_MIY { SRAM,  RIY,  MIY,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }

#define _SLI_(r)  { SLI,  R##r,  R##r,  RNR,  0x02, 0x02, 0x08,     FN+FH, 0x00, 0xff }
#define _SLI_x(r) { SLI,  R##r,  R##r,  RNR,  0x03, 0x03, 0x0c,     FN+FH, 0x00, 0xff }
#define _SLI_B	_SLI_(B)
#define _SLI_C	_SLI_(C)
#define _SLI_D	_SLI_(D)
#define _SLI_E	_SLI_(E)
#define _SLI_H	_SLI_(H)
#define _SLI_L	_SLI_(L)
#define _SLI_A	_SLI_(A)
#define _SLI_XH	_SLI_(XH)
#define _SLI_XL	_SLI_(XL)
#define _SLI_YH	_SLI_(YH)
#define _SLI_YL	_SLI_(YL)
#define _SLI_MHL { SLIM,  RHL,  MHL,  RNR,  0x02, 0x04, 0x0f,     FN+FH, 0x00, 0xff }
#define _SLI_MIX { SLIM,  RIX,  MIX,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }
#define _SLI_MIY { SLIM,  RIY,  MIY,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }

#define _SRL_(r)  { SRL,  R##r,  R##r,  RNR,  0x02, 0x02, 0x08,     FN+FH, 0x00, 0xff }
#define _SRL_x(r) { SRL,  R##r,  R##r,  RNR,  0x03, 0x03, 0x0c,     FN+FH, 0x00, 0xff }
#define _SRL_B	_SRL_(B)
#define _SRL_C	_SRL_(C)
#define _SRL_D	_SRL_(D)
#define _SRL_E	_SRL_(E)
#define _SRL_H	_SRL_(H)
#define _SRL_L	_SRL_(L)
#define _SRL_A	_SRL_(A)
#define _SRL_XH	_SRL_(XH)
#define _SRL_XL	_SRL_(XL)
#define _SRL_YH	_SRL_(YH)
#define _SRL_YL	_SRL_(YL)
#define _SRL_MHL { SRLM,  RHL,  MHL,  RNR,  0x02, 0x04, 0x0f,     FN+FH, 0x00, 0xff }
#define _SRL_MIX { SRLM,  RIX,  MIX,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }
#define _SRL_MIY { SRLM,  RIY,  MIY,  CDB,  0x04, 0x06, 0x17,     FN+FH, 0x00, 0xff }

#define _RLD	{ RLD,  RA,  RA,  RNR,  0x02, 0x05, 0x18,     FN+FH, 0x00, 0xfe }
#define _RRD	{ RRD,  RA,  RA,  RNR,  0x02, 0x05, 0x18,     FN+FH, 0x00, 0xfe }

/*************************** Instrukcje skoków *****************************************/

#define _JP	{ JP,  CDW,  NOC, RNR,  0x03, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _JPNZ	{ JP,  CDW,  NZ,  RNR,  0x03, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _JPZ	{ JP,  CDW,   Z,  RNR,  0x03, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _JPNC	{ JP,  CDW,  NC,  RNR,  0x03, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _JPC	{ JP,  CDW,   C,  RNR,  0x03, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _JPPO	{ JP,  CDW,  PO,  RNR,  0x03, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _JPPE	{ JP,  CDW,  PE,  RNR,  0x03, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _JPP	{ JP,  CDW,   P,  RNR,  0x03, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _JPM	{ JP,  CDW,   M,  RNR,  0x03, 0x03, 0x0a,     0x00, 0x00, 0x00 }

#define _JR	{ JR,  CDB,  NOC, RNR,  0x02, 0x03, 0x0c,     0x00, 0x00, 0x00 }
#define _JRNZ	{ JR,  CDB,  NZ,  RNR,  0x02, 0x03, 0x0c,     0x00, 0x00, 0x00 }
#define _JRZ	{ JR,  CDB,   Z,  RNR,  0x02, 0x03, 0x0c,     0x00, 0x00, 0x00 }
#define _JRNC	{ JR,  CDB,  NC,  RNR,  0x02, 0x03, 0x0c,     0x00, 0x00, 0x00 }
#define _JRC	{ JR,  CDB,   C,  RNR,  0x02, 0x03, 0x0c,     0x00, 0x00, 0x00 }

#define _JPHL	{ JPHL,  RHL,  NOC,  RNR,  0x01, 0x01, 0x04,     0x00, 0x00, 0x00 }
#define _JPIX	{ JPHL,  RIX,  NOC,  RNR,  0x02, 0x02, 0x08,     0x00, 0x00, 0x00 }
#define _JPIY	{ JPHL,  RIY,  NOC,  RNR,  0x02, 0x02, 0x08,     0x00, 0x00, 0x00 }

#define _DJNZ	{ DJNZ,RNR, RNR,  RNR,  0x02, 0x03, 0x0d,     0x00, 0x00, 0x00 }

/*************************** Instrukcje podprogramów *****************************************/

#define _CALL	{ CALL,  CDW,  NOC, RNR,  0x03, 0x05, 0x11,     0x00, 0x00, 0x00 }
#define _CALLNZ	{ CALL,  CDW,  NZ,  RNR,  0x03, 0x05, 0x11,     0x00, 0x00, 0x00 }
#define _CALLZ	{ CALL,  CDW,   Z,  RNR,  0x03, 0x05, 0x11,     0x00, 0x00, 0x00 }
#define _CALLNC	{ CALL,  CDW,  NC,  RNR,  0x03, 0x05, 0x11,     0x00, 0x00, 0x00 }
#define _CALLC	{ CALL,  CDW,   C,  RNR,  0x03, 0x05, 0x11,     0x00, 0x00, 0x00 }
#define _CALLPO	{ CALL,  CDW,  PO,  RNR,  0x03, 0x05, 0x11,     0x00, 0x00, 0x00 }
#define _CALLPE	{ CALL,  CDW,  PE,  RNR,  0x03, 0x05, 0x11,     0x00, 0x00, 0x00 }
#define _CALLP	{ CALL,  CDW,   P,  RNR,  0x03, 0x05, 0x11,     0x00, 0x00, 0x00 }
#define _CALLM	{ CALL,  CDW,   M,  RNR,  0x03, 0x05, 0x11,     0x00, 0x00, 0x00 }

#define _RET	{ RET,  RNR,  NOC, RNR,  0x01, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _RETNZ	{ RET,  RNR,  NZ,  RNR,  0x01, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RETZ	{ RET,  RNR,   Z,  RNR,  0x01, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RETNC	{ RET,  RNR,  NC,  RNR,  0x01, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RETC	{ RET,  RNR,   C,  RNR,  0x01, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RETPO	{ RET,  RNR,  PO,  RNR,  0x01, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RETPE	{ RET,  RNR,  PE,  RNR,  0x01, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RETP	{ RET,  RNR,   P,  RNR,  0x01, 0x03, 0x0b,     0x00, 0x00, 0x00 }
#define _RETM	{ RET,  RNR,   M,  RNR,  0x01, 0x03, 0x0b,     0x00, 0x00, 0x00 }

#define _RETI	{ RETI,  RNR,  NOC, RNR,  0x01, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _RETN	{ RETN,  RNR,  NOC, RNR,  0x01, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _RST(p)	{ RST,     p,  RNR, RNR,  0x01, 0x03, 0x0a,     0x00, 0x00, 0x00 }
#define _RST00	_RST(0x00)
#define _RST08	_RST(0x08)
#define _RST10	_RST(0x10)
#define _RST18	_RST(0x18)
#define _RST20	_RST(0x20)
#define _RST28	_RST(0x28)
#define _RST30	_RST(0x30)
#define _RST38	_RST(0x38)

#define _CB_	{ PREF_CB,  RNR,  RNR, RNR,  0x01, 0x01, 0x01,     0x00, 0x00, 0x00 }
#define _ED_	{ PREF_ED,  RNR,  RNR, RNR,  0x01, 0x01, 0x01,     0x00, 0x00, 0x00 }
#define _DD_	{ PREF_DD,  RNR,  RNR, RNR,  0x01, 0x01, 0x01,     0x00, 0x00, 0x00 }
#define _FD_	{ PREF_FD,  RNR,  RNR, RNR,  0x01, 0x01, 0x01,     0x00, 0x00, 0x00 }

/**************************************************************************************

 elementy tablicy dekodującej

*/

#define _LDXH_MHL _LDH_MIX
#define _LDXL_MHL _LDL_MIX
#define _LDYH_MHL _LDH_MIY
#define _LDYL_MHL _LDL_MIY
#define _LDMIX_MHL _NOP
#define _LDMIY_MHL _NOP

#define _X_R_R(n,r) n##r##_B, n##r##_C, n##r##_D, n##r##_E, n##r##_H, n##r##_L, n##r##_MHL, n##r##_A

#define _X_R_RX(n,r) n##r##_B, n##r##_C, n##r##_D, n##r##_E, n##r##_XH, n##r##_XL, n##r##_MIX, n##r##_A

#define _X_R_RY(n,r) n##r##_B, n##r##_C, n##r##_D, n##r##_E, n##r##_YH, n##r##_YL, n##r##_MIY, n##r##_A
#define _X_R(n)	_X_R_R(n,B),_X_R_R(n,C),_X_R_R(n,D),_X_R_R(n,E),_X_R_R(n,H),_X_R_R(n,L),\
    _X_R_R(n,MHL),_X_R_R(n,A)

#define _X_RX(n) _X_R_RX(n,B),_X_R_RX(n,C),_X_R_RX(n,D),_X_R_RX(n,E),_X_R_R(n,XH),_X_R_R(n,XL),\
    _X_R_R(n,MIX),_X_R_RX(n,A)

#define _X_RY(n) _X_R_RY(n,B),_X_R_RY(n,C),_X_R_RY(n,D),_X_R_RY(n,E),_X_R_R(n,YH),_X_R_R(n,YL),\
    _X_R_R(n,MIY),_X_R_RY(n,A)

#define _Y_R(n) _X_R_R(n,A)
#define _LD_R	_X_R(_LD)
#define _ADD_R	_Y_R(_ADD)
#define _ADC_R	_Y_R(_ADC)
#define _SUB_R	_Y_R(_SUB)
#define _SBC_R	_Y_R(_SBC)
#define _AND_R	_Y_R(_AND)
#define _XOR_R	_Y_R(_XOR)
#define _OR_R	_Y_R(_OR)
#define _CP_R	_Y_R(_CP)
#define _Y_RX(n) _X_R_RX(n,A)

#define _LD_RX	_X_RX(_LD)
#define _ADD_RX	_Y_RX(_ADD)
#define _ADC_RX	_Y_RX(_ADC)
#define _SUB_RX	_Y_RX(_SUB)
#define _SBC_RX	_Y_RX(_SBC)
#define _AND_RX	_Y_RX(_AND)
#define _XOR_RX	_Y_RX(_XOR)
#define _OR_RX	_Y_RX(_OR)
#define _CP_RX	_Y_RX(_CP)
#define _Y_RY(n) _X_R_RY(n,A)

#define _LD_RY	_X_RY(_LD)
#define _ADD_RY	_Y_RY(_ADD)
#define _ADC_RY	_Y_RY(_ADC)
#define _SUB_RY	_Y_RY(_SUB)
#define _SBC_RY	_Y_RY(_SBC)
#define _AND_RY	_Y_RY(_AND)
#define _XOR_RY	_Y_RY(_XOR)
#define _OR_RY	_Y_RY(_OR)
#define _CP_RY	_Y_RY(_CP)

#define PRE_00  \
        _NOP,      _LDBC_NN, _LDMBC_A, _INCBC,   _INCB, _DECB, _LDB_N, _RLCA, \
        _EXAF_AF, _ADDHL_BC, _LDA_MBC, _DECBC, _INCC, _DECC, _LDC_N, _RRCA,  _DJNZ, _LDDE_NN,\
        _LDMDE_A,    _INCDE,    _INCD,  _DECD,  _LDD_N,  _RLA,    _JR, _ADDHL_DE,\
        _LDA_MDE,    _DECDE,    _INCE,  _DECE,  _LDE_N,  _RRA,  _JRNZ, _LDHL_NN,\
        _LDMCDW_HL,  _INCHL,    _INCH,  _DECH,  _LDH_N,  _DAA,   _JRZ, _ADDHL_HL,\
        _LDHL_MCDW,  _DECHL,    _INCL,  _DECL,  _LDL_N,  _CPL,  _JRNC, _LDSP_NN,\
        _LDMCDW_A,   _INCSP,  _INCMHL,_DECMHL,_LDMHL_N,  _SCF,   _JRC, _ADDHL_SP,\
        _LDA_MCDW,   _DECSP,    _INCA,  _DECA,  _LDA_N,  _CCF,\
        _LD_R,  _ADD_R, _ADC_R, _SUB_R, _SBC_R,  _AND_R, _XOR_R, _OR_R, _CP_R,\
        _RETNZ, _POPBC, _JPNZ, _JP, _CALLNZ, _PUSHBC, _ADDA_N, _RST00, _RETZ,\
        _RET, _JPZ, _CB_, _CALLZ, _CALL, _ADCA_N, _RST08, _RETNC, _POPDE, _JPNC,\
        _OUTA, _CALLNC, _PUSHDE, _SUBA_N, _RST10, _RETC, _EXX, _JPC, _INA, _CALLC,\
        _DD_, _SBCA_N, _RST18, _RETPO, _POPHL, _JPPO, _EXMSP_HL, _CALLPO, _PUSHHL,\
        _ANDA_N, _RST20, _RETPE, _JPHL, _JPPE, _EXDE_HL, _CALLPE, _ED_, _XORA_N,\
        _RST28, _RETP, _POPAF, _JPP, _DI, _CALLP, _PUSHAF, _ORA_N, _RST30, _RETM,\
        _LDSP_HL, _JPM, _EI, _CALLM, _FD_, _CPA_N, _RST38

/* PO DD ->IX */
#define PRE_DD  \
        _NOP,      _LDBC_NN, _LDMBC_A, _INCBC,   _INCB, _DECB, _LDB_N, _RLCA, \
        _EXAF_AF, _ADDIX_BC, _LDA_MBC, _DECBC, _INCC, _DECC,  _LDC_N, _RRCA,  _DJNZ, _LDDE_NN,\
        _LDMDE_A,    _INCDE,    _INCD,  _DECD,  _LDD_N,  _RLA,    _JR, _ADDIX_DE,\
        _LDA_MDE,    _DECDE,    _INCE,  _DECE,  _LDE_N,  _RRA,  _JRNZ, _LDIX_NN,\
        _LDMCDW_IX,  _INCIX,    _INCXH,  _DECXH, _LDXH_N,  _DAA,   _JRZ, _ADDIX_IX,\
        _LDIX_MCDW,  _DECIX,    _INCXL,  _DECXL,  _LDXL_N,  _CPL,  _JRNC, _LDSP_NN,\
        _LDMCDW_A,   _INCSP,  _INCMIX,_DECMIX,_LDMIX_N,  _SCF,   _JRC, _ADDIX_SP,\
        _LDA_MCDW,   _DECSP,    _INCA,  _DECA,  _LDA_N,  _CCF,\
        _LD_RX,  _ADD_RX, _ADC_RX, _SUB_RX, _SBC_RX,  _AND_RX, _XOR_RX, _OR_RX, _CP_RX,\
        _RETNZ, _POPBC, _JPNZ, _JP, _CALLNZ, _PUSHBC, _ADDA_N, _RST00, _RETZ,\
        _RET, _JPZ, _CB_, _CALLZ, _CALL, _ADCA_N, _RST08, _RETNC, _POPDE, _JPNC,\
        _OUTA, _CALLNC, _PUSHDE, _SUBA_N, _RST10, _RETC, _EXX, _JPC, _INA, _CALLC,\
        _DD_, _SBCA_N, _RST18, _RETPO, _POPIX, _JPPO, _EXMSP_IX, _CALLPO, _PUSHIX,\
        _ANDA_N, _RST20, _RETPE, _JPIX, _JPPE, _EXDE_HL, _CALLPE, _ED_, _XORA_N,\
        _RST28, _RETP, _POPAF, _JPP, _DI, _CALLP, _PUSHAF, _ORA_N, _RST30, _RETM,\
        _LDSP_IX, _JPM, _EI, _CALLM, _FD_, _CPA_N, _RST38

/* PO FD ->IY */
#define PRE_FD  \
        _NOP,      _LDBC_NN, _LDMBC_A, _INCBC,   _INCB, _DECB, _LDB_N, _RLCA, \
        _EXAF_AF, _ADDIY_BC, _LDA_MBC, _DECBC, _INCC, _DECC,  _LDC_N, _RRCA,  _DJNZ, _LDDE_NN,\
        _LDMDE_A,    _INCDE,    _INCD,  _DECD,  _LDD_N,  _RLA,    _JR, _ADDIY_DE,\
        _LDA_MDE,    _DECDE,    _INCE,  _DECE,  _LDE_N,  _RRA,  _JRNZ, _LDIY_NN,\
        _LDMCDW_IY,  _INCIY,    _INCYH,  _DECYH, _LDYH_N,  _DAA,   _JRZ, _ADDIY_IY,\
        _LDIY_MCDW,  _DECIY,    _INCYL,  _DECYL,  _LDYL_N,  _CPL,  _JRNC, _LDSP_NN,\
        _LDMCDW_A,   _INCSP,  _INCMIY,_DECMIY,_LDMIY_N,  _SCF,   _JRC, _ADDIY_SP,\
        _LDA_MCDW,   _DECSP,    _INCA,  _DECA,  _LDA_N,  _CCF,\
        _LD_RY,  _ADD_RY, _ADC_RY, _SUB_RY, _SBC_RY,  _AND_RY, _XOR_RY, _OR_RY, _CP_RY,\
        _RETNZ, _POPBC, _JPNZ, _JP, _CALLNZ, _PUSHBC, _ADDA_N, _RST00, _RETZ,\
        _RET, _JPZ, _CB_, _CALLZ, _CALL, _ADCA_N, _RST08, _RETNC, _POPDE, _JPNC,\
        _OUTA, _CALLNC, _PUSHDE, _SUBA_N, _RST10, _RETC, _EXX, _JPC, _INA, _CALLC,\
        _DD_, _SBCA_N, _RST18, _RETPO, _POPIY, _JPPO, _EXMSP_IY, _CALLPO, _PUSHIY,\
        _ANDA_N, _RST20, _RETPE, _JPIY, _JPPE, _EXDE_HL, _CALLPE, _ED_, _XORA_N,\
        _RST28, _RETP, _POPAF, _JPP, _DI, _CALLP, _PUSHAF, _ORA_N, _RST30, _RETM,\
        _LDSP_IY, _JPM, _EI, _CALLM, _FD_, _CPA_N, _RST38

/* PO ED */
#define PRE_ED \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _IN_B, _OUT_B, _SBCHL_BC, _LDMCDW_BC, _NEG, _RETN,\
        _IM0, _LDI_A, _IN_C, _OUT_C, _ADCHL_BC, _LDBC_MCDW, _NOP, _RETI, _NOP,\
        _LDR_A, _IN_D, _OUT_D, _SBCHL_DE, _LDMCDW_DE, _NOP, _NOP,\
        _IM1, _LDA_I, _IN_E, _OUT_E, _ADCHL_DE, _LDDE_MCDW, _NOP, _NOP,\
        _IM2, _LDA_R, _IN_H, _OUT_H, _SBCHL_HL, _LDMCDW_HL,_NOP, _NOP, _NOP,\
        _RRD, _IN_L, _OUT_L, _ADCHL_HL, _LDHL_MCDW, _NOP, _NOP, _NOP,\
        _RLD, _IN_F, _NOP, _SBCHL_SP, _LDMCDW_SP, _NOP, _NOP, _NOP, _NOP,\
        _IN_A, _OUT_A, _ADCHL_SP, _LDSP_MCDW, _NOP, _NOP, _NOP, _NOP,\
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _LDI, _CPI, _INI, _OUTI, _NOP, _NOP, _NOP, _NOP, \
        _LDD, _CPD, _IND, _OUTD, _NOP, _NOP, _NOP, _NOP, \
        _LDIR, _CPIR, _INIR, _OTIR, _NOP, _NOP, _NOP, _NOP,\
        _LDDR, _CPDR, _INDR, _OTDR, _NOP, _NOP, _NOP, _NOP,\
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP

/* PO ED I PO DD */
#define PRE_ED_DD \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _IN_B, _OUT_B, _SBCIX_BC, _LDMCDW_BC, _NEG, _RETN,\
        _IM0, _LDI_A, _IN_C, _OUT_C, _ADCIX_BC, _LDBC_MCDW, _NOP, _RETI, _NOP,\
        _LDR_A, _IN_D, _OUT_D, _SBCIX_DE, _LDMCDW_DE, _NOP, _NOP,\
        _IM1, _LDA_I, _IN_E, _OUT_E, _ADCIX_DE, _LDDE_MCDW, _NOP, _NOP,\
        _IM2, _LDA_R, _IN_XH, _OUT_XH, _SBCIX_IX, _LDMCDW_IX,_NOP, _NOP, _NOP,\
        _RRD, _IN_XL, _OUT_XL, _ADCIX_IX, _LDIX_MCDW, _NOP, _NOP, _NOP,\
        _RLD, _IN_F, _NOP, _SBCIX_SP, _LDMCDW_SP, _NOP, _NOP, _NOP, _NOP,\
        _IN_A, _OUT_A, _ADCIX_SP, _LDSP_MCDW, _NOP, _NOP, _NOP, _NOP,\
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _LDI, _CPI, _INI, _OUTI, _NOP, _NOP, _NOP, _NOP, \
        _LDD, _CPD, _IND, _OUTD, _NOP, _NOP, _NOP, _NOP, \
        _LDIR, _CPIR, _INIR, _OTIR, _NOP, _NOP, _NOP, _NOP,\
        _LDDR, _CPDR, _INDR, _OTDR, _NOP, _NOP, _NOP, _NOP,\
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP

/* PO ED I PO FD */
#define PRE_ED_FD \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _IN_B, _OUT_B, _SBCIY_BC, _LDMCDW_BC, _NEG, _RETN,\
        _IM0, _LDI_A, _IN_C, _OUT_C, _ADCIY_BC, _LDBC_MCDW, _NOP, _RETI, _NOP,\
        _LDR_A, _IN_D, _OUT_D, _SBCIY_DE, _LDMCDW_DE, _NOP, _NOP,\
        _IM1, _LDA_I, _IN_E, _OUT_E, _ADCIY_DE, _LDDE_MCDW, _NOP, _NOP,\
        _IM2, _LDA_R, _IN_YH, _OUT_YH, _SBCIY_IY, _LDMCDW_IY,_NOP, _NOP, _NOP,\
        _RRD, _IN_YL, _OUT_YL, _ADCIY_IY, _LDIY_MCDW, _NOP, _NOP, _NOP,\
        _RLD, _IN_F, _NOP, _SBCIY_SP, _LDMCDW_SP, _NOP, _NOP, _NOP, _NOP,\
        _IN_A, _OUT_A, _ADCIY_SP, _LDSP_MCDW, _NOP, _NOP, _NOP, _NOP,\
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _LDI, _CPI, _INI, _OUTI, _NOP, _NOP, _NOP, _NOP, \
        _LDD, _CPD, _IND, _OUTD, _NOP, _NOP, _NOP, _NOP, \
        _LDIR, _CPIR, _INIR, _OTIR, _NOP, _NOP, _NOP, _NOP,\
        _LDDR, _CPDR, _INDR, _OTDR, _NOP, _NOP, _NOP, _NOP,\
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, \
        _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP, _NOP

/* PO CB */
#define TMPL_0(cmd,reg0,reg1,regx)	cmd##_B, cmd##_C, cmd##_D, cmd##_E, cmd##_##reg0,\
                    cmd##_##reg1, cmd##regx, cmd##_A

#define TMPL_1(cmd)	TMPL_0(cmd,H,L,_MHL)
#define TMPL_2(cmd)	TMPL_0(cmd,XH,XL,_MIX)
#define TMPL_3(cmd)	TMPL_0(cmd,YH,YL,_MIY)
#define TMPL_4(cmd)	TMPL_1(cmd##0),TMPL_1(cmd##1),TMPL_1(cmd##2),TMPL_1(cmd##3),\
                TMPL_1(cmd##4),TMPL_1(cmd##5),TMPL_1(cmd##6),TMPL_1(cmd##7)
#define TMPL_5(cmd)	TMPL_2(cmd##0),TMPL_2(cmd##1),TMPL_2(cmd##2),TMPL_2(cmd##3),\
                TMPL_2(cmd##4),TMPL_2(cmd##5),TMPL_2(cmd##6),TMPL_2(cmd##7)
#define TMPL_6(cmd)	TMPL_3(cmd##0),TMPL_3(cmd##1),TMPL_3(cmd##2),TMPL_3(cmd##3),\
                TMPL_3(cmd##4),TMPL_3(cmd##5),TMPL_3(cmd##6),TMPL_3(cmd##7)

#define PRE_CB \
        TMPL_1(_RLC), TMPL_1(_RRC), TMPL_1(_RL), TMPL_1(_RR), TMPL_1(_SLA), TMPL_1(_SRA),\
        TMPL_1(_SLI), TMPL_1(_SRL), TMPL_4(_BIT), TMPL_4(_RES), TMPL_4(_SET)

#define PRE_CB_DD \
        TMPL_2(_RLC), TMPL_2(_RRC), TMPL_2(_RL), TMPL_2(_RR), TMPL_2(_SLA), TMPL_2(_SRA),\
        TMPL_2(_SLI), TMPL_2(_SRL), TMPL_5(_BIT), TMPL_5(_RES), TMPL_5(_SET)

#define PRE_CB_FD \
        TMPL_3(_RLC), TMPL_3(_RRC), TMPL_3(_RL), TMPL_3(_RR), TMPL_3(_SLA), TMPL_3(_SRA),\
        TMPL_3(_SLI), TMPL_3(_SRL), TMPL_6(_BIT), TMPL_6(_RES), TMPL_6(_SET)

/************************************************************************************************/

/* główna tabela dekodowania instrukcji */
#define Z80_DECODE_TABLE { \
                PRE_00, PRE_DD,    PRE_FD,   \
                PRE_CB, PRE_CB_DD, PRE_CB_FD,\
                PRE_ED, PRE_ED_DD, PRE_ED_FD \
             }

#define Z80_PARITY_TABLE {\
               1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,\
               0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,\
               0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,\
               1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,\
               0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,\
               1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,\
               1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,\
               0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,\
               0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,\
               1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,\
               1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,\
               0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,\
               1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,\
               0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,\
               0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,\
               1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1 \
             }

/*
    Tablica generacji przeniesień i przepełnień.
    indeks tablicy stanowi 6 bitów, będących kopiami bitów trecich i siódmych argumentów i wyniku:

    5..0 -> r7,a7,b7,r3,a3,b3

    Wartością zwracaną jest maska OR bitowa na znacznikach

    tablica dla dodawania i odejmowania jest różna
*/
#define Z80_ADD_CARRY_TBL { \
            0x00,0x10,0x10,0x10,0x00,0x00,0x00,0x10,\
            0x01,0x11,0x11,0x11,0x01,0x01,0x01,0x11,\
            0x01,0x11,0x11,0x11,0x01,0x01,0x01,0x11,\
            0x05,0x15,0x15,0x15,0x05,0x05,0x05,0x15,\
            0x04,0x14,0x14,0x14,0x04,0x04,0x04,0x14,\
            0x00,0x10,0x10,0x10,0x00,0x00,0x00,0x10,\
            0x00,0x10,0x10,0x10,0x00,0x00,0x00,0x10,\
            0x01,0x11,0x11,0x11,0x01,0x01,0x01,0x11 \
              }

#define Z80_SUB_CARRY_TBL { \
            0x00,0x00,0x10,0x00,0x10,0x00,0x10,0x10,\
            0x04,0x04,0x14,0x04,0x14,0x04,0x14,0x14,\
            0x01,0x01,0x11,0x01,0x11,0x01,0x11,0x11,\
            0x00,0x00,0x10,0x00,0x10,0x00,0x10,0x10,\
            0x01,0x01,0x11,0x01,0x11,0x01,0x11,0x11,\
            0x00,0x00,0x10,0x00,0x10,0x00,0x10,0x10,\
            0x05,0x05,0x15,0x05,0x15,0x05,0x15,0x15,\
            0x01,0x01,0x11,0x01,0x11,0x01,0x11,0x11 \
              }

/* maska dla warunków testu rejestru flagowego */
#define Z80_COND_MASK { 0xff, FZ,  FZ, FC, FC, FP, FP, FS, FS }
/* maska negacji warunku */
#define Z80_COND_PLRT { 0x00, FZ, 0x00, FC, 0x00, FP, 0x00, FS, 0x00 }

#endif

/*
        /\
       /  \
      /\ II\
     /  \  /|
    /____\/ |
    |  _ |  |
    | | || /
    | | ||/
    ------
*/
