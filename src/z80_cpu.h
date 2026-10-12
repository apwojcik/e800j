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

#ifndef __cplusplus
#define Z80_CPU(z)	z->
#define Z80_PTR		z
#define Z80_CLASS
#define Z80_STRUCT	z80 *z
#define Z80_STRUCTC	z80 *z,
#define Z80_STRUCT_NAME z80
#define Z80_CLASS_NAME  typedef struct z80
#else
#define Z80_CPU(z)
#define Z80_PTR		this
#define Z80_CLASS	z80::
#define Z80_STRUCT
#define Z80_STRUCTC
#define Z80_STRUCT_NAME
#define Z80_CLASS_NAME  class z80
#endif

#ifndef ushort
#define ushort unsigned short
#endif

#ifndef uchar
#define uchar unsigned char
#endif

#define Z80_LAST_READ_BYTE(str,data)	((z80*)(str))->last_mem_rd = data
#define Z80_LAST_WRITE_BYTE(str,data)	((z80*)(str))->last_mem_wr = data

#define Z80_DATA_SIZE 1024

Z80_CLASS_NAME {
#ifdef __cplusplus
    public :
#endif
    union {
        struct {
            /* rejestry p-value */
            union {
                ushort* regs16[8];

                struct {
                    ushort *AF_r, *BC_r, *DE_r, *HL_r, *IX_r, *IY_r, *SP_r, *PC_r;
                };
            };

            union {
                uchar* regs8[20];

                struct {
                    uchar *A_r, *F_r, *B_r, *C_r, *D_r, *E_r, *H_r, *L_r;
                    uchar *XH_r, *XL_r, *YH_r, *YL_r, *I_r, *R_r;
                    uchar *MHL_r, *MIX_r, *MIY_r, *MSP_r, *MBC_r, *MDE_r;
                };
            };

            /* Rejestry l-value */
            union {
                struct {
                    ushort AFs0, BCs0, DEs0, HLs0, AFs1, BCs1, DEs1, HLs1, IXs, IYs;
                };

                struct {
                    uchar Fs0, As0, Cs0, Bs0, Es0, Ds0, Ls0, Hs0, Fs1, As1;
                    uchar Cs1, Bs1, Es1, Ds1, Ls1, Hs1, XLs, XHs, YLs, YHs;
                };
            };

            ushort SPs, PCs;
            uchar Is, Rs;
            uchar MHLs, MIXs, MIYs, MSPs, MBCs, MDEs;
            /* rejestry niejawne CPU */
            uchar iff1;
            uchar iff2;
            /* stany CPU */
            uchar imode; /* tryb przerwań                                       */
            uchar intr; /* wywołanie przerwania maskowalnego                   */
            uchar inmi; /* wywołanie przerwania niemaskowalnego                */
            uchar halt; /* CPU jest w stanie HALT                              */
            /* flagi dodatkowe, pomocnicze */
            uchar alt_rg; /* flaga określająca który bank rejestrów jest aktywny */
            uchar alt_af; /* flaga określająca który bank AF jest aktywny  */
            uchar ipend; /* jeśli ustawiona to instrukcja w toku */
            uchar irupt; /* wykonuje przerwanie */
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
            uchar p0, p1, p2;
            uchar b, c, t;
            uchar a, o, x;
            /* zmienne tymczasowe */
            uchar dd0, dd1, dd3;
            ushort dd2;
            /* dodatkowe informacje dla debuggera i flagi sterujące */
            uchar int_off; /* jeśli ustawiona ignorowane są przerwania i instrukcja HALT */
            uchar jmp_off; /* jeśli ustawione nie wykonywane są skoki przez jp, djnz, jr, call, ret */
            uchar dbg_on; /* załączenie debuggera */
            uchar cpu_off; /* ustawienie ignoruje wywołanie funkcji CPU */
            uchar djnz; /* cpu wykonuje instrukcję DJNZ */
            ushort adrs; /* wartość PC pod którą zaczyna się instrukcja */
            uchar cmd_type; /* typ instrukcji */
            uchar reg80; /* rejestr 8 bit   pierwszy parametr lub warunek JP,JR,CALL,RET */
            uchar reg81; /* rejestr 8 bit   drugi parametr */
            uchar regx0; /* rejestr 16 bit  pierwszy parametr */
            uchar regx1; /* rejestr 16 bit  drugi parametr */
            uchar exit; /* powoduje wyjście z pętli */
            char dd; /* stała 8 bitowa  */
            ushort nn; /* stała 16 bitowa */
            uchar last_in_data; /* ostatnio pobrana dana z portu */
            ushort last_in_addr; /* ostatni adres pobierania danych z portu */
            uchar last_out_data; /* ostatnio wpisana dana do portu */
            ushort last_out_addr; /* ostatni adres odwołania do portu wyjściowego */
            ushort last_mem_rd; /* ostatnio odczytana dana z pamięci */
            ushort last_mem_wr; /* ostatnio zapisana dana do pamięci */
            unsigned int nested; /* wskazuje zagłębienie podprogramów */
        int    force_in;		/* jesli > 0 wartośc będzie odczytana przez IN */
        }; /*koniec struktury */
        char z80_data[Z80_DATA_SIZE]; /* maksymalny dopuszczalny rozmiar pola danych */
    }; /* koniec unii */
#ifdef __cplusplus
    public :
    void z80_reset();
    void z80_nmi();
    void z80_int(uchar* bus);
    void z80_loop(int cnt, int call, void* param);

    virtual void callback(void* param);
    virtual uchar mem_rd(void* ptr, ushort addr); /* funkcja odczytu z pamięci */
    virtual void mem_wr(void* ptr, ushort addr, uchar data); /* funkcja zapisu do pamięci */
    virtual uchar input(void* ptr, ushort addr); /* funkcja odczytu IO        */
    virtual void output(void* ptr, ushort addr, uchar data); /* funkcja zapisu do IO      */
    virtual void debug(void* ptr, int mode, uchar data); /* funkcja debuggera         */
#else
    /* funkcje zewnętrzne */
    uchar (*mem_rd)(void* ptr, ushort addr); /* funkcja odczytu z pamięci */
    void (*mem_wr)(void* ptr, ushort addr, uchar data); /* funkcja zapisu do pamięci */
    uchar (*input)(void* ptr, ushort addr); /* funkcja odczytu IO        */
    void (*output)(void* ptr, ushort addr, uchar data); /* funkcja zapisu do IO      */
    void (*debug)(void* ptr, int mode, uchar data); /* funkcja debuggera         */
#endif
    void* ptr; /* dowolny wskaźnik          */
    void* ptx; /* dowolny wskaźnik          */
} Z80_STRUCT_NAME;

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

