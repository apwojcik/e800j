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

#ifndef __e800j_kbrd_h__
#define __e800j_kbrd_h__

#define KB_MODE_ZX	0	/* Mapowanie ZX_SPECTRUM 			   */
#define KB_MODE_E800J	1	/* Mapowanie Elwro 800 Junior 			   */
#define KB_MODE_PC	2	/* Mapowanie klawiatury PC (programisty ) na E800J */


#define KB_ROWS		10	/* ilo¶æ rzêdów matrycy klawiatury */
#define KB_ZX_ROW_MASK	0x0
#define KB_ZX_COL_MASK	0x0

typedef struct
{
    int		  key_code;		/* ostatni kod klawisza  */
    char	  state;		/* ostatni stan klawisza */
    char	  mode;			/* tryb klawiatury       */
    char	  alt_r;		/* wci¶niêty prawy ALT	 */
    unsigned char matrix[KB_ROWS];	/* matryca klawiszy      */
    char	  *matrix_map1;		/* pierwszy stopien mapowania klawiszy */    
    unsigned short int row_mask;	/* maska adresu (odcina czê¶æ linii adresowych ) */ 
    unsigned char col_mask;		/* maska danych (odcina czê¶æ linii kolumn     ) */
    
} KBE800J ;


/*******************************************************************/
extern void key_init(unsigned char mode);
/*	..:: Inicjuje klawiaturê w trybie 'mode' ::..
        mode - tryb mapowania klawiatury
*/
/*******************************************************************/
extern void key_update(int key, char state);
/*	..:: 'wciska' lub 'wyciska' odpowidni klawisz na emulacji matrycy klawiatury ::..
        key   - kod klawisza
	state - =1 wci¶niêto, =0 puszczono 
*/

/*******************************************************************/
extern unsigned char key_matrix(unsigned short int addr);
/*	..:: emulacja matrycy klawiszowej Elwro 800 Junior ::..
        addr - linie KA2, KA7..KA15
	warto¶æ zwracana - linie KL0..KL6
*/

/*********************************************

        ..:: Wspó³rzêdne klawiszy ::..

*********************************************/

#define KEY_SPC		0x00
#define KEY_ALT		0x01
#define KEY_M		0x02
#define KEY_N		0x03
#define KEY_B		0x04
#define KEY_CTRL	0x05
#define KEY_ROOF	0x07
#define KEY_CR		0x10
#define KEY_L		0x11
#define KEY_K		0x12
#define KEY_J		0x13
#define KEY_H		0x14
#define KEY_TAB		0x15
#define KEY_CAPS	0x17
#define KEY_P		0x20
#define KEY_O		0x21
#define KEY_I		0x22
#define KEY_U		0x23
#define KEY_Y		0x24
#define KEY_ESC		0x25
#define KEY_BS		0x27
#define KEY_0		0x30
#define KEY_9		0x31
#define KEY_8		0x32
#define KEY_7		0x33
#define KEY_6		0x34
#define KEY_DEL		0x35
#define KEY_BRACKET_R	0x37
#define KEY_1		0x40
#define KEY_2		0x41
#define KEY_3		0x42
#define KEY_4		0x43
#define KEY_5		0x44
#define KEY_SLASH	0x45
#define KEY_BACKSLASH	0x47
#define KEY_Q		0x50	
#define KEY_W		0x51
#define KEY_E		0x52
#define KEY_R		0x53
#define KEY_T		0x54
#define KEY_DOT		0x55
#define KEY_COMMA	0x57
#define KEY_A		0x60
#define KEY_S		0x61
#define KEY_D		0x62
#define KEY_F		0x63
#define KEY_G		0x64
#define KEY_MINUS	0x65
#define KEY_BRACKET_L	0x67
#define KEY_SHIFT	0x70
#define KEY_Z		0x71
#define KEY_X		0x72
#define KEY_C		0x73
#define KEY_V		0x74
#define KEY_SEMICOLON	0x75
#define KEY_PLUS	0x77
#define KEY_E_		0x80	/* ê */
#define KEY_O_		0x81	/* ó */
#define KEY_S_		0x82	/* ¶ */
#define KEY_L_		0x83	/* ³ */
#define KEY_Z__		0x84	/* ¼ */
#define KEY_N_		0x85	/* ñ */
#define KEY_A_		0x86	/* ± */
#define KEY_Z_		0x90	/* ¿ */
#define KEY_CUR_D	0x91
#define KEY_CUR_U	0x92
#define KEY_CUR_R	0x93
#define KEY_CUR_L	0x94
#define KEY_C_		0x95	/* æ */
#define KEY_DIR		0x97

