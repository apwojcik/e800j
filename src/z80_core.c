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
    z80_core.c
    Implementacja realizacji emulacji rozkazów

*/
#include <string.h>
#include "z80_cpu.h"
#include "z80_core.h"
#include "z80_dtbl.h"

#define Z80_COND(cnd,freg) 	((__z80_cond_mask[cnd] & (freg | F5)) ^ __z80_cond_plrt[cnd])
#define TICS_UPDATE 		z->cycles += z->c;   z->ticks  += z->t; z->_tic = z->t;
#define INST_END    		z->pref_dd_fd = z->pref_cb_ed = 0; z->ipend=0; z->irupt=0; z->ibus=0;
#define GET_REG8(v)		v = *(z->regs8[z->reg80 = z->p0])
#define SET_REG8(v)		*(z->regs8[z->p0]) = v
#define SET_MEM(v)		z->memory_write(z,z->dd2,v)
#define ROL_L(reg,bit_0)	((((reg) << 1) & 0xfe) | ((bit_0) & 0x1))
#define ROL_R(reg,bit_0)	((((reg) >> 1) & 0x7f) | ((bit_0) & 0x80))
#define VGET_REG8(b)		b = *(z->regs8[z->reg80=z->p1]);
#define VGET_CNS8(b)		b = z->dd=z80_fetch_byte(z);
#define VSET_REG8(a)		*(z->regs8[z->p1]) = (unsigned char)(a)
#define VSET_MEM8(a)		z->memory_write(z,z->dd2,(unsigned char)(a))

#define SET_FLAGSP(reg,flags) \
        ftmp = ((~(reg) & ((reg) - 1)) >> 1) & 0x40;\
        ftmp |= (reg) & 0xa8;\
        ftmp |= flags & 0x11; \
            ftmp |= z->o;\
        ftmp |= (__z80_parity__[reg] << 2) & 0x04;\
        ftmp &= ~z->a;\
        Fr &= ~z->x;\
        Fr |= z->x & ftmp;

#define SET_FLAGSV(reg,flags)\
        ftmp = (((~reg) & ((reg) - 1)) >> 1) & 0x40;\
        ftmp |= (reg) & 0xa8;\
        ftmp |= flags & 0x15;\
        ftmp |= z->o;\
        ftmp &= ~z->a;\
        Fr &= ~z->x;\
        Fr |= z->x & ftmp;

#define SET_FLAGSVX(reg,flags)\
        ftmp = ((~(reg) & ((reg) - 1)) >> 9) & 0x40;\
        ftmp |= ((reg) & 0xa800) >> 8;\
        ftmp |= flags & 0x15;\
            ftmp |= z->o;\
        ftmp &= ~z->a;\
        Fr   &= ~z->x;\
        Fr   |= z->x & ftmp;

#define GET_MEM(v) \
        z->dd2 = *z->regs16[z->regx0 = z->p0];\
        if(z->pref_dd_fd) z->memptr = z->dd2 += (char)(z->dd=z->dd3);\
        v = z->memory_read(z,z->dd2);

#define VGET_MEM8(b) \
        z->dd2 = *z->regs16[z->regx0=z->p1]; \
        if(z->pref_dd_fd) z->memptr = z->dd2 += (char)(z->dd=z80_fetch_byte(z));\
        b = z->memory_read(z,z->dd2);

/* 8-bitowa ALU: a +/- b +/- cy, ustawia znaczniki wg masek instrukcji, zwraca wynik */
static unsigned char z80_alu8(struct cpu* z, int sub, unsigned char a, unsigned char b, unsigned char cy) {
    unsigned char ftmp, res, fl;
    unsigned int r;

    if (sub) {
        r = (unsigned int)a - b - cy;
        fl = (unsigned char)(((a ^ b) & (a ^ r) & 0x80) >> 5);
    } else {
        r = (unsigned int)a + b + cy;
        fl = (unsigned char)((~(a ^ b) & (a ^ r) & 0x80) >> 5);
    }
    fl |= (unsigned char)(((a ^ b ^ r) & 0x10) | ((r >> 8) & 0x01));
    res = (unsigned char)r;
    SET_FLAGSV(res, fl)
    return res;
}

/* 16-bitowa ALU (ADD/ADC/SBC HL,rr): kind 0 = ADD, 1 = ADC, 2 = SBC; zwraca wynik */
static unsigned short z80_alu16(struct cpu* z, int kind, unsigned int a, unsigned int b) {
    unsigned char ftmp, fl;
    unsigned int r, cy = kind ? (Fr & FC) : 0;
    unsigned short res;

    if (kind == 2) {
        r = a - b - cy;
        fl = (unsigned char)(((a ^ b) & (a ^ r) & 0x8000) >> 13);
    } else {
        r = a + b + cy;
        fl = (unsigned char)((~(a ^ b) & (a ^ r) & 0x8000) >> 13);
    }
    fl |= (unsigned char)((((a ^ b ^ r) >> 8) & 0x10) | ((r >> 16) & 0x01));
    res = (unsigned short)r;
    SET_FLAGSVX(res, fl)
    return res;
}

/* tablice stałych */
static const struct z80dt __z80_decoder__[] = Z80_DECODE_TABLE;
static const unsigned char __z80_parity__[] = Z80_PARITY_TABLE;
static const unsigned char __z80_cond_mask[] = Z80_COND_MASK;
static const unsigned char __z80_cond_plrt[] = Z80_COND_PLRT;

/**************************************************************************************************/

static unsigned char z80_fetch_byte(struct cpu* z) {
    if (z->irupt) {
        if (z->ibus >= 8)
            return 0xff;
        return z->bus[z->ibus++];
    }

    z->dd0 = z->memory_read(z,PCr++);
    if (z->debug_on)
        z->debug(z, D_FETCH, z->dd0);
    return z->dd0;
}

static unsigned short z80_fetch_word(struct cpu* z) {
    unsigned short tmp = z80_fetch_byte(z);
    tmp |= z80_fetch_byte(z) << 8;
    return tmp;
}

unsigned short z80_mem_rword(struct cpu* z, unsigned short addr) {
    return z->memory_read(z, addr) | (z->memory_read(z, addr + 1) << 8);
}

void z80_mem_wword(struct cpu* z, unsigned short addr, unsigned short data) {
    z->memory_write(z, addr, data & 0xff);
    z->memory_write(z, addr + 1, data >> 8);
}