#define TAB_REGS8_NME { "A","F","B","C","D","E","H","L","XH","XL"\
            ,"YH","YL","I","R","(HL)","(IX)","(IY)","(SP)","(BC)","(DE)" }
#define TAB_REGSX_NME { "AF","BC","DE","HL","IX","IY","SP","PC","AF","AF'" }
#define TAB_RCND_NME  { "","NZ","Z","NC","C","PO","PE","P","M" }
#define DEB_DEC_TABL	\
{\
    "-","NOP","LD   %r,%z","LD   %r,%n","LD   %r,(%m)","LD   (%m),%r","LD   (%m),%f","LD   (%N),%r",\
    "LD   %R,%Z","LD   %R,%N  ","LD   %R,(%N)","LD   (%N),%R","HALT","IM   %n","DI","EI","JR   %c%N",\
    "JP   %c%N","DJNZ %N","CALL %c%N","RST  %n","RET  %c","RETI","RETN","POP  %R","PUSH %R","EXX",\
    "EX   %R,%Z","DAA","SCF","CCF","CPL","BIT  %b,%r","BIT  %b,(%m)","RES  %b,%r","RES  %b,(%m)",\
    "SET  %b,%r","SET  %b,(%m)","AND  %r","AND  %n","AND  (%m)","OR   %r","OR   %n","OR   (%m)",\
    "XOR  %r","XOR  %n","XOR  (%m)","RLC  %r","RLC  (%m)","RRC  %r","RRC  (%m)","RL   %r","RL   (%m)",\
    "RR   %r","RR   (%m)","SLA  %r","SLA  (%m)","SRA  %r","SRA  (%m)","SLI  %r","SLI  (%m)","SRL  %r",\
    "SRL  (%m)","RRD","RLD","IN   A,(%n)","OUT  (%n),A","IN   %r,(C)","OUT  (C),%r","ADD  %r",\
    "ADD  %n","ADD  (%m)","ADC  %r","ADC  %n","ADC  (%m)","SUB  %r","SUB  %n","SUB  (%m)","SBC  %r",\
    "SBC  %n","SBC  (%m)","CP   %r","CP   %n","CP   (%m)","INC  %r","INC  (%m)","DEC  %r","DEC  (%m)",\
    "INC  %R","DEC  %R","NEG","ADD  %R,%Z","ADC  %R,%Z","SBC  %R,%Z","LD%w","JP  (%R)","LD %r,(%N)",\
    "EX   (SP),%R", "CP%w", "IN%w", "OUT%w"\
}

#ifndef __cplusplus
/* reset CPU */

extern void z80_init(
    z80*, /* wskaźnik na objekt z80  */
    uchar (*mem_rd)(void* z, ushort addr), /* funkcja odczytu pamięci */
    void (*mem_wr)(void* z, ushort addr, uchar data), /* funkcja zapisu pamięci  */
    uchar (*io_rd)(void* z, ushort addr), /* funkcja odczytu I/O     */
    void (*io_wr)(void* z, ushort addr, uchar data), /* funkcja zapisu I/O      */
    void* ptr, /* wskaźnik do własnego użytku 1 */
    void* ptx /* wskaźnik do własnego użytku 1 */
);

extern void z80_reset(z80*);

extern void z80_step(z80*);

/* wykonuje pętlę zadaną ilością taktów, w trakcie wywołuje funkcję
   callback z parametrem callback_param (jeśli nie NULL)
   icall ilość obiegów pętli, po których zostanie wywołany callback
   zwraca ilość nadmiarowych taktów, nie mieszczacych się w zadanych ilościach taktów do wykonania
   funkcja callback przyjmuje dwa parametry, pierwszym z nich jest przepisywany wskaźnik
   callback_param, zaś drugim liczba taktów CPU.
*/
extern void z80_loop(z80*, int cnt, int icall, void (*callback)(z80*, void*), void* callback_param);

extern void z80_nmi(z80*);

extern void z80_int(z80*, uchar* bus);
#endif