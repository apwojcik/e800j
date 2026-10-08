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
    void	*e8;	
/* rozmiary okna */
    int 	width;
    int		height;
    int         scr_offs;
    int		bpp;
    void        *argc;
    void        *argv;
    char	use_sdl:1;
    char	use_gtk:1;
    char	use_palette_border:6;
/* pola gtk */    
    GdkColor	*gtk_8bpp_palette;
    GtkWidget	*wmain;
    GtkWidget	*drawing_area;
    GdkPixmap 	*gdk_da_buffer;
    GdkGC	*gdk_color[17];
    GtkWidget   *gtk_parent;
    long	winid;
/* pola SDL */
#ifndef NO_SDL
    SDL_Surface	*sdl_surface;
    SDL_Color   *sdl_8bpp_palette;
    SDL_Event   *sdl_event;
#endif    
/* ekran */
    e8_screen	*scr;
/* wska¼nik do funkcji stawiaj±cej i skaluj±cej piksel */
    void	(*put_pixel)(void *, int, int, int); 
    void	(*scrn_rfsh)(void *); 
} e8_gui;

//void load_snapshot(GtkObject *wg);
//void load_file(GtkWidget *wg);
void ed_ed(GtkWidget *wg);
void debugger();
void gui_key_pressed(GtkWidget *wg,GdkEventKey *ev,char *state);
#endif

void gui_put_pixel(e800j *e8,int color,int x,int y);
void gui_screen_rfsh(e800j *e8);
void gui_debug_out(char *a);
void gui_debug_in(char *a);
void gui_setup(void *ptr, void *argc, void *argv);
void gui_timer(void *func,long time,void *ptr);
void gui_start();
void gui_gtk_msgbox(char *,const char *);
#endif