void z80_bank_rg(struct cpu* z, char i) {
    if (i) {
        z->BC_r = &z->BCs1;
        z->B_r = &z->Bs1;
        z->DE_r = &z->DEs1;
        z->C_r = &z->Cs1;
        z->HL_r = &z->HLs1;
        z->D_r = &z->Ds1;
        z->E_r = &z->Es1;
        z->H_r = &z->Hs1;
        z->L_r = &z->Ls1;
    }
    else {
        z->BC_r = &z->BCs0;
        z->B_r = &z->Bs0;
        z->DE_r = &z->DEs0;
        z->C_r = &z->Cs0;
        z->HL_r = &z->HLs0;
        z->D_r = &z->Ds0;
        z->E_r = &z->Es0;
        z->H_r = &z->Hs0;
        z->L_r = &z->Ls0;
    }
    z->alt_rg = i;
}

void z80_bank_af(struct cpu* z, char i) {
    if (i) {
        z->AF_r = &z->AFs1;
        z->A_r = &z->As1;
        z->F_r = &z->Fs1;
    }
    else {
        z->AF_r = &z->AFs0;
        z->A_r = &z->As0;
        z->F_r = &z->Fs0;
    }
    z->alt_af = i;
}

/* t - czas przyjęcia przerwania w taktach (NMI 11, IM1 13, IM2 19) */
void z80_icall(struct cpu* z, unsigned short addr, const int t) {
    z->ticks += t;
    z->_tic += t;
    z->cycles += t == 11 ? 5 : 7;
    z->memory_write(z, --SPr, PCr >> 8);
    z->memory_write(z, --SPr, PCr & 0xff);
    PCr = addr;
}

void z80_setregs(struct cpu* z) {
    z->AF_r = &z->AFs0;
    z->A_r = &z->As0;
    z->XH_r = &z->XHs;
    z->BC_r = &z->BCs0;
    z->B_r = &z->Bs0;
    z->XL_r = &z->XLs;
    z->DE_r = &z->DEs0;
    z->C_r = &z->Cs0;
    z->YH_r = &z->YHs;
    z->HL_r = &z->HLs0;
    z->D_r = &z->Ds0;
    z->YL_r = &z->YLs;
    z->IX_r = &z->IXs;
    z->E_r = &z->Es0;
    z->I_r = &z->Is;
    z->IY_r = &z->IYs;
    z->H_r = &z->Hs0;
    z->R_r = &z->Rs;
    z->PC_r = &z->PCs;
    z->L_r = &z->Ls0;
    z->SP_r = &z->SPs;
    z->F_r = &z->Fs0;

    z->MHL_r = &z->MHLs;
    z->MSP_r = &z->MSPs;
    z->MIX_r = &z->MIXs;
    z->MBC_r = &z->MBCs;
    z->MIY_r = &z->MIYs;
    z->MDE_r = &z->MDEs;
}

/************************************************************************************************/

void z80_nop(struct cpu* z) {
    z->cmd_type = D_NOP;
    TICS_UPDATE;
    INST_END;
}

void z80_halt(struct cpu* z) {
    z->cmd_type = D_HALT;
    TICS_UPDATE;
    if (z->int_off == false) {
        z->halt = 1;
    }
    INST_END;
}

void z80_pref_dd(struct cpu* z) {
    TICS_UPDATE;
    z->pref_dd_fd = 256;
    z->ipend = 1;
}

void z80_pref_fd(struct cpu* z) {
    TICS_UPDATE;
    z->pref_dd_fd = 512;
    z->ipend = 1;
}

void z80_pref_ed(struct cpu* z) {
    TICS_UPDATE;
    z->pref_cb_ed = 1536;
    z->ipend = 1;
}

void z80_pref_cb(struct cpu* z) {
    TICS_UPDATE;
    z->pref_cb_ed = 768;
    z->ipend = 1;
    if (z->pref_dd_fd)
        z->dd3 = z80_fetch_byte(z);
}

void z80_im(struct cpu* z) {
    z->cmd_type = D_IM;
    TICS_UPDATE;
    z->dd = z->imode = z->p0;
    INST_END;
}

void z80_di(struct cpu* z) {
    z->cmd_type = D_DI;
    z->iff1 = z->iff2 = 0;
    TICS_UPDATE;
    INST_END;
}

void z80_ei(struct cpu* z) {
    z->cmd_type = D_EI;
    z->iff1 = z->iff2 = 1;
    TICS_UPDATE;
    INST_END;
}

void z80_jr(struct cpu* z) {
    z->cmd_type = D_JR;
    z->cycles += 2;
    z->ticks += 7;
    z->_tic = 7;
    z->dd1 = z80_fetch_byte(z);
    z->reg80 = z->p1;
    z->nn = PCr + (char)z->dd1;
    if (!Z80_COND(z->p1, Fr)) {
        INST_END;
        return;
    }
    z->ticks += 5;
    z->_tic += 5;
    z->cycles++;
    z->memptr = z->nn;
    if (z->jmp_off == false) {
        PCr = z->nn;
    }
    INST_END;
}

void z80_jp(struct cpu* z) {
    z->cmd_type = D_JP;
    z->cycles += 3;
    z->ticks += 10;
    z->_tic = 10;
    z->dd2 = z80_fetch_word(z);
    z->reg80 = z->p1;
    z->nn = z->dd2;
    z->memptr = z->nn;
    if (!Z80_COND(z->p1, Fr)) {
        INST_END;
        return;
    }
    if (z->jmp_off == false) {
        PCr = z->nn;
    }
    INST_END;
}

void z80_jphl(struct cpu* z) {
    z->cmd_type = D_JPHL;
    z->cycles += 1;
    z->ticks += 4;
    z->_tic = 4;
    z->reg80 = z->p1;
    z->nn = z->dd2;
    if (z->jmp_off == false) {
        PCr = *(z->regs16[z->regx0 = z->p0]);
    }
    INST_END;
}

void z80_djnz(struct cpu* z) {
    z->cmd_type = D_DJNZ;
    z->dd1 = z80_fetch_byte(z);
    z->nn = PCr + (char)z->dd1;
    if (!--Br) {
        z->djnz = 0;
        z->cycles += 2;
        z->ticks += 8;
        z->_tic = 8;
        INST_END;
        return;
    }
    z->djnz = 1;
    z->memptr = z->nn;
    /* czas i cykle pętli z tablicy (TICS_UPDATE) */
    if (z->jmp_off == false)
        PCr = z->nn;
    TICS_UPDATE;
    INST_END;
}

