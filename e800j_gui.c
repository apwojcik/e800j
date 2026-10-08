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
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#ifndef NO_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#endif

#include "e800j_kbrd.h"
#include "e800j_file.h"
#include "e800j_main.h"
#include "z80_debug.h"
#include "e800j_scrn.h"
#include "e800j_gui.h"
#include "e800j_debug.h"

e8_gui	__gui;
e8_screen screen_str;

/* paleta kolorów dla trybu 8bpp */
static GdkColor gtk_8bpp_palette_tab[17] = ZX_PALETTE;
#ifndef NO_SDL
static SDL_Color sdl_8bpp_palette_tab[17] = SDL_ZX_PALETTE;
#endif

GtkTextBuffer *txt;
GtkTextView   *txtv;

char cmd_buffer[1024];

#ifndef NOSDL
SDL_Event _sdl_event;
SDL_Surface _sdl_surface;

void zxscr_calc(e8_screen *scr);
void gui_sdl_setup(e8_gui*);
void gui_sdl_put_pixel(void *sc,int ,int ,int );
void gui_sdl_screen_rfsh(void *gui);
void gui_sdl_redraw(e8_gui *gui);
void gui_gtk_config(GtkWidget *, e8_gui *gui);
#endif

void gui_gtk_draw_border(e8_gui *gui);

char done = 0;

void gui_gtk_msgbox(char *format, const char *param)
{
    GtkWidget *wg, *tg;
    char temp[256];
    
    *temp = 0; sprintf(temp,format,param);
    
    wg = gtk_dialog_new();
    gtk_signal_connect(GTK_OBJECT(wg), "destroy",GTK_SIGNAL_FUNC(gtk_widget_destroy), wg);
    gtk_window_set_title(GTK_WINDOW(wg),"Komunikat");
    gtk_widget_show(wg);

    tg = gtk_button_new_with_label("OK");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(wg)->action_area),tg,TRUE,TRUE,0);
    gtk_signal_connect_object(GTK_OBJECT(tg), "clicked",GTK_SIGNAL_FUNC(gtk_widget_destroy), wg);

    tg = gtk_label_new(temp);
    gtk_misc_set_padding(GTK_MISC(tg),10,10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(wg)->vbox),tg,TRUE,TRUE,0);

    gtk_widget_show_all(wg);
}

/*******************************************************************************************
 *
 *	Wczytywanie "snapshotów" (*.z80,*.sna itp)
 *
 */

void gui_gtk_load_snapshot(GtkButton *wg, e8_gui *gui)
{

    FILE *f;
    const char *fname;
    char *msg;
    
    if(GTK_BUTTON(GTK_FILE_SELECTION(gui->gtk_parent)->ok_button)->in_button){
    
	fname  = gtk_file_selection_get_filename(GTK_FILE_SELECTION(gui->gtk_parent));

	if(!(f=fopen(fname,"r")))
	{
	    gui_gtk_msgbox("Nie mozna otworzyc pliku %s",fname);
	    return;
	}
    
	if(load_z80(((e800j *)gui->e8),f,fname,&msg)) gui_gtk_msgbox(msg,"");
	
	fclose(f);        
    }
    gtk_widget_destroy(gui->gtk_parent);
}

void gui_gtk_load_fileselector(GtkWidget *wg, e8_gui *gui)
{
    GtkWidget *fs;

    z80_set_run(NULL,0); /* zatrzymaj emulacjê CPU */
    fs=gtk_file_selection_new("Wczytaj z80");
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fs));
    gtk_widget_show(fs);
    gui->gtk_parent = fs;    
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),"clicked",
		    				    GTK_SIGNAL_FUNC(gui_gtk_load_snapshot),gui);    
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),"clicked",
		    				    GTK_SIGNAL_FUNC(gui_gtk_load_snapshot),gui);    
}

/*******************************************************************************************
 *
 *	Funkcje odpowiedzialne za wy¶wietlanie na ekranie w trybie wyswietlania GDK
 *
 */

/* zrobiæ dynamiczne skalowanie dla GDK */
void gui_gtk_put_pixel(void *gui, int x, int y, int color)
{
    GdkGC *gc = ((e8_gui *)gui)->gdk_color[color];
    color &= 0x0f;

    int x_scaled = x*2 + ((e8_gui *)gui)->scr->fr_spcl;
    int y_scaled = y*2 + ((e8_gui *)gui)->scr->fr_spcd;

    gdk_draw_point(((e8_gui *)gui)->gdk_da_buffer, gc, x_scaled,     y_scaled);
    gdk_draw_point(((e8_gui *)gui)->gdk_da_buffer, gc, x_scaled + 1, y_scaled);
    gdk_draw_point(((e8_gui *)gui)->gdk_da_buffer, gc, x_scaled,     y_scaled + 1);
    gdk_draw_point(((e8_gui *)gui)->gdk_da_buffer, gc, x_scaled + 1, y_scaled + 1);

}

