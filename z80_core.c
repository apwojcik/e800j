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
    z80_core.c
    Implementacja realizacji emulacji rozkazów

*/
#include "z80_cpu.h"
#include "z80_core.h"
#include "z80_dtbl.h"

#define Z80_COND(cnd,freg) 	((__z80_cond_mask[cnd] & (freg | F5)) ^ __z80_cond_plrt[cnd])
#define TICS_UPDATE 		z->cycles += z->c;   z->ticks  += z->t; z->_tic = z->t;
#define INST_END    		z->pref_dd_fd = z->pref_cb_ed = 0; z->ipend=0; z->irupt=0; z->ibus=0;
#define GET_REG8(v)		v = *(z->regs8[z->reg80 = z->p0])
#define SET_REG8(v)		*(z->regs8[z->p0]) = v
#define SET_MEM(v)		z->mem_wr(z,z->dd2,v)
#define ROL_L(reg,bit_0)	((((reg) << 1) & 0xfe) | ((bit_0) & 0x1))
#define ROL_R(reg,bit_0)	((((reg) >> 1) & 0x7f) | ((bit_0) & 0x80))
#define VGET_REG8(b)		b = *(z->regs8[z->reg80=z->p1]);
#define VGET_CNS8(b)		b = z->dd=z80_fetch_byte(z);
#define VSET_REG8(a)		*(z->regs8[z->p1]) = a
#define VSET_MEM8(a)		z->mem_wr(z,z->dd2,a)

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
        ftmp = ((~(reg) & ((reg) - 1)) >> 1) & 0x40;\
        ftmp |= ((reg) & 0xa800) >> 8;\
        ftmp |= flags & 0x15;\
            ftmp |= z->o;\
        ftmp &= ~z->a;\
        Fr   &= ~z->x;\
        Fr   |= z->x & ftmp;

#define GET_MEM(v) \
        z->dd2 = *z->regs16[z->regx0 = z->p0];\
        if(z->pref_dd_fd) z->dd2 += (char)(z->dd=z->dd3);\
        v = z->mem_rd(z,z->dd2);

#define ALU_INDEX_8(r,a,b,c)  \
        c = (((b) >> 3) & 0x11) | (((a) >> 2) & 0x22) | (((r) >> 1) & 0x44); \
        c = ((c) & 0x07) | (((c) >> 1) & 0x38);

#define ALU_INDEX_X(r,a,b,c)  \
        c = (((b) >> 11) & 0x11) | (((a) >> 10) & 0x22) | (((r) >> 9) & 0x44); \
        c = ((c) & 0x07) | (((c) >> 1) & 0x38);

#define VGET_MEM8(b) \
        z->dd2 = *z->regs16[z->regx0=z->p1]; \
        if(z->pref_dd_fd) z->dd2 += (char)(z->dd=z80_fetch_byte(z));\
        b = z->mem_rd(z,z->dd2);

/* tablice stałych */
static const z80dt __z80_decoder__[]  = Z80_DECODE_TABLE;
static const uchar __z80_parity__[]   = Z80_PARITY_TABLE;
static const uchar __z80_cond_mask[]  = Z80_COND_MASK;
static const uchar __z80_cond_plrt[]  = Z80_COND_PLRT;
static const uchar __z80_add_cy_tbl[] = Z80_ADD_CARRY_TBL;
static const uchar __z80_sub_cy_tbl[] = Z80_SUB_CARRY_TBL;

/**************************************************************************************************/

uchar  z80_fetch_byte(z80 *z)
{
    if(z->irupt){
        if(z->ibus >= 8) return 0xff;
        return z->bus[z->ibus++];
    }

    z->dd0 = z->mem_rd(z,PCr++);
    if(z->dbg_on) z->debug(z, D_FETCH, z->dd0);
    return z->dd0;
}

ushort  z80_fetch_word(z80 *z)
{
    ushort tmp = z80_fetch_byte(z);
    tmp |= z80_fetch_byte(z) << 8;
    return tmp;
}

ushort  z80_mem_rword(z80 *z, ushort addr)
{
    return z->mem_rd(z,addr) | (z->mem_rd(z,addr+1) << 8);
}

void  z80_mem_wword(z80 *z, ushort addr, ushort data)
{
    z->mem_wr(z,addr,data & 0xff);
    z->mem_wr(z,addr + 1, data >> 8);
}

void  z80_bank_rg(z80 *z, char i)
{
    if(i){
        z->BC_r = &z->BCs1;		z->B_r = &z->Bs1;
        z->DE_r = &z->DEs1;		z->C_r = &z->Cs1;
        z->HL_r = &z->HLs1;		z->D_r = &z->Ds1;
        z->E_r  = &z->Es1;		z->H_r = &z->Hs1;
        z->L_r  = &z->Ls1;
    } else {
        z->BC_r = &z->BCs0;		z->B_r = &z->Bs0;
        z->DE_r = &z->DEs0;		z->C_r = &z->Cs0;
        z->HL_r = &z->HLs0;		z->D_r = &z->Ds0;
        z->E_r  = &z->Es0;		z->H_r = &z->Hs0;
        z->L_r  = &z->Ls0;
    }
    z->alt_rg = i;
}

void  z80_bank_af(z80 *z, char i)
{
    if(i){
        z->AF_r = &z->AFs1;
        z->A_r = &z->As1;
        z->F_r = &z->Fs1;
    } else {
        z->AF_r = &z->AFs0;
        z->A_r = &z->As0;
        z->F_r = &z->Fs0;

    }
    z->alt_af = i;
}

void  z80_icall(z80 *z, ushort addr )
{
    z->mem_wr(z, --SPr, PCr >> 8);
    z->mem_wr(z, --SPr, PCr & 0xff);
    PCr = addr;
};