void z80_call(struct cpu* z) {
    z->cmd_type = D_CALL;
    z->dd2 = z80_fetch_word(z);
    z->reg80 = z->p1;
    z->nn = z->dd2;
    z->memptr = z->nn;
    if (!Z80_COND(z->p1, Fr)) {
        z->cycles += 3;
        z->ticks += 10;
        z->_tic = 10;
        INST_END;
        return;
    }
    z->nested++;
    SPr -= 2;
    z->memory_write(z, SPr, PCr & 0xff);
    z->memory_write(z, SPr + 1, PCr >> 8);
    if (z->jmp_off == false)
        PCr = z->dd2;
    TICS_UPDATE;
    INST_END;
}

void z80_rst(struct cpu* z) {
    z->cmd_type = D_RST;
    z->memory_write(z, --SPr, PCr >> 8);
    z->memory_write(z, --SPr, PCr & 0xff);
    z->dd = z->p0;
    z->nested++;
    z->memptr = z->p0;
    if (z->jmp_off == false)
        PCr = (unsigned short)z->p0;
    TICS_UPDATE;
    INST_END;
}

void z80_ret(struct cpu* z) {
    z->cmd_type = D_RET;
    z->reg80 = z->p1;
    if (!Z80_COND(z->p1, Fr)) {
        z->cycles += 1;
        z->ticks += 5;
        z->_tic = 5;
        INST_END;
        return;
    }
    z->nested--;
    z->dd2 = z->memory_read(z, SPr);
    z->dd2 |= z->memory_read(z, SPr + 1) << 8;
    SPr += 2;
    z->memptr = z->dd2;
    if (z->jmp_off == false)
        PCr = z->dd2;
    TICS_UPDATE;
    INST_END;
}

void z80_reti(struct cpu* z) {
    z->cmd_type = D_RETI;
    z->dd2 = z->memory_read(z, SPr);
    z->dd2 |= z->memory_read(z, SPr + 1) << 8;
    SPr += 2;
    z->nested--;
    z->memptr = z->dd2;
    if (z->jmp_off == false)
        PCr = z->dd2;
    TICS_UPDATE;
    INST_END;
}

void z80_retn(struct cpu* z) {
    z->cmd_type = D_RETN;
    z->dd2 = z->memory_read(z, SPr);
    z->dd2 |= z->memory_read(z, SPr + 1) << 8;
    SPr += 2;
    z->memptr = z->dd2;
    if (z->jmp_off == false)
        PCr = z->dd2;
    z->nested--;
    z->iff1 = z->iff2;
    TICS_UPDATE;
    INST_END;
}

void z80_pop(struct cpu* z) {
    z->cmd_type = D_POP;
    z->dd2 = z->memory_read(z, SPr);
    z->dd2 |= z->memory_read(z, SPr + 1) << 8;
    SPr += 2;
    z->regx0 = z->p0;
    *(z->regs16[z->p0 & 0x7]) = z->dd2;
    TICS_UPDATE;
    INST_END;
}

void z80_push(struct cpu* z) {
    z->cmd_type = D_PUSH;
    z->dd2 = *(z->regs16[z->p0 & 0x7]);
    z->memory_write(z, --SPr, z->dd2 >> 8);
    z->memory_write(z, --SPr, z->dd2 & 0xff);
    z->regx0 = z->p0;
    TICS_UPDATE;
    INST_END;
}

void z80_exx(struct cpu* z) {
    z->cmd_type = D_EXX;
    TICS_UPDATE;
    z80_bank_rg(z, !z->alt_rg);
    INST_END;
}

void z80_ex(struct cpu* z) {
    unsigned char a, b;

    z->regx0 = z->p0;
    z->regx1 = z->p1;
    if (z->p0 == R0AF) {
        z->cmd_type = D_EX;
        TICS_UPDATE;
        z->dd2 = z->AFs0;
        z->AFs0 = z->AFs1;
        z->AFs1 = z->dd2;
        z->alt_af = !z->alt_af;
    }
    else if (z->p0 == RDE) {
        z->cmd_type = D_EX;
        TICS_UPDATE;
        z->dd2 = DEr;
        DEr = HLr;
        HLr = z->dd2;
    }
    else if (z->p0 == MSP) {
        z->cmd_type = D_EXSP;
        TICS_UPDATE;
        a = z->memory_read(z,SPr);
        b = z->memory_read(z,SPr + 1);
        z->dd2 = *(z->regs16[z->regx0 = z->p1]);
        z->memory_write(z,SPr + 1, z->dd2 >> 8);
        z->memory_write(z,SPr, z->dd2 & 0xff);
        *(z->regs16[z->p1]) = z->memptr = (unsigned short)((b << 8) | a);
    }
    INST_END;
}

void z80_ldb(struct cpu* z) {
    z->cmd_type = D_LDB;
    TICS_UPDATE;
    *(z->regs8[z->p0]) = *(z->regs8[z->p1]);
    z->reg80 = z->p0;
    z->reg81 = z->p1;
    INST_END;
}

void z80_ldbrm(struct cpu* z) {
    z->cmd_type = D_LDBRM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->p1]);
    if (z->pref_dd_fd)
        z->memptr = z->dd2 += (char)(z->dd = z80_fetch_byte(z));
    *(z->regs8[z->p0]) = z->memory_read(z, z->dd2);
    if (z->p1 == RBC || z->p1 == RDE)
        z->memptr = (unsigned short)(z->dd2 + 1);
    z->reg80 = z->p0;
    z->regx0 = z->p1;
    INST_END;
}

void z80_ldbmr(struct cpu* z) {
    z->cmd_type = D_LDBMR;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0 = z->p0]);
    if (z->pref_dd_fd)
        z->memptr = z->dd2 += (char)(z->dd = z80_fetch_byte(z));
    z->memory_write(z, z->dd2, *z->regs8[z->reg80 = z->p1]);
    if (z->p0 == RBC || z->p0 == RDE)
        z->memptr = (unsigned short)(((z->dd2 + 1) & 0xff) | (Ar << 8));
    INST_END;
}

