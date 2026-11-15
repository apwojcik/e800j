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

#include "e800j.h"

#define SND_CHUNK 512 /* ilość ramek renderowanych jednorazowo przed przekazaniem do SDL */

struct SDL_AudioStream;

struct sound {
    struct SDL_AudioStream* stream; // strumień audio SDL (bezpieczny wątkowo)

    float volume; // głośność (0..128)
    int freq; /* ilość sampli / sekundę */
    int length; // rozmiar bufora urządzenia w ramkach (podpowiedź dla SDL)
    int chn; // ilość kanałów
    unsigned char nosound : 1; // czy włączyć obsługę dźwięku
    unsigned char alias : 1; // czy uśredniać sygnał prostokątny w obrębie próbki (anty-aliasing)

    /* stan renderowania; czas liczony w taktach CPU */
    double cpu_hz; // częstotliwość taktowania CPU
    double step; // ilość taktów CPU na jedną próbkę (z korektą tempa)
    double step_nom; // nominalna ilość taktów CPU na próbkę
    int target_frames; // docelowa ilość zakolejkowanych ramek
    double avg_q; // wygładzona ilość zakolejkowanych ramek
    double pos; // takty zużyte w bieżącej próbce
    double acc; // całka poziomu sygnału w bieżącej próbce
    long last_ticks; // takt, do którego wyrenderowano dźwięk
    float level; // aktualny poziom wyjścia (-1..1)
    float amp; // wzmocnienie wynikające z volume
    float dc_x, dc_y; // stan filtru górnoprzepustowego (usuwa składową stałą)
    int max_queue; // maksymalna ilość bajtów w kolejce SDL (opóźnienie)
    int nout; // ilość ramek w out
    short out[SND_CHUNK * 2];
    const struct cpu* cpu;
};

void snd_beeper(struct sound* snd, unsigned char port_fe);
void snd_flush(struct sound* snd);
void snd_init(struct sound* snd, const struct cpu* cpu, double cpu_hz);
void snd_free_buffers(struct sound* snd);