void  z80_setregs(z80 *z)
{
    z->AF_r = &z->AFs0;		z->A_r = &z->As0;	z->XH_r = &z->XHs;
    z->BC_r = &z->BCs0;		z->B_r = &z->Bs0;	z->XL_r = &z->XLs;
    z->DE_r = &z->DEs0;		z->C_r = &z->Cs0;	z->YH_r = &z->YHs;
    z->HL_r = &z->HLs0;		z->D_r = &z->Ds0;	z->YL_r = &z->YLs;
    z->IX_r = &z->IXs;		z->E_r = &z->Es0;	z->I_r  = &z->Is;
    z->IY_r = &z->IYs;		z->H_r = &z->Hs0;	z->R_r  = &z->Rs;
    z->PC_r = &z->PCs;		z->L_r = &z->Ls0;
    z->SP_r = &z->SPs;		z->F_r = &z->Fs0;

    z->MHL_r = &z->MHLs;	z->MSP_r = &z->MSPs;
    z->MIX_r = &z->MIXs;	z->MBC_r = &z->MBCs;
    z->MIY_r = &z->MIYs;	z->MDE_r = &z->MDEs;
}

/************************************************************************************************/

void  z80_nop(z80 *z)
{
    z->cmd_type=D_NOP;
    TICS_UPDATE;
    INST_END;
};

void  z80_halt(z80 *z)
{
    z->cmd_type=D_HALT;
    TICS_UPDATE;
    if(!z->int_off) z->halt=1;
    INST_END;
};

void  z80_pref_dd(z80 *z)
{
    TICS_UPDATE;
    z->pref_dd_fd = 256;
    z->ipend = 1;
};

void  z80_pref_fd(z80 *z)
{
    TICS_UPDATE;
    z->pref_dd_fd = 512;
    z->ipend = 1;
};

void  z80_pref_ed(z80 *z)
{
    TICS_UPDATE;
    z->pref_cb_ed = 1536;
    z->ipend = 1;
};

void  z80_pref_cb(z80 *z)
{
    TICS_UPDATE;
    z->pref_cb_ed = 768;
    z->ipend = 1;
    if(z->pref_dd_fd) z->dd3 = z80_fetch_byte(z);
};

void  z80_im(z80 *z)
{
    z->cmd_type=D_IM;
    TICS_UPDATE;
    z->dd = z->imode = z->p0;
    INST_END;
};

void  z80_di(z80 *z)
{
    z->cmd_type=D_DI;
    z->iff1 = z->iff2 = 0;
    TICS_UPDATE;
    INST_END;
};

void  z80_ei(z80 *z)
{
    z->cmd_type=D_EI;
    z->iff1 = z->iff2 = 1;
    TICS_UPDATE;
    INST_END;
};

void  z80_jr(z80 *z)
{
    z->cmd_type=D_JR;
    z->cycles +=2; z->ticks +=7; z->_tic = 7;
    z->dd1 = z80_fetch_byte(z);
    z->reg80 = z->p1;
    z->nn = PCr + (char)z->dd1;
    if(!Z80_COND(z->p1,Fr)){
        INST_END;
        return;
    }
    z->ticks +=5; z->_tic += 5; z->cycles++;
    if(!z->jmp_off) PCr = z->nn;
    INST_END;
};

void  z80_jp(z80 *z)
{
    z->cmd_type=D_JP;
    z->cycles +=3; z->ticks +=10; z->_tic = 10;
    z->dd2 = z80_fetch_word(z);
    z->reg80 = z->p1;
    z->nn = z->dd2;
    if(!Z80_COND(z->p1,Fr)){
        INST_END;
        return;
    }
    if(!z->jmp_off) PCr = z->nn;
    INST_END;
};

void  z80_jphl(z80 *z)
{
    z->cmd_type=D_JPHL;
    z->cycles +=1; z->ticks +=4; z->_tic = 4;
    z->reg80 = z->p1;
    z->nn = z->dd2;
    if(!z->jmp_off) PCr = *(z->regs16[z->regx0=z->p0]);
    INST_END;
};

void  z80_djnz(z80 *z)
{
    z->cmd_type=D_DJNZ;
    z->cycles+=2; z->ticks+=8; z->_tic=8;
    z->dd1 = z80_fetch_byte(z);
    z->nn = PCr + (char)z->dd1;
    if(!--Br){
        z->djnz=0;
        INST_END;
        return;
    }
    z->djnz=1;
    z->ticks+=5; z->_tic+=5; z->cycles++;
    if(!z->jmp_off) PCr = z->nn;
    TICS_UPDATE;
    INST_END;
};

void  z80_call(z80 *z)
{
    z->cmd_type=D_CALL;
    z->cycles+=3; z->ticks+=10; z->_tic=10;
    z->dd2 = z80_fetch_word(z);
    z->reg80 = z->p1;
    z->nn = z->dd2;
    if(!Z80_COND(z->p1,Fr)){
        INST_END;
        return;
    }
    z->nested++;
    z->ticks+=7; z->_tic+=7; z->cycles+=2;
    SPr -= 2;
    z->mem_wr(z, SPr, PCr & 0xff);
    z->mem_wr(z, SPr+1, PCr >> 8);
    if(!z->jmp_off) PCr = z->dd2;
    TICS_UPDATE;
    INST_END;
};

void  z80_rst(z80 *z)
{
    z->cmd_type=D_RST;
    z->mem_wr(z, --SPr, PCr >> 8);
    z->mem_wr(z, --SPr, PCr & 0xff);
    z->dd = z->p0;
    z->nested++;
    if(!z->jmp_off) PCr = (ushort)z->p0;
    TICS_UPDATE;
    INST_END;
};

void  z80_ret(z80 *z)
{
    z->cmd_type=D_RET;
    z->cycles+=1; z->ticks+=5; z->_tic=5;
    z->reg80 = z->p1;
    if(!Z80_COND(z->p1,Fr)){
        INST_END;
        return;
    }
    z->nested--;
    z->ticks+=6; z->_tic+=6; z->cycles+=2;
    z->dd2 =  z->mem_rd(z, SPr);
    z->dd2 |= z->mem_rd(z, SPr+1) << 8;
    SPr+=2;
    if(!z->jmp_off) PCr = z->dd2;
    TICS_UPDATE;
    INST_END;
};