void z80_ldbmn(struct cpu* z) {
    z->cmd_type = D_LDBMN;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0 = z->p0]);
    if (z->pref_dd_fd)
        z->memptr = z->dd2 += (char)(z->dd = z80_fetch_byte(z));
    z->memory_write(z, z->dd2, z->reg80 = z80_fetch_byte(z));
    INST_END;
}

void z80_ldbmnr(struct cpu* z) {
    z->cmd_type = D_LDBMNR;
    TICS_UPDATE;
    z->memory_write(z, z->nn = z80_fetch_word(z), *(z->regs8[z->reg80 = z->p1]));
    z->memptr = (unsigned short)(((z->nn + 1) & 0xff) | (Ar << 8));
    INST_END;
}

void z80_ldbrmn(struct cpu* z) {
    z->cmd_type = D_LDBRMN;
    TICS_UPDATE;
    *(z->regs8[z->reg80 = z->p0]) = z->memory_read(z, z->nn = z80_fetch_word(z));
    z->memptr = (unsigned short)(z->nn + 1);
    INST_END;
}

void z80_ldbn(struct cpu* z) {
    z->cmd_type = D_LDBN;
    TICS_UPDATE;
    *(z->regs8[z->reg80 = z->p0]) = z->dd = z80_fetch_byte(z);
    INST_END;
}

void z80_ldb_(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_LDB;
    TICS_UPDATE;
    *(z->regs8[z->reg80 = z->p0]) = *(z->regs8[z->reg81 = z->p1]);
    SET_FLAGSP(Ar, 0)
    Fr = (Fr & ~FP) | (z->iff2 ? FP : 0);
    INST_END;
}

void z80_ldw(struct cpu* z) {
    z->cmd_type = D_LDW;
    TICS_UPDATE;
    *(z->regs16[z->regx0 = z->p0]) = (*z->regs16[z->regx1 = z->p1]);
    INST_END;
}

void z80_ldwn(struct cpu* z) {
    z->cmd_type = D_LDWN;
    TICS_UPDATE;
    *(z->regs16[z->regx0 = z->p0]) = z->nn = z80_fetch_word(z);
    INST_END;
}

void z80_ldwrmn(struct cpu* z) {
    z->cmd_type = D_LDWRMN;
    TICS_UPDATE;
    *(z->regs16[z->regx0 = z->p0]) = z80_mem_rword(z, z->nn = z80_fetch_word(z));
    z->memptr = (unsigned short)(z->nn + 1);
    INST_END;
}

void z80_ldwmnr(struct cpu* z) {
    z->cmd_type = D_LDWMNR;
    TICS_UPDATE;
    z80_mem_wword(z, z->nn = z80_fetch_word(z), *(z->regs16[z->regx0 = z->p1]));
    z->memptr = (unsigned short)(z->nn + 1);
    INST_END;
}

void z80_scf(struct cpu* z) {
    z->cmd_type = D_SCF;
    TICS_UPDATE;
    /* flagi 5 i 3: (Q ^ F) | A, gdzie Q to flagi zapisane przez poprzednią instrukcję */
    Fr = (Fr & (FS | FZ | FP)) | FC | (((z->q ^ Fr) | Ar) & (F5 | F3));
    INST_END;
}

void z80_ccf(struct cpu* z) {
    z->cmd_type = D_CCF;
    TICS_UPDATE;
    Fr = (Fr & (FS | FZ | FP)) | ((Fr & FC) ? FH : FC) | (((z->q ^ Fr) | Ar) & (F5 | F3));
    INST_END;
}

void z80_cpl(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_CPL;
    TICS_UPDATE;
    Ar = ~Ar;
    SET_FLAGSP(Ar, 0)
    INST_END;
}

void z80_daa(struct cpu* z) {
    unsigned char ftmp = 0, a = Ar, corr = 0, fl = 0;
    z->cmd_type = D_DAA;
    TICS_UPDATE;

    if ((Fr & FH) || (a & 0x0f) > 0x09)
        corr |= 0x06;
    if ((Fr & FC) || a > 0x99) {
        corr |= 0x60;
        fl = FC;
    }
    if (Fr & FN) {
        Ar = (unsigned char)(a - corr);
        if ((Fr & FH) && (a & 0x0f) < 0x06)
            fl |= FH;
    }
    else {
        Ar = (unsigned char)(a + corr);
        if ((a & 0x0f) > 0x09)
            fl |= FH;
    }

    SET_FLAGSP(Ar, fl)
    INST_END;
}

void z80_bit(struct cpu* z) {
    unsigned char u;
    z->cmd_type = D_BIT;
    TICS_UPDATE;

    u = *(z->regs8[z->reg80 = z->p1]);
    Fr = (Fr & 0x01) | FH | ((u & z->p0) ? 0 : FZ + FP) | (u & 0x80 & z->p0) | (u & (F5 | F3));

    INST_END;
}

void z80_bitm(struct cpu* z) {
    unsigned char u;
    z->cmd_type = D_BITM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0 = z->p1]);
    if (z->pref_dd_fd)
        z->memptr = z->dd2 += (char)(z->dd = z->dd3);

    u = z->memory_read(z, z->dd2);
    /* flagi 5 i 3 pochodzą z górnego bajtu MEMPTR */
    Fr = (Fr & 0x01) | FH | ((u & z->p0) ? 0 : FZ + FP) | (u & 0x80 & z->p0) | ((z->memptr >> 8) & (F5 | F3));
    z->reg81 = z->p2;

    INST_END;
}

void z80_res(struct cpu* z) {
    z->cmd_type = D_RES;
    TICS_UPDATE;
    *(z->regs8[z->reg80 = z->p1]) &= z->reg81 = z->p0;
    INST_END;
}

void z80_resm(struct cpu* z) {
    z->cmd_type = D_RESM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0 = z->p1]);
    if (z->pref_dd_fd)
        z->memptr = z->dd2 += (char)(z->dd = z->dd3);
    z->memory_write(z, z->dd2, z->memory_read(z, z->dd2) & z->p0);
    z->reg81 = z->p2;
    INST_END;
}

void z80_set(struct cpu* z) {
    z->cmd_type = D_SET;
    TICS_UPDATE;
    *(z->regs8[z->reg80 = z->p1]) |= z->reg81 = z->p0;
    INST_END;
}

