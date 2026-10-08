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

#ifndef __Z80_DEBUG__
#define __Z80_DEBUG__

#define MAX_BYTES 32
#define LINE_LEN  256
#define MAX_CMDCHR 256

#define DBG_VERSION "0.1"

enum{
    C_PCEQ=1,
    C_PCGT,
    C_PCLT,
    C_CNTR
};

typedef struct
{
    char cond_type;	/* typ warunku    */
    int  address;	/* warto¶æ adresu */
    int  life;		/* ilo¶æ warunków  'czas ¿ycia warunku' */
    void *next;		/* nastêpny warunek */
} dbg_bp;

typedef struct
{
    char mach[MAX_BYTES]; 	/* kod maszynowy instrukcji */
    char cmdline[MAX_CMDCHR];
    char line[LINE_LEN]; 	/* bufor wyniku zdeasemblowanej linii */
    int  len;  			/* d³ugo¶æ ³añcucha kodu maszynowego */
    int  icnt; 			/* licznik instrukcji */
    char console;   		/* w³±czenie konsoli */    
    char deasm;			/* w³±czenie wy¶wietlania dekodowanej instr */
    char noregs;		/* jesli ustawiony, nie wy¶wietla stanu rejestrów */
    char trace;			/* flaga trybu ¶ledzenia */
    void (*out)(char *);    	/* funkcja zwrotna do wyrzucania tekstu */
    char (*in)();
    z80  *z;
/* warunek za³aczenia */
    char cond;
/* wartosc za³aczenia */
    int addr;
/* ilo¶æ instrukcji do wykonania */
    int cntr;
    dbg_bp *bp; /* break pointy */
/* adres przy wy¶wietlaniu mapy pamiêci */
    int daddr;
    int dline;
/* adres deasemblacji */    
    int zaddr;
    int zline;
} dbg;

enum{
    DBG_OFF=0,
    DBG_ON =1
};

/*
    funkcja ³±cz±ca debugger z emulacj± CPU i inicjuj±ca debugger
    komunikaty konsolowe I/O s± wyprowadzane domy¶lnie na stdin/stdout, chyba
    ¿e zostanie u¿yta funkcja dbg_set_stream.
*/
extern void dbg_init(dbg *d, z80 *z, char debugg_on, char console_on);

/*
    przekierowanie strumienia komunikatów I/O
*/
extern void dbg_set_stream(dbg *d, char (*s_in)(void), void (*s_out)(char *));

/*
    w³±czenie/wy³±czenie debuggera
*/
extern void dbg_don(dbg *d, char flag);

/*
    w³±czenie/wy³±czenie konsoli
*/
extern void dbg_con(dbg *d, char flag);
#endif
