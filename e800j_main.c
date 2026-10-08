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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "z80.h"
#include "e800j.h"
#include "e800j_kbrd.h"
#include "e800j_prom.h"
#include "e800j_file.h"
#include "e800j_gui.h"
#include "e800j_main.h"
#include "e800j_mem.h"
#include "e800j_io.h"
#include "i8251.h"
#include "z80_debug.h"

byte int_str[5]={0xff,0xff,0xff,0xff,0xff};

void int_irq(){
    z80_send_int(NULL,int_str);
}

int rfsh(Z80 *z)
{

    scr_rfsh(z);

/* os³uga seriala */
    if(E800J(z->cust)->wr51){
	E800J(z->cust)->wr51 = 0;
printf("8251 TxD: %x\n",i8251_txd(E800J(z->cust)->i8251p));
    }
    int_irq();
    return 1;
}

int cpu()
{
    z80_exec(NULL,NULL,NULL);
    return 1;
}

void reset()
{
    int i;
    e800j *e8 = (e800j *)z80_get_struct()->cust;

    for(i=0; i < 65536; i++) e8->ram[i] = random();

    load_rom(e8, ROM_NAME);
    
    e8->reg175 = 0;
    e8->addr_prom_mask=0;
    z80_reset(NULL);    
    z80_set_run(NULL,1);
}

void run_cpu()
{
    z80_set_run(NULL,1);
    z80_set_debug(z80_get_struct(),0);
}

void kill_me(){ exit(0); }

void e800j_init_mem(e800j *e8, byte *ram, byte *rom, byte *prom_mem,byte *prom_io,byte *prom_tv)
{
    e8->ram = ram;
    e8->prom_mem = prom_mem;
    e8->prom_jio = prom_io;    
    e8->eprom_tv = prom_tv;
    e8->rom_bas0 = rom;
    e8->rom_bas1 = rom + 0x2000;
    e8->rom_boot = rom + 0x4000;
    e8->scr.blink = 0;    
    e8->scr.screen[0] = ram + 0x4000;
    e8->scr.screen[1] = ram + 0xe000;
}

void e800j_init_io(e800j *e8, i8251 *serial)
{
    e8->i8251p = serial;
}


int main(int argc, char **argv)
{
    Z80		z80;
    Z80_DEB	debugger;
    i8251	serial;
//    i8255	parallel;
    e800j	e8;    
    byte	ram[65536];
    byte	rom[81292 * 3];
    byte	prom_mem[0x200] = PROM_MEM;
    byte	prom_jio[0x200] = PROM_IO;
    byte	eprom_tv[0x800] = EPROM_TV;


    e800j_init_mem(&e8, ram, rom, prom_mem, prom_jio, eprom_tv);
    e800j_init_io(&e8, &serial);

    z80_init(&z80, &e8, 10000, io, mem_rd, mem_wr);
    z80_debugger_init(&z80, &debugger, gui_debug_in, gui_debug_out, kill_me);

    key_init(KB_MODE_PC);
    i8251_reset(&serial);

    reset();
    
    z80_set_run(NULL,1);

    gui_setup(&z80);
    gui_timer(rfsh, 20, &z80);
    gui_timer(blink_attr, 500, &z80);
    gui_timer(cpu, 15, &z80);
    gui_start();

    return 0;
}

