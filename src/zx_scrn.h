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

#pragma once

#include <SDL3/SDL.h>

struct rgbi {
    unsigned char ri;
    unsigned char gi;
    unsigned char bi;
    unsigned char ai; /* bajt wyrównujący */
};

struct rgbf {
    float rf;
    float gf;
    float bf;
};

#define BORDER_EV_MAX 8192 /* maks. liczba zmian koloru ramki w jednej klatce */

struct border_ev {
    int t; /* takt CPU od początku klatki */
    unsigned char color;
};

/* zmiany koloru ramki w jednej klatce (okres przerwania) */
struct border_frame {
    unsigned char start; /* kolor na początku klatki */
    int len; /* długość klatki w taktach */
    int n;
    struct border_ev ev[BORDER_EV_MAX];
};

struct screen {
    char msg[256];
    unsigned char scr_buffer[192 * 256 * 3];
    struct rgbi palettei[16];
    struct rgbf palettef[16];
    int width;
    int height;
    int bpp;
    int rgb_size[3];
    int dbf;
    unsigned char* zx_patt; /* wzory bitowe - 6kB */
    unsigned char* zx_attr; /* kolory kranu - 1kB */
    unsigned char border_color;
    struct border_frame bframe[2]; /* bieżąca i ostatnia ukończona klatka */
    int bcur; /* indeks bieżącej klatki w bframe */
    bool bstarted; /* pierwsza klatka jest niepełna (start w dowolnym takcie) */
    bool bhave; /* czy jest ukończona klatka do narysowania */
    long frame_t0; /* takt CPU początku bieżącej klatki */
    float py0; /* dolna i górna krawędź obrazu w oknie (0..1) */
    float py1;
    int rfsh;
    SDL_AtomicInt refresh; // screen refresh pending (ustawiane z wątku timera)
    unsigned int t0;
    unsigned int t2;
    SDL_GLContext glctx;
    int flash; /* tempo migania atrybutu flash w ms */
    SDL_AtomicInt inverse; /* faza migania (zmieniana w wątku timera) */
    int fscrn;
    int bright;
    int nbrt;
    int bbrt;
    int wbrt;
    long parent; /* ID okna GTK */
    float bsize;
    int yresv; /* yrsv - rozmiar zarezerwowanego górnego pasa na menu GTK, bsize - rozmiar ramki*/
    SDL_Window* window;
};

void screen_make_palette(struct screen* s, int nrm, int brt, int wht);

void screen_refresh(struct screen* s);

/* argv -> -fullscreen, -noframe */
bool screen_init(struct screen* s, int width, int height, int bpp, int argc, const char** argv, int refresh);

void screen_border(struct screen* s, unsigned char color);

/* zmiana koloru ramki w takcie CPU 'ticks' (zapis do portu 0xFE) */
void screen_border_set(struct screen* s, long ticks, unsigned char color);

/* początek klatki (przerwanie) w takcie CPU 'ticks'; 'len' - długość klatki w taktach */
void screen_frame_start(struct screen* s, long ticks, int len);

void zx_quit(void);
