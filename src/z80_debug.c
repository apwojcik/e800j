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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "z80_core.h"
#include "z80_dtbl.h"
#include "z80_debug.h"

char fbytes[32];

static const char* blk[] = { "IR", "DR", "I", "D" };

static const char* ddt[] = {
    "-", "NOP", "LD   %r,%z", "LD   %r,%n", "LD   %r,(%m)", "LD   (%m),%r", "LD   (%m),%f", "LD   (%N),%r",
    "LD   %R,%Z", "LD   %R,%N  ", "LD   %R,(%N)", "LD   (%N),%R", "HALT", "IM   %n", "DI", "EI", "JR   %c%N",
    "JP   %c%N", "DJNZ %N", "CALL %c%N", "RST  %n", "RET  %c", "RETI", "RETN", "POP  %R", "PUSH %R", "EXX",
    "EX   %R,%Z", "DAA", "SCF", "CCF", "CPL", "BIT  %b,%r", "BIT  %b,(%m)", "RES  %b,%r", "RES  %b,(%m)",
    "SET  %b,%r", "SET  %b,(%m)", "AND  %r", "AND  %n", "AND  (%m)", "OR   %r", "OR   %n", "OR   (%m)",
    "XOR  %r", "XOR  %n", "XOR  (%m)", "RLC  %r", "RLC  (%m)", "RRC  %r", "RRC  (%m)", "RL   %r", "RL   (%m)",
    "RR   %r", "RR   (%m)", "SLA  %r", "SLA  (%m)", "SRA  %r", "SRA  (%m)", "SLI  %r", "SLI  (%m)", "SRL  %r",
    "SRL  (%m)", "RRD", "RLD", "IN   A,(%n)", "OUT  (%n),A", "IN   %r,(C)", "OUT  (C),%r", "ADD  %r",
    "ADD  %n", "ADD  (%m)", "ADC  %r", "ADC  %n", "ADC  (%m)", "SUB  %r", "SUB  %n", "SUB  (%m)", "SBC  %r",
    "SBC  %n", "SBC  (%m)", "CP   %r", "CP   %n", "CP   (%m)", "INC  %r", "INC  (%m)", "DEC  %r", "DEC  (%m)",
    "INC  %R", "DEC  %R", "NEG", "ADD  %R,%Z", "ADC  %R,%Z", "SBC  %R,%Z", "LD%w", "JP  (%R)", "LD %r,(%N)",
    "EX   (SP),%R", "CP%w", "IN%w", "OUT%w"
};

static const char* rX[] = {
    "AF", "BC", "DE", "HL", "IX", "IY", "SP", "PC", "AF", "AF'"
};

static const char* r8[] = {
    "A", "F", "B", "C", "D", "E", "H", "L", "XH", "XL", "YH","YL","I","R","(HL)","(IX)","(IY)","(SP)","(BC)","(DE)"
};

static const char* rcnd[] = {
    "","NZ","Z","NC","C","PO","PE","P","M"
};

void dbg_deasm(struct dbg*);
void dbg_test(struct dbg*, unsigned char);
void dbg_input(struct dbg*);
void dbg_cmd(struct dbg* d);
void dbg_quit(struct dbg* d);
void dbg_help(struct dbg* d);
void dbg_poke(struct dbg* d);
void dbg_trace(struct dbg* d);
void dbg_proc(struct dbg* d);
void dbg_var(struct dbg* d);
void dbg_memo(struct dbg* d);
void dbg_stck(struct dbg* d);
void dbg_dasm(struct dbg* d);
void dbg_trap(struct dbg* d);
void dbg_goto(struct dbg* d);
void dbg_regs(struct dbg* d);
void dbg_iout(struct dbg* d);
void dbg_iin(struct dbg* d);
void dbg_bps(struct dbg* d);
void dbg_dbps(struct dbg* d);
void dbg_bp_delete_all(struct dbg* d);
void dbg_bp_delete_first(struct dbg* d);
void dbg_sregs(struct dbg* d);
void dbg_deasembly(struct dbg* d);

/********************************************************************************************/
/* zwraca 1 jeśli łańcuch t nie jest liczbą */
char dbg_dcheck(char* t) {
    (void)t;
    return 0;
}

char* dbg_oc2hexstr(unsigned char n, char* t) {
    t[0] = n >> 4;
    t[1] = n & 0x0f;
    t[2] = '\0';
    t[0] = t[0] >= 10 ? 'A' + t[0] - 10 : '0' + t[0];
    t[1] = t[1] >= 10 ? 'A' + t[1] - 10 : '0' + t[1];
    return t;
}

char* dbg_xc2hexstr(ushort n, char* t) {
    char t0[3] = "", t1[3] = "";
    sprintf(t, "%s%s", dbg_oc2hexstr(n >> 8, t0), dbg_oc2hexstr(n & 0xff, t1));
    return t;
}

static char* dbg_equalize(char* str, int n) {
    int k = strlen(str);
    int i;
    for (i = k; i < n; i++)
        strcat(str, " ");
    return str;
}

