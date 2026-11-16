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

#define TOAST_QUEUE 4

struct screen {
    char msg[256];
    uint32_t scr_buffer[192 * 256]; /* piksele RGBA (bajty R,G,B,A), wiersz 0 = dół obrazu */
    unsigned int scr_tex; /* tekstura OpenGL 256x192 z obrazem */
    bool scr_valid; /* false -> następna klatka musi być przeliczona i wysłana do tekstury */
    unsigned char prev_patt[6144]; /* zawartość ekranu z ostatniej konwersji (wykrywanie zmian) */
    unsigned char prev_attr[768];
    int prev_inv;
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
    bool help; /* nakładka z pomocą (klawisze skrótów) jest widoczna */
    unsigned int help_tex; /* tekstura OpenGL z tekstem pomocy, 0 -> jeszcze nie utworzona */
    int help_w;
    int help_h;
    int toast_ms; /* czas wyświetlania komunikatu o operacji w ms, 0 -> wyłączony (osd_time w konfiguracji) */
    unsigned int toast_tex;
    int toast_w;
    int toast_h;
    char toast_next[TOAST_QUEUE][256]; /* kolejka komunikatów do pokazania po zniknięciu bieżącego */
    int toast_queued;
    bool toast_sticky; /* komunikat nie wygasa w czasie, dopóki widoczna jest pomoc (F1) */
    Uint64 toast_until; /* SDL_GetTicks(), po którym komunikat znika */
};

void screen_make_palette(struct screen* s, int nrm, int brt, int wht);

void screen_refresh(struct screen* s);

/* pokazuje/ukrywa nakładkę z pomocą, od następnego odświeżenia ekranu */
void screen_help(struct screen* s, bool show);

/* komunikat o operacji (printf) w lewym górnym rogu na czas s->toast_ms, zastępuje poprzedni */
void screen_toast(struct screen* s, const char* fmt, ...);

/* komunikat (wiersze rozdzielone '\n') dodawany do kolejki (TOAST_QUEUE) i pokazywany po zniknięciu bieżącego i wcześniejszych, a gdy
 * ich nie ma - od razu; screen_toast() czyści kolejkę */
void screen_toast_next(struct screen* s, const char* text);

/* argv -> -fullscreen, -noframe */
bool screen_init(struct screen* s, int width, int height, int bpp, int argc, const char** argv, int refresh);

void screen_border(struct screen* s, unsigned char color);

/* zmiana koloru ramki w takcie CPU 'ticks' (zapis do portu 0xFE) */
void screen_border_set(struct screen* s, long ticks, unsigned char color);

/* początek klatki (przerwanie) w takcie CPU 'ticks'; 'len' - długość klatki w taktach */
void screen_frame_start(struct screen* s, long ticks, int len);

void zx_quit(void);