void  z80_reti(z80 *z)
{
    z->cmd_type=D_RETI;
    z->dd2 =  z->mem_rd(z, SPr);
    z->dd2 |= z->mem_rd(z, SPr+1) << 8;
    SPr+=2;
    z->nested--;
    if(!z->jmp_off) PCr = z->dd2;
    TICS_UPDATE;
    INST_END;

};

void  z80_retn(z80 *z)
{
    z->cmd_type=D_RETN;
    z->dd2 =  z->mem_rd(z, SPr);
    z->dd2 |= z->mem_rd(z, SPr+1) << 8;
    SPr+=2;
    if(!z->jmp_off) PCr = z->dd2;
    z->nested--;
    z->iff1=z->iff2;
    TICS_UPDATE;
    INST_END;
};

void  z80_pop(z80 *z)
{
    z->cmd_type=D_POP;
    z->dd2 =  z->mem_rd(z, SPr);
    z->dd2 |= z->mem_rd(z, SPr+1) << 8;
    SPr+=2;
    z->regx0 = z->p0;
    *(z->regs16[z->p0 & 0x7]) = z->dd2;
    TICS_UPDATE;
    INST_END;
};

void  z80_push(z80 *z)
{
    z->cmd_type=D_PUSH;
    z->dd2 = *(z->regs16[z->p0 & 0x7]);
    z->mem_wr(z, --SPr, z->dd2 >> 8);
    z->mem_wr(z, --SPr, z->dd2 & 0xff);
    z->regx0 = z->p0;
    TICS_UPDATE;
    INST_END;
};

void  z80_exx(z80 *z)
{
    z->cmd_type=D_EXX;
    TICS_UPDATE;
    z80_bank_rg(z,!z->alt_rg);
    INST_END;
};

void  z80_ex(z80 *z)
{
    uchar a,b;

    z->regx0 = z->p0;
    z->regx1 = z->p1;
    if(z->p0 == R0AF){
        z->cmd_type=D_EX;
        TICS_UPDATE;
        z->dd2 = z->AFs0;
        z->AFs0 = z->AFs1;
        z->AFs1 = z->dd2;
        z->alt_af = !z->alt_af;
    } else
    if(z->p0 == RDE){
        z->cmd_type=D_EX;
        TICS_UPDATE;
        z->dd2 = DEr;
        DEr = HLr;
        HLr = z->dd2;
    }else
        if(z->p0 == MSP){
            z->cmd_type=D_EXSP;
            TICS_UPDATE;
            a = z->mem_rd(z,SPr);
            b = z->mem_rd(z,SPr + 1);
            z->dd2 = *(z->regs16[z->regx0=z->p1]);
            z->mem_wr(z,SPr+1, z->dd2 >> 8);
            z->mem_wr(z,SPr, z->dd2 & 0xff);
            *(z->regs16[z->p1]) = (b << 8) | a;
        }
    INST_END;
};

void  z80_ldb(z80 *z)
{
    z->cmd_type=D_LDB;
    TICS_UPDATE;
    *(z->regs8[z->p0]) = *(z->regs8[z->p1]);
    z->reg80 = z->p0;
    z->reg81 = z->p1;
    INST_END;
};

void  z80_ldbrm(z80 *z)
{
    z->cmd_type=D_LDBRM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->p1]);
    if(z->pref_dd_fd) z->dd2 += (char) (z->dd=z80_fetch_byte(z));
    *(z->regs8[z->p0]) = z->mem_rd(z, z->dd2);
    z->reg80 = z->p0;
    z->regx0 = z->p1;
    INST_END;
};

void  z80_ldbmr(z80 *z)
{
    z->cmd_type=D_LDBMR;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0=z->p0]);
    if(z->pref_dd_fd) z->dd2 += (char) (z->dd=z80_fetch_byte(z));
    z->mem_wr(z,z->dd2, *z->regs8[z->reg80=z->p1]);
    INST_END;
};

void  z80_ldbmn(z80 *z)
{
    z->cmd_type=D_LDBMN;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0 =z->p0]);
    if(z->pref_dd_fd) z->dd2 += (char) (z->dd=z80_fetch_byte(z));
    z->mem_wr(z,z->dd2, z->reg80 = z80_fetch_byte(z));
    INST_END;
};

void  z80_ldbmnr(z80 *z)
{
    z->cmd_type=D_LDBMNR;
    TICS_UPDATE;
    z->mem_wr(z,z->nn=z80_fetch_word(z),*(z->regs8[z->reg80=z->p1]));
    INST_END;
};

void  z80_ldbrmn(z80 *z)
{
    z->cmd_type=D_LDBRMN;
    TICS_UPDATE;
    *(z->regs8[z->reg80=z->p0]) = z->mem_rd(z,z->nn=z80_fetch_word(z));
    INST_END;
};

void  z80_ldbn(z80 *z)
{
    z->cmd_type=D_LDBN;
    TICS_UPDATE;
    *(z->regs8[z->reg80=z->p0]) = z->dd = z80_fetch_byte(z);
    INST_END;
};

void  z80_ldb_(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_LDB;
    TICS_UPDATE;
    *(z->regs8[z->reg80=z->p0]) = *(z->regs8[z->reg81=z->p1]);
    SET_FLAGSP(Ar,0)
    INST_END;
};

void  z80_ldw(z80 *z)
{
    z->cmd_type=D_LDW;
    TICS_UPDATE;
    *(z->regs16[z->regx0=z->p0]) = (*z->regs16[z->regx1=z->p1]);
    INST_END;
};

void  z80_ldwn(z80 *z)
{
    z->cmd_type=D_LDWN;
    TICS_UPDATE;
    *(z->regs16[z->regx0=z->p0]) = z->nn=z80_fetch_word(z);
    INST_END;
};

void  z80_ldwrmn(z80 *z)
{
    z->cmd_type=D_LDWRMN;
    TICS_UPDATE;
    *(z->regs16[z->regx0=z->p0]) = z80_mem_rword(z,z->nn=z80_fetch_word(z));
    INST_END;
};

void  z80_ldwmnr(z80 *z)
{
    z->cmd_type=D_LDWMNR;
    TICS_UPDATE;
    z80_mem_wword(z,z->nn=z80_fetch_word(z),*(z->regs16[z->regx0=z->p1]));
    INST_END;
};

