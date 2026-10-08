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

#include "e800j_kbrd.h"

KBE800J	keyboard;	/* wewnêtrzna zmienna globalna - nie wykorzystywaæ na zewn±trz ! */
unsigned char keymap_table[] = {KEY_MAP_TABLE}; /* zamiana kodu klawisza na wspó³rzêdne w matrycy */

void key_init(unsigned char mode)
{
    int i;
    /* zerowanie ca³ej struktury */
    for(i=0; i < sizeof(keyboard); i++) *((char *)&keyboard + i) = 0;
    keyboard.mode = mode;
    /* wype³nienie matrycy klawiatury warto¶ciami 0xff - nie naci¶niêty ¿aden klawisz */
    for(i=0; i < sizeof(keyboard.matrix); i++) *(keyboard.matrix + i) = 0xff;
}

unsigned char key_matrix(unsigned short int addr)
{
    /* zamiana adresu na indeks w matrycy klawiatury */
    switch(addr | keyboard.row_mask){
	case 0x7ffe: addr = 0; break;	/* dla linii KA15 */
	case 0xbffe: addr = 1; break;	/* dla linii KA14 */
	case 0xdffe: addr = 2; break;	/* dla linii KA13 */
	case 0xeffe: addr = 3; break;	/* dla linii KA12 */
	case 0xf7fe: addr = 4; break;	/* dla linii KA11 */
	case 0xfbfe: addr = 5; break;	/* dla linii KA10 */
	case 0xfdfe: addr = 6; break;	/* dla linii KA9  */
	case 0xfefe: addr = 7; break;	/* dla linii KA8  */	
	case 0x007f: addr = 8; break;	/* dla linii KA7  */	
	case 0x00fb: addr = 9; break;	/* dla linii KA2  */	
	default    : return 0xff;
    }
    return keyboard.matrix[addr] | keyboard.col_mask;    
}

/* 
  Ustawia stan klawisza w matrycy na wspó³rzêdnych row,col 

coord:
    06 05 04 03 02 01 00
    16 15 14 13 12 11 10
    .. .. .. .. .. .. ..
    96 95 94 93 92 91 90
state:
    1 - wci¶niêto
    0 - zwolniono
*/
void key_matrix_update(unsigned char coord, unsigned char  state)
{
    unsigned char col,row;
    
    row = (coord >> 4) & 0xf;
    col = coord & 0x7;    
    
    col = 0x01 << col;

    if( row >= KB_ROWS ) return;

    if(state) 
	keyboard.matrix[row] &= ~col;
    else 
	keyboard.matrix[row] |= col;
}


void key_update(int key_code, char state)
{
    int key_idx=0xff;

    

    if(keyboard.mode == KB_MODE_PC)
    {
	if(key_code < KEY_CONTROL_CNT) key_idx = key_code;
	switch(key_code){
	    case '~' : key_idx = KB_ROOF; break;
	    case '[' : key_idx = KB_BRL; break;
	    case ']' : key_idx = KB_BRR; break;
	    case '/' : key_idx = KB_SLASH; break;
	    case '\\' : key_idx = KB_BSLASH; break;
	    case '.' : key_idx = KB_DOT; break;
	    case ';' : key_idx = KB_SEMIC; break;
	    case ',' : key_idx = KB_COM; break;
	    case '-' : key_idx = KB_MINUS; break;
	    case '=' : key_idx = KB_PLUS; break;
	}
	if(key_code == KB_ALT_R){
	    keyboard.alt_r = state;
	    return;
	}
	if(keyboard.alt_r){
	    switch(key_code){
		case 'a' : key_code = 0xff; key_idx = KB_A_; break;
		case 'c' : key_code = 0xff; key_idx = KB_C_; break;	    
		case 'e' : key_code = 0xff; key_idx = KB_E_; break;
		case 'l' : key_code = 0xff; key_idx = KB_L_; break;
		case 'n' : key_code = 0xff; key_idx = KB_N_; break;
		case 'o' : key_code = 0xff; key_idx = KB_O_; break;
		case 's' : key_code = 0xff; key_idx = KB_S_; break;
		case 'z' : key_code = 0xff; key_idx = KB_Z_; break;
		case 'x' : key_code = 0xff; key_idx = KB_Z__; break;
	    }
	}
    }
    if((key_code >= 'a') && (key_code <= 'z')) key_idx = key_code - 'a' + KB_LET_START;
    if((key_code >= '0') && (key_code <= '9')) key_idx = key_code - '0' + KB_DIG_START;
    if(key_idx >= KB_ALL_CNT ) return;
    key_matrix_update(keymap_table[key_idx],state);
/*
{ int i;
for(i=0; i < 10; i++) 
    printf("%x,",keyboard.matrix[i]);
    
    printf("\n");
}    
*/
}

