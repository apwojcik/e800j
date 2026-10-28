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
    z80_cpu.h
    Interfejs biblioteki emulacji Z80

    Autor: Krzysztof Komarnicki
    Ver: 0.1
    Data: 19.08.2006
    Licencja: GPL v2
*/

#pragma once

#ifndef BIG_ENDIAN
#define LITTLE_ENDIAN
#endif

#define Z80_LAST_READ_BYTE(str,data)	((struct z80*)(str))->last_mem_rd = data
#define Z80_LAST_WRITE_BYTE(str,data)	((struct z80*)(str))->last_mem_wr = data

#define Z80_DATA_SIZE 1024

typedef void (*debug_t)(const void*, int, unsigned char);
typedef void (*memory_write_t)(const void*, unsigned short, unsigned char);
typedef unsigned char (*memory_read_t)(const void*, unsigned short);
typedef void (*io_write_t)(void*, unsigned short, unsigned char);
typedef unsigned char (*io_read_t)(const void*, unsigned short);

struct cpu {
    union {
        struct {
            /* rejestry p-value */
            union {
                unsigned short* regs16[8];

                struct {
                    unsigned short *AF_r, *BC_r, *DE_r, *HL_r, *IX_r, *IY_r, *SP_r, *PC_r;
                };
            };

            union {
                unsigned char* regs8[20];

                struct {
                    unsigned char *A_r, *F_r, *B_r, *C_r, *D_r, *E_r, *H_r, *L_r;
                    unsigned char *XH_r, *XL_r, *YH_r, *YL_r, *I_r, *R_r;
                    unsigned char *MHL_r, *MIX_r, *MIY_r, *MSP_r, *MBC_r, *MDE_r;
                };
            };

            /* Rejestry l-value */
            union {
                struct {
                    unsigned short AFs0, BCs0, DEs0, HLs0, AFs1, BCs1, DEs1, HLs1, IXs, IYs;
                };

                struct {
                    unsigned char Fs0, As0, Cs0, Bs0, Es0, Ds0, Ls0, Hs0, Fs1, As1;
                    unsigned char Cs1, Bs1, Es1, Ds1, Ls1, Hs1, XLs, XHs, YLs, YHs;
                };
            };

            unsigned short SPs, PCs;
            unsigned char Is, Rs;
            unsigned char MHLs, MIXs, MIYs, MSPs, MBCs, MDEs;
            /* rejestry niejawne CPU */
            unsigned char iff1;
            unsigned char iff2;
            /* stany CPU */
            unsigned char imode; /* tryb przerwań                                       */
            unsigned char intr; /* wywołanie przerwania maskowalnego                   */
            unsigned char inmi; /* wywołanie przerwania niemaskowalnego                */
            unsigned char halt; /* CPU jest w stanie HALT                              */
            /* flagi dodatkowe, pomocnicze */
            unsigned char alt_rg; /* flaga określająca, który bank rejestrów jest aktywny */
            unsigned char alt_af; /* flaga określająca, który bank AF jest aktywny  */
            unsigned char ipend; /* jeśli ustawiona to instrukcja w toku */
            unsigned char irupt; /* wykonuje przerwanie */
            /* zmienne sterowania CPU */
            int pref_cb_ed;
            int pref_dd_fd;
            int ibus;
            unsigned char bus[8]; /* bajty wystawione na magistrale w trybie IM 0 */
            int    call_cnt;   /* licznik co ile ma być wywołana procedura w petli */
            /* liczniki */
            long int cycles; /* przechowuje ilość cykli  */
            long int ticks; /* przechowuje ilość taktów */
            int _tic; /* ilość taktów na wykonaną instrukcję */
            /* parametry wywołania instrukcji */
            unsigned char p0, p1, p2;
            unsigned char b, c, t;
            unsigned char a, o, x;
            /* zmienne tymczasowe */
            unsigned char dd0, dd1, dd3;
            unsigned short dd2;
            /* dodatkowe informacje dla debuggera i flagi sterujące */
            unsigned char int_off; /* jeśli ustawiona ignorowane są przerwania i instrukcja HALT */
            unsigned char jmp_off; /* jeśli ustawione nie wykonywane są skoki przez jp, djnz, jr, call, ret */
            unsigned char dbg_on; /* załączenie debuggera */
            unsigned char cpu_off; /* ustawienie ignoruje wywołanie funkcji CPU */
            unsigned char djnz; /* cpu wykonuje instrukcję DJNZ */
            unsigned short adrs; /* wartość PC pod którą zaczyna się instrukcja */
            unsigned char cmd_type; /* typ instrukcji */
            unsigned char reg80; /* rejestr 8 bit   pierwszy parametr lub warunek JP,JR,CALL,RET */
            unsigned char reg81; /* rejestr 8 bit   drugi parametr */
            unsigned char regx0; /* rejestr 16 bit  pierwszy parametr */
            unsigned char regx1; /* rejestr 16 bit  drugi parametr */
            unsigned char exit; /* powoduje wyjście z pętli */
            char dd; /* stała 8 bitowa  */
            unsigned short nn; /* stała 16 bitowa */
            unsigned char last_in_data; /* ostatnio pobrana dana z portu */
            unsigned short last_in_addr; /* ostatni adres pobierania danych z portu */
            unsigned char last_out_data; /* ostatnio wpisana dana do portu */
            unsigned short last_out_addr; /* ostatni adres odwołania do portu wyjściowego */
            unsigned short last_mem_rd; /* ostatnio odczytana dana z pamięci */
            unsigned short last_mem_wr; /* ostatnio zapisana dana do pamięci */
            unsigned int nested; /* wskazuje zagłębienie podprogramów */
        int    force_in;		/* jesli > 0 wartośc będzie odczytana przez IN */
        }; // koniec struktury
        char z80_data[Z80_DATA_SIZE]; // maksymalny dopuszczalny rozmiar pola danych
    }; // koniec unii

