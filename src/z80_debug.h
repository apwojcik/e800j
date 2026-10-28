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

#pragma once

#define MAX_BYTES 32
#define LINE_LEN  256
#define MAX_CMDCHR 256

#define DBG_VERSION "0.1"

enum {
    C_PCEQ = 1,
    C_PCGT,
    C_PCLT,
    C_CNTR
};

struct dbg_bp {
    char cond_type; /* typ warunku    */
    int address; /* wartość adresu */
    int life; /* ilość warunków  'czas życia warunku' */
    void* next; /* następny warunek */
};

struct cpu;

struct dbg {
    char mach[MAX_BYTES]; /* kod maszynowy instrukcji */
    char cmdline[MAX_CMDCHR];
    char line[LINE_LEN]; /* bufor wyniku zdeasemblowanej linii */
    int len; /* długość łańcucha kodu maszynowego */
    int icnt; /* licznik instrukcji */
    char console; /* włączenie konsoli */
    char deasm; /* włączenie wyświetlania dekodowanej instr */
    char noregs; /* jeśli ustawiony, nie wyświetla stanu rejestrów */
    char trace; /* flaga trybu śledzenia */
    void (*out)(char*); /* funkcja zwrotna do wyrzucania tekstu */
    char (*in)();

    struct cpu* z;
/* warunek załaczenia */
    char cond;
/* wartosc załaczenia */
    int addr;
    /* ilość instrukcji do wykonania */
    int cntr;
    struct dbg_bp* bp; /* break pointy */
    /* adres przy wyświetlaniu mapy pamięci */
    int daddr;
    int dline;
    /* adres deasemblacji */
    int zaddr;
    int zline;
};

enum {
    DBG_OFF = 0,
    DBG_ON = 1
};

/*
    funkcja łącząca debugger z emulacją CPU i inicjująca debugger
    komunikaty konsolowe I/O są wyprowadzane domyślnie na stdin/stdout, chyba
    że zostanie użyta funkcja dbg_set_stream.
*/
void dbg_init(struct dbg* d, struct cpu* z, char debugg_on, char console_on);

/*
    przekierowanie strumienia komunikatów I/O
*/
void dbg_set_stream(struct dbg* d, char (*s_in)(void), void (*s_out)(char*));

/*
    włączenie/wyłączenie debuggera
*/
void dbg_don(struct dbg* d, char flag);

/*
    włączenie/wyłączenie konsoli
*/
void dbg_con(struct dbg* d, char flag);
