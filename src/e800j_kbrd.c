/* Elwro 800 Junior emulator v0.3
 * Copyright (C) 2006 Krzysztof Komarnicki
 * Copyright (C) 2026 Artur Wojcik
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
    size_t i;
    /* zerowanie całej struktury */
    memset(kb, 0, sizeof(*kb));
    kb->mode = KB_MODE_PC;
    /* wypełnienie matrycy klawiatury wartościami 0xff - nie naciśnięty żaden klawisz */
    for (i = 0; i < sizeof(kb->matrix); i++)
        *(kb->matrix + i) = 0xff;
    for (i = 0; i < KB_HELD_MAX; i++)
        kb->held[i].idx = KB_NONE;
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
  Ustawia stan punktu matrycy dla klawisza o indeksie idx w keymap_table.
  Punkt może być wciskany przez kilka klawiszy fizycznych naraz (np. F3 i J,
  lewy i prawy Shift), więc zwolnienie następuje dopiero po puszczeniu ostatniego.

coord:
    06 05 04 03 02 01 00
    16 15 14 13 12 11 10
    .. .. .. .. .. .. ..
    96 95 94 93 92 91 90
*/
static void key_matrix_update(struct keyboard* kb, const unsigned int idx, const int state) {
    const unsigned char coord = keymap_table[idx];
    const unsigned char row = (coord >> 4) & 0xf;
    const unsigned char bit = coord & 0x7;

    if (row >= KB_ROWS)
        return;
    unsigned char* cnt = &kb->down[row][bit];
    if (state) {
        if (*cnt < 0xff)
            (*cnt)++;
    } else if (*cnt) {
        (*cnt)--;
    }
    if (*cnt)
        kb->matrix[row] &= (unsigned char)~(1u << bit);
    else
        kb->matrix[row] |= (unsigned char)(1u << bit);
}

/* naciśnięcie klawisza fizycznego 'key', który wciska klawisz matrycy 'idx' */
static void key_press(struct keyboard* kb, const unsigned int key, const unsigned int idx) {
    int free_slot = -1;
    for (int i = 0; i < KB_HELD_MAX; i++) {
        if (kb->held[i].idx == KB_NONE) {
            if (free_slot < 0)
                free_slot = i;
        } else if (kb->held[i].key == key) {
            return; /* już wciśnięty */
        }
    }
    if (free_slot < 0)
        return;
    kb->held[free_slot].key = key;
    kb->held[free_slot].idx = (unsigned char)idx;
    key_matrix_update(kb, idx, 1);
}

/* zwolnienie klawisza fizycznego: zwalnia dokładnie to, co wcisnął */
static void key_release(struct keyboard* kb, const unsigned int key) {
    for (int i = 0; i < KB_HELD_MAX; i++) {
        if (kb->held[i].idx != KB_NONE && kb->held[i].key == key) {
            key_matrix_update(kb, kb->held[i].idx, 0);
            kb->held[i].idx = KB_NONE;
            return;
        }
    }
}

/* zamiana kodu SDL (litera, cyfra, znak) na indeks w keymap_table lub KB_NONE */
static unsigned int key_resolve(const struct keyboard* kb, const unsigned int key) {
    if (kb->mode == KB_MODE_PC) {
        if (kb->alt_r) {
            switch (key) {
            case 'a':
                return KB_A_;
            case 'c':
                return KB_C_;
            case 'e':
                return KB_E_;
            case 'l':
                return KB_L_;
            case 'n':
                return KB_N_;
            case 'o':
                return KB_O_;
            case 's':
                return KB_S_;
            case 'z':
                return KB_Z_;
            case 'x':
                return KB_Z__;
            default:
                break;
            }
        }
        switch (key) {
        case '~':
        case SDLK_GRAVE:
            return KB_ROOF;
        case '[':
            return KB_BRL;
        case ']':
            return KB_BRR;
        case '/':
            return KB_SLASH;
        case '\\':
            return KB_BSLASH;
        case '.':
            return KB_DOT;
        case ';':
            return KB_SEMIC;
        case ',':
            return KB_COM;
        case '-':
            return KB_MINUS;
        case '=':
            return KB_PLUS;
        default:
            break;
        }
    }
    if ((key >= 'a') && (key <= 'z'))
        return key - 'a' + KB_LET_START;
    if ((key >= '0') && (key <= '9'))
        return key - '0' + KB_DIG_START;
    return KB_NONE;
}

void key_update(struct keyboard* kb, const unsigned int key, const int state) {
    if (!state) {
        key_release(kb, key);
        return;
    }
    const unsigned int key_idx = key_resolve(kb, key);
    if (key_idx < KB_ALL_CNT)
        key_press(kb, key, key_idx);
}

/* klawisze specjalne SDL -> indeks w keymap_table lub KB_NONE */
static unsigned int key_special(const unsigned int key) {
    switch (key) {
    case E8K_ENTER:
        return KB_ENTER;
    case E8K_SHIFT_L:
    case E8K_SHIFT_R:
        return KB_SHIFT;
    case E8K_CAPS:
        return KB_CAPS;
    case E8K_SPC:
        return KB_SPACE;
    case E8K_ALT_L:
        return KB_ALT_L;
    case E8K_CTRL_L:
    case E8K_CTRL_R:
        return KB_CTRL;
    case E8K_DEL:
        return KB_DEL;
    case E8K_BS:
        return KB_BS;
    case E8K_F1:
        return KB_CPJ;
    case E8K_F2:
        return KB_DIR;
    case E8K_F3:
        return KB_LOAD;
    case E8K_F4:
        return KB_SAVE;
    case E8K_F5:
        return KB_ERASE;
    case E8K_ARROW_L:
        return KB_CUR_L;
    case E8K_ARROW_R:
        return KB_CUR_R;
    case E8K_ARROW_U:
        return KB_CUR_U;
    case E8K_ARROW_D:
        return KB_CUR_D;
    case E8K_ESC:
        return KB_ESC;
    case E8K_TAB:
        return KB_TAB;
    default:
        return KB_NONE;
    }
}

void key_set(struct keyboard* kb, const unsigned int key, const int state) {
    /* prawy ALT tylko przełącza warstwę polskich liter, nie wciska klawisza matrycy */
    if (key == E8K_ALT_R) {
        kb->alt_r = state;
        return;
    }
    /* klawisze specjalne omijają key_resolve (np. F4 z prawym ALT to nadal SAVE, nie ś) */
    const unsigned int idx = key_special(key);
    if (idx != KB_NONE) {
        if (state)
            key_press(kb, key, idx);
        else
            key_release(kb, key);
        return;
    }
    key_update(kb, key, state);
}

//unsigned char key_kempston(struct keyboard *kb){ return kb->in1f & 0x1f; }