static void dbg_resolver(struct cpu* z, const char* code, char* str) {
    unsigned int i, j = 0, k;
    char xn[8] = "", kn[8] = "", fn[8] = "";
    if (code == NULL)
        return;
    if (code[0] == '\0')
        return;
    for (i = 0; i < strlen(code); i++) {
        if (code[i] == '%') {
            i++;
            switch (code[i]) {
            case 'r':
                strcat(str + j, r8[z->reg80]);
                j += strlen(str + j);
                break;
            case 'R':
                strcat(str + j, rX[z->regx0]);
                j += strlen(str + j);
                break;
            case 'z':
                strcat(str + j, r8[z->reg81]);
                j += strlen(str + j);
                break;
            case 'Z':
                strcat(str + j, rX[z->regx1]);
                j += strlen(str + j);
                break;
            case 'n':
                sprintf(kn, "$%s", dbg_oc2hexstr(z->dd, xn));
                strcat(str + j, kn);
                j += strlen(str + j);
                break;
            case 'N':
                sprintf(kn, "$%s", dbg_xc2hexstr(z->nn, xn));
                strcat(str + j, kn);
                j += strlen(str + j);
                break;
            case 'c':
                strcat(str + j, rcnd[z->reg80]);
                if (*rcnd[z->reg80])
                    strcat(str + j, ",");
                j += strlen(str + j);
                break;
            case 'm':
                strcat(str + j, rX[z->regx0]);
                if ((z->regx0 == RIX) || (z->regx0 == RIY)) {
                    k = z->dd < 0 ? -z->dd : z->dd;
                    sprintf(kn, "%s$%s", z->dd < 0 ? "-" : "+", dbg_oc2hexstr(k, fn));
                    strcat(str + j, kn);
                }
                j += strlen(str + j);
                break;
            case 'b':
                kn[0] = '0' + (z->reg81 & 0x07);
                kn[1] = '\0';
                strcat(str + j, kn);
                j += strlen(str + j);
                break;
            case 'f':
                sprintf(kn, "$%s", dbg_oc2hexstr(z->reg80, xn));
                strcat(str + j, kn);
                j += strlen(str + j);
                break;

            case 'w':
                strcat(str + j, blk[(int)z->dd]);
                j += strlen(str + j);
                break;
            }
            continue;
        }
        str[j++] = code[i];
    }
}

void dbg_addmach(struct dbg* deb, unsigned char n) {
    /* jeśli więcej niż MAX_BYTES zawijaj */
    if (deb->len >= MAX_BYTES - 1)
        deb->len = 0;
    dbg_oc2hexstr(n, deb->mach + deb->len);
    deb->len += 2;
}

void dbg_call(struct dbg* deb, unsigned char mode, unsigned char data) {
    deb->z->exit = deb->console;
    dbg_test(deb, mode);
    if (mode == D_FETCH) {
        dbg_addmach(deb, data);
        return;
    }
    /* jeśli instrukcja w toku wyjdź */
    if (deb->z->ipend)
        return;

    if (mode == D_PRE) {
        if (deb->console)
            dbg_input(deb);
        deb->z->exit = deb->console;
        return;
    }

    if (deb->deasm) {
        dbg_deasm(deb);
        if (deb->out)
            deb->out(deb->line);
    }

    /* zakończenie deasemblacji instrukcji */
    deb->len = 0;
    deb->mach[0] = 0;
    deb->line[0] = 0;
}

char* dbg_getregs(struct dbg* d, char* str) {
    char af[5], bc[5], de[5], hl[5], af_[5], bc_[5], de_[5], hl_[5];
    char ix[5], iy[5], sp[5], pc[5], i[3], r[3];

    if (d->noregs)
        return str;

    dbg_xc2hexstr(*d->z->AF_r, af);
    dbg_xc2hexstr(*d->z->BC_r, bc);
    dbg_xc2hexstr(*d->z->DE_r, de);
    dbg_xc2hexstr(*d->z->HL_r, hl);

    if (d->z->alt_af)
        dbg_xc2hexstr(d->z->AFs0, af_);
    else
        dbg_xc2hexstr(d->z->AFs1, af_);

    if (d->z->alt_rg) {
        dbg_xc2hexstr(d->z->BCs0, bc_);
        dbg_xc2hexstr(d->z->DEs0, de_);
        dbg_xc2hexstr(d->z->HLs0, hl_);
    }
    else {
        dbg_xc2hexstr(d->z->BCs1, bc_);
        dbg_xc2hexstr(d->z->DEs1, de_);
        dbg_xc2hexstr(d->z->HLs1, hl_);
    }

    dbg_xc2hexstr(*d->z->IX_r, ix);
    dbg_xc2hexstr(*d->z->IY_r, iy);
    dbg_xc2hexstr(*d->z->SP_r, sp);
    dbg_xc2hexstr(*d->z->PC_r, pc);
    dbg_oc2hexstr(*d->z->I_r, i);
    dbg_oc2hexstr(*d->z->R_r, r);

    sprintf(str,
            "%s %s %s %s  %s %s %s %s  %s %s %s %s  %s %s",
            af, bc, de, hl, af_, bc_, de_, hl_, ix, iy, sp, pc, i, r
    );
    return str;
}

