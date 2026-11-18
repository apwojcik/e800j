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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "zx_scrn.h"
#include <SDL3/SDL_video.h>
#include "e800j.h"
#include "e800j_compat.h"
#include "z80_cpu.h"
#include "z80_debug.h"
#include "e800j_kbrd.h"
#include "e800j_fdc.h"
#include "e800j_sound.h"
#include "e800j_file.h"
#include "e800j_disk.h"
#include "e800j_msg.h"
#include "zx_scrn.h"
#include "e800j_fdc.h"

static bool sdl_init_ = false;

void screen_border(struct screen* s, const unsigned char color) {
    if (color < 16)
        s->border_color = color;
}

void screen_make_palette(struct screen* s, int nrm, int brt, int wht) {
    s->scr_valid = false; /* zmiana palety wymaga ponownej konwersji */
    for (int i = 0; i < 16; i++) {
        const unsigned char palgen[8] = {0, 1, 4, 5, 2, 3, 6, 7};
        int color = (i == 7) ? wht : (i & 0x08 ? brt : nrm);
        s->palettef[i].bf = (float)(s->palettei[i].bi = (unsigned char)((palgen[i & 7] & 1) ? color : 0)) / 256.0f;
        s->palettef[i].gf = (float)(s->palettei[i].gi = (unsigned char)((palgen[i & 7] & 2) ? color : 0)) / 256.0f;
        s->palettef[i].rf = (float)(s->palettei[i].ri = (unsigned char)((palgen[i & 7] & 4) ? color : 0)) / 256.0f;
    }
}

/* obraz jako tekstura 256x192 rozciągnięta na pole obrazu (py0..py1, bsize..1-bsize) */
static void screen_draw(struct screen* s) {
    const float x0 = s->bsize;
    const float x1 = 1.0f - s->bsize;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, s->scr_tex);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x0, s->py0);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x1, s->py0);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x1, s->py1);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x0, s->py1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

static void screen_upload(struct screen* s) {
    glBindTexture(GL_TEXTURE_2D, s->scr_tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 192, GL_RGBA, GL_UNSIGNED_BYTE, s->scr_buffer);
}

#define PCPY_SCRN(b) \
        *fb++ = pal[(aa >> ((pt >> b) & 0x04)) & 0x0f];

