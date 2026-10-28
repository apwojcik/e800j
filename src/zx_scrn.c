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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "zx_scrn.h"
#include "e800j.h"
#include "z80_cpu.h"
#include "e800j_kbrd.h"
#include "e800j_fdc.h"
#include "e800j_sound.h"
#include "e800j_file.h"
#include "e800j_msg.h"

static char sdl_init_ = {};

void screen_border(struct screen* s, const unsigned char color) {
    if (color < 16)
        s->border_color = color;
}

void screen_make_palette(struct screen* s, int nrm, int brt, int wht) {
    for (int i = 0; i < 16; i++) {
        const unsigned char palgen[8] = {0, 1, 4, 5, 2, 3, 6, 7};
        int color = (i == 7) ? wht : (i & 0x07 ? brt : nrm);
        s->palettef[i].bf = (float)(s->palettei[i].bi = (palgen[i & 7] & 1) ? color : 0) / 256.0f;
        s->palettef[i].gf = (float)(s->palettei[i].gi = (palgen[i & 7] & 2) ? color : 0) / 256.0f;
        s->palettef[i].rf = (float)(s->palettei[i].ri = (palgen[i & 7] & 4) ? color : 0) / 256.0f;
    }
}

static void screen_draw(struct screen* s) {
    glDrawPixels(256, 192,GL_RGB,GL_UNSIGNED_BYTE, s->scr_buffer);
}

#define PCPY_SCRN(b) \
        *((int*)fb) = *((int*)(pal + ((aa >> ((pt >> b) & 0x04)) & 0x0f)));\
        fb +=3;

static void screen_transform(struct screen* s) {
    char* fb = s->scr_buffer;
    struct rgbi* pal = s->palettei;
    unsigned char* attr = s->zx_attr;
    unsigned char* x = s->zx_patt;
    short yy, xx, aa, i, pt = 0;
    char inv = s->inverse ? -1 : 0;

    for (i = 0; i < 6144; i++) {
        yy = i >> 5;
        xx = (i & 0x1f);
        aa = (((yy & 0xc0) | ((yy >> 3) & 7) | ((yy << 3) & 0x38))
            ^ ((~(yy << 1) & 0x80) | 0x3f)) << 5;
        pt = *(x + aa + xx) << 2;
        aa = (23 - (yy >> 3)) << 5;
        aa = *(attr + aa + xx);
        if (aa & 0x80)
            pt ^= inv;
        pt = ~pt;
        aa = ((aa << 1) & 0xf0) | (aa & 0x07) | ((aa >> 3) & 0x08);
        PCPY_SCRN(7)
        PCPY_SCRN(6)
        PCPY_SCRN(5)
        PCPY_SCRN(4)
        PCPY_SCRN(3)
        PCPY_SCRN(2)
        PCPY_SCRN(1)
        PCPY_SCRN(0)
    }
}

static void screen_draw_border(struct screen* s) {
    glColor3f(s->palettef[s->border_color].rf, s->palettef[s->border_color].gf, s->palettef[s->border_color].bf);
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(1.0f, 0.0f);
    glVertex2f(1.0f, s->scrh);
    glVertex2f(0.0f, s->scrh);
    glEnd();
}

void screen_refresh(struct screen* s) {
    /* zawartość bufora po zamianie jest nieokreślona, więc ramka musi być rysowana w każdej klatce */
    screen_draw_border(s);
    screen_transform(s);
    screen_draw(s);
    SDL_GL_SwapWindow(s->window);
}