void dbg_deasm(struct dbg* d) {
    char a[3], b[3], c[32] = "";
    char tmp[256] = "", tmp1[256] = "";

    sprintf(tmp, "%s%s %s", dbg_oc2hexstr(d->z->adrs >> 8, a), dbg_oc2hexstr(d->z->adrs, b), d->mach);
    dbg_equalize(tmp, 16);
    dbg_resolver(d->z, ddt[d->z->cmd_type], c);
    dbg_equalize(c, 16);
    snprintf(d->line, sizeof(d->line), "%.100s%.31s%.100s\n", tmp, c, dbg_getregs(d, tmp1));
    d->z->dd = d->z->nn = d->z->reg80 = 0;
    d->z->reg81 = d->z->regx0 = d->z->regx1 = d->z->cmd_type = 0;
}

void dbg_test(struct dbg* d, unsigned char mode) {
    if (d->console)
        return;
    if (d->trace) {
        if (mode != D_PRE)
            return; /* jeśli nie na początku */
        if (d->icnt <= 1)
            d->deasm = 1;
        if (!d->icnt) {
            d->console = 1;
            d->trace = 0;
            d->deasm = 0;
            return;
        }
        d->icnt--;
    }
    else {
        if (!d->cond)
            return;
        if ((d->cond == C_PCEQ) && (d->z->adrs == d->addr))
            d->console = 1, d->cond = 0;
        if ((d->cond == C_PCGT) && (d->z->adrs > d->addr))
            d->console = 1, d->cond = 0;
        if ((d->cond == C_PCLT) && (d->z->adrs < d->addr))
            d->console = 1, d->cond = 0;
    }
}

/***********************************************************************************************/

void dbg_input(struct dbg* d) {
    int i;
    if (!d->in)
        return;
    d->out("dbg# ");
    for (i = 0; ((d->cmdline[i] = d->in()) != '\n') && i < 255; i++);
    d->cmdline[i] = 0;
    if (i)
        dbg_cmd(d);
}

void dbg_getcmd(struct dbg* d, char* cmd) {
    char* n = d->cmdline;
    int i, j, k = strlen(n);

    /* znajdź początek wyrazu */
    for (i = 0; i < k && n[i] < '!'; i++);

    /* znajdź koniec wyrazu */
    for (j = i; j < k && n[j] > ' '; j++);

    strncpy(cmd, n + i, j - i);
}

/* analiza składni wprowadzonej linii */
void dbg_cmd(struct dbg* d) {
    char cmd[256] = "";

    /* pobranie rozkazu debuggera z linii poleceń */
    dbg_getcmd(d, cmd);
    if (!strcmp(cmd, "quit")) {
        dbg_quit(d);
        return;
    }
    if (!strcmp(cmd, "q")) {
        dbg_quit(d);
        return;
    }
    if (!strcmp(cmd, "help")) {
        dbg_help(d);
        return;
    }
    if (!strcmp(cmd, "h")) {
        dbg_help(d);
        return;
    }
    /* instrukcje debuggera */
    if (!strcmp(cmd, "a")) {
        dbg_poke(d);
        return;
    }
    if (!strcmp(cmd, "t")) {
        dbg_trace(d);
        return;
    }
    if (!strcmp(cmd, "p")) {
        dbg_proc(d);
        return;
    }
    if (!strcmp(cmd, "v")) {
        dbg_var(d);
        return;
    }
    if (!strcmp(cmd, "d")) {
        dbg_memo(d);
        return;
    }
    if (!strcmp(cmd, "s")) {
        dbg_stck(d);
        return;
    }
    if (!strcmp(cmd, "u")) {
        dbg_dasm(d);
        return;
    }
    if (!strcmp(cmd, "b")) {
        dbg_trap(d);
        return;
    }
    if (!strcmp(cmd, "g")) {
        dbg_goto(d);
        return;
    }
    if (!strcmp(cmd, "r")) {
        dbg_regs(d);
        return;
    }
    if (!strcmp(cmd, "o")) {
        dbg_iout(d);
        return;
    }
    if (!strcmp(cmd, "i")) {
        dbg_iin(d);
        return;
    }
    if (!strcmp(cmd, "y")) {
        dbg_bps(d);
        return;
    }
    if (!strcmp(cmd, "x")) {
        dbg_dbps(d);
        return;
    }
    if (!strcmp(cmd, "w")) {
        dbg_sregs(d);
        return;
    }
    d->out("unknown command: ");
    d->out(cmd);
    d->out("\n");
}

void dbg_quit(struct dbg* d) {
    d->z->dbg_on = 0;
    dbg_bp_delete_all(d);
    d->out("Exit debugger...\n");
    d->z->force_in = -1;
    d->console = 0;
}

