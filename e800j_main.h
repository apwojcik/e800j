/* Elwro 800 Junior emulator
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

#ifndef __e800j_main_h__
#define __e800j_main_h__
#include "z80.h"

#define DEBUG_HW_MSG

//#define ZX
//#define JR_1
#define JR_2
//#define JR_3

#ifdef ZX
#define ROM_NAME "./rom/zx_spec.bin",NULL,NULL
#endif
#ifdef JR_1
#define ROM_NAME "./rom/bas01.bin","./rom/bas11.bin","./rom/boot01.bin"
#endif
#ifdef JR_2
#define ROM_NAME "./rom/bas02.bin","./rom/bas12.bin","./rom/boot02.bin"
#endif
#ifdef JR_3
#define ROM_NAME "./rom/bas03.bin","./rom/bas13.bin","./rom/boot03.bin"
#endif


void scr_rfsh();
int vid_addr( int pix_nb);
int rfsh(Z80 *z);
int blink_attr(Z80 *z);
void reg_mem_74157(byte data, e800j *z);
unsigned char io(word addr,byte data, byte type, Z80 *z);
void mem_wr(Z80 *z, word addr, byte data);
byte mem_rd(Z80 *z, word addr);
void reset(void);
void run_cpu();
void kill_me();
void scr_rfsh(Z80 *z);
void e800j_str_init(e800j *e8, byte *ram, byte *rom, byte *prom_mem,byte *prom_io,byte *prom_tv);

#endif