/* elementy tablicy mapowania */
/* cyfry: 0...9 */
#define KEY_DIGITS	KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9
#define KEY_DIGITS_CNT	10
/* litery: a..z */
#define KEY_LETTERS	KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,\
			KEY_G,KEY_H,KEY_I,KEY_J,KEY_K,KEY_L,\
			KEY_M,KEY_N,KEY_O,KEY_P,KEY_Q,KEY_R,\
			KEY_S,KEY_T,KEY_U,KEY_V,KEY_W,KEY_X,\
			KEY_Y,KEY_Z
#define KEY_LETTERS_CNT	26
/* pliterki: ±,æ,ê,³,ñ,ó,¶,¼,¿ */
#define KEY_PLETTERS	KEY_A_,KEY_C_,KEY_E_,KEY_L_,KEY_N_,\
			KEY_O_,KEY_S_,KEY_Z__,KEY_Z_
#define KEY_PLETTERS_CNT 9
/* klawisze steruj±ce */
#define KEY_CONTROL	KEY_CR,KEY_SHIFT,KEY_CAPS,KEY_SPC,KEY_ALT,KEY_ALT,KEY_CTRL,KEY_DEL,KEY_BS,KEY_ESC,\
			KEY_TAB,KEY_DIR,KEY_CUR_L,KEY_CUR_R,KEY_CUR_U,KEY_CUR_D
#define KEY_CONTROL_CNT 16
/* znaki: ^[]/\.;.-+ */
#define KEY_SIGNS	KEY_ROOF,KEY_BRACKET_L,KEY_BRACKET_R,KEY_SLASH,KEY_BACKSLASH,\
			KEY_DOT,KEY_SEMICOLON,KEY_COMMA,KEY_MINUS,KEY_PLUS
#define KEY_SIGNS_CNT	10

/* zawarto¶æ tablicy mapowania */
#define KEY_MAP_TABLE	KEY_CONTROL,KEY_SIGNS,KEY_PLETTERS,KEY_DIGITS,KEY_LETTERS

/* numer klawisza w tabeli mapowania */
#define KB_ENTER	0
#define KB_SHIFT	1
#define KB_CAPS		2
#define KB_SPACE	3
#define KB_ALT_L	4
#define KB_ALT_R	5
#define KB_CTRL		6
#define KB_DEL		7
#define KB_BS		8
#define KB_ESC		9
#define KB_TAB		10
#define KB_DIR		11
#define KB_CUR_L	12
#define KB_CUR_R	13
#define KB_CUR_U	14
#define KB_CUR_D	15
#define KB_ROOF		0 + KEY_CONTROL_CNT
#define KB_BRL		1 + KEY_CONTROL_CNT
#define KB_BRR		2 + KEY_CONTROL_CNT
#define KB_SLASH	3 + KEY_CONTROL_CNT
#define KB_BSLASH	4 + KEY_CONTROL_CNT
#define KB_DOT		5 + KEY_CONTROL_CNT
#define KB_SEMIC	6 + KEY_CONTROL_CNT
#define KB_COM		7 + KEY_CONTROL_CNT
#define KB_MINUS	8 + KEY_CONTROL_CNT
#define KB_PLUS		9 + KEY_CONTROL_CNT
#define KB_A_		0 + KEY_CONTROL_CNT + KEY_SIGNS_CNT
#define KB_C_		1 + KEY_CONTROL_CNT + KEY_SIGNS_CNT
#define KB_E_		2 + KEY_CONTROL_CNT + KEY_SIGNS_CNT
#define KB_L_		3 + KEY_CONTROL_CNT + KEY_SIGNS_CNT
#define KB_N_		4 + KEY_CONTROL_CNT + KEY_SIGNS_CNT
#define KB_O_		5 + KEY_CONTROL_CNT + KEY_SIGNS_CNT
#define KB_S_		6 + KEY_CONTROL_CNT + KEY_SIGNS_CNT
#define KB_Z__		7 + KEY_CONTROL_CNT + KEY_SIGNS_CNT
#define KB_Z_		8 + KEY_CONTROL_CNT + KEY_SIGNS_CNT

#define KB_DIG_START	KEY_CONTROL_CNT + KEY_SIGNS_CNT + KEY_PLETTERS_CNT
#define KB_LET_START	KB_DIG_START + KEY_DIGITS_CNT
#define KB_ALL_CNT	KB_LET_START + KEY_LETTERS_CNT

#define KB_LOAD		'j' - 'a' + KB_LET_START
#define KB_SAVE		's' - 'a' + KB_LET_START
#define KB_ERASE	'7' - '0' + KB_DIG_START
#define KB_CPJ		'6' - '0' + KB_DIG_START

#endif