void dbg_help(struct dbg* d) {
    d->out(
        "Z80 CPU debugger version " DBG_VERSION "\n\n"
        "Commands:\n\tq, quit - exit debugger\n"
        "\th, help - this text\n"
        "\tt [steps] [out] - trace 'steps' instructions.\n"
        //	    "\tp [steps] [out] - proceed 'steps' instructions.\n"
        "\tv <address> - display value of variable 8 and 16 bit at given address\n"
        "\td [address] [n x 16] - display n lines of memory map (one line - 16 bytes)\n"
        "\ts [n] - display stack\n"
        "\ta <address data,....> - insert opcodes at specified address, succesive opcodes are comma separated,\n"
        "\tu [address] [n] - disassemble code\n"
        "\tb [address] [condition] - set trap\n"
        "\ty - show all breakpoints\n"
        "\tx [nb] - remove breakpoint\n"
        "\tg [address] - run\n"
        "\tr <register_name=value,....> - set new register value\n"
        "\tw - show all registers\n"
        "\to <address> <value> - out instruction\n"
        "\ti <address> <value> - give next 'in' instruction value\n"
        "\n"
    );
}

void dbg_trap(struct dbg* d) {
    char addrs[7] = "";
    char slife[8] = "";
    char cnd[3] = "";
    char cond = 0;
    int addr = 0;
    int life = 1;
    struct dbg_bp *tmp, *n;

    /* pobranie argumentów komendy */
    sscanf(d->cmdline, "b %6s %2s %8s", addrs, cnd, slife);

    /* test czy podano drugi parametr */
    if (cnd[0]) {
        if (!strcmp(cnd, "eq"))
            cond = 1;
        if (!strcmp(cnd, "gt"))
            cond = 2;
        if (!strcmp(cnd, "lt"))
            cond = 3;
        if (!cond) {
            d->out("unknown condition name: '");
            d->out(cnd);
            d->out("' valid conditions are eq,gt,lt\n");
            return;
        }
    }
    else
        cond = 1;

    /* test czy podano pierwszy argument */
    if (addrs[0]) {
        if (addrs[0] < '0' || addrs[0] > '9') {
            addr = *d->z->PC_r;
            if (!strcmp(addrs, "eq"))
                cond = 0;
            if (!strcmp(addrs, "gt"))
                cond = 1;
            if (!strcmp(addrs, "lt"))
                cond = 2;
            if (!cond) {
                d->out("unknown condition name: '");
                d->out(addrs);
                d->out("' valid conditions are eq,gt,lt\n");
                return;
            }
        }
        else {
            if (dbg_dcheck(addrs)) {
                d->out("Address parameter is not a digit.\n");
                return;
            }
            addr = strtol(addrs,NULL, 0);
        }
    }
    else
        addr = *d->z->PC_r;

    if (slife[0]) {
        if (slife[0] < '0' || slife[0] > '9') {
            d->out("3-rd is a not numerical argument\n");
            return;
        }
        if (dbg_dcheck(slife)) {
            d->out("Life parameter is not a digit.\n");
            return;
        }
        life = strtol(slife,NULL, 0);
    }

    /* argumenty pobrane czas dodać pułapkę do listy */
    tmp = (struct dbg_bp*)malloc(sizeof(struct dbg_bp));
    tmp->next = NULL;
    tmp->cond_type = cond;
    tmp->address = addr;
    tmp->life = life;
    n = d->bp;
    if (!n)
        d->bp = tmp;
    else {
        while (n->next)
            n = n->next;
        n->next = tmp;
    }
}

void dbg_bps(struct dbg* d) {
    char* nme[3] = {"eq", "gt", "lt"};
    char tmp[256] = "";
    struct dbg_bp* bp = d->bp;
    int i = 0;

    d->out("Breakpoints:\nnb\taddress\tcondition\tlife\n");
    while (bp) {
        sprintf(tmp, "%3i.\t%6x\t%s\t%6x\n", i++, bp->address, nme[(int)bp->cond_type], bp->life);
        d->out(tmp);
        bp = bp->next;
    }
}

void dbg_bp_delete(struct dbg* d, int k) {
    struct dbg_bp* bp = d->bp, *old_bp;
    int i = 0;

    old_bp = bp;
    if (!bp)
        return;
    while (bp->next && i != k) {
        old_bp = bp;
        bp = bp->next;
        i++;
    }

    if (!i) {
        old_bp = bp->next;
        free(bp);
        d->bp = old_bp;
        return;
    }

    old_bp->next = bp->next;
    free(bp);
}

void dbg_bp_delete_first(struct dbg* d) {
    dbg_bp_delete(d, 0);
}

void dbg_bp_delete_all(struct dbg* d) {
    struct dbg_bp *bp, *b;

    if (!d->bp)
        return;

    bp = d->bp;

    while (bp->next) {
        b = bp->next;
        free(bp);
        bp = b;
    }
}

