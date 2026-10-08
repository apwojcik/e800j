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

#define BIGENDIAN

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
    byte *Ap,*Fp;
    byte *Bp,*Cp;
    byte *Dp,*Ep;
    byte *Hp,*Lp;	/* wskazuja zawsze na HL */
    byte *XYHp,*XYLp;   /* wskazuja na HL, IX lub IY */

    word *AFp;
    word *BCp;
    word *DEp;
    word *HLp;

    word *HLs;	/* tutaj przechowywany jest wska¼nik do rejestru HL, sprzed IX,IY */

    union{
	struct{    
	    word AFr;
	    word BCr;
	    word DEr;
	    word HLr;

	    word AFr_;
	    word BCr_;
	    word DEr_;
	    word HLr_;
	};
#ifndef BIGENDIAN
	struct{
	    byte Ar,Fr;
	    byte Br,Cr;
	    byte Dr,Er;
	    byte Hr,Lr;

	    byte Ar_,Fr_;
    	    byte Br_,Cr_;
	    byte Dr_,Er_;
	    byte Hr_,Lr_;
	};
#else
	struct{
	    byte Fr,Ar;
	    byte Cr,Br;
	    byte Er,Dr;
	    byte Lr,Hr;

	    byte Fr_,Ar_;
	    byte Cr_,Br_;
	    byte Er_,Dr_;
	    byte Lr_,Hr_;
	};
#endif    
    };
    union{
	struct{
	    word IXp;
	    word IYp;
	};
	struct{
	    byte IXHr;
	    byte IXLr;
	    byte IYHr;
	    byte IYLr;
	};
    };

    byte Ip;
    byte Rp;
    
    word PCp;
    word SPp;
    long Tr;		/* takty zegarowe */
    
    byte IMr;
    byte IFF1p:1;
    byte IFF2p:1;
    byte Alt:1;
    byte AltAF:1;
    byte Halt_state:1;

    byte INT:1;
    byte NMI:1;    
    byte INT_FETCH:1; /* flaga wskazuj±ca, ¿e jest cykl pobrania rozkazu z magistrali danych       */
    byte M1:1;	      /* flaga wskazuj±ca na cykl M1 procesora - pobieranie/dekodowanie instrukcji */
    byte run:1;	      /* stopuje/wznawia pracê CPU 	                   */
    byte DD_FD:1;     /* wskazuje, ¿e HL jest przemianowane na IX lub IY   */
    byte xy:1;	      /* wskazuje, czy rejestrem w operacji jest IX czy IY */    
    byte busack:1;    /* zezwolenie na korzystanie z magistrali            */	
    byte busrq:1;     /* rz±danie korzystania z magistrali	           */

    /* flagi dla debuggera */
    byte deb_on:1;		/* 1 - debugger w³±czony		*/
    byte deb_dry:1;		/* dekoduj instrukcjê, ale nie wykonuj  */
    byte debug_int:1;		/* jesli =1 pomijaj obslugê IRQ		*/
    byte deb_halt:1;		/* nie dekoduj/wykonuj instrukcji       */

    byte trap:1;	/* nakazuje przerwaæ wykonanie instrukcji i powrót do debuggera */
    byte :4;

    byte *INT_DATA;	/* zawarto¶æ magistrali danych, przy obs³udze INT */
    word INT_f;		/* indeks tablicy INT_DATA                        */
    
    byte *cust;	/* wska¼nik struktury zewn przekazywany do funkcji IO/MEM */
    byte (*io)(word addr, byte data, byte type, void *);
    void (*WR_MEM)(void *z80,word addr, byte data);
    byte (*RD_MEM)(void *z80,word addr);
/* po ka¿dym wykonaniu instrukcji pola te s± na nowo ustawiane */
//    byte ticks; /* takty */
//    byte mach;  /* cykle maszynowe */
    byte dd;	  /* ofset dla adresowania za pomoc± rej indeksowych */
    int  lp_freq;
    int  act_freq;
    int  int_cnt; /* co ile cykli ma siê wykonaæ INT */
    int  nmi_cnt; /* co ile cykli ma siê wykonaæ NMI */
    word old_pc;  /* warto¶æ PC na pocz±tku wywo³ania funkcji z80_exe() */

/* poni¿sze pola s³u¿± do obs³ugi debuggera  */
    void (*debugger)(void *z, char );	/* funkcja po³±czenia z debuggerem	*/
    void *deb_struct;			/* wewnêtrzna struktura debuggera	*/
    char *deb_par_0;			/* nazwa instrukcji			*/
    char *deb_par_1;			/* pierwszy parametr			*/
    char *deb_par_2;			/* drugi parametr			*/
    byte  deb_par_3;			/* reakcja na HL			*/
    byte deb_class; 			/* klasa instrukcji 			*/
    word NNp;				/* dodatkowa warto¶æ 2B kodu rozkazu	*/
    byte Np;				/* jw, ale 1B				*/

}Z80;

/*************************************************************************************************/
extern void z80_init(Z80 *z, void *ptr, int freq, void *io, void *rd, void *wr);
/*
                   ..::inicjuje strukture Z80::..
    z  : struktura Z80, je¶li warto¶æ inna ni¿ NULL, wówczas wewnêtrzna struktura jest ustawiana
    ptr : dodatkowy wska¼nik, przekazywany do funkcji obs³ugi pamiêci i I/O, mo¿na pod niego podpi±æ
	  w³asne struktury danych
    freq : ilosc wykonanych instrukcji na jedno wywolanie z80_exec()
    io : funkcja obslugi instrukcji in/out
    rd : funkcja obslugi odczytu pamieci
    wr : funkcja obslugi zapisu do pamieci
*/
/*************************************************************************************************/

extern void z80_reset(Z80 *z);
/*
	..::Reset procesora::..
	z- jesli NULL, uzyj struktury wewn
*/
/*************************************************************************************************/

extern void z80_send_int(Z80 *z, byte *data);
/*
    ..::Wysyla sygnal przerwania INT::..
	z- jesli NULL, uzyj struktury wewn    
    data : dane wystawione na magistrale danych (istotne dla trybu IM0, IM2 )

*/
/*************************************************************************************************/

extern void z80_send_nmi(Z80 *z);
/*
    ..::Wysyla sygnal przerwania NMI::..
	z- jesli NULL, uzyj struktury wewn
    Uwaga: wa¿ne zbocze opadaj±ce
*/
/*************************************************************************************************/
extern void z80_exe(Z80 *z);
/*
    ..::Wykonuje pojedyncz± instrukcjê::..
	z- jesli NULL, uzyj struktury wewn
*/
/*************************************************************************************************/
extern void z80_exec(Z80 *z, void (*irqm)(void),void (*irqn)(void));
/*
    ..::Wykonuje seriê instrukcji::..
	z- jesli NULL, uzyj struktury wewn
    irqm - funkcja wywo³ania INT, jesli NULL to nieaktywne
    irqn - funkcja wywo³ania NMI, jesli NULL to nieaktywne
*/

/*************************************************************************************************/
extern void z80_set_run(Z80 *z, char flag);
/*
	z- jesli NULL, uzyj struktury wewn
    flag=1 - uruchamia b±dz flag=0 - zatrzymuje procesor
    domyslnie jest zatrzymany
*/

/*************************************************************************************************/
extern Z80 *z80_get_struct(void);
/*
    Zwraca warto¶c wewnêtrznej (domy¶lnej) struktury Z80
*/

#endif