void z80_setm(struct cpu* z) {
    z->cmd_type = D_SETM;
    TICS_UPDATE;

    z->dd2 = *(z->regs16[z->regx0 = z->p1]);
    if (z->pref_dd_fd)
        z->memptr = z->dd2 += (char)(z->dd = z->dd3);
    z->memory_write(z, z->dd2, z->memory_read(z, z->dd2) | z->p0);
    z->reg81 = z->p2;
    INST_END;
}

void z80_and(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_AND;
    TICS_UPDATE;
    Ar &= *(z->regs8[z->reg80 = z->p1]);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_andn(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_ANDN;
    TICS_UPDATE;
    Ar &= z->dd = z80_fetch_byte(z);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_andm(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_ANDM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0 = z->p1]);
    if (z->pref_dd_fd)
        z->memptr = z->dd2 += (char)(z->dd = z80_fetch_byte(z));
    Ar &= z->memory_read(z, z->dd2);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_or(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_OR;
    TICS_UPDATE;
    Ar |= *(z->regs8[z->reg80 = z->p1]);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_orn(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_ORN;
    TICS_UPDATE;
    Ar |= z->dd = z80_fetch_byte(z);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_orm(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_ORM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0 = z->p1]);
    if (z->pref_dd_fd)
        z->memptr = z->dd2 += (char)(z->dd = z80_fetch_byte(z));
    Ar |= z->memory_read(z, z->dd2);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_xor(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_XOR;
    TICS_UPDATE;
    Ar ^= *(z->regs8[z->reg80 = z->p1]);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_xorn(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_XORN;
    TICS_UPDATE;
    Ar ^= z->dd = z80_fetch_byte(z);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_xorm(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_XORM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0 = z->p1]);
    if (z->pref_dd_fd)
        z->memptr = z->dd2 += (char)(z->dd = z80_fetch_byte(z));
    Ar ^= z->memory_read(z, z->dd2);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_rlc(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_RLC;
    TICS_UPDATE;

    GET_REG8(z->dd1);
    z->dd1 = ROL_L(z->dd1, z->dd1 >> 7);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, z->dd1 & 0x01);
    INST_END;
}

void z80_rlcm(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_RLCM;
    TICS_UPDATE;

    GET_MEM(z->dd1);
    z->dd1 = ROL_L(z->dd1, z->dd1 >> 7);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, z->dd1 & 0x01);
    INST_END;
}

void z80_rrc(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_RRC;
    TICS_UPDATE;

    GET_REG8(z->dd1);
    z->dd1 = ROL_R(z->dd1, z->dd1 << 7);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd1 >> 7) & 1);
    INST_END;
}

void z80_rrcm(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_RRC;
    TICS_UPDATE;

    GET_MEM(z->dd1);
    z->dd1 = ROL_R(z->dd1, z->dd1 << 7);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd1 >> 7) & 1);
    INST_END;
}

void z80_rls(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_RL;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_L(z->dd3, Fr);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
}

void z80_rlsm(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_RLM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_L(z->dd3, Fr);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
}

void z80_rr(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_RR;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_R(z->dd3, Fr << 7);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
}

void z80_rrm(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_RRM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_R(z->dd3, Fr << 7);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
}

void z80_sla(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_SLA;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_L(z->dd3, 0);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
}

void z80_slam(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_SLAM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_L(z->dd3, 0);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
}

void z80_sra(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_SRA;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_R(z->dd3, z->dd3);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
}

void z80_sram(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_SRAM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_R(z->dd3, z->dd3);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
}

void z80_sli(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_SLI;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_L(z->dd3, 1);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
}

void z80_slim(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_SLIM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_L(z->dd3, 1);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
}

void z80_srl(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_SRL;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_R(z->dd3, 0);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
}

void z80_srlm(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_SRLM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_R(z->dd3, 0);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
}

void z80_rrd(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_RRD;
    TICS_UPDATE;
    z->memptr = (unsigned short)(HLr + 1);
    z->dd1 = z->memory_read(z,HLr);
    z->dd3 = z->dd1;
    z->dd1 >>= 4;
    z->dd3 &= 0x0f;
    z->dd1 = (z->dd1 & 0x0f) | ((Ar << 4) & 0xf0);
    Ar = (Ar & 0xf0) | z->dd3;
    z->memory_write(z,HLr, z->dd1);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_rld(struct cpu* z) {
    unsigned char ftmp = 0;
    z->cmd_type = D_RLD;
    TICS_UPDATE;

    z->memptr = (unsigned short)(HLr + 1);
    z->dd1 = z->memory_read(z,HLr);
    z->dd3 = z->dd1;
    z->dd1 <<= 4;
    z->dd3 >>= 4;
    z->dd1 = (z->dd1 & 0xf0) | (Ar & 0x0f);
    Ar = (Ar & 0xf0) | z->dd3;
    z->memory_write(z,HLr, z->dd1);
    SET_FLAGSP(Ar, 0);
    INST_END;
}

void z80_outa(struct cpu* z) {
    z->cmd_type = D_OUTA;
    TICS_UPDATE;
    z->io_write(z, z->last_out_addr = ((z->dd = z80_fetch_byte(z)) & 0xff) | (Ar << 8), z->last_out_data = Ar);
    z->memptr = (unsigned short)(((z->dd + 1) & 0xff) | (Ar << 8));
    INST_END;
}

void z80_ina(struct cpu* z) {
    z->cmd_type = D_INA;
    TICS_UPDATE;
    z->memptr = (unsigned short)(((z->dd = z80_fetch_byte(z)) & 0xff) | (Ar << 8)) + 1;
    z->last_in_data = Ar = z->io_read(z, z->last_in_addr = (unsigned short)(z->memptr - 1));
    if (z->force_in > 0)
        z->last_in_data = Ar = (unsigned char)z->force_in;
    INST_END;
}

void z80_inbc(struct cpu* z) {
    unsigned char ftmp = 0, v;
    z->cmd_type = D_INBC;
    TICS_UPDATE;
    z->memptr = (unsigned short)(BCr + 1);
    z->last_in_data = v = z->io_read(z, z->last_in_addr = BCr);
    if (z->force_in > 0)
        z->last_in_data = v = (unsigned char)z->force_in;
    z->reg80 = z->p0;
    if (z->p0 != RF) /* IN F,(C) tylko ustawia znaczniki */
        *(z->regs8[z->p0]) = v;
    SET_FLAGSP(v, 0)
    INST_END;
}

void z80_outbc(struct cpu* z) {
    z->cmd_type = D_OUTBC;
    TICS_UPDATE;
    z->io_write(z, z->last_out_addr = BCr, z->last_out_data = *(z->regs8[z->reg80 = z->p1]));
    z->memptr = (unsigned short)(BCr + 1);
    INST_END;
}

void z80_add8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_ADD8;
    TICS_UPDATE;

    VGET_REG8(b);
    Ar = z80_alu8(z, 0, Ar, b, 0);

    INST_END;
}