void gui_gtk_set_color_gc(e8_gui *gui, int i, int border)
{
    if(i != ZX_BORDER)
    {
	gui->gdk_color[i] = gdk_gc_new(gui->gdk_da_buffer);
	gdk_gc_set_foreground(gui->gdk_color[i], gui->gtk_8bpp_palette + i);
	return;
    } 

    gui->gdk_color[i] = gdk_gc_new(gui->gdk_da_buffer);
    gdk_gc_set_foreground(gui->gdk_color[i], gui->gtk_8bpp_palette + border);

}

void gui_gtk_draw_border(e8_gui *gui)
{

    gdk_draw_rectangle(gui->gdk_da_buffer, gui->gdk_color[gui->scr->frame_color],TRUE,
	0,0,gui->scr->area_w,gui->scr->fr_spcu);
    gdk_draw_rectangle(gui->gdk_da_buffer, gui->gdk_color[gui->scr->frame_color],TRUE,
	0,gui->scr->fr_spcu + gui->scr->scr_hs ,gui->scr->area_w,gui->scr->fr_spcd);
    gdk_draw_rectangle(gui->gdk_da_buffer, gui->gdk_color[gui->scr->frame_color],TRUE,
	0,gui->scr->fr_spcu,gui->scr->fr_spcl,gui->scr->fr_spcu + gui->scr->scr_hs );
    gdk_draw_rectangle(gui->gdk_da_buffer, gui->gdk_color[gui->scr->frame_color],TRUE,
	gui->scr->fr_spcl+gui->scr->scr_ws,gui->scr->fr_spcu,
	gui->scr->fr_spcl+gui->scr->scr_ws+gui->scr->fr_spcr,
	gui->scr->fr_spcu + gui->scr->scr_hs );
}

int gui_gtk_da_configure(GtkWidget *wg, GdkEventExpose *ev, e8_gui *gui)
{
    int i;
    if(gui->gdk_da_buffer)
	    gdk_pixmap_unref(gui->gdk_da_buffer);
    
    gui->scr->area_w = wg->allocation.width;
    gui->scr->area_h = wg->allocation.height;    
    gui->gdk_da_buffer = gdk_pixmap_new(wg->window,gui->scr->area_w,gui->scr->area_h,-1);

    for(i=0;i<17;i++) gui_gtk_set_color_gc(gui,i,ZX_YELLOW);
    
/* CLS - wype³nia obszar rysunkowy kolorem ramki */
    gdk_draw_rectangle(gui->gdk_da_buffer,gui->gdk_color[ZX_BORDER],TRUE,0,0,
		    			gui->scr->area_w,gui->scr->area_h);
    return TRUE;
}

/* przerysowanie zawarto¶ci bufora na ekran w trybie wy¶wietlania GDK */
int gui_gtk_da_expose(GtkWidget *wg, GdkEventExpose *ev,e8_gui *gui)
{
    gdk_draw_pixmap(wg->window,wg->style->fg_gc[GTK_WIDGET_STATE(wg)],gui->gdk_da_buffer,
	    ev->area.x,ev->area.y,
	    ev->area.x,ev->area.y,
	    ev->area.width,ev->area.height);
    return FALSE;
}

/* Wykonanie odswierzania, punkt wejscia dla ekranu GDK */
void gui_gtk_screen_rfsh(void *gui)
{
    GdkRectangle tmp;

    if(((e8_gui*)gui)->scr->frame_color != (*((e8_gui*)gui)->scr->new_frame_color & OUT_FE_BORDERMASK)){
	((e8_gui*)gui)->scr->frame_color = *((e8_gui*)gui)->scr->new_frame_color & OUT_FE_BORDERMASK;
    	gui_gtk_draw_border(gui);
    }
    tmp.x=tmp.y=0;
    tmp.width  = ((e8_gui*)gui)->drawing_area->allocation.width;
    tmp.height = ((e8_gui*)gui)->drawing_area->allocation.height;
    gtk_widget_draw(((e8_gui*)gui)->drawing_area, &tmp);
}

void gui_gtk_make_8bpp_palette(e8_gui *gui)
{
    int i;
    
    for(i=0; i < 17; i++) 
	gdk_color_alloc(gdk_colormap_get_system(), gui->gtk_8bpp_palette + i );
}

/*******************************************************************************************
 *
 *	Obs³uga klawiatury
 *
 */