/* przelicza ekran Spectrum na piksele; zwraca false, gdy nic się nie zmieniło od poprzedniej konwersji */
static bool screen_transform(struct screen* s) {
    uint32_t* fb = s->scr_buffer;
    uint32_t pal[16];
    const unsigned char* attr = s->zx_attr;
    const unsigned char* patt = s->zx_patt;
    int pt = 0;
    const int inv = SDL_GetAtomicInt(&s->inverse) ? -1 : 0;

    if (s->scr_valid && inv == s->prev_inv && memcmp(patt, s->prev_patt, sizeof(s->prev_patt)) == 0 &&
        memcmp(attr, s->prev_attr, sizeof(s->prev_attr)) == 0)
        return false;
    memcpy(s->prev_patt, patt, sizeof(s->prev_patt));
    memcpy(s->prev_attr, attr, sizeof(s->prev_attr));
    s->prev_inv = inv;
    s->scr_valid = true;
    for (int i = 0; i < 16; i++)
        memcpy(&pal[i], &s->palettei[i], sizeof(pal[i]));

    for (short i = 0; i < 6144; i++) {
        const short yy = (short)(i >> 5);
        const short xx = (short)(i & 0x1f);
        int aa = (((yy & 0xc0) | ((yy >> 3) & 7) | ((yy << 3) & 0x38))
            ^ ((~(yy << 1) & 0x80) | 0x3f)) << 5;
        pt = *(patt + aa + xx) << 2;
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
    return true;
}

/* układ linii ekranu (jak w ZX Spectrum 48K): 312 linii na klatkę, widoczne 288 */
#define SCR_LINES_TOTAL 312
#define SCR_LINE_FIRST 16 /* pierwsza widoczna linia klatki */
#define SCR_LINES_TOP 48
#define SCR_LINES_PAPER 192
#define SCR_LINES_VIS 288
#define SHOT_BORDER_X 48 /* szerokość ramki z boków na zrzucie ekranu (jak u góry i u dołu) */
#define SHOT_W (256 + 2 * SHOT_BORDER_X)

void screen_border_set(struct screen* s, const long ticks, const unsigned char color) {
    struct border_frame* f = &s->bframe[s->bcur];
    const unsigned char last = f->n ? f->ev[f->n - 1].color : f->start;

    s->border_color = color;
    if (color == last || f->n >= BORDER_EV_MAX)
        return;
    long t = ticks - s->frame_t0;
    if (t < 0)
        t = 0;
    f->ev[f->n].t = (int)t;
    f->ev[f->n].color = color;
    f->n++;
}

void screen_frame_start(struct screen* s, const long ticks, const int len) {
    s->bframe[s->bcur].len = len;
    if (s->bstarted)
        s->bhave = true;
    s->bstarted = true;
    s->bcur ^= 1;
    s->bframe[s->bcur].n = 0;
    s->bframe[s->bcur].start = s->border_color;
    s->frame_t0 = ticks;
}

/* górna krawędź widocznej linii v (0..288) we współrzędnych okna; linie ramki są
 * rozciągnięte na górny i dolny margines, linie obrazu na pole obrazu */
static float screen_line_y(const struct screen* s, const int v) {
    if (v <= SCR_LINES_TOP)
        return 1.0f - (1.0f - s->py1) * (float)v / SCR_LINES_TOP;
    if (v <= SCR_LINES_TOP + SCR_LINES_PAPER)
        return s->py1 - (s->py1 - s->py0) * (float)(v - SCR_LINES_TOP) / SCR_LINES_PAPER;
    return s->py0 - s->py0 * (float)(v - SCR_LINES_TOP - SCR_LINES_PAPER) / (SCR_LINES_VIS - SCR_LINES_TOP - SCR_LINES_PAPER);
}

/* kolor ramki każdej z widocznych linii (SCR_LINES_VIS) z zapisów portu w ostatniej pełnej klatce */
static void screen_border_colors(const struct screen* s, unsigned char* col) {
    if (s->bhave) {
        const struct border_frame* f = &s->bframe[s->bcur ^ 1];
        unsigned char c = f->start;
        int idx = 0;
        for (int v = 0; v < SCR_LINES_VIS; v++) {
            const int t = (int)(((double)(SCR_LINE_FIRST + v) + 0.5) * f->len / SCR_LINES_TOTAL);
            while (idx < f->n && f->ev[idx].t <= t)
                c = f->ev[idx++].color;
            col[v] = c;
        }
    }
    else {
        memset(col, s->border_color, SCR_LINES_VIS);
    }
}

/* ramka rysowana liniami: kolor każdej linii z zapisów portu w ostatniej pełnej klatce */
static void screen_draw_border(struct screen* s) {
    unsigned char col[SCR_LINES_VIS];

    screen_border_colors(s, col);
    glBegin(GL_QUADS);
    for (int v = 0; v < SCR_LINES_VIS;) {
        int e = v + 1;
        while (e < SCR_LINES_VIS && col[e] == col[v])
            e++;
        const struct rgbf* c = &s->palettef[col[v] & 0x0f];
        const float ytop = screen_line_y(s, v);
        const float ybot = screen_line_y(s, e);
        glColor3f(c->rf, c->gf, c->bf);
        glVertex2f(0.0f, ybot);
        glVertex2f(1.0f, ybot);
        glVertex2f(1.0f, ytop);
        glVertex2f(0.0f, ytop);
        v = e;
    }
    glEnd();
}

static const char* const help_lines[] = {
    "ELWRO 800 JUNIOR - HOT KEYS",
    "",
    "F1            this help",
    "F5 / F6       insert disk in drive A / B",
    "Shift+F5/F6   eject disk from drive A / B",
    HK_ALT_NAME "+F5/F6" HK_ALT_PAD "     toggle RO/RW of disk in A / B",
    HK_WIN_NAME "+F5/F6" HK_WIN_PAD "    new blank disk in A / B (save dialog)",
    HK_CTRL_NAME "+F1..F5" HK_CTRL_PAD "   window scale x1 .. x3, step 0.5",
    "F2 / F3       volume down / up",
    "F8            screenshot with border (e800j_NNN.png)",
    HK_CTRL_NAME "+F8" HK_CTRL_PAD "       screenshot without border",
    "F9            fullscreen on / off",
    "Pause         pause / resume",
    "Drop file     *.dsk: left half = A, right half = B; snapshot: load",
    "F11           Z80 debugger (console)",
    "F12           save snapshot (e800j_NNN.sna)",
    "Shift+F12     soft reset (reset button)",
    HK_CTRL_NAME "+F12" HK_CTRL_PAD "      cold restart (fresh start)",
    HK_ALT_NAME "+F12" HK_ALT_PAD "       cold restart + reload command-line snapshot",
    HK_WIN_NAME "+F12" HK_WIN_PAD "      cold restart + snapshot from file dialog",
    "",
    "Elwro keys:",
    "Home          CP/J",
    "F10           DIR",
    "End           ERASE",
    "Page Up       LOAD",
    "Page Down     SAVE",
    "",
    "Esc           close this help",
};

#define OSD_PAD 12
#define OSD_LINE_H 12
#define OSD_TOAST_VPAD 4 /* komunikat ma ciaśniejsze marginesy pionowe niż pomoc */
#define OSD_VOL_CELL 10 /* szerokość jednego poziomu głośności na pasku */
#define OSD_VOL_GAP 6 /* odstęp paska od cyfr 0 i max */
#define OSD_VOL_MS 1500 /* czas wyświetlania paska głośności (nie dłużej niż osd_time) */
#define OSD_TOAST_MARGIN 6 /* odstęp komunikatu od lewej i górnej krawędzi okna (przed powiększeniem) */

/* tekst rysowany wbudowaną czcionką SDL na powierzchni programowej, następnie wysłany jako tekstura GL;
 * zwraca 0 w razie błędu */
static void screen_text_size(const char* const* lines, const int n, const int vpad, int* width, int* height) {
    int chars = 0;
    for (int i = 0; i < n; i++) {
        chars = SDL_max(chars, (int)strlen(lines[i]));
    }
    *width = chars * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + 2 * OSD_PAD;
    *height = n * OSD_LINE_H + 2 * vpad - (OSD_LINE_H - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE);
}

/* wysyła powierzchnię RGBA32 jako teksturę GL i zwalnia ją */
static unsigned int screen_surface_texture(SDL_Surface* surface) {
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch / 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    SDL_DestroySurface(surface);
    return tex;
}

static unsigned int screen_text_texture(const char* const* lines, const int n, const int vpad, int* width, int* height) {
    int w, h;
    screen_text_size(lines, n, vpad, &w, &h);
    SDL_Surface* surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL) {
        msg_printf(MSG_ERROR, "Could not create overlay surface: %s\n", SDL_GetError());
        return 0;
    }
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    if (renderer == NULL) {
        msg_printf(MSG_ERROR, "Could not create overlay renderer: %s\n", SDL_GetError());
        SDL_DestroySurface(surface);
        return 0;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 210);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < n; i++) {
        SDL_RenderDebugText(renderer, OSD_PAD, (float)(vpad + i * OSD_LINE_H), lines[i]);
    }
    SDL_FlushRenderer(renderer);
    SDL_DestroyRenderer(renderer);

    *width = w;
    *height = h;
    return screen_surface_texture(surface);
}

