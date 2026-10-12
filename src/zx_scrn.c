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

static char sdl_init_ = 0;

void zx_border(struct screen* zx, unsigned int color) {
    if (color < 16)
        zx->border_color = color;
}

void zx_make_palette(struct screen* zx, int nrm, int brt, int wht) {
    unsigned char palgen[8] = {0, 1, 4, 5, 2, 3, 6, 7};
    int color, i;
    for (i = 0; i < 16; i++) {
        color = (i == 7) ? wht : (i & 0x07 ? brt : nrm);
        zx->palettef[i].bf = (zx->palettei[i].bi = (palgen[i & 7] & 1) ? color : 0) / 256.0;
        zx->palettef[i].gf = (zx->palettei[i].gi = (palgen[i & 7] & 2) ? color : 0) / 256.0;
        zx->palettef[i].rf = (zx->palettei[i].ri = (palgen[i & 7] & 4) ? color : 0) / 256.0;
    }
}

static void zx_draw_zxscreen(struct screen* zx) {
    glDrawPixels(256, 192,GL_RGB,GL_UNSIGNED_BYTE, zx->scr_buffer);
}

#define PCPY_SCRN(b) \
        *((int*)fb) = *((int*)(pal + ((aa >> ((pt >> b) & 0x04)) & 0x0f)));\
        fb +=3;

static void zx_trans_scr(struct screen* zx) {
    char* fb = zx->scr_buffer;
    rgbi* pal = zx->palettei;
    char* attr = zx->zx_attr;
    char* x = zx->zx_patt;
    short yy, xx, aa, i, pt = 0;
    char inv = zx->inverse ? 0xff : 0;

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

static void zx_draw_border(struct screen* zx, int color) {
    glColor3f(zx->palettef[color].rf, zx->palettef[color].gf, zx->palettef[color].bf);
    glBegin(GL_POLYGON);
    glVertex2f(0.0, 0.0);
    glVertex2f(1.0, 0.0);
    glVertex2f(1.0, zx->scrh);
    glVertex2f(0.0, zx->scrh);
    glEnd();
}

void zx_scr_rfsh(struct screen* zx) {
    /* zawartość bufora po zamianie jest nieokreślona, więc ramka musi być rysowana w każdej klatce */
    zx_draw_border(zx, zx->border_color);
    zx_trans_scr(zx);
    zx_draw_zxscreen(zx);
    SDL_GL_SwapWindow(zx->window);
}

static void zx_glinit(struct screen* zx) {
    memset(zx->scr_buffer, 0x80, sizeof(zx->scr_buffer));
    zx_make_palette(zx, 0xb0, 0xf0, 0xe0);
    glViewport(0, 0, zx->width, zx->height);
    glClearColor(0.0, 0.3, 0.8, 0.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    //    glMatrixMode(GL_MODELVIEW);
    //    glLoadIdentity();

    zx->scrh = 1.0 - (float)zx->yresv / zx->height;
    if (zx->bsize > 0.5)
        zx->bsize = 0.4;
    glPixelZoom((((float)zx->width - 2 * zx->width * zx->bsize) / 256),
                (((float)zx->height - 2 * zx->height * zx->bsize - zx->yresv) / 192));
    glRasterPos2d(zx->bsize, zx->bsize);

    //    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);

    //    glEnable(GL_POINT_SMOOTH);
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    //    glDisable(GL_POINT_SMOOTH);
    //    glDisable(GL_BLEND);

    zx_draw_zxscreen(zx);
    zx_draw_border(zx, 1);
    SDL_GL_SwapWindow(zx->window);
}

static Uint32 zx_refresh(void* zx, SDL_TimerID id, Uint32 interval) {
    (void)id;
    SDL_Event event;
    SDL_zero(event);
    if (((struct screen*)zx)->rfsh == 1)
        return interval;
    ((struct screen*)zx)->rfsh = 1;
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

char zx_initvideo(struct screen* zx, int width, int height, int bpp, int argc, char** argv, int rfsh) {
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
    zx_glinit(zx);

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
    key_set(((struct elwro*)a)->kb, key, state);
}

void zx_quit() {
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&event);
}

void (*zx_kbrd)(int, int, void*) = zx_kbrd_;

void time_int(z80* z, void* ptr) {
    (void)ptr;
    z80_int(z,NULL);
}

void zx_main(struct elwro* ptr) {
    SDL_Event event;
    static char ik = 1;

    if (ik) {
        ik = 0;
        if (!(ptr->t1 = (void*)(uintptr_t)SDL_AddTimer(ptr->cpu_rfsh, zx_cpu_rfsh, ptr))) {
            msg_printf(MSG_CRITICAL_ERROR, "Could not create timer for cpu: %s\n", SDL_GetError());
            exit(0);
        }
        on_exit(zx_free_cpu_rfsh, (void*)ptr);
        msg_printf(MSG_MSG, "Set cpu refresh to     : %ims ( %iHz )\n", ptr->cpu_rfsh, (1000 / ptr->cpu_rfsh));
        msg_printf(MSG_MSG, "Set cpu tics/refresh to: %i\n", ptr->tr);
        msg_printf(MSG_MSG, "Set cpu interrupt to   : %i tics\n", ptr->ifreq);
        msg_printf(MSG_MSG, "Emulated CPU frequency : %fMHz\n", ((1000 / ptr->cpu_rfsh) * ptr->tr) / 1000000.0);
    }
    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat)
                zx_kbrd(event.key.key, 1, ptr);
            break;
        case SDL_EVENT_KEY_UP:
            zx_kbrd(event.key.key, 0, ptr);
            break;
        case SDL_EVENT_QUIT:
            msg_printf(MSG_MSG, "bye...\n");
            return;
        case SDL_EVENT_USER:
            if (ptr->zx->rfsh) {
                ptr->zx->rfsh = 0;
                zx_scr_rfsh(ptr->zx);
            }
            if (ptr->tcpu) {
                ptr->tcpu = 0;
                z80_loop(ptr->cpu, ptr->tr, ptr->ifreq, time_int, NULL);
            }
            fdc_clk(ptr->fdc);
            break;
        }
    }
}