void gui_gtk_key_pressed(GtkWidget *wg,GdkEventKey *ev,char *state)
{
    int key, key_m;
    char kb_state;
    
    kb_state = (*state == '1') ? TRUE:FALSE; /* wci¶niêto/wyci¶niêto */
    key      = ev->keyval;       /* kod klawisza w GTK   */
    if((key >='A')&&(key <='Z')) key = key - 'A' +'a';

    switch(key){
	case '~': key = '`'; break;
	case '!': key = '1'; break;
	case '@': key = '2'; break;
	case '#': key = '3'; break;
	case '$': key = '4'; break;
	case '%': key = '5'; break;
	case '^': key = '6'; break;
	case '&': key = '7'; break;
	case '*': key = '8'; break;
	case '(': key = '9'; break;
	case ')': key = '0'; break;
	case '_': key = '-'; break;
	case '+': key = '='; break;
	case ':': key = ';'; break;
	case '"': key = '\''; break;
	case '|': key = '\\'; break;
	case '<': key = ','; break;
	case '>': key = '.'; break;
	case '?': key = '/'; break;
	case '{': key = '['; break;
	case '}': key = ']'; break;
    }
/* pliterki */
    switch(ev->hardware_keycode){
	case 52 : key = 'z'; break;
	case 32 : key = 'o'; break;	
	case 46 : key = 'l'; break;
	case 54 : key = 'c'; break;
	case 26 : key = 'e'; break;
	case 39 : key = 's'; break;
	case 38 : key = 'a'; break;
	case 53 : key = 'x'; break;
	case 57 : key = 'n'; break;
    }

    switch(key){
	case GDK_Return   : key_m = KB_ENTER; break;
	case GDK_Shift_L  : 
	case GDK_Shift_R  : key_m = KB_SHIFT; break;	
	case GDK_Caps_Lock: key_m = KB_CAPS;  break;
	case ' '          : key_m = KB_SPACE; break;
	case GDK_Alt_L	  : key_m = KB_ALT_L; break;
	case 0xFE03	  : key_m = KB_ALT_R; break;
	case GDK_Control_L:
	case GDK_Control_R: key_m = KB_CTRL ; break;
	case GDK_Delete   : key_m = KB_DEL  ; break;
	case GDK_BackSpace: key_m = KB_BS   ; break;
	case GDK_F1	  : key_m = KB_CPJ  ; break;
	case GDK_F2	  : key_m = KB_DIR  ; break;
	case GDK_F3	  : key_m = KB_LOAD ; break;
	case GDK_F4	  : key_m = KB_SAVE ; break;
	case GDK_F5	  : key_m = KB_ERASE; break;
	case GDK_Left	  : key_m = KB_CUR_L; break;
	case GDK_Right	  : key_m = KB_CUR_R; break;
	case GDK_Up	  : key_m = KB_CUR_U; break;
	case GDK_Down	  : key_m = KB_CUR_D; break;

	case GDK_Escape	  : key_m = KB_ESC; break;
	case GDK_Tab	  : key_m = KB_TAB; break;
    
	default 	  : key_m = key;
    }
    key_update(key_m,kb_state);
}


/*******************************************************************************************
*
*
*  Funkcje ogólne, interface na zewn±trz biblioteki
*
*
*/
void gui_timer(void *func,long time,void *ptr)
					{ gtk_timeout_add(time, func, ptr); }
void gui_put_pixel(e800j *e8, int color,int x,int y)
					{ ((e8_gui *)e8->gui)->put_pixel(e8->gui, x, y, color); }
void gui_screen_rfsh(e800j *e8)
					{ ((e8_gui *)e8->gui)->scrn_rfsh(e8->gui); }

void gui_gtk_about(GtkWidget *wg, e8_gui *gui)
{
    gui_gtk_msgbox(
"Elwro 800 Junior Emulator v 0.0.1\n\n\
    Autor: Krzysztof Komarnicki\n\
            krzkomar@wp.pl\n\n\
        Licencja: GPL version 2\
","");

}

void gui_gtk_kill_me()
{
    exit(0);
}

void gui_setup_start(e8_gui *gui)
{
    char swinid[32];
    GtkWidget *vbox_wg, *menu, *elm, *submenu;

    gui->gtk_8bpp_palette = gtk_8bpp_palette_tab;
    gui->winid  = 0; 
    gui->wmain  = NULL;
    gui->scr    = &screen_str;
    gui->scr->new_frame_color = &((e800j*)gui->e8)->reg373;
    
#ifndef NO_SDL    
    gui->sdl_surface = NULL;
    gui->sdl_8bpp_palette = sdl_8bpp_palette_tab;    
    gui->sdl_event = &_sdl_event;
#endif
    gui->scr->area_w = gui->width;
    gui->scr->area_h = gui->height;
    gui->scr->zx_w = 256;
    gui->scr->zx_h = 192;
    gui->scr->bpp  = gui->bpp;
    gui->scr->scr_buffer = NULL;
    gui->gdk_da_buffer = NULL;
    gui->scr->frame_color = ZX_YELLOW; /* zainicjowany kolor ramki */

    /* wymu¶ aktualizacjê koloru ramki */
    gui->scr->frame_color = *gui->scr->new_frame_color + 1;
    zxscr_calc(gui->scr); /* przeliczenie parametrów ekranu */

    gtk_init(gui->argc, gui->argv);
    gui_gtk_make_8bpp_palette(gui);

    gui->wmain = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gui->wmain),"Elwro 800 Junior Emulator");
    gtk_widget_set_usize(gui->wmain, gui->width, gui->height);
    gtk_window_set_policy(GTK_WINDOW(gui->wmain),FALSE,FALSE,FALSE);
    gtk_widget_set_events(gui->wmain,GDK_KEY_RELEASE_MASK);
    gtk_widget_realize(gui->wmain);
    gtk_widget_add_events(gui->wmain, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(gui->wmain),"destroy",GTK_SIGNAL_FUNC(kill_me),NULL);
    gtk_signal_connect(GTK_OBJECT(gui->wmain),"key_press_event",GTK_SIGNAL_FUNC(gui_gtk_key_pressed),"1");
    gtk_signal_connect(GTK_OBJECT(gui->wmain),"key_release_event",GTK_SIGNAL_FUNC(gui_gtk_key_pressed),"0");

    gui->winid = GDK_WINDOW_XWINDOW(gui->wmain->window);
    sprintf(swinid, "SDL_WINDOWID=%ld", gui->winid); 
    putenv(swinid); 

