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
#include <SDL3/SDL.h>
#include <math.h>
#include <string.h>
#include "e800j_sound.h"
#include "e800j.h"
#include "z80_cpu.h"

#define SND_CHUNK 1024 /* maksymalny rozmiar porcji danych przekazywanej do SDL */

static SDL_AudioStream* snd_stream;

static void snd_close_audio(void) {
    SDL_DestroyAudioStream(snd_stream);
}

void snd_free_buffers(struct sound* w) {
    if (w->nosound)
        return;
    free(w->lbuffer);
    free(w->rbuffer);
}

static void snd_init_rbuffer(struct sound* w, int lbsize) {
    w->head = w->tail = 0;
    if (!(w->lbuffer = (unsigned char*)malloc(lbsize))) {
        printf("(EE) Error memory allocation i sound buffer init\n");
        exit(0);
    }
    if (!(w->rbuffer = (unsigned char*)malloc(w->size))) {
        printf("(EE) Error memory allocation i sound buffer init\n");
        free(w->lbuffer);
        exit(0);
    }
    w->ovr = false;
    w->old_ticks = 0;
    w->old_r373 = 0;
}

/* kopiuje len bajtów z bufora rbuffer i zwraca ilość skopiowanych bajtów */
static char snd_rbuffer_pop(struct sound* w) {
    if (w->tail == w->head)
        return w->rbuffer[w->tail];
    const char a = w->rbuffer[w->tail++];
    if (w->tail == w->size)
        w->tail = 0;
    return a;
}

static void snd_rbuffer_push(struct sound* w, char data) {
    /* detekcja przepełnienia bufora */
    if (((w->head == w->size) && (w->tail == 0)) || (w->head + 1 == w->tail)) {
        if (!w->ovr)
            printf("(WW)Sound buffer(0x%x) overflow.\n", w->size);
        w->ovr = true;
        return;
    }
    w->rbuffer[w->head] = data; /* wkładam daną do bufora    */
    if (w->head < w->size)
        w->head++; /* jeśli nie doszło do końca bufora, to przesuń wskaźnik head */
    else
        w->head = 0; /* inaczej zawiń na początek */
}

void snd_beeper_callback(void* ud, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    (void) total_amount;
    struct sound* w = (struct sound*)ud;
    Uint8 mix[SND_CHUNK];

    while (additional_amount > 0) {
        const int n = additional_amount < SND_CHUNK ? additional_amount : SND_CHUNK;
        /* kopiowanie n bajtów z bufora kołowego do liniowego */
        for (int i = 0; i < n; i++)
            w->lbuffer[i] = snd_rbuffer_pop(w);
        /* zmiksowanie ciszy z buforem liniowym i przekazanie do karty */
        memset(mix, 0x80, n); /* cisza dla AUDIO_U8 */
        SDL_MixAudio(mix, w->lbuffer, SDL_AUDIO_U8, n, w->volume / 128.0f /* dawne SDL_MIX_MAXVOLUME */);
        SDL_PutAudioStreamData(stream, mix, n);
        additional_amount -= n;
    }
}

void snd_init(struct sound* snd, const struct cpu* cpu) {
    SDL_AudioSpec spec;
    char samples[16];

    if (snd->nosound)
        return;
    printf("(II)Sound init\n");
    snd->cpu = cpu;

    spec.freq = snd->freq; // ilość sampli/sec
    spec.format = SDL_AUDIO_U8; /* format danych    */
    spec.channels = snd->chn; // ilość kanałów

    snprintf(samples, sizeof(samples), "%d", snd->length);
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, samples); /* rozmiar bufora w samplach */

    if (!(snd_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, snd_beeper_callback, snd))) {
        printf("(WW)Couldn't open audio: %s\n", SDL_GetError());
        snd->nosound = true;
        return;
    }

    printf("(II)Using audio driver: %s\n", SDL_GetCurrentAudioDriver());
    atexit(snd_close_audio);
    snd_init_rbuffer(snd, SND_CHUNK);
    SDL_ResumeAudioStreamDevice(snd_stream);
}

void snd_beeper(struct sound* snd, char r373) {
    if (snd->nosound)
        return;
    if (!((r373 ^ snd->old_r373) & 0x10))
        return; /* jesli nie nastąpiła zmiana bitu 4 portu wyjdą */
    snd->old_r373 = r373;
    const double ticks = (double)(snd->cpu->ticks - snd->old_ticks);
    double period = ticks / snd->scale; /* ile upłynęło przedziałów 20ms od zmiany */
    const int smpls = (int)(period * 256);
    snd->old_ticks = snd->cpu->ticks;

    r373 &= 0x10;
    if (snd->alias) {
        const double data = r373 ? 127 : -128;
        period = 3.14 / smpls;
        for (int i = 0; i < smpls; i++)
            snd_rbuffer_push(snd, (char)(128 + data * sin(i * period)));
    }
    else {
        const double data = r373 ? 0 : 0xff;
        for (int i = 0; i < smpls; i++)
            snd_rbuffer_push(snd, (char)data);
    }
}
