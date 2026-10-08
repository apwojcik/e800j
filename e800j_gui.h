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

#ifndef __e800j_gui_h__
#define __e800j_gui_h__

#ifdef __GTK_H__

typedef struct 
{
    int zoom_x;
    int zoom_y;
    int vid_offs;
    int xx,yy;
    char flash;
    unsigned char *mem;
    GdkPixmap *pix;
    GdkGC  *gc[16];
    GtkWidget *draw_area;
    void *ptr;
} config;

void load_snapshot(GtkObject *wg);
int configure_dr(GtkWidget *wg, GdkEventExpose *ev, config *cf);
int expose_dr(GtkWidget *wg, GdkEventExpose *ev,config *cf);
void load_file(GtkWidget *wg);
void ed_ed(GtkWidget *wg);
void debugger();
void gui_key_pressed(GtkWidget *wg,GdkEventKey *ev,char *state);
#endif

void gui_put_pixel(int color,int x,int y);
void gui_screen_rfsh();
void gui_debug_out(char *a);
void gui_debug_in(char *a);
void gui_setup(void *ptr);
void gui_timer(void *func,long time,void *ptr);
void gui_start();

#endif
