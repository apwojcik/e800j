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
#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#include <math.h>
#include "e800j_sound.h"
#include "e800j.h"

void snd_free_buffers(sound *w)
{
    if(w->nosound) return;
    free(((sound*)w)->lbuffer); free(((sound*)w)->rbuffer);
}

void snd_init_rbuffer(sound *w, int lbsize)
{
    w->head = w->tail = 0;
    if(!(w->lbuffer = (unsigned char*)malloc(lbsize))){
        printf("(EE) Error memory allocation i sound buffer init\n");
        exit(0);
    }
    if(!(w->rbuffer = (unsigned char*)malloc(w->size))){
        printf("(EE) Error memory allocation i sound buffer init\n");
        free(w->lbuffer);
        exit(0);
    }
    w->ovr=0;
    w->old_ticks=0;
    w->old_r373=0;
}

/* kopiuje len bajtów z bufora rbuffer i zwraca ilość skopiowanych bajtów */
char snd_rbuffer_pop(sound *w)
{
    char a;
    if(w->tail == w->head) return w->rbuffer[w->tail];
    a = w->rbuffer[w->tail++];
    if(w->tail == w->size ) w->tail = 0;
    return a;
}

void snd_rbuffer_push(sound *w, char data)
{
    /* detekcja przepełnienia bufora */
    if( ((w->head == w->size) && (w->tail == 0)) || ( w->head + 1 == w->tail )){
        if(!w->ovr) printf("(WW)Sound buffer(0x%x) overflow.\n", w->size);
        w->ovr=1;
        return;
    }
    w->rbuffer[w->head] = data;      /* wkładam daną do bufora    */
    if(w->head < w->size) w->head++; /* jeśli nie doszło do końca bufora, to przesuń wskaźnik head */
    else w->head = 0;            /* inaczej zawiń na początek */
}

void snd_beeper_callback(void *ud, Uint8 *stream, int len)
{
    int i;
    /* kopiowanie len bajtów z bufora kołowego do liniowego */
    for(i=0; i < len; i++) ((sound*)ud)->lbuffer[i] = snd_rbuffer_pop(ud);
    /* skopiowanie bufora liniowego do bufora karty */
    SDL_MixAudio(stream, ((sound*)ud)->lbuffer, len, ((sound*)ud)->volume);
}

void snd_init(void *elw)
{
    SDL_AudioSpec spec, spc;
    char name[32];
    sound *ww = ((elwro *)elw)->snd;

    if(ww->nosound) return;
    printf("(II)Sound init\n");

    spec.freq     = ww->freq;	/* ilość sampli/sec */
    spec.format   = AUDIO_U8;	/* format danych    */
    spec.channels = ww->chn;	/* ilość kanałów    */
    spec.samples  = ww->len;	/* rozmiar bufora w samplach */
    spec.callback = snd_beeper_callback;
    spec.userdata = ww; /* bufor danych */

    if(SDL_OpenAudio(&spec, &spc) < 0){
        printf("(WW)Couldn't open audio: %s\n",SDL_GetError());
        return;
    }

    printf("(II)Using audio driver: %s\n", SDL_AudioDriverName(name,32));
    atexit(SDL_CloseAudio);
    snd_init_rbuffer(ww, spc.samples);
    SDL_PauseAudio(0);
}

void snd_beeper(char r373, z80 *z)
{
    sound *ww = ((elwro *)(z->ptr))->snd;
    int i, smpls;
    double period, data;

    if(ww->nosound) return;
    if(!((r373 ^ ww->old_r373) & 0x10)) return; /* jeśli nie nastąpiła zmiana bitu 4 portu wyjdź */
    ww->old_r373 = r373;
    period = (z->ticks - ww->old_ticks) / (float)ww->scale; /* ile upłynęło przedziałów 20ms od zmiany */
    smpls = period * 256;
    ww->old_ticks = z->ticks;

    r373 &= 0x10;
    if(ww->alias){
        data = r373 ? 127: -128;
        period = 3.14/smpls;
        for(i=0; i < smpls; i++)
            snd_rbuffer_push(ww, 128 + data * sin(i*period));
        } else{
        data = r373 ? 0:0xff;
        for(i=0; i < smpls; i++) snd_rbuffer_push(ww, data );
    }
}

