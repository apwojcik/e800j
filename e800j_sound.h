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

#ifndef __E800_SOUND_H__
#define __E800_SOUND_H__
#include "z80_cpu.h"

typedef struct {
    unsigned char* rbuffer; /* bufor kołowy dźwięku */
    unsigned char* lbuffer; /* bufor liniowy        */
    char* sin; /* tablica sinus do antyaliasingu   */

    int  head; /* początek bufora kołowego  */
    int tail; /* koniec bufora kołowego    */
    int volume; /* głośność               */
    int size; /* rozmiar bufora         */
    int  freq; /* ilość sampli / sekundę */
    int len; /* długość bufora na karcie, wartość musi być pot 2 i może być skorygowana */
    char chn; /* ilość kanałów */
    char nosound : 1; /* czy włą obsł dźwięku */
    char alias:1; /* czy ma być włączony antyaliasing */
    char ovr : 1; /* wystąpiło przepełnienie bufora   */
    int scale; /* skalowanie długości okresów      */
    long old_ticks;
    char old_r373;
} sound;

extern void snd_beeper(char r373, z80* z);

extern void snd_beeper_play();

extern void snd_init(void* elw);

extern void snd_free_buffers(sound*);

#endif