/* rysuje teksturę o rozmiarze w x h pikseli z lewym górnym rogiem w (x, y) pikseli okna */
static void screen_draw_texture(const struct screen* s, const unsigned int tex, const int x, const int y, const int w, const int h) {
    const float x0 = (float)x / (float)s->width;
    const float x1 = (float)(x + w) / (float)s->width;
    const float y0 = 1.0f - (float)(y + h) / (float)s->height;
    const float y1 = 1.0f - (float)y / (float)s->height;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, tex);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x0, y0);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x1, y0);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x1, y1);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x0, y1);
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

/* wspólna krotność powiększenia nakładek: największa całkowita, przy której mieści się okno pomocy */
static int screen_osd_zoom(const struct screen* s) {
    int w, h;
    screen_text_size(help_lines, (int)SDL_arraysize(help_lines), OSD_PAD, &w, &h);
    return SDL_max(1, SDL_min(s->width * 9 / 10 / w, s->height * 9 / 10 / h));
}

/* nakładka z pomocą wyśrodkowana w oknie, powiększana całkowitą krotnością, o ile się mieści */
static void screen_draw_help(struct screen* s) {
    if (s->help_tex == 0) {
        s->help_tex = screen_text_texture(help_lines, (int)SDL_arraysize(help_lines), OSD_PAD, &s->help_w, &s->help_h);
        if (s->help_tex == 0) {
            s->help = false;
            return;
        }
    }
    const int zoom = screen_osd_zoom(s);
    const int w = s->help_w * zoom;
    const int h = s->help_h * zoom;
    screen_draw_texture(s, s->help_tex, (s->width - w) / 2, (s->height - h) / 2, w, h);
}

/* tekst (wiersze rozdzielone '\n') jako nowy komunikat na czas s->toast_ms */
static void screen_toast_show(struct screen* s, const char* text) {
    char buf[sizeof(s->toast_next[0])];
    const char* lines[8];
    int n = 0;
    snprintf(buf, sizeof(buf), "%s", text);
    for (char* p = buf; n < 8 && p != nullptr;) {
        lines[n++] = p;
        p = strchr(p, '\n');
        if (p != nullptr) {
            *p++ = 0;
        }
    }
    if (s->toast_tex != 0) {
        glDeleteTextures(1, &s->toast_tex);
    }
    s->toast_tex = screen_text_texture(lines, n, OSD_TOAST_VPAD, &s->toast_w, &s->toast_h);
    s->toast_until = SDL_GetTicks() + (Uint64)s->toast_ms;
    s->toast_sticky = false;
}

/* komunikat o ostatniej operacji w lewym górnym rogu, znika po s->toast_ms, potem ewentualny następny */
static void screen_draw_toast(struct screen* s) {
    if (s->toast_tex == 0) {
        return;
    }
    /* komunikat przypięty do pomocy trwa, dopóki pomoc jest widoczna, i znika razem z nią */
    const bool expired = s->toast_sticky ? !s->help : SDL_GetTicks() >= s->toast_until;
    if (expired) {
        s->toast_sticky = false;
        glDeleteTextures(1, &s->toast_tex);
        s->toast_tex = 0;
        if (s->toast_queued > 0) {
            char text[sizeof(s->toast_next[0])];
            snprintf(text, sizeof(text), "%s", s->toast_next[0]);
            memmove(s->toast_next[0], s->toast_next[1], sizeof(s->toast_next[0]) * (TOAST_QUEUE - 1));
            s->toast_queued--;
            screen_toast_show(s, text);
        }
        if (s->toast_tex == 0) {
            return;
        }
    }
    /* ten sam rozmiar znaków co pomoc, zmniejszany tylko, gdy komunikat nie mieści się w oknie */
    const int zoom = SDL_max(1, SDL_min(screen_osd_zoom(s), (s->width - OSD_TOAST_MARGIN) / s->toast_w));
    const int margin = OSD_TOAST_MARGIN * zoom;
    screen_draw_texture(s, s->toast_tex, margin, margin, s->toast_w * zoom, s->toast_h * zoom);
}

/* pasek głośności: tytuł oraz "0 [pasek] max", w stylu komunikatu; zwraca 0 w razie błędu */
static unsigned int screen_volume_texture(const int level, const int max, int* width, int* height) {
    const int ch = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
    char title[32], maxs[16];
    snprintf(title, sizeof(title), "Volume %d%%", max > 0 ? level * 100 / max : 0);
    snprintf(maxs, sizeof(maxs), "%d", max);
    const int bar_w = max * OSD_VOL_CELL + 4;
    const int bar_x = OSD_PAD + ch + OSD_VOL_GAP;
    const int w = bar_x + bar_w + OSD_VOL_GAP + (int)strlen(maxs) * ch + OSD_PAD;
    const int h = 2 * OSD_LINE_H + 2 * OSD_TOAST_VPAD - (OSD_LINE_H - ch);
    SDL_Surface* surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL) {
        msg_printf(MSG_ERROR, "Could not create overlay surface: %s\n", SDL_GetError());
        return 0;
    }
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    if (renderer == NULL) {
        msg_printf(MSG_ERROR, "Could not create overlay renderer: %s\n", SDL_GetError());
        SDL_DestroySurface(surface);
        return 0;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 210);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    const float row = (float)(OSD_TOAST_VPAD + OSD_LINE_H);
    SDL_RenderDebugText(renderer, (float)(w - (int)strlen(title) * ch) / 2.0f, (float)OSD_TOAST_VPAD, title);
    SDL_RenderDebugText(renderer, OSD_PAD, row, "0");
    SDL_RenderDebugText(renderer, (float)(bar_x + bar_w + OSD_VOL_GAP), row, maxs);
    const SDL_FRect frame = { (float)bar_x, row, (float)bar_w, (float)ch };
    SDL_RenderRect(renderer, &frame);
    const SDL_FRect fill = { (float)(bar_x + 2), row + 2, (float)(level * OSD_VOL_CELL), (float)(ch - 4) };
    SDL_RenderFillRect(renderer, &fill);
    SDL_FlushRenderer(renderer);
    SDL_DestroyRenderer(renderer);

    *width = w;
    *height = h;
    return screen_surface_texture(surface);
}