void z80_addn8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_ADDN8;
    TICS_UPDATE;

    VGET_CNS8(b);
    Ar = z80_alu8(z, 0, Ar, b, 0);

    INST_END;
}

void z80_addm8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_ADDM8;
    TICS_UPDATE;

    VGET_MEM8(b);
    Ar = z80_alu8(z, 0, Ar, b, 0);

    INST_END;
}

void z80_adc8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_ADC8;
    TICS_UPDATE;

    VGET_REG8(b);
    Ar = z80_alu8(z, 0, Ar, b, Fr & FC);

    INST_END;
}

void z80_adcn8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_ADCN8;
    TICS_UPDATE;

    VGET_CNS8(b);
    Ar = z80_alu8(z, 0, Ar, b, Fr & FC);

    INST_END;
}

void z80_adcm8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_ADCM8;
    TICS_UPDATE;

    VGET_MEM8(b);
    Ar = z80_alu8(z, 0, Ar, b, Fr & FC);

    INST_END;
}

void z80_sub8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_SUB8;
    TICS_UPDATE;

    VGET_REG8(b);
    Ar = z80_alu8(z, 1, Ar, b, 0);

    INST_END;
}

void z80_subn8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_SUBN8;
    TICS_UPDATE;

    VGET_CNS8(b);
    Ar = z80_alu8(z, 1, Ar, b, 0);

    INST_END;
}

void z80_subm8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_SUBM8;
    TICS_UPDATE;

    VGET_MEM8(b);
    Ar = z80_alu8(z, 1, Ar, b, 0);

    INST_END;
}

void z80_sbc8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_SBC8;
    TICS_UPDATE;

    VGET_REG8(b);
    Ar = z80_alu8(z, 1, Ar, b, Fr & FC);

    INST_END;
}

void z80_sbcn8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_SBCN8;
    TICS_UPDATE;

    VGET_CNS8(b);
    Ar = z80_alu8(z, 1, Ar, b, Fr & FC);

    INST_END;
}

void z80_sbcm8(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_SBCM8;
    TICS_UPDATE;

    VGET_MEM8(b);
    Ar = z80_alu8(z, 1, Ar, b, Fr & FC);

    INST_END;
}

void z80_cp(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_CP;
    TICS_UPDATE;

    VGET_REG8(b);
    z80_alu8(z, 1, Ar, b, 0);
    Fr = (Fr & 0xd7) | (b & 0x28);

    INST_END;
}

void z80_cpn(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_CPN;
    TICS_UPDATE;

    VGET_CNS8(b);
    z80_alu8(z, 1, Ar, b, 0);
    Fr = (Fr & 0xd7) | (b & 0x28);

    INST_END;
}

void z80_cpm(struct cpu* z) {
    unsigned char b;
    z->cmd_type = D_CPM;
    TICS_UPDATE;

    VGET_MEM8(b);
    z80_alu8(z, 1, Ar, b, 0);
    Fr = (Fr & 0xd7) | (b & 0x28);

    INST_END;
}

void z80_inc8(struct cpu* z) {
    unsigned char a;
    z->cmd_type = D_INC8;
    TICS_UPDATE;

    VGET_REG8(a);
    VSET_REG8(z80_alu8(z, 0, a, 1, 0));

    INST_END;
}

void z80_incm8(struct cpu* z) {
    unsigned char a;
    z->cmd_type = D_INCM8;
    TICS_UPDATE;

    VGET_MEM8(a);
    VSET_MEM8(z80_alu8(z, 0, a, 1, 0));

    INST_END;
}

void z80_dec8(struct cpu* z) {
    unsigned char a;
    z->cmd_type = D_DEC8;
    TICS_UPDATE;

    VGET_REG8(a);
    VSET_REG8(z80_alu8(z, 1, a, 1, 0));

    INST_END;
}

void z80_decm8(struct cpu* z) {
    unsigned char a;
    z->cmd_type = D_DECM8;
    TICS_UPDATE;

    VGET_MEM8(a);
    VSET_MEM8(z80_alu8(z, 1, a, 1, 0));

    INST_END;
}

void z80_neg(struct cpu* z) {
    z->cmd_type = D_NEG;
    TICS_UPDATE;

    Ar = z80_alu8(z, 1, 0, Ar, 0);

    INST_END;
}

void z80_incx(struct cpu* z) {
    z->cmd_type = D_INCX;
    TICS_UPDATE;
    (*z->regs16[z->regx0 = z->p0])++;
    INST_END;
}

void z80_decx(struct cpu* z) {
    z->cmd_type = D_DECX;
    TICS_UPDATE;
    (*z->regs16[z->regx0 = z->p0])--;
    INST_END;
}

void z80_addx(struct cpu* z) {
    unsigned int a = *(z->regs16[z->regx0 = z->p0]), b = *(z->regs16[z->regx1 = z->p1]);
    z->cmd_type = D_ADDX;
    TICS_UPDATE;

    z->memptr = (unsigned short)(a + 1);
    *(z->regs16[z->p0]) = z80_alu16(z, 0, a, b);

    INST_END;
}

void z80_adcx(struct cpu* z) {
    unsigned int a = *(z->regs16[z->regx0 = z->p0]), b = *(z->regs16[z->regx1 = z->p1]);
    z->cmd_type = D_ADCX;
    TICS_UPDATE;

    z->memptr = (unsigned short)(a + 1);
    *(z->regs16[z->p0]) = z80_alu16(z, 1, a, b);

    INST_END;
}

