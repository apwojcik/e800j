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

#pragma once

#include "e800j.h"

struct sound {
    unsigned char* rbuffer; // bufor kołowy dźwięku
    unsigned char* lbuffer; // bufor liniowy
    char* sin; // tablica sinus do antyaliasingu

    int head; /* początek bufora kołowego  */
    int tail; // koniec bufora kołowego
    int volume; // głośność
    int size; // rozmiar bufora
    int freq; /* ilość sampli / sekundę */
    int length; // długość bufora na karcie, wartość musi być pot 2 i może być skorygowana
    int chn; // ilość kanałów
    unsigned char nosound : 1; // czy włączyć obsługę dźwięku
    unsigned char alias : 1; // czy ma być włączony antyaliasing
    unsigned char ovr : 1; // wystąpiło przepełnienie bufora
    int scale; // skalowanie długości okresów
    long old_ticks;
    unsigned char old_r373;
    const struct cpu* cpu;
};

void snd_beeper(struct sound* snd, char r373);
void snd_beeper_play(struct sound* snd);
void snd_init(struct sound* snd, const struct cpu* cpu);
void snd_free_buffers(struct sound* snd);