#ifndef NO_SDL
    if(gui->use_sdl){
	gui_sdl_setup(gui);
	gui->use_gtk = 0;
	gui->put_pixel = gui_sdl_put_pixel;
	gui->scrn_rfsh = gui_sdl_screen_rfsh;	
    }
#endif

    if(gui->use_gtk){
	gui->put_pixel = gui_gtk_put_pixel;
	gui->scrn_rfsh = gui_gtk_screen_rfsh;
	gui->use_sdl = 0;
    }

    vbox_wg = gtk_vbox_new(FALSE,1);
    gtk_container_add(GTK_CONTAINER(gui->wmain), vbox_wg);

    menu = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox_wg), menu, FALSE, FALSE, 0);

    elm = gtk_menu_item_new_with_label("Plik");
    gtk_menu_bar_append(GTK_MENU_BAR(menu),elm);
    submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(elm),submenu);
    elm = gtk_menu_item_new_with_label("Wczytaj snapshot");
    gtk_menu_append(GTK_MENU(submenu),elm);    
    gtk_signal_connect(GTK_OBJECT(elm),"activate",GTK_SIGNAL_FUNC(gui_gtk_load_fileselector),gui);
//    elm = gtk_menu_item_new_with_label("Zapisz snapshot");
//    gtk_menu_append(GTK_MENU(submenu),elm);    
//    gtk_signal_connect(GTK_OBJECT(elm),"activate",GTK_SIGNAL_FUNC(gui_gtk_save_fileselector),gui);
    elm = gtk_menu_item_new_with_label("O programie");
    gtk_menu_append(GTK_MENU(submenu),elm);    
    gtk_signal_connect(GTK_OBJECT(elm),"activate",GTK_SIGNAL_FUNC(gui_gtk_about),gui);
    elm = gtk_menu_item_new_with_label("Wyjdz");
    gtk_menu_append(GTK_MENU(submenu),elm);    
    gtk_signal_connect(GTK_OBJECT(elm),"activate",GTK_SIGNAL_FUNC(gui_gtk_kill_me),NULL);

    elm = gtk_menu_item_new_with_label("Narzedzia");
    gtk_menu_bar_append(GTK_MENU_BAR(menu),elm);
    submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(elm),submenu);
    elm = gtk_menu_item_new_with_label("Debugger Z80");
    gtk_menu_append(GTK_MENU(submenu),elm);
    gtk_signal_connect(GTK_OBJECT(elm),"activate",GTK_SIGNAL_FUNC(gui_gtk_debugger),gui);
    elm = gtk_menu_item_new_with_label("Konfiguracja");
    gtk_menu_append(GTK_MENU(submenu),elm);    
    gtk_signal_connect(GTK_OBJECT(elm),"activate",GTK_SIGNAL_FUNC(gui_gtk_config),gui);
    elm = gtk_menu_item_new_with_label("Pauza");
    gtk_menu_bar_append(GTK_MENU_BAR(menu),elm);
    gtk_signal_connect(GTK_OBJECT(elm),"activate",GTK_SIGNAL_FUNC(run_cpu),NULL);

    elm = gtk_menu_item_new_with_label("Reset");
    gtk_menu_bar_append(GTK_MENU_BAR(menu),elm);
    gtk_signal_connect(GTK_OBJECT(elm),"activate",GTK_SIGNAL_FUNC(reset),NULL);

    if(gui->use_gtk){
	printf("[MSG] Wyswietlanie GDK\n");    
	gui->drawing_area = gtk_drawing_area_new();
	gtk_drawing_area_size(GTK_DRAWING_AREA(gui->drawing_area),gui->scr->area_w,gui->scr->area_h);
	gtk_container_add(GTK_CONTAINER(vbox_wg),gui->drawing_area);
	gtk_signal_connect(GTK_OBJECT(gui->drawing_area),"expose_event",GTK_SIGNAL_FUNC(gui_gtk_da_expose),gui);
	gtk_signal_connect(GTK_OBJECT(gui->drawing_area),"configure_event",GTK_SIGNAL_FUNC(gui_gtk_da_configure),gui);
    }

    gtk_widget_show_all(gui->wmain);
}

