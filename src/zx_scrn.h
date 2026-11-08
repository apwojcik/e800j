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
    unsigned char ai; /* bajt nadmiarowy do kopiowania 32 bitowego */
};

struct rgbf {
    float rf;
    float gf;
    float bf;
};

struct screen {
    char msg[256];
    unsigned char scr_buffer[192 * 256 * 3 + 1]; /* +1 aby można było zrobić szybkie kopiowanie 32 bitowe */
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
    int rfsh;
    bool refresh; // screen refresh pending
    unsigned int t0;
    unsigned int t2;
    void* glctx; /* SDL_GLContext */
    int flash; /* tempo migania atrybutu flash w ms */
    bool inverse;
    int fscrn;
    int bright;
    int nbrt;
    int bbrt;
    int wbrt;
    long parent; /* ID okna GTK */
    float bsize;
    float scrh;
    int yresv; /* yrsv - rozmiar zarezerwowanego górnego pasa na menu GTK, bsize - rozmiar ramki*/
    SDL_Window* window;
    SDL_GLContext gl_context;
};

void screen_make_palette(struct screen* s, int nrm, int brt, int wht);

void screen_refresh(struct screen* s);

/* argv -> -fullscreen, -noframe */
bool screen_init(struct screen* s, int width, int height, int bpp, int argc, const char** argv, int refresh);

void screen_border(struct screen* s, unsigned char color);

void zx_quit(void);
