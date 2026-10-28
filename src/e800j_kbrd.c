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

#include "e800j_kbrd.h"

static unsigned char keymap_table[] = KEY_MAP_TABLE; /* zamiana kodu klawisza na współrzędne w matrycy */

void key_init(struct keyboard* kb) {
    int i;
    /* zerowanie całej struktury */
    for (i = 0; i < sizeof(struct keyboard); i++)
        *((char*)kb + i) = 0;
    kb->mode = KB_MODE_PC;
    /* wypełnienie matrycy klawiatury wartościami 0xff - nie naciśnięty żaden klawisz */
    for (i = 0; i < sizeof(kb->matrix); i++)
        *(kb->matrix + i) = 0xff;
}

unsigned char key_matrix(const struct keyboard* kb, unsigned short int addr) {
    /* skanowanie klawiatury dla linii KA0..KA8 */
    int i = 0x8000;
    unsigned char kbr = 0xff;
    for (int j = 0; j < 9; i >>= 1, j++)
        if (!(addr & i))
            kbr &= kb->matrix[j];

    /* test linii E800J dla KA2 */
    if ((addr & 0xff) == 0xfb)
        kbr &= kb->matrix[9];

    return kbr;
}

/*
  Ustawia stan klawisza w matrycy na współrzędnych row,col

coord:
    06 05 04 03 02 01 00
    16 15 14 13 12 11 10
    .. .. .. .. .. .. ..
    96 95 94 93 92 91 90
state:
    1 - wciśnięto
    0 - zwolniono
*/
static void key_matrix_update(struct keyboard* kb, unsigned char coord, unsigned char state) {
    unsigned char col, row;

    row = (coord >> 4) & 0xf;
    col = coord & 0x7;

    col = 0x01 << col;

    if (row >= KB_ROWS)
        return;

    if (state)
        kb->matrix[row] &= ~col;
    else
        kb->matrix[row] |= col;
}

void key_update(struct keyboard* kb, int key_code, char state) {
    int key_idx = 0xff;

    if (kb->mode == KB_MODE_PC) {
        if (key_code < KEY_CONTROL_CNT)
            key_idx = key_code;
        switch (key_code) {
        case '~':
            key_idx = KB_ROOF;
            break;
        case '[':
            key_idx = KB_BRL;
            break;
        case ']':
            key_idx = KB_BRR;
            break;
        case '/':
            key_idx = KB_SLASH;
            break;
        case '\\':
            key_idx = KB_BSLASH;
            break;
        case '.':
            key_idx = KB_DOT;
            break;
        case ';':
            key_idx = KB_SEMIC;
            break;
        case ',':
            key_idx = KB_COM;
            break;
        case '-':
            key_idx = KB_MINUS;
            break;
        case '=':
            key_idx = KB_PLUS;
            break;
        }
        if (key_code == KB_ALT_R) {
            kb->alt_r = state;
            return;
        }
        if (kb->alt_r) {
            switch (key_code) {
            case 'a':
                key_code = 0xff;
                key_idx = KB_A_;
                break;
            case 'c':
                key_code = 0xff;
                key_idx = KB_C_;
                break;
            case 'e':
                key_code = 0xff;
                key_idx = KB_E_;
                break;
            case 'l':
                key_code = 0xff;
                key_idx = KB_L_;
                break;
            case 'n':
                key_code = 0xff;
                key_idx = KB_N_;
                break;
            case 'o':
                key_code = 0xff;
                key_idx = KB_O_;
                break;
            case 's':
                key_code = 0xff;
                key_idx = KB_S_;
                break;
            case 'z':
                key_code = 0xff;
                key_idx = KB_Z_;
                break;
            case 'x':
                key_code = 0xff;
                key_idx = KB_Z__;
                break;
            }
        }
    }
    if ((key_code >= 'a') && (key_code <= 'z'))
        key_idx = key_code - 'a' + KB_LET_START;
    if ((key_code >= '0') && (key_code <= '9'))
        key_idx = key_code - '0' + KB_DIG_START;
    if (key_idx >= KB_ALL_CNT)
        return;
    key_matrix_update(kb, keymap_table[key_idx], state);
}

/*
unsigned int key_joymap(struct keyboard *kb,  unsigned int key, unsigned int nkey, char state)
{
    unsigned char nk=0;
    if(kb->joy_mode & JOY_KEM){

        if(nkey == 32 ) nk = 0x10;
        if(nkey == 273) nk = 0x08;
        if(nkey == 274) nk = 0x04;
        if(nkey == 276) nk = 0x02;
        if(nkey == 275) nk = 0x01;

        kb->in1f &= ~nk;
        if(state) kb->in1f |= nk;
        if(nk) return -1;
        return key;
    }

    return key;
}
*/

void key_set(struct keyboard* kb, int key, char kb_state) {
    //    int nkey=key;
    //kb->joy_mode = JOY_KEM;
    /* mapowanie klawiszy specjalnych */
    switch (key) {
    case E8K_ENTER:
        key = KB_ENTER;
        break;
    case E8K_SHIFT_L:
    case E8K_SHIFT_R:
        key = KB_SHIFT;
        break;
    case E8K_CAPS:
        key = KB_CAPS;
        break;
    case E8K_SPC:
        key = KB_SPACE;
        break;
    case E8K_ALT_L:
        key = KB_ALT_L;
        break;
    case E8K_ALT_R:
        key = KB_ALT_R;
        break;
    case E8K_CTRL_L:
    case E8K_CTRL_R:
        key = KB_CTRL;
        break;
    case E8K_DEL:
        key = KB_DEL;
        break;
    case E8K_BS:
        key = KB_BS;
        break;
    case E8K_F1:
        key = KB_CPJ;
        break;
    case E8K_F2:
        key = KB_DIR;
        break;
    case E8K_F3:
        key = KB_LOAD;
        break;
    case E8K_F4:
        key = KB_SAVE;
        break;
    case E8K_F5:
        key = KB_ERASE;
        break;
    case E8K_ARROW_L:
        key = KB_CUR_L;
        break;
    case E8K_ARROW_R:
        key = KB_CUR_R;
        break;
    case E8K_ARROW_U:
        key = KB_CUR_U;
        break;
    case E8K_ARROW_D:
        key = KB_CUR_D;
        break;
    case E8K_ESC:
        key = KB_ESC;
        break;
    case E8K_TAB:
        key = KB_TAB;
        break;
    }

    //    if(kb->joy_mode) key = key_joymap(kb, key,nkey,kb_state);
    if ((int)key < 0)
        return;

    key_update(kb, key, kb_state);
}

//unsigned char key_kempston(struct keyboard *kb){ return kb->in1f & 0x1f; }