void gui_start()
{ 
    atexit(gtk_main_quit); 
    gtk_main(); 
#ifndef NO_SDL
    SDL_Quit();
#endif
}

void gui_setup(void *ptr, void *argc, void *argv)
{

    e8_gui *gui = &__gui;

    ((e800j*)ptr)->gui = gui;

    gui->e8     = ptr;
    gui->width  = ((e800j*)ptr)->config->wwidth;
    gui->height = ((e800j*)ptr)->config->wheight;
    gui->bpp    = ((e800j*)ptr)->config->bpp;
    gui->argc   = argc;
    gui->argv   = argv;     
    gui->use_sdl = ((e800j*)ptr)->config->use_sdl;
    gui->use_gtk = ((e800j*)ptr)->config->use_gtk;
    gui->use_palette_border = ((e800j*)ptr)->config->bpalette;
    gui_setup_start(gui);
}

/********************************************************************************************/
#ifndef NO_SDL

void gui_sdl_setup(e8_gui *gui)
{
    if(!gui->use_sdl) return;
    printf("[MSG] Wyswietlanie SDL\n");    
    if(SDL_Init(SDL_INIT_VIDEO) < 0){
	gui_gtk_msgbox("Couldn't initialize SDL %s\n",SDL_GetError());
	gtk_main_quit();
    }

    if(!(gui->sdl_surface = SDL_SetVideoMode(gui->width,gui->height,gui->bpp,SDL_NOFRAME | SDL_DOUBLEBUF))){
	gui_gtk_msgbox("Couldn't initialize video mode SDL %s\n",SDL_GetError());
	gui->use_sdl = 0;
	gui->use_gtk = 1;	
	SDL_Quit();
	return;
    }

    if(gui->sdl_surface->format->BytesPerPixel < 2)
	SDL_SetPalette(gui->sdl_surface, SDL_LOGPAL, gui->sdl_8bpp_palette, 0, 16);    

    memset((byte *)gui->sdl_surface->pixels, ZX_BORDER, 
				gui->scr->area_w * gui->scr->area_h * (gui->bpp / 8));
}

void gui_sdl_draw_border(e8_gui *gui)
{
    SDL_Rect area;
    SDL_Color *color;

    gui->scr->frame_color = *gui->scr->new_frame_color & OUT_FE_BORDERMASK;        

    if(gui->sdl_surface->format->BytesPerPixel != 2){
	SDL_SetPalette(gui->sdl_surface, SDL_LOGPAL, gui->sdl_8bpp_palette + gui->scr->frame_color, 
										    ZX_BORDER, 1);
    } else {
	color = gui->sdl_8bpp_palette + gui->scr->frame_color;	
	area.w = gui->scr->area_w;
	area.h = gui->scr->area_h;
	area.x = 0;
	area.y = 0;
	SDL_FillRect(gui->sdl_surface, &area, SDL_MapRGB(gui->sdl_surface->format,
								color->r,color->g,color->b));
    }
}

void gui_sdl_screen_rfsh(void *gui)
{
    if(((e8_gui*)gui)->scr->frame_color != (*((e8_gui*)gui)->scr->new_frame_color & OUT_FE_BORDERMASK)){
    	gui_sdl_draw_border(gui);
    }
/* przyda³a by siê optymalizacja */
    gui_sdl_redraw(gui); 
}

void gui_sdl_put_pixel(void *gui, int x, int y, int color)
{
    e8_screen *sc = ((e8_gui *)gui)->scr;
    
    int offs_y = y * sc->scr_ws*2;
    int offs_y_ = (y*2+1) * sc->scr_ws;
    int offs_x = x * 2;

    *(sc->scr_buffer + offs_y  + offs_x) = color;        
    *(sc->scr_buffer + offs_y  + offs_x+1) = color;        
    *(sc->scr_buffer + offs_y_  + offs_x) = color;        
    *(sc->scr_buffer + offs_y_  + offs_x+1) = color;        
}

void gui_sdl_redraw(e8_gui *gui)
{
    byte *buffer,*src_buff;
    word *buff16;
    int i,j;
    SDL_Color *color;

    for(j=0; j <2;j++){
	if(SDL_LockSurface(gui->sdl_surface) < 0){
	    gui_gtk_msgbox("Couldn't lock display surface: %s\n",SDL_GetError());
	    exit(1);
	}
	src_buff = gui->scr->scr_buffer;
	buffer = (byte *)gui->sdl_surface->pixels;
	buff16 = (word *)buffer;
    	if(gui->sdl_surface->format->BytesPerPixel != 2){
	    buffer += gui->scr->scr_offs + gui->scr->scr_hf;
	    for(i = 0; i < gui->scr->scr_hs; i++){
		memcpy( buffer,	src_buff, gui->scr->scr_bline);
	        buffer += gui->sdl_surface->pitch;
		src_buff += gui->scr->scr_bline;
	    }
	} else {
	    buffer += gui->scr->scr_offs + gui->scr->scr_hf;
	    for(i = 0; i < gui->scr->scr_hs; i++){
		for(j=0; j < gui->scr->scr_ws;j++){
		    color = gui->sdl_8bpp_palette + *(src_buff + j);
		    *((word *)buffer + j) = SDL_MapRGB(gui->sdl_surface->format,
								    color->r, color->g, color->b);
		}
	        buffer += gui->sdl_surface->pitch;
		src_buff += gui->scr->scr_bline >> 1;
	    }
	}
        SDL_UnlockSurface(gui->sdl_surface);    
	if(gui->sdl_surface->flags & SDL_DOUBLEBUF){
	    SDL_Flip(gui->sdl_surface);
        } else {
	    SDL_UpdateRect(gui->sdl_surface,0,0,0,0);
	    break;
	}
    }    
}