/* pasek głośności na dole okna, wyśrodkowany, znika po OSD_VOL_MS */
static void screen_draw_volume(struct screen* s) {
    if (s->vol_tex == 0) {
        return;
    }
    if (SDL_GetTicks() >= s->vol_until) {
        glDeleteTextures(1, &s->vol_tex);
        s->vol_tex = 0;
        return;
    }
    const int zoom = SDL_max(1, SDL_min(screen_osd_zoom(s), s->width / s->vol_w));
    const int w = s->vol_w * zoom;
    const int h = s->vol_h * zoom;
    screen_draw_texture(s, s->vol_tex, (s->width - w) / 2, s->height - h - OSD_TOAST_MARGIN * zoom, w, h);
}

void screen_volume(struct screen* s, const int level, const int max) {
    if (s->toast_ms <= 0) {
        return;
    }
    if (s->vol_tex != 0) {
        glDeleteTextures(1, &s->vol_tex);
    }
    s->vol_tex = screen_volume_texture(level, max, &s->vol_w, &s->vol_h);
    s->vol_until = SDL_GetTicks() + (Uint64)SDL_min(s->toast_ms, OSD_VOL_MS);
}

void screen_toast(struct screen* s, const char* fmt, ...) {
    if (s->toast_ms <= 0) {
        return;
    }
    char text[sizeof(s->toast_next[0])];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    s->toast_queued = 0;
    screen_toast_show(s, text);
}

void screen_toast_next(struct screen* s, const char* text) {
    if (s->toast_ms <= 0) {
        return;
    }
    if (s->toast_tex == 0) {
        screen_toast_show(s, text);
        return;
    }
    if (s->toast_queued < TOAST_QUEUE) {
        snprintf(s->toast_next[s->toast_queued++], sizeof(s->toast_next[0]), "%s", text);
    }
}

void screen_help(struct screen* s, const bool show) {
    s->help = show;
}

void screen_pause(struct screen* s, const bool paused) {
    s->paused = paused;
}

/* wskaźnik wstrzymania w prawym górnym rogu, w stylu komunikatu, niezależny od komunikatów operacji */
static void screen_draw_pause(struct screen* s) {
    if (s->pause_tex == 0) {
        static const char* const lines[] = { "PAUSED" };
        s->pause_tex = screen_text_texture(lines, 1, OSD_TOAST_VPAD, &s->pause_w, &s->pause_h);
        if (s->pause_tex == 0) {
            return;
        }
    }
    const int zoom = SDL_max(1, SDL_min(screen_osd_zoom(s), (s->width - OSD_TOAST_MARGIN) / s->pause_w));
    const int margin = OSD_TOAST_MARGIN * zoom;
    const int w = s->pause_w * zoom;
    screen_draw_texture(s, s->pause_tex, s->width - w - margin, margin, w, s->pause_h * zoom);
}

void screen_fullscreen_toggle(struct screen* s) {
    const bool fullscreen = (SDL_GetWindowFlags(s->window) & SDL_WINDOW_FULLSCREEN) != 0;
    if (!SDL_SetWindowFullscreen(s->window, !fullscreen)) {
        msg_printf(MSG_ERROR, "Fullscreen switch failed: %s\n", SDL_GetError());
        return;
    }
    s->fscrn = !fullscreen;
}

bool screen_screenshot(struct screen* s, const char* path, const bool border) {
    /* bufor bywa nieaktualny, jeśli od ostatniej klatki ekran się zmienił - przelicz i odśwież teksturę */
    if (screen_transform(s)) {
        screen_upload(s);
    }
    /* z ramką: widoczny obszar jak na prawdziwym Spectrum (obraz 256x192 w ramce o szerokości SHOT_BORDER_X z boków
     * oraz SCR_LINES_TOP u góry i u dołu, kolor ramki liniami, też zmiany w trakcie klatki); bez ramki: sam obraz */
    unsigned char col[SCR_LINES_VIS];
    screen_border_colors(s, col);
    const int w = border ? SHOT_W : 256;
    const int h = border ? SCR_LINES_VIS : SCR_LINES_PAPER;
    const int left = border ? SHOT_BORDER_X : 0;
    const int top = border ? SCR_LINES_TOP : 0;
    unsigned char* rgba = SDL_malloc((size_t)w * h * 4);
    if (rgba == nullptr) {
        return false;
    }
    /* bajty R,G,B,A, bez przezroczystości; wiersz 0 bufora obrazu to jego dół, a PNG jest zapisywany od góry */
    for (int v = 0; v < h; v++) {
        unsigned char* row = rgba + (size_t)v * w * 4;
        if (border) {
            const struct rgbi* c = &s->palettei[col[v] & 0x0f];
            for (int x = 0; x < w; x++) {
                row[x * 4 + 0] = c->ri;
                row[x * 4 + 1] = c->gi;
                row[x * 4 + 2] = c->bi;
                row[x * 4 + 3] = 0xff;
            }
        }
        const int r = v - top;
        if (r >= 0 && r < SCR_LINES_PAPER) {
            const unsigned char* src = (const unsigned char*)s->scr_buffer + (size_t)(SCR_LINES_PAPER - 1 - r) * 256 * 4;
            memcpy(row + (size_t)left * 4, src, 256 * 4);
            for (int x = 0; x < 256; x++) {
                row[(left + x) * 4 + 3] = 0xff;
            }
        }
    }
    bool ok = false;
    SDL_Surface* surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, rgba, w * 4);
    if (surface != nullptr) {
        ok = SDL_SavePNG(surface, path);
        SDL_DestroySurface(surface);
    }
    if (!ok) {
        msg_printf(MSG_ERROR, "Screenshot %s failed: %s\n", path, SDL_GetError());
    }
    SDL_free(rgba);
    return ok;
}