void dbg_dbps(struct dbg* d) {
    char tmp[6] = "";
    int k = -1;

    sscanf(d->cmdline, "x %3s", tmp);
    if (*tmp) {
        if (dbg_dcheck(tmp)) {
            d->out("Parameter is not a digit.\n");
            return;
        }
        k = strtol(tmp,NULL, 0);
    }
    dbg_bp_delete(d, k);
}

unsigned char dbg_peek(struct dbg* d, int addrs) {
    return d->z->memory_read(d->z, addrs);
}

void dbg_memo(struct dbg* d) {
    char tmp0[8] = "", tmp1[8] = "", tmp2[128] = "", tmp3[64] = "";
    int i, j;
    unsigned char k;

    sscanf(d->cmdline, "d %6s %6s", tmp0, tmp1);
    if (*tmp0) {
        if (dbg_dcheck(tmp0)) {
            d->out("Parameter is not a digit.\n");
            return;
        }
        d->daddr = strtol(tmp0,NULL, 0);
    }
    if (*tmp1) {
        if (dbg_dcheck(tmp1)) {
            d->out("Parameter is not a digit.\n");
            return;
        }
        d->dline = strtol(tmp1,NULL, 0);
    }
    for (j = 0; j < d->dline; j++) {
        *tmp2 = '\0';
        *tmp3 = '\0';
        dbg_xc2hexstr(d->daddr, tmp2);
        strcat(tmp2, "  ");
        for (i = 0; i < 16; i++) {
            dbg_oc2hexstr(k = dbg_peek(d, d->daddr + i), tmp0);
            strcat(tmp2, tmp0);
            strcat(tmp2, i < 15 ? "," : "");
            tmp0[0] = (k >= ' ' && k < 0x80) ? k : '.';
            tmp0[1] = '\0';
            strcat(tmp3, tmp0);
        }
        strcat(tmp2, "  ");
        strcat(tmp2, tmp3);
        strcat(tmp2, "\n");
        d->out(tmp2);
        d->daddr += 16;
    }
}

void dbg_var(struct dbg* d) {
    char tmp0[64] = "";
    int addr = 0;
    unsigned char obit = 0;
    unsigned short int xbit = 0;

    sscanf(d->cmdline, "v %7s", tmp0);
    if (!tmp0[0]) {
        d->out("Missing address parameter for v command.\n");
        return;
    }
    if (dbg_dcheck(tmp0)) {
        d->out("Parameter is not a digit.\n");
        return;
    }
    addr = strtol(tmp0,NULL, 0);
    if (addr < 0 || addr > 0xffff) {
        d->out("Address parameter is out of range.");
        return;
    }
    obit = dbg_peek(d, addr);
    xbit = (int)obit * 256 + dbg_peek(d, addr + 1);
    sprintf(tmp0, "8bit: mem[%x]=%x,  16bit: mem[%x]=%x\n", addr, obit, addr, xbit);
    d->out(tmp0);
}

void dbg_stck(struct dbg* d) {
    char tmp[8] = "";
    int lines = 4;
    int i = 0, t = 0;

    sscanf(d->cmdline, "s %6s", tmp);
    if (tmp[0]) {
        if (dbg_dcheck(tmp)) {
            d->out("Parameter is not a digit.\n");
            return;
        }
        lines = strtol(tmp,NULL, 0);
    }

    d->out("Stack:\n");
    for (i = 0; i < lines; i++) {
        t = dbg_peek(d, (*d->z->SP_r) + i * 2 + 1) * 256 + dbg_peek(d, (*d->z->SP_r) + i * 2);
        dbg_xc2hexstr(t, tmp);
        strcat(tmp, "\n");
        d->out(tmp);
    }
}

void dbg_pokebyte(struct dbg* d, int addr, unsigned char data) {
    d->z->memory_write(d->z, addr, data);
}

void dbg_poke(struct dbg* d) {
    char tmp[256] = "", t[8] = "";
    int addr = 0, i, k, j, data;

    sscanf(d->cmdline, "a %6s %250s", t, tmp);
    if (!t[0]) {
        d->out("Missing address parameter.\n");
        return;
    }
    if (!tmp[0]) {
        d->out("Missing data.\n");
        return;
    }
    if (dbg_dcheck(t)) {
        d->out("Adress parameter is not a digit.\n");
        return;
    }
    addr = strtol(t,NULL, 0);
    i = k = 0;
    do {
        for (; i < (int)strlen(tmp) && tmp[i] != ','; i++);
        if (tmp[i] == ',')
            i++;
        if (i - k > 7) {
            d->out("To long data parameter.\n");
            return;
        }
        for (j = 0; j < i - k; j++)
            t[j] = tmp[k + j];
        t[j] = '\0';
        if (j > 0)
            if (t[j - 1] == ',')
                t[j - 1] = '\0';

        if (dbg_dcheck(t)) {
            d->out("Data parameter '");
            d->out(t);
            d->out("' is not a digit.\n");
            return;
        }
        data = strtol(t,NULL, 0);
        if (data < 0 || data > 0xff) {
            d->out("Data parameter '");
            d->out(t);
            d->out("' is out of range 0..255.\n");
            return;
        }
        dbg_pokebyte(d, addr, data);
        addr++;
        k = i;
    }
    while (i < (int)strlen(tmp));
}