#endif

GtkWidget *gui_gtk_spin_button(GtkWidget *wg,char *name, int val,int min, int max)
{
    GtkAdjustment *adj;
    GtkWidget	  *ww, *hbox;

    hbox = gtk_hbox_new(TRUE,0);
    gtk_box_pack_start(GTK_BOX(wg),hbox,FALSE,FALSE,0);
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(val,min,max,1,16,16));
    ww = gtk_spin_button_new(adj,1,0);
    gtk_container_add(GTK_CONTAINER(hbox), ww);
    gtk_container_add(GTK_CONTAINER(hbox), gtk_label_new(name));
    
    return ww;    
}

void gui_gtk_browser(GtkWidget *tt, char *label, char *value, char *browse_lab)
{
    GtkWidget *hbox, *el;

    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(tt),hbox,FALSE,FALSE,0);    
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new(label));
    el = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(hbox),el);
    gtk_entry_set_text(GTK_ENTRY(el), value);
    gtk_entry_set_editable(GTK_ENTRY(el),TRUE);
//    gtk_signal_connect(GTK_OBJECT(el),"activate",GTK_SIGNAL_FUNC(gui_gtk_set_bas0),gui);
    el = gtk_button_new_with_label(browse_lab);
    gtk_container_add(GTK_CONTAINER(hbox),el);
//    gtk_signal_connect(GTK_OBJECT(el),"activate",GTK_SIGNAL_FUNC(gui_gtk_set_bas0),);
}


//    gui_gtk_spin_button(vbox, "Szeroko¶æ pola rysunkowego: ", 5, 100);
GtkWidget *gui_palette_bt = NULL;

void gui_radio_1(GtkWidget *wg, e8_gui *gui)
    { ((e800j *)gui->e8)->config->wwidth = 320; ((e800j *)gui->e8)->config->wheight = 200;}

void gui_radio_2(GtkWidget *wg, e8_gui *gui)
    { ((e800j *)gui->e8)->config->wwidth = 640; ((e800j *)gui->e8)->config->wheight = 480; }

void gui_radio_3(GtkWidget *wg, e8_gui *gui)
    { ((e800j *)gui->e8)->config->wwidth = 800; ((e800j *)gui->e8)->config->wheight = 600;}
    
void gui_radio_4(GtkWidget *wg, e8_gui *gui)
{ 
    ((e800j *)gui->e8)->config->use_gtk = 1; 
    ((e800j *)gui->e8)->config->use_sdl = 0;
    if(gui_palette_bt)
	gtk_widget_set_sensitive(gui_palette_bt, FALSE);
}

void gui_radio_5(GtkWidget *wg, e8_gui *gui)
{ 
    ((e800j *)gui->e8)->config->use_gtk = 0; 
    ((e800j *)gui->e8)->config->use_sdl = 1;
    if(gui_palette_bt)
	gtk_widget_set_sensitive(gui_palette_bt, ((e800j *)gui->e8)->config->bpp != 16);
}

void gui_check_1(GtkWidget *wg, e8_gui *gui)
{ 
    ((e800j *)gui->e8)->config->bpp = GTK_TOGGLE_BUTTON(wg)->active ? 16:8; 
    if(gui_palette_bt)
	gtk_widget_set_sensitive(gui_palette_bt, ((e800j *)gui->e8)->config->bpp != 16);
}

void gui_check_2(GtkWidget *wg, e8_gui *gui)
    { ((e800j *)gui->e8)->config->bpalette = GTK_TOGGLE_BUTTON(wg)->active; }

void gui_spin_scr_rsfsh(GtkWidget *wg, e8_gui *gui)
{
    ((e800j*)gui->e8)->config->scr_rfsh = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wg));
}

void gui_spin_blink(GtkWidget *wg, e8_gui *gui)
{
    ((e800j*)gui->e8)->config->blink_rfsh = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wg));
}
/**/

void gui_spin_cpu_rsfsh(GtkWidget *wg, e8_gui *gui)
{
    ((e800j*)gui->e8)->config->cpu_rfsh = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wg));
}

void gui_spin_cpu_tick(GtkWidget *wg, e8_gui *gui)
{
    ((e800j*)gui->e8)->config->cpu_tick = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wg));
}

void gui_spin_int_rsfsh(GtkWidget *wg, e8_gui *gui)
{
    ((e800j*)gui->e8)->config->int_rfsh = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wg));
}

