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

#ifndef __ZX_SCRN_H__
#define __ZX_SCRN_H__

typedef struct {
    unsigned char ri;
    unsigned char gi;
    unsigned char bi;
    unsigned char ai; /* bajt nadmiarowy do kopiowania 32 bitowego */
} rgbi;

typedef struct {
    float rf;
    float gf;
    float bf;
} rgbf;

typedef struct {
    char msg[256];
    char scr_buffer[192 * 256 * 3 + 1]; /* +1 aby można było zrobić szybkie kopiowanie 32 bitowe */
    rgbi palettei[16];
    rgbf palettef[16];
    int width;
    int height;
    int bpp;
    int rgb_size[3];
    int dbf;
    char* zx_patt; /* wzory bitowe - 6kB */
    char* zx_attr; /* kolory kranu - 1kB */
    char border_color;
    char old_border_color;
    char rfsh;
    void* t0;
    void* t2;
    int flash; /* tempo migania atrybutu flash w ms */
    char inverse;
    char fscrn;
    int bright;
    int nbrt;
    int bbrt;
    int wbrt;
    long parent; /* ID okna GTK */
    float bsize;
    float scrh;
    int yresv; /* yrsv - rozmiar zarezerwowanego górnego pasa na menu GTK, bsize - rozmiar ramki*/
} zx_scrn;

extern char sdl_init; /* jeśli =1 tzn, że już była inicjowana SDL */
extern void zx_make_palette(zx_scrn* zx, int nrm, int brt, int wht);

extern void zx_scr_rfsh(zx_scrn* zx);

/* argv -> -fullscreen, -noframe */
extern char zx_initvideo(zx_scrn* zx, int width, int height, int bpp, int argc, char** argv, int rfsh);

extern void zx_setgamma(float brt, float r, float g, float b);

extern void zx_border(zx_scrn* zx, unsigned int color);

extern void (*zx_kbrd)(int key, int state, void* kbptr);

extern void zx_quit();

#endif