#define M(bit) (f >> bit) & 0x01

void dbg_sregs(struct dbg* d) {
    char tmp[1024] = "";
    unsigned char f = *d->z->F_r;
    char af[5] = "", bc[5] = "", de[5] = "", hl[5] = "";
    char af_[5] = "", bc_[5] = "", de_[5] = "", hl_[5] = "";
    char ix[5] = "", iy[5] = "", sp[5] = "", pc[5] = "";

    dbg_xc2hexstr(*d->z->AF_r, af);
    dbg_xc2hexstr(*d->z->BC_r, bc);
    dbg_xc2hexstr(*d->z->DE_r, de);
    dbg_xc2hexstr(*d->z->HL_r, hl);

    if (d->z->alt_af)
        dbg_xc2hexstr(d->z->AFs0, af_);
    else
        dbg_xc2hexstr(d->z->AFs1, af_);

    if (d->z->alt_rg) {
        dbg_xc2hexstr(d->z->BCs0, bc_);
        dbg_xc2hexstr(d->z->DEs0, de_);
        dbg_xc2hexstr(d->z->HLs0, hl_);
    }
    else {
        dbg_xc2hexstr(d->z->BCs1, bc_);
        dbg_xc2hexstr(d->z->DEs1, de_);
        dbg_xc2hexstr(d->z->HLs1, hl_);
    }

    dbg_xc2hexstr(*d->z->IX_r, ix);
    dbg_xc2hexstr(*d->z->IY_r, iy);
    dbg_xc2hexstr(*d->z->SP_r, sp);
    dbg_xc2hexstr(*d->z->PC_r, pc);

    sprintf(tmp,
            "AF=0x%s  BC=0x%s  DE=0x%s  HL=0x%s\n"
            "AF'=0x%s BC'=0x%s DE'=0x%s HL'=0x%s\n"
            "IX=0x%s  IY=0x%s  SP=0x%s  PC=0x%s\n"
            "I=0x%x R=0x%x IM=%i IFF1=%i IFF2=%i HALT=%i\n"
            "FLAGS: S Z 5 H 3 V N C\n"
            "       %i %i %i %i %i %i %i %i\n"
            "TICKS:  %li\n"
            "CYCLES: %li\n",
            af, bc, de, hl,
            af_, bc_, de_, hl_,
            ix, iy, sp, pc,
            d->z->Is, d->z->Rs, d->z->imode, d->z->iff1, d->z->iff2, d->z->halt,
            M(7),M(6),M(5),M(4),M(3),M(2),M(1),M(0),
            d->z->ticks, d->z->cycles
    );
    d->out(tmp);
}