void screen_refresh(struct screen* s) {
    /* zawartość bufora po zamianie jest nieokreślona, więc ramka musi być rysowana w każdej klatce */
    screen_draw_border(s);
    if (screen_transform(s))
        screen_upload(s);
    screen_draw(s);
    if (s->help) {
        screen_draw_help(s);
    }
    screen_draw_toast(s);
    screen_draw_volume(s);
    if (s->paused) {
        screen_draw_pause(s);
    }
    SDL_GL_SwapWindow(s->window);
}

/* geometria obrazu i ramki; wywoływana też po zmianie rozmiaru okna */
static void screen_layout(struct screen* s) {
    glViewport(0, 0, s->width, s->height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

    const float height = (float)s->height;
    const float yresv = (float)s->yresv;

    if (s->bsize > 0.5f)
        s->bsize = 0.4f;
    s->py0 = s->bsize;
    s->py1 = s->bsize + (height - 2 * height * s->bsize - yresv) / height;
}

/* ustawia rozmiar okna tak, by obraz Spectrum (256x192) miał podane powiększenie (z ramką) */
static void screen_set_scale(struct screen* s, const float scale) {
    if ((SDL_GetWindowFlags(s->window) & SDL_WINDOW_FULLSCREEN) != 0)
        return;
    const float inner = 1.0f - 2 * s->bsize;
    const float density = SDL_max(SDL_GetWindowPixelDensity(s->window), 1.0f);
    const int w = (int)(256 * scale / inner / density + 0.5f);
    const int h = (int)((192 * scale + (float)s->yresv) / inner / density + 0.5f);
    SDL_RestoreWindow(s->window);
    SDL_SetWindowSize(s->window, w, h);
    screen_toast(s, "Window scale x%g", (double)scale / 2.0);
}

static void screen_glinit(struct screen* s) {
    memset(s->scr_buffer, 0x80, sizeof(s->scr_buffer));
    s->scr_valid = false;
    glGenTextures(1, &s->scr_tex);
    glBindTexture(GL_TEXTURE_2D, s->scr_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, s->scr_buffer);
    /* poziomy jasności z konfiguracji; gdy brak (0) - wartości domyślne */
    screen_make_palette(s, s->nbrt ? s->nbrt : 0xb0, s->bbrt ? s->bbrt : 0xf0, s->wbrt ? s->wbrt : 0xe0);
    glClearColor(0.0f, 0.3f, 0.8f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    screen_layout(s);

    screen_border(s, 1);
    screen_draw(s);
    screen_draw_border(s);
    SDL_GL_SwapWindow(s->window);
}

static Uint32 zx_screen_refresh(void *userdata, SDL_TimerID timerID, Uint32 interval) {
    (void)timerID;
    struct screen* s = userdata;
    if (!SDL_GetAtomicInt(&s->refresh)) {
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_EVENT_USER;
        SDL_SetAtomicInt(&s->refresh, 1);
        SDL_PushEvent(&event);
    }
    return interval;
}

static Uint32 zx_cpu_refresh(void *userdata, SDL_TimerID timerID, Uint32 interval) {
    (void)timerID;
    struct elwro* e = userdata;
    if (!e->tcpu) {
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_EVENT_USER;
        e->tcpu = true;
        SDL_PushEvent(&event);
    }
    return interval;
}

static Uint32 zx_flash(void* zx, SDL_TimerID timerID, Uint32 interval) {
    (void)timerID;
    struct screen* s = zx;
    SDL_SetAtomicInt(&s->inverse, !SDL_GetAtomicInt(&s->inverse));
    return interval;
}

static void zx_free_refresh(const int n, void* zx) {
    (void)n;
    SDL_RemoveTimer(((struct screen*)zx)->t0);
}

static void zx_free_cpu_refresh(const int n, void* zx) {
    (void)n;
    SDL_RemoveTimer(((struct elwro*)zx)->t1);
}

static void zx_free_gl(const int n, void* zx) {
    (void)n;
    struct screen* s = zx;
    if (s->glctx)
        SDL_GL_DestroyContext(s->glctx);
    if (s->window)
        SDL_DestroyWindow(s->window);
}

static void zx_free_flash(const int n, void* zx) {
    (void)n;
    SDL_RemoveTimer(((struct screen*)zx)->t2);
}

bool screen_init(struct screen* s, const int refresh) {
    if (refresh <= 0 || s->flash <= 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Invalid screen refresh (%i ms) or blink (%i ms) time\n", refresh, s->flash);
        return false;
    }

    /* zainicjowanie ekranu SDL */
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        msg_printf(MSG_CRITICAL_ERROR, "Couldn't initialise SDL: %s\n", SDL_GetError());
        return false;
    }

    msg_printf(MSG_MSG, "SDL initialized\n");
    const int compiled = SDL_VERSION;
    msg_printf(MSG_MSG, "SDL compiled version: %i.%i.%i\n",
        SDL_VERSIONNUM_MAJOR(compiled),
        SDL_VERSIONNUM_MINOR(compiled),
        SDL_VERSIONNUM_MICRO(compiled));
    const int linked = SDL_GetVersion();
    msg_printf(MSG_MSG, "SDL linked version: %i.%i.%i\n",
        SDL_VERSIONNUM_MAJOR(linked),
        SDL_VERSIONNUM_MINOR(linked),
        SDL_VERSIONNUM_MICRO(linked));

    if (sdl_init_ == false) {
        atexit(SDL_Quit);
        sdl_init_ = true;
    }

    /* automatyczne ustawienie głębi kolorów */
    if (s->bpp == 0) {
        const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
        if (mode && SDL_BITSPERPIXEL(mode->format) <= 8)
            s->bpp = 8;
        else
            s->bpp = 16;
    }

    Uint32 video_flags = SDL_WINDOW_OPENGL;
    if (s->fscrn)
        video_flags |= SDL_WINDOW_FULLSCREEN;
    if (s->noframe)
        video_flags |= SDL_WINDOW_BORDERLESS;

    /* rozmiar bitowy składowych koloru */
    switch (s->bpp) {
    case 8:
        s->rgb_size[0] = 3;
        s->rgb_size[1] = 3;
        s->rgb_size[2] = 2;
        break;
    case 15:
    case 16:
        s->rgb_size[0] = 5;
        s->rgb_size[1] = 5;
        s->rgb_size[2] = 5;
        break;
    default:
        s->rgb_size[0] = 8;
        s->rgb_size[1] = 8;
        s->rgb_size[2] = 8;
        break;
    }
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, s->rgb_size[0]);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, s->rgb_size[1]);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, s->rgb_size[2]);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    /* inicjowanie trybu graficznego */
    SDL_Window* window = SDL_CreateWindow("Elwro 800 Junior", s->width, s->height, video_flags);
    if (window == 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Couldn't set GL mode: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    s->window = window;
    s->glctx = SDL_GL_CreateContext(window);
    if (s->glctx == 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Couldn't create GL context: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    on_exit(zx_free_gl, (void*)s);

    /* rozmiar w pikselach może różnić się od żądanego (pełny ekran, HiDPI) */
    SDL_GetWindowSizeInPixels(window, &s->width, &s->height);

    /* informacja o ekranie */
    msg_printf(MSG_MSG, "Vendor     : %s\n", glGetString(GL_VENDOR));
    msg_printf(MSG_MSG, "Renderer   : %s\n", glGetString(GL_RENDERER));
    msg_printf(MSG_MSG, "Version    : %s\n", glGetString(GL_VERSION));

    /* pobranie informacji o bieżącym trybie graficznym */
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &s->rgb_size[0]);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &s->rgb_size[1]);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &s->rgb_size[2]);
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &s->dbf);

    msg_printf(MSG_MSG, "SDL GL RED SIZE: %i\n", s->rgb_size[0]);
    msg_printf(MSG_MSG, "SDL GL GREEN SIZE: %i\n", s->rgb_size[1]);
    msg_printf(MSG_MSG, "SDL GL BLUE SIZE: %i\n", s->rgb_size[2]);
    msg_printf(MSG_MSG, "SDL GL DOUBLE BUFFER: %s\n", s->dbf ? "Yes" : "No");
    /*
        SDL_VERSION(&(info.version));
        if( SDL_GetWMInfo(&info) == -1){
        printf("(EE)Error getting WM info: %s\n",SDL_GetError());
        exit(0);
        }
        XReparentWindow(info.info.x11.display, info.info.x11.wmwindow, zx->parent,0,0 );
    */
    screen_glinit(s);

    /* w konfiguracji rfsh było wczytane do flagi - zerujemy, inaczej timer nigdy nie wyśle zdarzenia */
    SDL_SetAtomicInt(&s->refresh, 0);
    if ((s->t0 = SDL_AddTimer(refresh, zx_screen_refresh, s)) == 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Could not create timer for refresh: %s\n", SDL_GetError());
        exit(0);
    }
    on_exit(zx_free_refresh, (void*)s);
    msg_printf(MSG_MSG, "Screen refresh: %ims (%iHz)\n", refresh, (1000 / refresh));

    if ((s->t2 = SDL_AddTimer(s->flash, zx_flash, s)) == 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Could not create timer for refresh: %s\n", SDL_GetError());
        exit(0);
    }
    on_exit(zx_free_flash, (void*)s);
    msg_printf(MSG_MSG, "Flash time: %ims ( %iHz )\n", s->flash, (1000 / s->flash));

    return true;
}