static void screen_glinit(struct screen* s) {
    memset(s->scr_buffer, 0x80, sizeof(s->scr_buffer));
    screen_make_palette(s, 0xb0, 0xf0, 0xe0);
    glViewport(0, 0, s->width, s->height);
    glClearColor(0.0f, 0.3f, 0.8f, 0.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    //    glMatrixMode(GL_MODELVIEW);
    //    glLoadIdentity();

    s->scrh = 1.0f - (float)s->yresv / (float)s->height;
    if (s->bsize > 0.5)
        s->bsize = 0.4;
    glPixelZoom((((float)s->width - 2 * s->width * s->bsize) / 256),
                (((float)s->height - 2 * s->height * s->bsize - s->yresv) / 192));
    glRasterPos2d(s->bsize, s->bsize);

    //    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);

    //    glEnable(GL_POINT_SMOOTH);
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    //    glDisable(GL_POINT_SMOOTH);
    //    glDisable(GL_BLEND);

    screen_border(s, 1);
    screen_draw(s);
    screen_draw_border(s);
    SDL_GL_SwapWindow(s->window);
}

static Uint32 zx_refresh(void* zx, SDL_TimerID id, Uint32 interval) {
    (void)id;
    SDL_Event event;
    SDL_zero(event);
    if (((struct screen*)zx)->refresh == 1)
        return interval;
    ((struct screen*)zx)->refresh = 1;
    event.type = SDL_EVENT_USER;
    SDL_PushEvent(&event);
    return interval;
}

static Uint32 zx_cpu_rfsh(void* z, SDL_TimerID id, Uint32 interval) {
    (void)id;
    SDL_Event event;
    SDL_zero(event);
    if (((struct elwro*)z)->tcpu == 1)
        return interval;
    ((struct elwro*)z)->tcpu = 1;
    event.type = SDL_EVENT_USER;
    SDL_PushEvent(&event);
    return interval;
}

static Uint32 zx_flash(void* zx, SDL_TimerID id, Uint32 interval) {
    (void)id;
    ((struct screen*)zx)->inverse = !((struct screen*)zx)->inverse;
    return interval;
}

static void zx_free_rfsh(int n, void* zx) {
    (void)n;
    SDL_RemoveTimer((SDL_TimerID)(uintptr_t)((struct screen*)zx)->t0);
}

static void zx_free_cpu_rfsh(int n, void* zx) {
    (void)n;
    SDL_RemoveTimer((SDL_TimerID)(uintptr_t)((struct elwro*)zx)->t1);
}

static void zx_free_flash(int n, void* zx) {
    (void)n;
    SDL_RemoveTimer((SDL_TimerID)(uintptr_t)((struct screen*)zx)->t2);
}

char screen_init(struct screen* zx, int width, int height, int bpp, int argc, char** argv, int rfsh) {
    int i, ver;
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL;

    if ((i = file_lookup_option(argc, argv, "-bpp")))
        file_option_param(argc, argv, i, 0, 32, &bpp);
    if ((i = file_lookup_option(argc, argv, "-xres")))
        file_option_param(argc, argv, i, 256, 2048, &width);
    if ((i = file_lookup_option(argc, argv, "-yres")))
        file_option_param(argc, argv, i, 256, 2048, &height);

    zx->width = width;
    zx->height = height;
    zx->bpp = bpp;

    /* zainicjowanie ekranu SDL */
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        msg_printf(MSG_CRITICAL_ERROR, "Couldn't initialise SDL: %s\n", SDL_GetError());
        return 1;
    }

    ver = SDL_GetVersion();
    msg_printf(MSG_MSG, "SDL initialized\n");
    msg_printf(MSG_MSG, "SDL compiled version: %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
    msg_printf(MSG_MSG, "SDL linked version: %d.%d.%d\n",
               SDL_VERSIONNUM_MAJOR(ver), SDL_VERSIONNUM_MINOR(ver), SDL_VERSIONNUM_MICRO(ver));

    if (sdl_init_ == 0) {
        atexit(SDL_Quit);
        sdl_init_ = 1;
    }

    /* automatyczne ustawienie głębi kolorów */
    if (bpp == 0) {
        const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
        if (mode && SDL_BITSPERPIXEL(mode->format) <= 8)
            zx->bpp = 8;
        else
            zx->bpp = 16;
    }

    if (file_lookup_option(argc, argv, "-fs"))
        window_flags |= SDL_WINDOW_FULLSCREEN;
    if (file_lookup_option(argc, argv, "-nf"))
        window_flags |= SDL_WINDOW_BORDERLESS;

    /* rozmiar bitowy składowych koloru */
    switch (zx->bpp) {
    case 8:
        zx->rgb_size[0] = 3;
        zx->rgb_size[0] = 3;
        zx->rgb_size[0] = 2;
        break;
    case 15:
    case 16:
        zx->rgb_size[0] = 5;
        zx->rgb_size[0] = 5;
        zx->rgb_size[0] = 5;
        break;
    default:
        zx->rgb_size[0] = 8;
        zx->rgb_size[0] = 8;
        zx->rgb_size[0] = 8;
        break;
    }
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, zx->rgb_size[0]);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, zx->rgb_size[0]);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, zx->rgb_size[0]);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    /* inicjowanie trybu graficznego */
    if (!(zx->window = SDL_CreateWindow("Elwro 800 Junior", zx->width, zx->height, window_flags))
        || !(zx->glctx = SDL_GL_CreateContext(zx->window))) {
        msg_printf(MSG_CRITICAL_ERROR, "Couldn't set GL mode: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    /* informacja o ekranie */
    msg_printf(MSG_MSG, "Vendor     : %s\n", glGetString(GL_VENDOR));
    msg_printf(MSG_MSG, "Renderer   : %s\n", glGetString(GL_RENDERER));
    msg_printf(MSG_MSG, "Version    : %s\n", glGetString(GL_VERSION));

    /* pobranie informacji o bieżącym trybie graficznym */
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &zx->rgb_size[0]);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &zx->rgb_size[1]);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &zx->rgb_size[2]);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &zx->bpp);
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &zx->dbf);
    msg_printf(MSG_MSG, "SDL GL RED SIZE    : %d\n", zx->rgb_size[0]);
    msg_printf(MSG_MSG, "SDL GL GREEN SIZE  : %d\n", zx->rgb_size[1]);
    msg_printf(MSG_MSG, "SDL GL BLUE SIZE   : %d\n", zx->rgb_size[2]);
    msg_printf(MSG_MSG, "SDL GL DEPTH SIZE  : %d\n", zx->bpp);
    msg_printf(MSG_MSG, "SDL GL DOUBLEBUFFER: %s\n", zx->dbf ? "Yes" : "No");
    screen_glinit(zx);

    if (!(zx->t0 = (void*)(uintptr_t)SDL_AddTimer(rfsh, zx_refresh, zx))) {
        msg_printf(MSG_CRITICAL_ERROR, "Could not create timer for refresh: %s\n", SDL_GetError());
        exit(0);
    }
    on_exit(zx_free_rfsh, (void*)zx);
    msg_printf(MSG_MSG, "Set screen refresh to  : %ims ( %iHz )\n", rfsh, (1000 / rfsh));

    if (!(zx->t2 = (void*)(uintptr_t)SDL_AddTimer(zx->flash, zx_flash, zx))) {
        msg_printf(MSG_CRITICAL_ERROR, "Could not create timer for refresh: %s\n", SDL_GetError());
        exit(0);
    }
    on_exit(zx_free_flash, (void*)zx);
    msg_printf(MSG_MSG, "Set flash time         : %ims ( %iHz )\n", zx->flash, (1000 / zx->flash));

    return 0;
}