    /* funkcje zewnętrzne */
    debug_t debug;               // funkcja debuggera

    memory_read_t memory_read;   // funkcja odczytu z pamięci
    memory_write_t memory_write; // funkcja zapisu do pamięci
    io_read_t io_read;           // funkcja odczytu IO
    io_write_t io_write;         // funkcja zapisu do IO

    void* ptr; // dowolny wskaźnik
    void* ptx; // dowolny wskaźnik
};

/* ID instrukcji i index w tabeli DEB_DEC_TABL */
enum {
    D_NOP = 1, D_LDB, D_LDBN, D_LDBRM, D_LDBMR, D_LDBMN, D_LDBMNR, D_LDW,
    D_LDWN, D_LDWRMN, D_LDWMNR, D_HALT, D_IM, D_DI, D_EI, D_JR, D_JP,
    D_DJNZ, D_CALL, D_RST, D_RET, D_RETI, D_RETN, D_POP, D_PUSH, D_EXX,
    D_EX, D_DAA, D_SCF, D_CCF, D_CPL, D_BIT, D_BITM, D_RES, D_RESM, D_SET,
    D_SETM, D_AND, D_ANDN, D_ANDM, D_OR, D_ORN, D_ORM, D_XOR, D_XORN, D_XORM,
    D_RLC, D_RLCM, D_RRC, D_RRCM, D_RL, D_RLM, D_RR, D_RRM, D_SLA, D_SLAM, D_SRA,
    D_SRAM, D_SLI, D_SLIM, D_SRL, D_SRLM, D_RRD, D_RLD, D_INA, D_OUTA, D_INBC, D_OUTBC,
    D_ADD8, D_ADDN8, D_ADDM8, D_ADC8, D_ADCN8, D_ADCM8,
    D_SUB8, D_SUBN8, D_SUBM8, D_SBC8, D_SBCN8, D_SBCM8, D_CP, D_CPN, D_CPM,
    D_INC8, D_INCM8, D_DEC8, D_DECM8, D_INCX, D_DECX, D_NEG, D_ADDX, D_ADCX, D_SBCX, D_LDBL, D_JPHL,
    D_LDBRMN, D_EXSP, D_CPBL, D_INBL, D_OUTBL
};

/* tryby debuggera */
enum {
    D_PRE = 0x00,
    D_FETCH = 0xfe,
    D_FLUSH = 0xff,
    D_TESTIN = 0xfd
};

void z80_init(struct cpu*, memory_read_t, memory_write_t, io_read_t, io_write_t, void* ptr, void* ptx);
void z80_reset(struct cpu*);
void z80_step(struct cpu*);

/* wykonuje pętlę zadaną ilością taktów, w trakcie wywołuje funkcję
   callback z parametrem callback_param (jesli nie NULL)
   icall ilość obiegów pętli, po których zostanie wywołany callback
   zwraca ilość nadmiarowych taktów, nie mieszczacych się w zadanych ilościach taktów do wykonania
   funkcja callback przyjmuje dwa parametry, pierwszym z nich jest przepisywany wskaźnik
   callback_param, zaś drugim liczba taktów CPU.
*/
void z80_loop(struct cpu*, int cnt, int icall, void (*callback)(struct cpu*, void*), void* callback_param);
void z80_nmi(struct cpu*);
void z80_int(struct cpu*, unsigned char* bus);