static void zx_kbrd(const unsigned int key, const int state, void* a) {
    key_set(((struct elwro*)a)->kbd, key, state);
}

static void time_int(struct cpu* z, void* ptr) {
    const struct elwro* e = ptr;
    /* call_cnt przekracza okres o takty, które minęły od faktycznego początku klatki */
    screen_frame_start(e->scn, z->ticks - (z->call_cnt - e->ifreq), e->ifreq);
    key_macro_frame(e->kbd);
    z80_int(z, NULL);
}

/* SDL_EVENT_USER = zegar CPU/ekranu, +1 = dyskietki (e800j_disk.c) */
#define SNAP_EVENT_TYPE (SDL_EVENT_USER + 2)

/* rozszerzenia snapshotów: filtr okna wyboru pliku i rozpoznawanie upuszczonych plików */
#define SNAP_EXTS "sna;z80;szx;sp;snp;zxs;sit;slt"

static const SDL_DialogFileFilter snap_filters[] = {
    { "Snapshots", SNAP_EXTS },
    { "All files", "*" },
};

/* wywoływane przez SDL, często z innego wątku - wynik (ścieżka albo nullptr po anulowaniu/błędzie) trafia do głównej pętli */
static void SDLCALL snap_dialog_done(void* userdata, const char* const* filelist, const int filter) {
    (void)userdata;
    (void)filter;
    SDL_Event event;
    SDL_zero(event);
    event.type = SNAP_EVENT_TYPE;
    if (filelist == nullptr) {
        msg_printf(MSG_ERROR, "File dialog failed: %s\n", SDL_GetError());
    } else if (filelist[0] != nullptr) {
        event.user.data1 = SDL_strdup(filelist[0]);
    }
    if (!SDL_PushEvent(&event)) {
        SDL_free(event.user.data1);
    }
}