void  z80_scf(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_SCF;
    TICS_UPDATE;
    SET_FLAGSP(Ar & 0x28,0)
    INST_END;
};

void  z80_ccf(z80 *z)
{
    z->cmd_type=D_CCF;
    TICS_UPDATE;

    Fr = (Fr & 0xc5) | (Ar & 0x28) | ((Fr << 4) & 0x10);
    Fr ^= 0x01;

    INST_END;
};

void  z80_cpl(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_CPL;
    TICS_UPDATE;
    Ar = ~Ar;
    SET_FLAGSP(0,0)
    INST_END;
};

void  z80_daa(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_DAA;
    TICS_UPDATE;

    z->dd1 = Ar > 0x99;
    z->dd3 = 0;
    if((Fr & FC) || z->dd1){
         Ar += (Fr & FN) ? -0x60: 0x60;
         z->dd3 = FC;
    }

    z->dd1 = (Ar & 0x0f) > 0x09;
    if((Fr & FH) || z->dd1){
         Ar += (Fr & FN) ? -0x06: 0x06;
         z->dd3 |= FH;
    }

    SET_FLAGSP(Ar,z->dd3)
    INST_END;
};

void  z80_bit(z80 *z)
{
    unsigned char u;
    z->cmd_type=D_BIT;
    TICS_UPDATE;

    u = *(z->regs8[z->reg80=z->p1]);
    Fr = (Fr & 0x01) | FH | (( u & z->p0) ? 0 : FZ+FP) | ( u & 0x80 & z->p0);

    INST_END;
};

void  z80_bitm(z80 *z)
{
    unsigned char u;
    z->cmd_type=D_BITM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0=z->p1]);
    if(z->pref_dd_fd) z->dd2 += (char)(z->dd=z->dd3);

    u =  z->mem_rd(z,z->dd2);
    Fr = (Fr & 0x01) | FH | (( u & z->p0) ? 0 : FZ+FP) | ( u & 0x80 & z->p0);
    z->reg81=z->p2;

    INST_END;
};

void  z80_res(z80 *z)
{
    z->cmd_type=D_RES;
    TICS_UPDATE;
    *(z->regs8[z->reg80=z->p1]) &= z->reg81=z->p0;
    INST_END;
};

void  z80_resm(z80 *z)
{
    z->cmd_type=D_RESM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0=z->p1]);
    if(z->pref_dd_fd) z->dd2 += (char)(z->dd=z->dd3);
    z->mem_wr(z,z->dd2, z->mem_rd(z,z->dd2) & z->p0);
    z->reg81=z->p2;
    INST_END;
};

void  z80_set(z80 *z)
{
    z->cmd_type=D_SET;
    TICS_UPDATE;
    *(z->regs8[z->reg80=z->p1]) |= z->reg81=z->p0;
    INST_END;
};

void  z80_setm(z80 *z)
{
    z->cmd_type=D_SETM;
    TICS_UPDATE;

    z->dd2 = *(z->regs16[z->regx0=z->p1]);
    if(z->pref_dd_fd) z->dd2 += (char)(z->dd=z->dd3);
    z->mem_wr(z,z->dd2, z->mem_rd(z,z->dd2) | z->p0);
    z->reg81=z->p2;
    INST_END;
};

void  z80_and(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_AND;
    TICS_UPDATE;
    Ar &= *(z->regs8[z->reg80=z->p1]);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_andn(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_ANDN;
    TICS_UPDATE;
    Ar &= z->dd=z80_fetch_byte(z);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_andm(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_ANDM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0=z->p1]);
    if(z->pref_dd_fd) z->dd2 += (char)(z->dd=z80_fetch_byte(z));
    Ar &= z->mem_rd(z,z->dd2);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_or(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_OR;
    TICS_UPDATE;
    Ar |= *(z->regs8[z->reg80=z->p1]);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_orn(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_ORN;
    TICS_UPDATE;
    Ar |= z->dd=z80_fetch_byte(z);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_orm(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_ORM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0=z->p1]);
    if(z->pref_dd_fd) z->dd2 += (char)(z->dd=z80_fetch_byte(z));
    Ar |= z->mem_rd(z,z->dd2);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_xor(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_XOR;
    TICS_UPDATE;
    Ar ^= *(z->regs8[z->reg80=z->p1]);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_xorn(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_XORN;
    TICS_UPDATE;
    Ar ^= z->dd=z80_fetch_byte(z);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_xorm(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_XORM;
    TICS_UPDATE;
    z->dd2 = *(z->regs16[z->regx0=z->p1]);
    if(z->pref_dd_fd) z->dd2 += (char)(z->dd=z80_fetch_byte(z));
    Ar ^= z->mem_rd(z,z->dd2);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_rlc(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_RLC;
    TICS_UPDATE;

    GET_REG8(z->dd1);
    z->dd1 = ROL_L(z->dd1, z->dd1 >> 7);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, z->dd1 & 0x01);
    INST_END;
};

void  z80_rlcm(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_RLCM;
    TICS_UPDATE;

    GET_MEM(z->dd1);
    z->dd1 = ROL_L(z->dd1, z->dd1 >> 7);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, z->dd1 & 0x01);
    INST_END;
};

void  z80_rrc(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_RRC;
    TICS_UPDATE;

    GET_REG8(z->dd1);
    z->dd1 = ROL_R(z->dd1, z->dd1 << 7);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1,(z->dd1 >> 7) & 1);
    INST_END;
};

void  z80_rrcm(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_RRC;
    TICS_UPDATE;

    GET_MEM(z->dd1);
    z->dd1 = ROL_R(z->dd1, z->dd1 << 7);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1,(z->dd1 >> 7) & 1);
    INST_END;
};

void  z80_rls(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_RL;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_L(z->dd3, Fr );
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
};

void  z80_rlsm(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_RLM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_L(z->dd3, Fr );
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
};

void  z80_rr(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_RR;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_R(z->dd3, Fr << 7);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
};

void  z80_rrm(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_RRM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_R(z->dd3, Fr << 7);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
};

void  z80_sla(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_SLA;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_L(z->dd3, 0);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
};

void  z80_slam(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_SLAM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_L(z->dd3, 0);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
};

void  z80_sra(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_SRA;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_R(z->dd3, z->dd3);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
};

void  z80_sram(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_SRAM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_R(z->dd3, z->dd3);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
};

void  z80_sli(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_SLI;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_L(z->dd3, 1);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
};

void  z80_slim(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_SLIM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_L(z->dd3, 1);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, (z->dd3 >> 7) & 1);
    INST_END;
};

