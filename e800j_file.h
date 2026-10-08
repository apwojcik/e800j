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

#ifndef __e800j_file_h__
#define __e800j_file_h__

#include <stdio.h>
#include "z80.h"
#include "e800j.h"

/* Hw_mode  dla wersji v2 */
#define H2W_48K		0
#define H2W_48K_IF	1
#define H2W_SAMRAM	2
#define H2W_128K	3
#define H2W_128K_IF	4

/* Hw_mode  dla wersji v3 */
#define H3W_48K_MGT	3
#define H3W_128K	4
#define H3W_128K_IF	5
#define H3W_128K_MGT	6

/* Hw_mode rozszerzone przez emulatory */
#define HW_SPEC_3	7
#define HW_SPEC_3m	8
#define HW_PENTAGON	9
#define HW_Scorpion	10
#define HW_DIDAKTIK	11
#define HW_SPEC_2	12
#define HW_SPEC_2A	13
#define HW_TC2048	14
#define HW_TC2068	15
#define HW_TS2068	128

/* znaczenie numerów stron */
#define PG_ROM_BAS	0
#define PG_IFACE_ROM	1
#define PG_ROM_RST	2
#define PG_0		3	/* sam ram            */
#define PG_1		4	/* 8000...bfff        */
#define PG_2		5	/* c000...ffff        */
#define PG_3		6	/* shadow 8000...bfff */
#define PG_4		7	/* shadow c000...bfff */
#define PG_5		8	/* 4000...7fff        */
#define PG_6		9	/* page 6 - spec 128  */
#define PG_7		10	/* page 7 - spec 128  */
#define PG_MFCE		11      /* Multiface ROM      */

#ifndef byte
#define byte		unsigned char 
#endif
#ifndef word
#define word		unsigned short int
#endif

typedef struct
{
//    byte A;
//    byte F;
    byte AF[2];
    byte BC[2];
    byte HL[2];
    union{
	word Next_Version; /* jesli jest ==0 to znaczy ¿e wersja 2 lub 3 formatu z80 */
	byte PC[2];
    };
    byte SP[2];
    byte I;
    byte R; /* ignorowany bit siódmy */
    union{
	byte Byte_12;
	struct{
	    byte R7:1;
	    byte Border:3; /* kolor obrze¿a ekranu */
	    byte SamROM:1;
	    byte Compressed:1;
	    byte :2;
	};
    };
    byte DE[2];
    byte BCa[2];
    byte DEa[2];
    byte HLa[2];
//    byte Aa;
//    byte Fa;
    byte AFa[2];
    byte IY[2];
    byte IX[2];
    byte IFF1; /* flip-flop */
    byte IFF2;
    union{
	byte Byte_29;
	struct{
	    byte IM:2; /* IM0...IM2*/
	    byte Issue_2:1; /* wersja druga emulacji */
	    byte If2:1;	/* podwojona czêstotliwo¶c przerwañ */
	    byte Video_sync:2; /* predkosc synchronizacji video */
	    byte Joystick:2;        
	};
    };
} z80_fst_hdr;


typedef struct{
    byte Hdr_length[2];
    byte PC[2];
    byte Hw_mode;
    union{
	byte LS259;	/* dla trybu SamRAM stan bitów rejestru 74LS259  */
	byte OUT_7FFD;	/* dla trybu ZX128 zawiera ostatni OUT do 0x7ffd */
	byte OUT_F4;	/* dla trybu Timex zawiera ostatni OUT do 0xf4   */
    };
    union{
	byte Paged; 	/* zawiera 0xff jezeli stronicowany ROM Interface I */
	byte OUT_FF;	/* w trybie Timex zawiera ostatni OUT do 0xff       */
    };
    byte R_emul:1;
    byte LDIR_emul:1;
    byte AY_emul:1;
    byte :3;
    byte FAB_emul:1;
    byte MH:1;
    byte OUT_fffD;
    byte Sound_reg[16];
    byte LT_sc[2];
    byte HT_sc;
    byte Spectator;
    byte MGT_ROM_paged;
    byte Multiface_ROM_paged;
    byte L_ROM_RAM;
    byte H_ROM_RAM;
    byte KB_map[10];
    byte KEY_map[10];
    byte MGT_type;
    byte DIBT_stat;
    byte DIF;
    byte OUT_1FFD; 
} z80_scd_hdr;

typedef struct
{
    byte length[2];
    byte page_nr;
} memo_block;


extern int	decompress_data(FILE *f,int clength,int dlength, char *memo, int addr);
extern int	load_z80(Z80 *z, FILE *f);
extern byte	load_rom(e800j *e8,char *fname_bas0,char *fname_bas1, char *fname_boot);

#endif