void gui_spin_int_tick(GtkWidget *wg, e8_gui *gui)
{
    ((e800j*)gui->e8)->config->int_tick = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wg));
}

void gui_spin_fdc_rsfsh(GtkWidget *wg, e8_gui *gui)
{
    ((e800j*)gui->e8)->config->fdc_rfsh = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wg));
}

/**/
void gui_load_default_conf(e8_gui *gui)
{
//    main_config_
}

void gui_save_conf(e8_gui *gui)
{

}


void gui_gtk_config(GtkWidget *wg, e8_gui *gui)
{
    GtkWidget *wconf, *vbox, *nb, *page_scr,*page_clk,*page_mem, *hbox, *el, *tt;
    GSList  *gr1 = NULL, *gr2 = NULL;
//    GtkWidget *vbox_wg,*page_io;
    
    gui_palette_bt = NULL;
    wconf = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_usize(wconf,320,320);
    gtk_signal_connect(GTK_OBJECT(wconf), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroy),NULL);    
    gtk_window_set_title(GTK_WINDOW(wconf),"Konfiguracja");

    vbox = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(wconf), vbox);

    nb = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(vbox), nb);
    page_scr = gtk_vbox_new(FALSE,0);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),page_scr,gtk_label_new("Ekran"));
    page_clk = gtk_vbox_new(FALSE,0);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),page_clk,gtk_label_new("CLK"));
    page_mem = gtk_vbox_new(FALSE,0);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),page_mem,gtk_label_new("Pamiec"));
//    page_io = gtk_vbox_new(FALSE,0);
//    gtk_notebook_append_page(GTK_NOTEBOOK(nb),page_io,gtk_label_new("I/O"));

/* zakladka Ekran */
    el = gtk_frame_new("Rozdzielczosc");
    gtk_container_add(GTK_CONTAINER(page_scr),el);
    tt = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(el),tt);

    el = gtk_radio_button_new_with_label(gr1,"320x200");    
    gtk_box_pack_start(GTK_BOX(tt),el,FALSE,FALSE,0);
    gr1 = gtk_radio_button_group(GTK_RADIO_BUTTON(el));
    gtk_signal_connect(GTK_OBJECT(el),"toggled",GTK_SIGNAL_FUNC(gui_radio_1),gui);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(el), ((e800j *)gui->e8)->config->wwidth == 320 );
    el = gtk_radio_button_new_with_label(gr1,"640x480");    
    gtk_box_pack_start(GTK_BOX(tt),el,FALSE,FALSE,0);
    gr1 = gtk_radio_button_group(GTK_RADIO_BUTTON(el));
    gtk_signal_connect(GTK_OBJECT(el),"toggled",GTK_SIGNAL_FUNC(gui_radio_2),gui);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(el), ((e800j *)gui->e8)->config->wwidth == 640 );
    el = gtk_radio_button_new_with_label(gr1,"800x600");    
    gtk_box_pack_start(GTK_BOX(tt),el,FALSE,FALSE,0);
    gr1 = gtk_radio_button_group(GTK_RADIO_BUTTON(el));
    gtk_signal_connect(GTK_OBJECT(el),"toggled",GTK_SIGNAL_FUNC(gui_radio_3),gui);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(el), ((e800j *)gui->e8)->config->wwidth == 800 );

    el = gtk_check_button_new_with_label("16 bitowy kolor");
    gtk_box_pack_start(GTK_BOX(tt),el,FALSE,FALSE,0);    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(el), ((e800j *)gui->e8)->config->bpp == 16 );
    gtk_signal_connect(GTK_OBJECT(el),"toggled",GTK_SIGNAL_FUNC(gui_check_1),gui);

    el = gtk_frame_new("Wyswietlanie");
    gtk_container_add(GTK_CONTAINER(page_scr),el);
    tt = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(el),tt);

    el = gtk_radio_button_new_with_label(gr2,"GDK");    
    gtk_box_pack_start(GTK_BOX(tt),el,FALSE,FALSE,0);
    gr2 = gtk_radio_button_group(GTK_RADIO_BUTTON(el));
    gtk_signal_connect(GTK_OBJECT(el),"toggled",GTK_SIGNAL_FUNC(gui_radio_4),gui);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(el), !((e800j *)gui->e8)->config->use_sdl );

    el = gtk_radio_button_new_with_label(gr2,"SDL");    
    gtk_box_pack_start(GTK_BOX(tt),el,FALSE,FALSE,0);
    gr2 = gtk_radio_button_group(GTK_RADIO_BUTTON(el));
    gtk_signal_connect(GTK_OBJECT(el),"toggled",GTK_SIGNAL_FUNC(gui_radio_5),gui);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(el), ((e800j *)gui->e8)->config->use_sdl );
    el = gtk_check_button_new_with_label("Ramka na palecie kolorow");
    gtk_box_pack_start(GTK_BOX(tt),el,FALSE,FALSE,0);    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(el), ((e800j *)gui->e8)->config->bpalette );
    gtk_signal_connect(GTK_OBJECT(el),"toggled",GTK_SIGNAL_FUNC(gui_check_2),gui);
    gtk_widget_set_sensitive(el, 
	(((e800j *)gui->e8)->config->bpp != 16) && (((e800j *)gui->e8)->config->use_sdl) );
    gui_palette_bt = el;
    el = gui_gtk_spin_button(tt, "Odswiezanie [ms]", ((e800j*)gui->e8)->config->scr_rfsh, 5, 200);
    gtk_signal_connect(GTK_OBJECT(el),"value_changed",GTK_SIGNAL_FUNC(gui_spin_scr_rsfsh),gui);
    el = gui_gtk_spin_button(tt, "Migotanie [ms]", ((e800j*)gui->e8)->config->blink_rfsh, 100, 1000);
    gtk_signal_connect(GTK_OBJECT(el),"value_changed",GTK_SIGNAL_FUNC(gui_spin_blink),gui);