static void snap_dialog(struct elwro* e) {
    char* cwd = nullptr;
    const char* start = e->snap_path[0] != 0 ? e->snap_path : (cwd = SDL_GetCurrentDirectory());
    SDL_ShowOpenFileDialog(snap_dialog_done, nullptr, e->scn->window,
        snap_filters, SDL_arraysize(snap_filters), start, false);
    SDL_free(cwd);
}

/* wczytanie snapshotu (okno wyboru pliku lub upuszczony plik): zimny restart + komunikaty */
static void snap_open(struct elwro* e, const char* path) {
    char status[128];
    const int error = elwro_restart_snap(e, path);
    disk_status(e, "Floppy ", status, sizeof(status));
    screen_toast_next(e->scn, status);
    elwro_snap_announce(e, path, error);
}

static bool path_has_ext(const char* path, const char* ext) {
    const char* dot = strrchr(path, '.');
    return dot != nullptr && SDL_strcasecmp(dot + 1, ext) == 0;
}

static bool path_is_snapshot(const char* path) {
    const char* dot = strrchr(path, '.');
    if (dot == nullptr) {
        return false;
    }
    const size_t ext_len = strlen(dot + 1);
    for (const char* p = SNAP_EXTS; *p != 0;) {
        const size_t len = strcspn(p, ";");
        if (len == ext_len && SDL_strncasecmp(dot + 1, p, len) == 0) {
            return true;
        }
        p += len + (p[len] == ';' ? 1 : 0);
    }
    return false;
}

/* napęd pierwszego pliku *.dsk bieżącego upuszczenia (-1 przed pierwszym, zerowane w SDL_EVENT_DROP_BEGIN) */
static int first_drop_drive = -1;

/* upuszczony plik: *.dsk do napędu A (lewa połowa okna) lub B (prawa), drugi plik *.dsk z tego samego upuszczenia do
 * drugiego napędu; snapshot jest wczytywany; reszta to komunikat o nieobsługiwanym pliku */
static void drop_file(struct elwro* e, const SDL_DropEvent* drop) {
    const char* path = drop->data;
    if (path == nullptr) {
        return;
    }
    screen_help(e->scn, false);
    if (path_has_ext(path, "dsk")) {
        int w = 0, h = 0;
        SDL_GetWindowSize(e->scn->window, &w, &h);
        const int drive = first_drop_drive >= 0 ? 1 - first_drop_drive : (w > 0 && drop->x >= (float)w / 2.0f ? 1 : 0);
        first_drop_drive = drive;
        disk_insert(e, drive, path);
    } else if (path_is_snapshot(path)) {
        snap_open(e, path);
    } else {
        screen_toast(e->scn, "Unsupported file: %s", disk_basename(path));
    }
}

/* pierwsza wolna nazwa e800j_NNN.<ext> w katalogu bieżącym (przy 1000 zajętych ostatnia, nadpisywana) */
static void next_free_name(char* name, const size_t size, const char* ext) {
    for (int n = 0; n < 1000; n++) {
        snprintf(name, size, "e800j_%03d.%s", n, ext);
        FILE* f = fopen(name, "rb");
        if (f == nullptr) {
            return;
        }
        fclose(f);
    }
}