char dbg_setreg(struct dbg* d, char* t) {
    char tmp0[8] = "", tmp1[8] = "";
    int i, val;

    for (i = 0; i < (int)strlen(t) && t[i] != '='; i++);
    if (t[i] != '=') {
        d->out("Missing '=' operator for argument.\n");
        return 1;
    }
    if (i == 0) {
        d->out("Missing register name.\n");
        return 1;
    }

    strncpy(tmp0, t, i);
    tmp0[i] = '\0';
    strcpy(tmp1, t + i + 1);

    if (!tmp1[0]) {
        d->out("Missing value for register ");
        d->out(tmp0);
        d->out(".\n");
        return 1;
    }

    if (dbg_dcheck(tmp1)) {
        d->out("Value for register ");
        d->out(tmp0);
        d->out(" is not a digit.\n");
        return 1;
    }

    val = strtol(tmp1,NULL, 0);

    for (i = 0; i < (int)strlen(tmp0); i++)
        tmp0[i] = tolower(tmp0[i]);

    if (!strcmp(tmp0, "a"))
        *d->z->A_r = val;
    else if (!strcmp(tmp0, "b"))
        *d->z->B_r = val;
    else if (!strcmp(tmp0, "c"))
        *d->z->C_r = val;
    else if (!strcmp(tmp0, "d"))
        *d->z->D_r = val;
    else if (!strcmp(tmp0, "e"))
        *d->z->E_r = val;
    else if (!strcmp(tmp0, "h"))
        *d->z->H_r = val;
    else if (!strcmp(tmp0, "l"))
        *d->z->L_r = val;
    else if (!strcmp(tmp0, "a'")) {
        if (d->z->alt_af)
            d->z->As0 = val;
        else
            d->z->As1 = val;
    }
    else if (!strcmp(tmp0, "b'")) {
        if (d->z->alt_rg)
            d->z->Bs0 = val;
        else
            d->z->Bs1 = val;
    }
    else if (!strcmp(tmp0, "c'")) {
        if (d->z->alt_rg)
            d->z->Cs0 = val;
        else
            d->z->Cs1 = val;
    }
    else if (!strcmp(tmp0, "d'")) {
        if (d->z->alt_rg)
            d->z->Ds0 = val;
        else
            d->z->Ds1 = val;
    }
    else if (!strcmp(tmp0, "e'")) {
        if (d->z->alt_rg)
            d->z->Es0 = val;
        else
            d->z->Es1 = val;
    }
    else if (!strcmp(tmp0, "h'")) {
        if (d->z->alt_rg)
            d->z->Hs0 = val;
        else
            d->z->Hs1 = val;
    }
    else if (!strcmp(tmp0, "l'")) {
        if (d->z->alt_rg)
            d->z->Ls0 = val;
        else
            d->z->Ls1 = val;
    }
    else if (!strcmp(tmp0, "af"))
        *d->z->AF_r = val;
    else if (!strcmp(tmp0, "bc"))
        *d->z->BC_r = val;
    else if (!strcmp(tmp0, "de"))
        *d->z->DE_r = val;
    else if (!strcmp(tmp0, "hl"))
        *d->z->HL_r = val;
    else if (!strcmp(tmp0, "af'")) {
        if (d->z->alt_af)
            d->z->AFs0 = val;
        else
            d->z->AFs1 = val;
    }
    else if (!strcmp(tmp0, "bc'")) {
        if (d->z->alt_rg)
            d->z->BCs0 = val;
        else
            d->z->BCs1 = val;
    }
    else if (!strcmp(tmp0, "de'")) {
        if (d->z->alt_rg)
            d->z->DEs0 = val;
        else
            d->z->DEs1 = val;
    }
    else if (!strcmp(tmp0, "hl'")) {
        if (d->z->alt_rg)
            d->z->HLs0 = val;
        else
            d->z->HLs1 = val;
    }
    else if (!strcmp(tmp0, "ix"))
        *d->z->A_r = val;
    else if (!strcmp(tmp0, "iy"))
        *d->z->A_r = val;
    else if (!strcmp(tmp0, "sp"))
        *d->z->A_r = val;
    else if (!strcmp(tmp0, "pc"))
        *d->z->A_r = val;
    else if (!strcmp(tmp0, "i"))
        *d->z->A_r = val;
    else if (!strcmp(tmp0, "r"))
        *d->z->A_r = val;
    else {
        d->out("Unknown register name.\n");
        return 1;
    }
    return 0;
}

void dbg_regs(struct dbg* d) {
    int i, j, k;
    char tmp[1024] = "", tmp1[16];

    sscanf(d->cmdline, "r %s", tmp);

    if (!tmp[0]) {
        d->out("Missing argument.\n");
        return;
    }

    i = k = 0;
    do {
        tmp1[0] = '\0';
        for (; i < (int)strlen(tmp) && tmp[i] != ','; i++);
        if (tmp[i] == ',')
            i++;

        if (i - k > 15) {
            d->out("To long argument.\n");
            return;
        }

        for (j = 0; j < i - k; j++)
            tmp1[j] = tmp[k + j];
        tmp1[j] = '\0';
        if (j > 0)
            if (tmp1[j - 1] == ',')
                tmp1[j - 1] = '\0';

        if (dbg_setreg(d, tmp1))
            return;

        k = i;
    }
    while (i < (int)strlen(tmp));
}

void dbg_goto(struct dbg* d) {
    char tmp[8] = "";
    d->addr = -1;
    d->cond = 0;

    sscanf(d->cmdline, "g %6s", tmp);

    if (!tmp[0]) {
        if (d->bp) {
            d->addr = d->bp->address;
            d->cond = d->bp->cond_type;
            if ((--d->bp->life) <= 0)
                dbg_bp_delete_first(d);
        }
    }
    else {
        d->cond = C_PCEQ;
        if (dbg_dcheck(tmp)) {
            d->out("g command parameter incorrect.");
            return;
        }
        d->addr = strtol(tmp,NULL, 0);
    }
    d->console = 0;
}

void dbg_trace(struct dbg* d) {
    char tmp0[8] = "", tmp1[4] = "";
    int outs = 0;
    int cnt = 0;

    sscanf(d->cmdline, "t %6s %2s", tmp0, tmp1);

    if (tmp1[0] == '1')
        outs = 1;
    if (tmp0[0]) {
        if (dbg_dcheck(tmp0)) {
            d->out("Incorrect parameter.\n");
            return;
        }
        cnt = strtol(tmp0,NULL, 0);
        if (cnt <= 0) {
            d->out("Incorrect parameter.\n");
            return;
        }
        cnt--;
    }
    d->icnt = cnt;
    d->noregs = 0;
    d->deasm = d->icnt ? outs : 1;
    d->trace = 1;
    d->console = 0;
}