void z80_sbcx(struct cpu* z) {
    unsigned int a = *(z->regs16[z->regx0 = z->p0]), b = *(z->regs16[z->regx1 = z->p1]);
    z->cmd_type = D_SBCX;
    TICS_UPDATE;

    z->memptr = (unsigned short)(a + 1);
    *(z->regs16[z->p0]) = z80_alu16(z, 2, a, b);

    INST_END;
}

void z80_ldbl(struct cpu* z) {
    const bool inc = (z->p0 == PI || z->p0 == PIR);
    const bool rep = (z->p0 == PIR || z->p0 == PDR);
    bool again;
    unsigned char v;
    z->cmd_type = D_LDBL;
    TICS_UPDATE;
    z->dd = z->p0;
    if (rep) { /* tablica zawiera czas obiegu powtarzanego, ostatni obieg trwa o 5 taktów krócej */
        z->ticks -= 5;
        z->cycles -= 1;
        z->_tic -= 5;
    }
    if (z->jmp_off == false)
        do {
            unsigned char n;
            z->memory_write(z, DEr, z->dd3 = v = z->memory_read(z, HLr));
            if (inc)
                HLr++, DEr++;
            else
                HLr--, DEr--;
            --BCr;
            n = (unsigned char)(v + Ar);
            Fr = (Fr & (FS | FZ | FC)) | (n & F3) | ((n << 4) & F5) | (BCr ? FP : 0);
            again = rep && BCr;
            if (again)
                z->memptr = (unsigned short)(z->adrs + 1);
            if (again) {
                z->ticks += 21;
                z->cycles += 5;
                z->_tic += 21;
            }
        }
        while (again);
    INST_END;
}

void z80_cpbl(struct cpu* z) {
    const bool inc = (z->p0 == PI || z->p0 == PIR);
    const bool rep = (z->p0 == PIR || z->p0 == PDR);
    bool again;
    unsigned char v;
    z->cmd_type = D_CPBL;
    TICS_UPDATE;
    z->dd = z->p0;
    if (rep) { /* tablica zawiera czas obiegu powtarzanego, ostatni obieg trwa o 5 taktów krócej */
        z->ticks -= 5;
        z->cycles -= 1;
        z->_tic -= 5;
    }
    if (z->jmp_off == false)
        do {
            unsigned char a = Ar, r, n, f;
            v = z->memory_read(z, HLr);
            r = (unsigned char)(a - v);
            f = (r & FS) | (r ? 0 : FZ) | ((a ^ v ^ r) & FH) | FN;
            n = (unsigned char)(r - ((f & FH) ? 1 : 0));
            f |= (n & F3) | ((n << 4) & F5);
            if (inc)
                HLr++;
            else
                HLr--;
            --BCr;
            if (BCr)
                f |= FP;
            Fr = f | (Fr & FC);
            again = rep && BCr && !(f & FZ);
            if (again)
                z->memptr = (unsigned short)(z->adrs + 1);
            else
                z->memptr += inc ? 1 : -1;
            if (again) {
                z->ticks += 21;
                z->cycles += 5;
                z->_tic += 21;
            }
        }
        while (again);
    INST_END;
}

void z80_inbl(struct cpu* z) {
    const bool inc = (z->p0 == PI || z->p0 == PIR);
    const bool rep = (z->p0 == PIR || z->p0 == PDR);
    bool again;
    unsigned char v;
    z->cmd_type = D_INBL;
    TICS_UPDATE;
    z->dd = z->p0;
    if (rep) { /* tablica zawiera czas obiegu powtarzanego, ostatni obieg trwa o 5 taktów krócej */
        z->ticks -= 5;
        z->cycles -= 1;
        z->_tic -= 5;
    }
    if (z->jmp_off == false)
        do {
            unsigned short k;
            z->memptr = (unsigned short)(BCr + (inc ? 1 : -1));
            z->memory_write(z, HLr, z->last_in_data = z->dd3 = v = z->io_read(z, z->last_in_addr = BCr));
            if (inc)
                HLr++;
            else
                HLr--;
            --Br;
            k = (unsigned short)(v + ((Cr + (inc ? 1 : -1)) & 0xff));
            Fr = (Br & (FS | F5 | F3)) | (Br ? 0 : FZ) | ((v & 0x80) ? FN : 0) | (k > 255 ? FH | FC : 0) |
                 ((__z80_parity__[(k & 7) ^ Br] << 2) & FP);
            again = rep && Br;
            if (again) {
                z->ticks += 21;
                z->cycles += 5;
                z->_tic += 21;
            }
        }
        while (again);
    INST_END;
}

void z80_outbl(struct cpu* z) {
    const bool inc = (z->p0 == PI || z->p0 == PIR);
    const bool rep = (z->p0 == PIR || z->p0 == PDR);
    bool again;
    unsigned char v;
    z->cmd_type = D_OUTBL;
    TICS_UPDATE;
    z->dd = z->p0;
    if (rep) { /* tablica zawiera czas obiegu powtarzanego, ostatni obieg trwa o 5 taktów krócej */
        z->ticks -= 5;
        z->cycles -= 1;
        z->_tic -= 5;
    }
    if (z->jmp_off == false)
        do {
            unsigned short k;
            v = z->memory_read(z, HLr);
            --Br;
            z->io_write(z, z->last_out_addr = BCr, z->last_out_data = v);
            z->memptr = (unsigned short)(BCr + (inc ? 1 : -1));
            if (inc)
                HLr++;
            else
                HLr--;
            k = (unsigned short)(v + (HLr & 0xff));
            Fr = (Br & (FS | F5 | F3)) | (Br ? 0 : FZ) | ((v & 0x80) ? FN : 0) | (k > 255 ? FH | FC : 0) |
                 ((__z80_parity__[(k & 7) ^ Br] << 2) & FP);
            again = rep && Br;
            if (again) {
                z->ticks += 21;
                z->cycles += 5;
                z->_tic += 21;
            }
        }
        while (again);
    INST_END;
}

/*************************************************************************************************/

void z80_reset(struct cpu* z) {
    unsigned int i;

    for (i = 0; i < Z80_DATA_SIZE; i++)
        z->z80_data[i] = 0;
    z80_setregs(z);
    z80_bank_rg(z, 0);
    z80_bank_af(z, 0);
    for (i = 0; i < 8; i++)
        z->bus[i] = 0xff;
    z->force_in = -1;
}

void z80_nmi(struct cpu* z) {
    if (z->int_off == false) {
        z->inmi = 1;
    }
}

