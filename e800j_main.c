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
#include "i8272.h"

//int init_irq = 5000;

byte int_str[5]={0xff,0xff,0xff,0xff,0xff};

void int_irq(){
    z80_send_int(NULL,int_str);
}

int e800j_screen_rfsh(e800j *e)
{
    scr_rfsh(e);
    return 1;
}

int int_tic(e800j *e)
{
/* os³uga seriala */
    if(e->wr51){
	e->wr51 = 0;
//printf("8251 TxD: %x\n",i8251_txd(E800J(z->cust)->i8251p));
    }
/* normalne wywo³anie przerwania co 20ms */
//    if(!init_irq)
	    int_irq();
    return 1;
}

void fdc_ck(){ int i; for(i=0;i<4;i++) i8272_ck(); }

int cpu()
{
//    if(init_irq){
//	z80_exec(NULL,int_irq,NULL);
//	init_irq--;
//	return 1;
//    }
    z80_exec(NULL,int_irq,fdc_ck);
//    z80_exec(NULL,NULL,NULL);
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
/* Reset IO */
    i8272_rst();
}

void run_cpu()
{
    static char pause = 0;
    
    if(pause){
	z80_set_run(NULL,1);
	z80_set_debug(z80_get_struct(),0);	
    } else {
	z80_set_run(NULL,0);
	z80_set_debug(z80_get_struct(),0);	
    }
    pause = !pause;
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

void  main_clocks_set(e800j *e8)
{
    ((Z80 *)e8->cpu)->nmi_cnt = e8->config->fdc_rfsh;   /* FDC freq */  
    ((Z80 *)e8->cpu)->int_cnt = e8->config->int_rfsh;  /* INT freq */  
    gui_timer(int_tic, e8->config->int_tick, e8);
    gui_timer(e800j_screen_rfsh, e8->config->scr_rfsh, e8); /* czas odswierzania ekranu */
    gui_timer(blink_attr, e8->config->blink_rfsh, e8->cpu);
    gui_timer(cpu, e8->config->cpu_rfsh, e8->cpu);
}

void main_e8struct_set(e800j *e8, Z80 *z80, e8_config *e8_cfg)
{
    e8->cpu    = z80;
    e8->config = e8_cfg;
}

void main_config_set_default(e800j *e8)
{   
    e8->config->wwidth = 640;
    e8->config->wheight = 480;
    e8->config->fwidth = 320;
    e8->config->fheight = 200;
    e8->config->bpp = 8;
    e8->config->use_sdl = 1;    
    e8->config->use_gtk = 1;
    e8->config->bpalette = 1;
    e8->config->cpu_rfsh = 15;
    e8->config->cpu_tick = 5000;
    e8->config->int_rfsh = 1000;
    e8->config->int_tick = 20;
    e8->config->fdc_rfsh = 100;    
    e8->config->scr_rfsh = 30;        
    e8->config->blink_rfsh = 500;        
    e8->config->disk_version = 1;
    strcpy(e8->config->bas0_fname,"./rom/bas01.bin");
    strcpy(e8->config->bas1_fname,"./rom/bas11.bin");    
    strcpy(e8->config->boot_fname,"./rom/boot01.bin");
}    

int main(int argc, char **argv)
{
    Z80		z80;
    Z80_DEB	debugger;
    i8251	serial;
    e8_config   e8_conf;
//    i8255	parallel;
    e800j	e8;    
    byte	ram[65536];
    byte	rom[81292 * 3];
    byte	prom_mem[0x200] = PROM_MEM;
    byte	prom_jio[0x200] = PROM_IO;
    byte	eprom_tv[0x800] = EPROM_TV;

    main_e8struct_set(&e8,&z80,&e8_conf);    
    main_config_set_default(&e8);
//    file_load_config(&e8,CONFIG_FNAME);
    e800j_init_mem(&e8, ram, rom, prom_mem, prom_jio, eprom_tv);
    e800j_init_io(&e8, &serial);

    z80_init(e8.cpu, &e8, e8.config->cpu_tick, io, mem_rd, mem_wr);
    z80_debugger_init(e8.cpu, &debugger, gui_debug_in, gui_debug_out, kill_me);
    key_init(KB_MODE_PC);
    i8272_ini(NULL,NULL);

    reset();
    z80_set_run(NULL,1);
    gui_setup(&e8, &argc, &argv);
    main_clocks_set(&e8);
    gui_start();

    return 0;
}