void dbg_proc(struct dbg* d) {
    char tmp0[8] = "", tmp1[4] = "";
    int outs = 0;
    int cnt = 0;

    sscanf(d->cmdline, "p %6s %2s", tmp0, tmp1);

    if (tmp1[0] == '1')
        outs = 1;
    if (tmp0[0]) {
        if (dbg_dcheck(tmp0)) {
            d->out("Incorrect parameter.\n");
            return;
        }
        cnt = strtol(tmp0,NULL, 0);
        if (cnt <= 0) {
            d->out("Incorrect parameter.\n");
            return;
        }
        cnt--;
    }
    d->icnt = cnt;
    d->noregs = 0;
    d->deasm = d->icnt ? outs : 1;
    d->trace = 1;
    d->console = 0;
    /************** dodać ******************/
}

/* zapis przy deasemblacji ma iść w pustkę */
void dbg_dummy_wr(const void* z, ushort addr, unsigned char data) {
    (void)z;
    (void)addr;
    (void)data;
}

void dbg_dummy_output(void* z, ushort addr, unsigned char data) {
    (void)z;
    (void)addr;
    (void)data;
}

unsigned char dbg_dummy_input(const void* z, ushort addr) {
    (void)z;
    (void)addr;
    return 0;
}

static void dbg_debug(const void* z, int mode, unsigned char data) {
    dbg_call(((const struct cpu*)z)->ptx, mode, data);
}

void dbg_deasembly(struct dbg* d) {
    int i;
    struct cpu z;
    struct dbg dr;

    z80_reset(&z);
    z.memory_read = d->z->memory_read;
    z.memory_write = dbg_dummy_wr;
    z.io_read = dbg_dummy_input;
    z.io_write = dbg_dummy_output;
    z.dbg_on = 1;
    z.ptx = &dr;
    z.ptr = d->z->ptr;
    z.debug = dbg_debug;
    z.jmp_off = 1;
    *z.PC_r = d->zaddr;
    memset(&dr, 0, sizeof(struct dbg));
    dr.noregs = 1;
    dr.deasm = 1;
    dr.z = &z;
    dr.out = d->out;

    for (i = 0; i < d->zline; i++)
        z80_step(&z);
    d->zaddr = *z.PC_r;
}

void dbg_dasm(struct dbg* d) {
    char tmp0[8] = "", tmp1[8] = "";

    sscanf(d->cmdline, "u %6s %6s", tmp0, tmp1);

    if (tmp1[0]) {
        if (dbg_dcheck(tmp1)) {
            d->out("Incorrect parameter.\n");
            return;
        }
        d->zline = strtol(tmp1, NULL, 0);
    }

    if (tmp0[0]) {
        if (dbg_dcheck(tmp0)) {
            d->out("Incorrect parameter.\n");
            return;
        }
        d->zaddr = strtol(tmp0, NULL, 0);
    }

    dbg_deasembly(d);
}

void dbg_iout(struct dbg* d) {
    char tmp0[8] = "", tmp1[8] = "";
    int addr = 0, value = 0;

    sscanf(d->cmdline, "o %6s %4s", tmp0, tmp1);

    if (!tmp0[0] || !tmp1[0]) {
        d->out("Missing parameter.\n");
        return;
    }

    if (dbg_dcheck(tmp0) || dbg_dcheck(tmp1)) {
        d->out("Incorrect parameter.\n");
        return;
    }
    addr = strtol(tmp0,NULL, 0);
    value = strtol(tmp1,NULL, 0);
    if (addr < 0 || addr > 0xffff || value < 0 || value > 0xff) {
        d->out("Parameter out of range.\n");
        return;
    }

    d->z->io_write(d->z, addr, value);
}

void dbg_iin(struct dbg* d) {
    char tmp0[8] = "", tmp1[8] = "";
    int addr = 0, value = 0;

    sscanf(d->cmdline, "i %6s %4s", tmp0, tmp1);

    if (!tmp0[0] || !tmp1[0]) {
        d->out("Missing parameter.\n");
        return;
    }

    if (dbg_dcheck(tmp0) || dbg_dcheck(tmp1)) {
        d->out("Incorrect parameter.\n");
        return;
    }
    addr = strtol(tmp0,NULL, 0);
    value = strtol(tmp1,NULL, 0);
    if (addr < 0 || addr > 0xffff || value < 0 || value > 0xff) {
        d->out("Parameter out of range.\n");
        return;
    }
    d->z->force_in = value;
}

void dbg_sout(char* a) {
    printf("%s", a);
}

char dbg_sin() {
    return getchar();
}

void dbg_init(struct dbg* d, struct cpu* z, char deb_on, char console_on) {
    if (!d || !z)
        return;
    memset(d, 0, sizeof(struct dbg));
    d->z = z;
    d->out = dbg_sout;
    d->in = dbg_sin;
    d->dline = 8;
    d->zline = 16;
    d->console = console_on;
    z->dbg_on = deb_on;
    z->ptx = d;
    z->debug = dbg_debug;
}

void dbg_set_stream(struct dbg* d, char (*s_in)(void), void (*s_out)(char*)) {
    d->out = s_out;
    d->in = s_in;
}

void dbg_don(struct dbg* d, char flag) {
    d->z->dbg_on = flag;
}

void dbg_con(struct dbg* d, char flag) {
    d->console = flag;
}