void  z80_srl(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_SRL;
    TICS_UPDATE;

    GET_REG8(z->dd3);
    z->dd1 = ROL_R(z->dd3, 0);
    SET_REG8(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
};

void  z80_srlm(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_SRLM;
    TICS_UPDATE;

    GET_MEM(z->dd3);
    z->dd1 = ROL_R(z->dd3, 0);
    SET_MEM(z->dd1);

    SET_FLAGSP(z->dd1, z->dd3 & 1);
    INST_END;
};

void  z80_rrd(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_RRD;
    TICS_UPDATE;
    z->dd1 = z->mem_rd(z,HLr);
    z->dd3 = z->dd1;
    z->dd1 >>= 4;
    z->dd3 &= 0x0f;
    z->dd1 = (z->dd1 & 0x0f) | ((Ar << 4) & 0xf0);
    Ar = (Ar & 0xf0) | z->dd3;
    z->mem_wr(z,HLr,z->dd1);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_rld(z80 *z)
{
    uchar ftmp=0;
    z->cmd_type=D_RLD;
    TICS_UPDATE;

    z->dd1 = z->mem_rd(z,HLr);
    z->dd3 = z->dd1;
    z->dd1 <<= 4;
    z->dd3 >>= 4;
    z->dd1 = (z->dd1 & 0xf0) | (Ar & 0x0f);
    Ar = (Ar & 0xf0) | z->dd3;
    z->mem_wr(z,HLr,z->dd1);
    SET_FLAGSP(Ar,0);
    INST_END;
};

void  z80_outa(z80 *z)
{
    z->cmd_type=D_OUTA;
    TICS_UPDATE;
    z->output(z, z->last_out_addr =((z->dd=z80_fetch_byte(z))& 0xff) | (Ar << 8), z->last_out_data = Ar);
    INST_END;
};

void  z80_ina(z80 *z)
{
    z->cmd_type=D_INA;
    TICS_UPDATE;
    z->last_in_data = Ar = z->input(z, z->last_in_addr = ((z->dd = z80_fetch_byte(z)) & 0xff) | (Ar << 8));
    if(z->force_in > 0) z->last_in_data = Ar = z->force_in;
    INST_END;
};

void  z80_inbc(z80 *z)
{
    z->cmd_type=D_INBC;
    TICS_UPDATE;
    z->last_in_data = *(z->regs8[z->reg80=z->p0]) = z->input(z, z->last_in_addr =BCr);
    if(z->force_in > 0) z->last_in_data = *(z->regs8[z->reg80=z->p0]) = z->force_in;
    INST_END;
};

void  z80_outbc(z80 *z)
{
    z->cmd_type=D_OUTBC;
    TICS_UPDATE;
    z->output(z, z->last_out_addr = BCr, z->last_out_data = *(z->regs8[z->reg80=z->p1]));
    INST_END;
};

void  z80_add8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_ADD8;
    TICS_UPDATE;

    VGET_REG8(b);
    a = Ar;
    Ar = a + b;
    ALU_INDEX_8(Ar, a, b, c);
    SET_FLAGSV(Ar, __z80_add_cy_tbl[c]);

    INST_END;
}

void  z80_addn8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_ADDN8;
    TICS_UPDATE;

    VGET_CNS8(b);
    a = Ar;
    Ar = a + b;
    ALU_INDEX_8(Ar, a, b, c);
    SET_FLAGSV(Ar, __z80_add_cy_tbl[c]);

    INST_END;
}

void  z80_addm8(z80 *z)
{
    ushort a, b, c;
    uchar ftmp=0;
    z->cmd_type=D_ADDM8;
    TICS_UPDATE;

    VGET_MEM8(b);
    a = Ar;
    Ar = a + b;
    ALU_INDEX_8(Ar, a, b, c);
    SET_FLAGSV(Ar, __z80_add_cy_tbl[c]);

    INST_END;
}

void  z80_adc8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_ADC8;
    TICS_UPDATE;

    VGET_REG8(b);
    b += Fr & 1;
    a = Ar;
    Ar = a + b;
    ALU_INDEX_8(Ar, a, b, c);
    SET_FLAGSV(Ar, __z80_add_cy_tbl[c]);

    INST_END;
}

void  z80_adcn8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_ADCN8;
    TICS_UPDATE;

    VGET_CNS8(b);
    b += Fr & 1;
    a = Ar;
    Ar = a + b;
    ALU_INDEX_8(Ar, a, b, c);
    SET_FLAGSV(Ar, __z80_add_cy_tbl[c]);

    INST_END;
}

void  z80_adcm8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_ADCM8;
    TICS_UPDATE;

    VGET_MEM8(b);
    b += Fr & 1;
    a = Ar;
    Ar = a + b;
    ALU_INDEX_8(Ar, a, b, c);
    SET_FLAGSV(Ar, __z80_add_cy_tbl[c]);

    INST_END;
}

void  z80_sub8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_SUB8;
    TICS_UPDATE;

    VGET_REG8(b);
    a = Ar;
    Ar = a - b;
    ALU_INDEX_8(Ar, b, a, c);
    SET_FLAGSV(Ar, __z80_sub_cy_tbl[c]);

    INST_END;
}

void  z80_subn8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_SUBN8;
    TICS_UPDATE;

    VGET_CNS8(b);
    a = Ar;
    Ar = a - b;
    ALU_INDEX_8(Ar, b, a, c);
    SET_FLAGSV(Ar, __z80_sub_cy_tbl[c]);

    INST_END;
}