void zx_kbrd_(int key, int state, void* a) {
    //    if(key == 316) ((elwro*)a)->cpu->dbg_on = state;
    key_set(((struct elwro*)a)->kbd, key, state);
}

void zx_quit() {
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&event);
}

void (*zx_kbrd)(int, int, void*) = zx_kbrd_;

void time_int(struct cpu* z, void* ptr) {
    (void)ptr;
    z80_int(z, nullptr);
}

void zx_main(struct elwro* e) {
    SDL_Event event;
    static char ik = 1;

    if (ik != 0) {
        ik = 0;
        e->t1 = SDL_AddTimer(e->cpu_rfsh, zx_cpu_rfsh, e);
        if (e->t1 == 0) {
            msg_printf(MSG_CRITICAL_ERROR, "Could not create timer for cpu: %s\n", SDL_GetError());
            exit(0);
        }
        on_exit(zx_free_cpu_rfsh, (void*)e);
        msg_printf(MSG_MSG, "Set cpu refresh to     : %ims ( %iHz )\n", e->cpu_rfsh, (1000 / e->cpu_rfsh));
        msg_printf(MSG_MSG, "Set cpu tics/refresh to: %i\n", e->tr);
        msg_printf(MSG_MSG, "Set cpu interrupt to   : %i tics\n", e->ifreq);
        msg_printf(MSG_MSG, "Emulated CPU frequency : %fMHz\n", ((1000 / e->cpu_rfsh) * e->tr) / 1000000.0);
    }
    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat)
                zx_kbrd(event.key.key, 1, e);
            break;
        case SDL_EVENT_KEY_UP:
            zx_kbrd(event.key.key, 0, e);
            break;
        case SDL_EVENT_QUIT:
            msg_printf(MSG_MSG, "bye...\n");
            return;
        case SDL_EVENT_USER:
            if (e->scn->refresh) {
                e->scn->refresh = 0;
                screen_refresh(e->scn);
            }
            if (e->tcpu) {
                e->tcpu = 0;
                z80_loop(e->cpu, e->tr, e->ifreq, time_int, NULL);
            }
            fdc_clk(e->fdc);
            break;
        }
    }
}
