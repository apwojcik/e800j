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

typedef struct 
{
    unsigned char *rbuffer;	/* bufor ko³owy d¼wiêku */
    unsigned char *lbuffer;	/* bufor liniowy        */
    char *sin;		/* tablica sinus do antyaliasingu   */
    
    int  head;		/* poczatek bufora ko³owego  */
    int  tail;		/* koniec bufora ko³owego    */
    int  volume;	/* g³o¶no¶æ               */
    int  size;		/* rozmiar bufora         */
    int  freq;		/* ilo¶æ sampli / sekunde */
    int  len;		/* d³ugo¶æ bufora na karcie, warto¶æ musi byæ pot 2 i mo¿e byæ skorygowana */
    char chn;           /* ilo¶æ kana³ów */
    char nosound:1;     /* czy w³± obs³ d¼wiêku */
    char alias:1;	/* czy ma byæ w³aczony antyaliasing */
    char ovr:1;		/* wyst±pi³o przepe³nienie bufora   */
    int  scale;		/* skalowanie d³ugo¶ci okresów      */
    long old_ticks;
    char old_r373;
} sound;

extern void snd_beeper(char r373, z80 *z);
extern void snd_beeper_play();
extern void snd_init( void *elw);
extern void snd_free_buffers(sound *);

#endif

