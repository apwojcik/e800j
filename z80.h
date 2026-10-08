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

#ifndef __Z80H__
#define __Z80H__

#define DEBUGGER	/* Czy ma byæ wkompilowany debugger */

#ifndef _word_
typedef unsigned short int word;
#define _word_
#endif

#ifndef _dword_
typedef unsigned int dword;
#define _dword_
#endif


#ifndef _byte_
typedef unsigned char byte;
#define _byte_
#endif

typedef struct
{
    byte *Ap;
    byte *Fp;
    byte *Bp;
    byte *Cp;
    byte *Dp;
    byte *Ep;
    byte *Hp;
    byte *Lp;

    word *AFp;
    word *BCp;
    word *DEp;
    word *HLp;
    
    word AFr;
    word BCr;
    word DEr;
    word HLr;

    word AFr_;
    word BCr_;
    word DEr_;
    word HLr_;

    union{
	struct{
	    word IXp;
	    word IYp;
	};
	struct{
	    byte IXHp;
	    byte IXLp;
	    byte IYHp;
	    byte IYLp;
	};
    };

    byte Ip;
    byte Rp;
    
    word PCp;
    word SPp;
    
    byte IMr;
    byte IFF1p:1;
    byte IFF2p:1;
    byte Alt:1;
    byte AltAF:1;
    byte Halt_state:1;
    byte DD_FD:1;
    byte INT:1;
    byte NMI:1;    
    byte *INT_DATA; /* zawarto¶æ magistrali danych, przy obs³udze INT */
    word INT_f;

    byte :1;
    byte :1;
    byte INT_FETCH:1; /* flaga wskazuj±ca, ¿e jest cykl pobrania rozkazu z magistrali danych       */
    byte M1:1;	      /* flaga wskazuj±ca na cykl M1 procesora - pobieranie/dekodowanie instrukcji */
    byte :4;

    byte *cust;	/* wska¼nik struktury zewn przekazywany do funkcji IO/MEM */
    byte (*io)(word addr, byte data, byte type, void *);
    void (*WR_MEM)(void *z80,word addr, byte data);
    byte (*RD_MEM)(void *z80,word addr);
    void (*deb_in)(char *);
    void (*deb_out)(char *);
    void (*exit)(void);
/* po ka¿dym wykonaniu instrukcji pola te s± na nowo ustawiane */
    byte ticks; /* takty */
    byte mach;  /* cykle maszynowe */
    byte dd;
    int  lp_freq;
    int  act_freq;
#ifdef DEBUGGER
    word old_pc;
    word NNp;
    byte Np;
    int  call_d; /* call zwieksza, ret zmniejsza */
    byte xy:1;    
    byte djnz_d:1;    
    byte debug_print:1;
    byte deb_halt:1;
    byte dry_run:1;
    byte proc:1;
    byte run:1;
    byte deb_stdout:1;
    byte debug:1;
    byte debug_int:1;
    byte :6;
    byte deb_class; /* rodzaj instrukcji */
    long cnt;
#endif
}Z80;


/*************************************************************************************************/
extern void z80_init(Z80 *z,int freq, void *io, void *rd, void *wr, void *deb_in, void *deb_out, void *exit, void *ptr);
/*
                   ..::inicjuje strukture Z80::..
    freq : ilosc wykonanych instrukcji na jedno wywolanie z80_exec()
    io : funkcja obslugi instrukcji in/out
    rd : funkcja obslugi odczytu pamieci
    wr : funkcja obslugi zapisu do pamieci
    deb_in : funkcja wejscia dla debuggera
    deb_out: funkcja wyjscia dla debuggera
    exit: funkcja wyjscia z debbugera
    ptr : dodatkowy wska¼nik, przekazywany do funkcji obs³ugi pamiêci i I/O, mo¿na pod niego podpi±æ
	  w³asne struktury danych
*/
/*************************************************************************************************/

extern void z80_reset(Z80 *z);
/*
	..::Reset procesora::..
*/
/*************************************************************************************************/

extern void z80_send_int(Z80 *z, byte *data);
/*
    ..::Wysyla sygnal przerwania INT::..
    
    data : dane wystawione na magistrale danych (istotne dla trybu IM0, IM2 )

*/
/*************************************************************************************************/

extern void z80_send_nmi(Z80 *z);
/*
    ..::Wysyla sygnal przerwania NMI::..
    Uwaga: wa¿ne zbocze opadaj±ce
*/
/*************************************************************************************************/
extern void z80_exe(Z80 *z);
/*
    ..::Wykonuje pojedyncz± instrukcjê::..
*/
/*************************************************************************************************/
extern void z80_exec(Z80 *z);
/*
    ..::Wykonuje seriê instrukcji::..
*/

/*************************************************************************************************/
extern void z80_set_run(Z80 *z, char flag);
/*
    flag=1 - uruchamia b±dz flag=0 - zatrzymuje procesor
    domyslnie jest zatrzymany
*/

extern void z80_set_debug(Z80 *z, char flag);
/*
    flag=1 - wlacza flag=0 - wylacza debugger
    domyslnie jest wylaczony
*/
/*************************************************************************************************/

#endif