void z80_int(struct cpu* z, const unsigned char* bus) {
    if (z->int_off == false) {
        z->intr = 1;
        z->int_t = 32;
        if (bus)
            memcpy(z->bus, bus, sizeof(z->bus));
        else
            memset(z->bus, 0xff, sizeof(z->bus));
    }
}

void z80_loop(struct cpu* z, int cnt, int call, void (*callback)(struct cpu*, void*), void* callback_param)
{
    if (z->cpu_off) {
        return;
    }
    unsigned char d;
    /* i - zużyte takty; każda instrukcja jest rozliczana raz (_tic) przed następną */
    for (int i = 0; i < cnt;) {
        if (!z->exit) {
            z->call_cnt += z->_tic;
            /* gwarancja postępu, gdy instrukcja nie zgłosiła czasu */
            i += z->_tic ? z->_tic : 1;
            z->_tic = 0;
            if (z->call_cnt >= call) {
                if (callback) {
                    callback(z, callback_param);
                }
                z->call_cnt = z->call_cnt - call;
            }
        }
        if (z->debug_on && z->debug) {
            z->debug(z, D_PRE, 0);
        }
        if (z->exit) {
            return;
        }
        *z->R_r = ((*z->R_r) + 1) & 0x7f;
        if (!z->halt) {
            if (!z->ipend) {
                z->adrs = *z->PC_r;
            }
            d = z80_fetch_byte(z);
        } else {
            d = 0;
        }

        struct z80dt xf = __z80_decoder__[d + z->pref_dd_fd + z->pref_cb_ed];
        z->p0 = xf.reg0;
        z->b = xf.bytes;
        z->a = xf.f_mask_and;
        z->p1 = xf.reg1;
        z->c = xf.cycles;
        z->o = xf.f_mask_or;
        z->p2 = xf.reg2;
        z->t = xf.ticks;
        z->x = xf.f_mask_chg;
        xf.code(z);

        if (!z->ipend) {
            /* Q: flagi zapisane przez tę instrukcję, lub 0 jeśli ich nie zmieniała (dla SCF/CCF) */
            z->q = (xf.f_mask_chg && z->cmd_type != D_POP) ? Fr : 0;
            /* zabronienie przerwania, gdy instrukcja jest w trakcie wykonywania */
            if (z->inmi) {
                z->halt = 0;
                z->inmi = 0;
                z->iff2 = z->iff1;
                z->iff1 = 0;
                z80_icall(z, 0x66, 11);
            }
            /* po EI przerwanie dopiero po następnej instrukcji */
            if (z->intr && z->cmd_type != D_EI) {
                if (z->iff1) {
                    z->halt = 0; /* przy DI HALT trwa do NMI/reset */
                    z->intr = 0;
                    switch (z->imode) {
                    case 0: z->irupt = 1;
                        break; /* następna instrukcja będzie pobrana z magistrali */
                    case 1:
                        z80_icall(z, 0x38, 13);
                        break;
                    case 2:
                        z80_icall(z, z80_mem_rword(
                                      z, (unsigned short)(*z->I_r << 8) | (z->bus[0] & 0xff)), 19);
                        break;
                    }
                }
            }
        }
        /* linia INT trwa ok. 32 taktów, potem przerwanie niezaakceptowane znika */
        if (z->intr && (z->int_t -= z->_tic) <= 0)
            z->intr = 0;
        if (!z->halt && z->debug_on && z->debug) {
            z->debug(z, D_FLUSH, 0);
        }
    }
}

void z80_step(struct cpu* z)
{
    if (z->cpu_off) {
        return;
    }
    if (z->debug_on && z->debug) {
        z->debug(z, D_PRE, 0);
    }
    *z->R_r = ((*z->R_r) + 1) & 0x7f;
    unsigned char d = 0;
    if (!z->halt) {
        if (!z->ipend) {
            z->adrs = *z->PC_r;
        }
        d = z80_fetch_byte(z);
    }

    const struct z80dt xf = __z80_decoder__[d + z->pref_dd_fd + z->pref_cb_ed];
    z->p0 = xf.reg0;
    z->b = xf.bytes;
    z->a = xf.f_mask_and;
    z->p1 = xf.reg1;
    z->c = xf.cycles;
    z->o = xf.f_mask_or;
    z->p2 = xf.reg2;
    z->t = xf.ticks;
    z->x = xf.f_mask_chg;
    xf.code(z);

    if (!z->ipend) {
        z->q = (xf.f_mask_chg && z->cmd_type != D_POP) ? Fr : 0;
        /* zabronienie przerwania, gdy instrukcja jest w trakcie wykonywania */
        if (z->inmi) {
            z->halt = 0;
            z->inmi = 0;
            z->iff2 = z->iff1;
            z->iff1 = 0;
            z80_icall(z, 0x66, 11);
        }
        /* po EI przerwanie dopiero po następnej instrukcji */
        if (z->intr && z->cmd_type != D_EI) {
            if (z->iff1) {
                z->halt = 0; /* przy DI HALT trwa do NMI/reset */
                z->intr = 0;
                switch (z->imode) {
                case 0: z->irupt = 1;
                    break; /* następna instrukcja będzie pobrana z magistrali */
                case 1:
                    z80_icall(z, 0x38, 13);
                    break;
                case 2:
                    z80_icall(z, z80_mem_rword(
                                  z, (unsigned short)(*z->I_r << 8) | (z->bus[0] & 0xff)), 19);
                    break;
                }
            }
        }
    }

    if (z->intr && (z->int_t -= z->_tic) <= 0)
        z->intr = 0;

    if (!z->halt && z->debug_on && z->debug)
        z->debug(z, D_FLUSH, 0);
}

void z80_init(struct cpu* z, const memory_read_t memory_read, const memory_write_t memory_write,
    const io_read_t io_read, const io_write_t io_write, void* ptr, void* ptx)
{
    z->memory_read = memory_read;
    z->memory_write = memory_write;
    z->io_read = io_read;
    z->io_write = io_write;
    z->ptr = ptr;
    z->ptx = ptx;
    z80_reset(z);
}

/*

         _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_
        /_                                       /
       /_/       Z80 CPU                        /|
      /_*______________________________________/
       / / / / / / / / / / / / / / / / / / / /
       | | | | | | | | | | | | | | | | | | | |

*/