/* zakladka CLK */
    el = gtk_frame_new("CPU");
    gtk_container_add(GTK_CONTAINER(page_clk),el);
    tt = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(el), tt);
    el = gui_gtk_spin_button(tt, "Okres wywolan [ms]", ((e800j*)gui->e8)->config->cpu_rfsh, 5, 200);
    gtk_signal_connect(GTK_OBJECT(el),"value_changed",GTK_SIGNAL_FUNC(gui_spin_cpu_rsfsh),gui);
    el = gui_gtk_spin_button(tt, "Takty / wywolanie", ((e800j*)gui->e8)->config->cpu_tick, 1, 10000);
    gtk_signal_connect(GTK_OBJECT(el),"value_changed",GTK_SIGNAL_FUNC(gui_spin_cpu_tick),gui);
    el = gtk_frame_new("Przerwanie INT");
    gtk_container_add(GTK_CONTAINER(page_clk),el);
    tt = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(el), tt);
    el = gui_gtk_spin_button(tt, "Okres wywolan [ms]", ((e800j*)gui->e8)->config->int_rfsh, 5, 200);
    gtk_signal_connect(GTK_OBJECT(el),"value_changed",GTK_SIGNAL_FUNC(gui_spin_int_rsfsh),gui);
    el = gui_gtk_spin_button(tt, "Takty / wywolanie", ((e800j*)gui->e8)->config->int_tick, 1, 10000);
    gtk_signal_connect(GTK_OBJECT(el),"value_changed",GTK_SIGNAL_FUNC(gui_spin_int_tick),gui);
    el = gtk_frame_new("FDC");
    gtk_container_add(GTK_CONTAINER(page_clk),el);
    tt = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(el), tt);
    el = gui_gtk_spin_button(tt, "Okres wywolan [ms]", ((e800j*)gui->e8)->config->fdc_rfsh, 5, 200);
    gtk_signal_connect(GTK_OBJECT(el),"value_changed",GTK_SIGNAL_FUNC(gui_spin_fdc_rsfsh),gui);

/* zakladka pamieci */
    el = gtk_frame_new("ROM");
    gtk_container_add(GTK_CONTAINER(page_mem),el);
    tt = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(el), tt);
    gui_gtk_browser(tt,"BAS 0",((e800j*)gui->e8)->config->bas0_fname,"Przegladaj");
    gui_gtk_browser(tt,"BOOT ",((e800j*)gui->e8)->config->boot_fname,"Przegladaj");
    gui_gtk_browser(tt,"BAS 1",((e800j*)gui->e8)->config->bas1_fname,"Przegladaj");

    el = gtk_frame_new("RAM");
    gtk_container_add(GTK_CONTAINER(page_mem),el);
    tt = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(el), tt);


/* przyciski u do³u - ogólne dla ca³ego okna */
    hbox = gtk_hbox_new(FALSE,0);    
    gtk_container_add(GTK_CONTAINER(vbox),hbox);    
    gtk_container_border_width(GTK_CONTAINER(hbox),5);
    el = gtk_button_new_with_label("Rezygnuj");    
    gtk_box_pack_end(GTK_BOX(hbox),el,FALSE,FALSE,0);
    gtk_signal_connect_object(GTK_OBJECT(el),"clicked",GTK_SIGNAL_FUNC(gtk_widget_destroy),wconf);
//    el = gtk_button_new_with_label("Zastosuj");
//    gtk_box_pack_end(GTK_BOX(hbox),el,FALSE,FALSE,0);
//    gtk_signal_connect(GTK_OBJECT(el),"clicked",GTK_SIGNAL_FUNC(gui_save_conf),gui);
//    el = gtk_button_new_with_label("Laduj domyslne");    
//    gtk_box_pack_end(GTK_BOX(hbox),el,FALSE,FALSE,0);
//    gtk_signal_connect_object(GTK_OBJECT(el),"clicked",GTK_SIGNAL_FUNC(gui_load_default_conf),gui);
    gtk_widget_show_all(wconf);
}