void  z80_subm8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_SUB8;
    TICS_UPDATE;

    VGET_MEM8(b);
    a = Ar;
    Ar = a - b;
    ALU_INDEX_8(Ar, b, a, c);
    SET_FLAGSV(Ar, __z80_sub_cy_tbl[c]);

    INST_END;
}

void  z80_sbc8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_SBC8;
    TICS_UPDATE;

    VGET_REG8(b);
    a = Ar;
    b += Fr & FC;
    Ar = a - b;
    ALU_INDEX_8(Ar, b, a, c);
    SET_FLAGSV(Ar, __z80_sub_cy_tbl[c]);

    INST_END;
}

void  z80_sbcn8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_SBCN8;
    TICS_UPDATE;

    VGET_CNS8(b);
    a = Ar;
    b += Fr & FC;
    Ar = a - b;
    ALU_INDEX_8(Ar, b, a, c);
    SET_FLAGSV(Ar, __z80_sub_cy_tbl[c]);

    INST_END;
}

void  z80_sbcm8(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c;
    z->cmd_type=D_SBCM8;
    TICS_UPDATE;

    VGET_MEM8(b);
    a = Ar;
    b += Fr & FC;
    Ar = a - b;
    ALU_INDEX_8(Ar, b, a, c);
    SET_FLAGSV(Ar, __z80_sub_cy_tbl[c]);

    INST_END;
}

void  z80_cp(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c, d, s;
    z->cmd_type=D_CP;
    TICS_UPDATE;

    VGET_REG8(b);
    s=b;
    a = Ar;
    d = a - b;
    ALU_INDEX_8(d, b, a, c);
    SET_FLAGSV(d, __z80_sub_cy_tbl[c]);
    Fr = (Fr & 0xd7) | (s & 0x28);

    INST_END;
}

void  z80_cpn(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c, d, s;
    z->cmd_type=D_CPN;
    TICS_UPDATE;

    VGET_CNS8(b);
    s=b;
    a = Ar;
    d = a - b;
    ALU_INDEX_8(d, b, a, c);
    SET_FLAGSV(d, __z80_sub_cy_tbl[c]);
    Fr = (Fr & 0xd7) | (s & 0x28);

    INST_END;
}

void  z80_cpm(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c, d, s;
    z->cmd_type=D_CPM;
    TICS_UPDATE;

    VGET_MEM8(b);
    s = b;
    a = Ar;
    d = a - b;
    ALU_INDEX_8(d, b, a, c);
    SET_FLAGSV(d, __z80_sub_cy_tbl[c]);
    Fr = (Fr & 0xd7) | (s & 0x28);

    INST_END;
}

void  z80_inc8(z80 *z)
{
    uchar ftmp=0;
    ushort a,b,c;
    z->cmd_type=D_INC8;
    TICS_UPDATE;

    VGET_REG8(a);
    b = a + 1;
    ALU_INDEX_8(b, 1, a, c);
    SET_FLAGSV(b, __z80_add_cy_tbl[c]);
    VSET_REG8(b);

    INST_END;
}

void  z80_incm8(z80 *z)
{
    uchar ftmp=0;
    ushort a,b,c;
    z->cmd_type=D_INCM8;
    TICS_UPDATE;

    VGET_MEM8(a);
    b = a + 1;
    ALU_INDEX_8(b, 1, a, c);
    SET_FLAGSV(b, __z80_add_cy_tbl[c]);
    VSET_MEM8(b);

    INST_END;
}

void  z80_dec8(z80 *z)
{
    uchar ftmp=0;
    ushort a,b,c;
    z->cmd_type=D_DEC8;
    TICS_UPDATE;

    VGET_REG8(a);
    b = a - 1;
    ALU_INDEX_8(b, 1, a, c);
    SET_FLAGSV(b, __z80_sub_cy_tbl[c]);
    VSET_REG8(b);

    INST_END;
}

void  z80_decm8(z80 *z)
{
    uchar ftmp=0;
    ushort a,b,c;
    z->cmd_type=D_DECM8;
    TICS_UPDATE;

    VGET_MEM8(a);
    b = a - 1;
    ALU_INDEX_8(b, 1, a, c);
    SET_FLAGSV(b, __z80_sub_cy_tbl[c]);
    VSET_MEM8(b);

    INST_END;
}

void  z80_neg(z80 *z)
{
    uchar ftmp=0;
    ushort a,c;
    z->cmd_type=D_NEG;
    TICS_UPDATE;

    Ar = -(a = Ar);
    ALU_INDEX_8(Ar, 0, a, c);
    SET_FLAGSV(Ar, __z80_sub_cy_tbl[c]);

    INST_END;
};

void  z80_incx(z80 *z)
{

    z->cmd_type=D_INCX;
    TICS_UPDATE;
    (*z->regs16[z->regx0=z->p0])++;
    INST_END;
}

void  z80_decx(z80 *z)
{
    z->cmd_type=D_DECX;
    TICS_UPDATE;
    (*z->regs16[z->regx0=z->p0])--;
    INST_END;
}

void  z80_addx(z80 *z)
{
    uchar ftmp=0;
    int a = *(z->regs16[z->regx0=z->p0]), b = *(z->regs16[z->regx1=z->p1]);
    int c,d;
    z->cmd_type=D_ADDX;
    TICS_UPDATE;

    d = a + b;
    ALU_INDEX_X(d, a, b, c);
    SET_FLAGSV(d >> 8, __z80_add_cy_tbl[c]);
    *(z->regs16[z->p0]) = d;

    INST_END;
}

void  z80_adcx(z80 *z)
{
    uchar ftmp=0;
    int a = *(z->regs16[z->regx0=z->p0]), b = *(z->regs16[z->regx1=z->p1]);
    int c,d;
    z->cmd_type=D_ADCX;
    TICS_UPDATE;
    b += Fr & 1;
    d = a + b;
    ALU_INDEX_X(d, a, b, c);
    SET_FLAGSV(d >> 8, __z80_add_cy_tbl[c]);
    *(z->regs16[z->p0]) = d;

    INST_END;
}