int zx_main(struct elwro* e) {
    e->tcpu = false;
    e->t1 = SDL_AddTimer(e->cpu_refresh, zx_cpu_refresh, e);
    if (e->t1 == 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Could not create timer for cpu: %s\n", SDL_GetError());
        return -1;
    }
    on_exit(zx_free_cpu_refresh, e);
    const float refresh = 1000.f / (float)e->cpu_refresh;
    msg_printf(MSG_MSG, "CPU refresh: %ims (%iHz)\n", e->cpu_refresh, (int)refresh);
    msg_printf(MSG_MSG, "CPU ticks/refresh: %i\n", e->tr);
    msg_printf(MSG_MSG, "CPU interrupt: %i ticks\n", e->ifreq);
    const float frequency = refresh * (float)e->tr;
    msg_printf(MSG_MSG, "CPU frequency: %fMHz\n", frequency / 1000000.0);

    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        if (disk_event(e, &event)) {
            continue;
        }
        if (event.type == SNAP_EVENT_TYPE) {
            char* path = event.user.data1;
            if (path != nullptr) {
                snap_open(e, path);
            }
            SDL_free(path);
            continue;
        }
        switch (event.type) {
        case SDL_EVENT_DROP_BEGIN:
            first_drop_drive = -1;
            screen_toast(e->scn, "Drop *.dsk: left half = drive A, right half = drive B; snapshot = load");
            break;
        case SDL_EVENT_DROP_FILE:
            drop_file(e, &event.drop);
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_ESCAPE && e->scn->help) {
                screen_help(e->scn, false);
            } else if (event.key.key == SDLK_F1 && (event.key.mod & HK_CTRL_MOD) == 0) {
                if (!event.key.repeat) {
                    char status[128];
                    screen_help(e->scn, true);
                    disk_status(e, "Floppy ", status, sizeof(status));
                    screen_toast(e->scn, "%s", status);
                    e->scn->toast_sticky = e->scn->toast_tex != 0;
                }
            } else if ((event.key.mod & HK_CTRL_MOD) != 0 && event.key.key >= SDLK_F1 && event.key.key <= SDLK_F5) {
                static const float scales[] = { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
                if (!event.key.repeat) {
                    screen_help(e->scn, false);
                    screen_set_scale(e->scn, scales[event.key.key - SDLK_F1]);
                }
            } else if (event.key.key == SDLK_F11) {
                /* debugger w dowolnej chwili; okno emulatora czeka na polecenia z konsoli */
                screen_help(e->scn, false);
                if (!event.key.repeat) {
                    dbg_break(e->cpu->ptx);
                }
            } else if (event.key.key == SDLK_F8) {
                /* zrzut ekranu do pierwszego wolnego pliku e800j_NNN.png; Ctrl: sam obraz bez ramki */
                screen_help(e->scn, false);
                if (!event.key.repeat) {
                    char name[32];
                    next_free_name(name, sizeof(name), "png");
                    if (screen_screenshot(e->scn, name, (event.key.mod & HK_CTRL_MOD) == 0)) {
                        screen_toast(e->scn, "Screenshot saved: %s", name);
                    } else {
                        screen_toast(e->scn, "Screenshot failed: %s", name);
                    }
                }
            } else if (event.key.key == SDLK_F9) {
                screen_help(e->scn, false);
                if (!event.key.repeat) {
                    screen_fullscreen_toggle(e->scn);
                }
            } else if (event.key.key == SDLK_PAUSE) {
                screen_help(e->scn, false);
                if (!event.key.repeat) {
                    screen_pause(e->scn, !e->scn->paused);
                }
            } else if (disk_hotkey(e, event.key.key, event.key.mod)) {
                /* wymiana dyskietki */
                screen_help(e->scn, false);
            } else if ((event.key.key == SDLK_F2 || event.key.key == SDLK_F3) && e->snd != nullptr) {
                /* głośność; przytrzymanie klawisza powtarza zmianę */
                screen_help(e->scn, false);
                if (e->snd->nosound) {
                    if (!event.key.repeat) {
                        screen_toast(e->scn, "Sound is disabled");
                    }
                } else {
                    const int level = snd_volume_step(e->snd, event.key.key == SDLK_F3 ? 1 : -1);
                    screen_volume(e->scn, level, SND_VOL_LEVELS);
                }
            } else if (event.key.key == SDLK_F12 && (event.key.mod & HK_WIN_MOD) != 0) {
                screen_help(e->scn, false);
                if (!event.key.repeat) {
                    snap_dialog(e);
                }
            } else if (event.key.key == SDLK_F12 && (event.key.mod & (HK_CTRL_MOD | SDL_KMOD_SHIFT | HK_ALT_MOD)) != 0) {
                screen_help(e->scn, false);
                if (!event.key.repeat) {
                    char status[128];
                    const bool with_snap = (event.key.mod & HK_ALT_MOD) != 0;
                    int error = 0;
                    if (with_snap) {
                        error = elwro_snap_restart(e);
                    } else if ((event.key.mod & HK_CTRL_MOD) != 0) {
                        elwro_cold_reset(e);
                    } else {
                        elwro_reset(e);
                    }
                    disk_status(e, "Floppy ", status, sizeof(status));
                    screen_toast_next(e->scn, status);
                    if (with_snap)
                        elwro_snap_announce(e, e->snap_path, error);
                }
            } else if (event.key.key == SDLK_F12) {
                screen_help(e->scn, false);
                /* zapis snapshotu do pierwszego wolnego pliku e800j_NNN.sna */
                if (!event.key.repeat) {
                    char name[32];
                    next_free_name(name, sizeof(name), "sna");
                    if (file_snap_save(name, e) == 0) {
                        screen_toast(e->scn, "Snapshot saved: %s", name);
                    } else {
                        screen_toast(e->scn, "Snapshot failed: %s", name);
                    }
                }
            } else if (!event.key.repeat) {
                /* dowolny inny klawisz zamyka też ekran pomocy */
                screen_help(e->scn, false);
                zx_kbrd(event.key.key, 1, e);
            }
            break;
        case SDL_EVENT_KEY_UP:
            zx_kbrd(event.key.key, 0, e);
            break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            e->scn->width = event.window.data1;
            e->scn->height = event.window.data2;
            screen_layout(e->scn);
            break;
        case SDL_EVENT_QUIT:
            return 0;
        case SDL_EVENT_USER:
            if (SDL_GetAtomicInt(&e->scn->refresh)) {
                screen_refresh(e->scn);
                SDL_SetAtomicInt(&e->scn->refresh, 0);
            }
            /* wstrzymana emulacja nie wywołuje CPU, dźwięku i FDC (ekran jest dalej odświeżany) */
            if (!e->scn->paused) {
                if (e->tcpu) {
                    z80_loop(e->cpu, e->tr, e->ifreq, time_int, e);
                    snd_flush(e->snd);
                }
                fdc_clk(e->fdc);
            }
            e->tcpu = false;
            break;
        default:
            break;
        }
    }
    return 0;
}