void  z80_sbcx(z80 *z)
{
    uchar ftmp=0;
    int a = *(z->regs16[z->regx0=z->p0]), b = *(z->regs16[z->regx1=z->p1]);
    int c,d;
    z->cmd_type=D_SBCX;
    TICS_UPDATE;
    b += (Fr & FC);
    d = a - b;
    ALU_INDEX_X(d, b, a, c);
    SET_FLAGSVX(d, __z80_sub_cy_tbl[c]);
    *(z->regs16[z->p0]) = d;

    INST_END;
}

void  z80_ldbl(z80 *z)
{
    z->cmd_type=D_LDBL;
    TICS_UPDATE;
    z->dd=z->p0;
    Fr &= ~(FH | FP | FN | F5 | F3);
    if(!z->jmp_off) do{
        z->mem_wr(z,DEr,z->dd3=z->mem_rd(z,HLr));
        if(z->p0 == PI || z->p0 == PIR) HLr++, DEr++;
            else HLr--, DEr--;

        if(--BCr){
            Fr |= FP;
            z->ticks += 5;
            z->cycles += 1;
            z->_tic += 5;
        }
    }while((z->p0 == PIR || z->p0 == PDR) && BCr);
    z->dd3 += Ar;
    z->dd3 = (z->dd3 & 0x08) | ((z->dd3 << 5) & 0x20);
    Fr |= z->dd3;
    INST_END;
};

void  z80_cpbl(z80 *z)
{
    uchar ftmp=0;
    ushort a, b, c, d,e;
    z->cmd_type=D_CPBL;
    TICS_UPDATE;
    z->dd=z->p0;
    e = Fr;
    if(!z->jmp_off) do{
        a = Ar;
        b = z->mem_rd(z,HLr);
        d = a - b;
        ALU_INDEX_8(d, b, a, c);
        SET_FLAGSV(d, __z80_sub_cy_tbl[c]);
        if(z->p0 == PI || z->p0 == PIR) HLr++;
            else HLr--;
        Fr &= ~FP;
        if(--BCr){
            Fr |= FP;
            z->ticks += 5;
            z->cycles += 1;
            z->_tic += 5;
        }

        if( Fr & FZ) break;
    }while((z->p0 == PIR || z->p0 == PDR) && BCr);

    Fr = (Fr & 0xfe) | (e & 0xfe);
    INST_END;
};

void  z80_inbl(z80 *z)
{
    z->cmd_type=D_INBL;
    TICS_UPDATE;
    z->dd=z->p0;
    Fr |= FN | FZ;
    if(!z->jmp_off) do{
        z->mem_wr(z,HLr,z->last_in_data=z->dd3=z->input(z,z->last_in_addr=Cr));
        if(z->p0 == PI || z->p0 == PIR) HLr++;
            else HLr--;

        if(--Br){
            Fr &= ~FZ;
            z->ticks += 5;
            z->cycles += 1;
            z->_tic += 5;
        }
    }while((z->p0 == PIR || z->p0 == PDR) && Br);
    INST_END;
};

void  z80_outbl(z80 *z)
{
    z->cmd_type=D_OUTBL;
    TICS_UPDATE;
    z->dd=z->p0;
    Fr |= FN | FZ;
    if(!z->jmp_off) do{
        z->output(z,z->last_out_addr=Cr,z->last_out_data = z->mem_rd(z,HLr));
        if(z->p0 == PI || z->p0 == PIR) HLr++;
            else HLr--;

        if(--Br){
            Fr &= ~FZ;
            z->ticks += 5;
            z->cycles += 1;
            z->_tic += 5;
        }
    }while((z->p0 == PIR || z->p0 == PDR) && Br);
    INST_END;
};

/*************************************************************************************************/

void Z80_CLASS z80_reset(Z80_STRUCT)
{
    unsigned int i;

    for(i=0; i < Z80_DATA_SIZE; i++)  Z80_CPU(z)z80_data[i]= 0;
    z80_setregs(Z80_PTR);
    z80_bank_rg(Z80_PTR,0);
    z80_bank_af(Z80_PTR,0);
    for(i=0; i < 8; i++) Z80_CPU(z)bus[i] = 0xff;
    z->force_in = -1;
}

void Z80_CLASS z80_nmi(Z80_STRUCT)
{
    if(Z80_CPU(z)int_off) return;
    Z80_CPU(z)inmi = 1;
}

void Z80_CLASS z80_int(Z80_STRUCTC uchar *bus)
{
    int i;

    if(Z80_CPU(z)int_off) return;
    Z80_CPU(z)intr = 1;
    if(bus)
        for(i=0; i < 8; i++) Z80_CPU(z)bus[i] = bus[i];
    else
        for(i=0; i < 8; i++) Z80_CPU(z)bus[i] = 0xff;
}

#ifdef __cplusplus
void  Z80_CLASS callback(void *param){}

void Z80_CLASS z80_loop(int cnt, int call, void *param)
#else
void z80_loop(z80 *z, int cnt, int call, void (*callback)(z80*,void*), void *param)
#endif
{
    uchar d;
    int i;
    z80dt xf;
    if(Z80_CPU(z)cpu_off) return;
    for(i = 0; i < cnt; i++){
    if(!Z80_CPU(z)exit){
        Z80_CPU(z)call_cnt += Z80_CPU(z)_tic;

        if( Z80_CPU(z)call_cnt >= call){
#ifdef __cplusplus
            callback(param);
#else
            if(callback) callback(z, param);
#endif
        Z80_CPU(z)call_cnt = Z80_CPU(z)call_cnt - call;;
        }

        if(Z80_CPU(z)_tic){
            i += Z80_CPU(z)_tic;
        Z80_CPU(z)_tic = 0;
        if(i >= cnt){
            Z80_CPU(z)_tic = i - cnt + 1;
            break;
        }
        }
    }
#ifdef __cplusplus
    if(Z80_CPU(z)dbg_on) Z80_CPU(z)debug(Z80_PTR,D_PRE,0);
#else
    if(Z80_CPU(z)dbg_on && Z80_CPU(z)debug) Z80_CPU(z)debug(Z80_PTR,D_PRE,0);
#endif
    if(Z80_CPU(z)exit) return;
    *Z80_CPU(z)R_r = ((*Z80_CPU(z)R_r) + 1) & 0x7f;

    if(!Z80_CPU(z)halt){
        if(!Z80_CPU(z)ipend) Z80_CPU(z)adrs = *Z80_CPU(z)PC_r;
        d = z80_fetch_byte(Z80_PTR);
    } else d = 0;

    xf = __z80_decoder__[d + Z80_CPU(z)pref_dd_fd + Z80_CPU(z)pref_cb_ed];
    Z80_CPU(z)p0 = xf.reg0;	Z80_CPU(z)b = xf.bytes;	 Z80_CPU(z)a = xf.f_mask_and;
    Z80_CPU(z)p1 = xf.reg1;	Z80_CPU(z)c = xf.cycles; Z80_CPU(z)o = xf.f_mask_or;
    Z80_CPU(z)p2 = xf.reg2;	Z80_CPU(z)t = xf.ticks;	 Z80_CPU(z)x = xf.f_mask_chg;
    xf.code(Z80_PTR);

    if(!Z80_CPU(z)ipend) { /* zabronienie przerwania, gdy instrukcja jest w trakcie wykonywania */
        if(Z80_CPU(z)inmi){
            Z80_CPU(z)halt = 0;
            Z80_CPU(z)inmi = 0;
            Z80_CPU(z)iff2 = Z80_CPU(z)iff1;
                Z80_CPU(z)iff1 = 0;
            z80_icall(Z80_PTR,0x66);
            }
            if(Z80_CPU(z)intr){
            Z80_CPU(z)halt=0;
            if(Z80_CPU(z)iff1){
                Z80_CPU(z)intr=0;
                    switch(Z80_CPU(z)imode){
                case 0: Z80_CPU(z)irupt=1; break; /* następna instrukcja będzie pobrana z magistrali */
                case 1: z80_icall(Z80_PTR,0x38); break;
                case 2: z80_icall(Z80_PTR, z80_mem_rword(
                    Z80_PTR,(ushort)(*Z80_CPU(z)I_r << 8) | (Z80_CPU(z)bus[0] & 0xff)));
                    break;
                }
            }
        }
    }
#ifdef __cplusplus
    if(!Z80_CPU(z)halt && Z80_CPU(z)dbg_on) Z80_CPU(z)debug(Z80_PTR,D_FLUSH,0);
#else
    if(!Z80_CPU(z)halt && Z80_CPU(z)dbg_on && Z80_CPU(z)debug) Z80_CPU(z)debug(Z80_PTR,D_FLUSH,0);
#endif
    }
}


#ifdef __cplusplus
void Z80_CLASS z80_step()
#else
void z80_step(z80 *z)
#endif
{
    uchar d;
    z80dt xf;
    if(Z80_CPU(z)cpu_off) return;

#ifdef __cplusplus
    if(Z80_CPU(z)dbg_on) Z80_CPU(z)debug(Z80_PTR,D_PRE,0);
#else
    if(Z80_CPU(z)dbg_on && Z80_CPU(z)debug) Z80_CPU(z)debug(Z80_PTR,D_PRE,0);
#endif

    *Z80_CPU(z)R_r = ((*Z80_CPU(z)R_r) + 1) & 0x7f;

    if(!Z80_CPU(z)halt){
        if(!Z80_CPU(z)ipend) Z80_CPU(z)adrs = *Z80_CPU(z)PC_r;
        d = z80_fetch_byte(Z80_PTR);
    } else d = 0;

    xf = __z80_decoder__[d + Z80_CPU(z)pref_dd_fd + Z80_CPU(z)pref_cb_ed];
    Z80_CPU(z)p0 = xf.reg0;	Z80_CPU(z)b = xf.bytes;	 Z80_CPU(z)a = xf.f_mask_and;
    Z80_CPU(z)p1 = xf.reg1;	Z80_CPU(z)c = xf.cycles; Z80_CPU(z)o = xf.f_mask_or;
    Z80_CPU(z)p2 = xf.reg2;	Z80_CPU(z)t = xf.ticks;	 Z80_CPU(z)x = xf.f_mask_chg;
    xf.code(Z80_PTR);

    if(!Z80_CPU(z)ipend) { /* zabronienie przerwania, gdy instrukcja jest w trakcie wykonywania */
        if(Z80_CPU(z)inmi){
        Z80_CPU(z)halt = 0;
        Z80_CPU(z)inmi = 0;
        Z80_CPU(z)iff2 = Z80_CPU(z)iff1;
            Z80_CPU(z)iff1 = 0;
        z80_icall(Z80_PTR,0x66);
        }
        if(Z80_CPU(z)intr){
        Z80_CPU(z)halt=0;
        if(Z80_CPU(z)iff1){
            Z80_CPU(z)intr=0;
                switch(Z80_CPU(z)imode){
            case 0: Z80_CPU(z)irupt=1; break; /* następna instrukcja będzie pobrana z magistrali */
            case 1: z80_icall(Z80_PTR,0x38); break;
            case 2: z80_icall(Z80_PTR, z80_mem_rword(
                Z80_PTR,(ushort)(*Z80_CPU(z)I_r << 8) | (Z80_CPU(z)bus[0] & 0xff)));
                break;
            }
        }
        }
    }
#ifdef __cplusplus
    if(!Z80_CPU(z)halt && Z80_CPU(z)dbg_on) Z80_CPU(z)debug(Z80_PTR,D_FLUSH,0);
#else
    if(!Z80_CPU(z)halt && Z80_CPU(z)dbg_on && Z80_CPU(z)debug) Z80_CPU(z)debug(Z80_PTR,D_FLUSH,0);
#endif

}

void z80_init(
    z80 *z,
    uchar (*mem_rd)(void *, ushort), void  (*mem_wr)(void *, ushort, uchar),
    uchar (*io_rd)(void *, ushort),void  (*io_wr)(void *, ushort, uchar),
    void *ptr, void *ptx
    )
{
    z->mem_rd = mem_rd;
    z->mem_wr = mem_wr;
    z->input  = io_rd;
    z->output = io_wr;
    z->ptr    = ptr;
    z->ptx    = ptx;
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

