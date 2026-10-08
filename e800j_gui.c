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
#include <gdk/gdkkeysyms.h>

#include "e800j_kbrd.h"
#include "e800j_file.h"
#include "e800j_gui.h"
#include "e800j_main.h"
#include "z80_debug.h"

config cfg; 
GtkTextBuffer *txt;
GtkTextView   *txtv;
GdkColor gcc[16] = ZX_PALETTE;
char cmd_buffer[1024];

void load_snapshot(GtkObject *wg)
{

    FILE *f;
    const char *fname;
    
    fname  = gtk_file_selection_get_filename(GTK_FILE_SELECTION(wg));

    if(!(f=fopen(fname,"r"))){
	perror("Nie mozna zaladowac pliku\n");
	return;
    }
    
    if(load_z80(cfg.ptr,f,fname)){
	g_print("B³±d odczytu pliku!\n");
    }

    fclose(f);        
    gtk_widget_destroy(GTK_WIDGET(wg));
}

int configure_dr(GtkWidget *wg, GdkEventExpose *ev, config *cf)
{
    int i,j;
    if(cf->pix)
	gdk_pixmap_unref(cf->pix);
    
    cf->xx = wg->allocation.width;
    cf->yy = wg->allocation.height;    
    cf->pix = gdk_pixmap_new(wg->window,cf->xx,cf->yy,-1);

    for(i=0;i<16;i++){
	cfg.gc[i] = gdk_gc_new(cfg.pix);
	gdk_gc_set_foreground(cfg.gc[i], &gcc[i]);
    }

    for(j=0; j < cf->yy; j++)
	for(i=0; i < cf->xx; i++)
	    gdk_draw_point(cfg.pix,cfg.gc[4],i,j);
    return TRUE;
}

int expose_dr(GtkWidget *wg, GdkEventExpose *ev,config *cf)
{
    gdk_draw_pixmap(wg->window,wg->style->fg_gc[GTK_WIDGET_STATE(wg)],cf->pix,
    ev->area.x,ev->area.y,
    ev->area.x,ev->area.y,
    ev->area.width,ev->area.height);
    return FALSE;
}


void gui_put_pixel(int color,int x,int y)
{
//    int sx,sy;
//    int zx, zy, scale_x,scale_y;

/*
    zx = cfg.xx << 8;
    zy = cfg.yy << 8;
    
    zx = zx / 256;
    zy = zy / 192;
    
    sx = (x * zx) >> 8;
    sy = (y * zy) >> 8;
    
    scale_x= (cfg.xx / 256) + 1;
    scale_y= (cfg.yy / 192) + 1;
*/    
//    gdk_draw_rectangle(cfg.pix,cfg.gc[color],TRUE,sx,sy,scale_x,scale_y);
    gdk_draw_point(cfg.pix,cfg.gc[color],x,y);
}


void gui_screen_rfsh()
{
    GdkRectangle tmp;
    tmp.x=tmp.y=0;
    tmp.width=cfg.draw_area->allocation.width;
    tmp.height=cfg.draw_area->allocation.height;

    gtk_widget_draw(cfg.draw_area, &tmp);
}

void cancel_load_file(GtkWidget *wg)
{
    z80_set_run(NULL,1); /* wznów emulacjê CPU */
    gtk_widget_destroy(wg);
}

void load_file(GtkWidget *wg)
{
    GtkWidget *fs;

    z80_set_run(NULL,0); /* zatrzymaj emulacjê CPU */
    fs=gtk_file_selection_new("Wczytaj z80");
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fs));
    gtk_widget_show(fs);
    
    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),
    "clicked",(GtkSignalFunc)cancel_load_file,GTK_OBJECT(fs));

    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),
    "clicked",(GtkSignalFunc)load_snapshot,GTK_OBJECT(fs));

}


void gui_key_pressed(GtkWidget *wg,GdkEventKey *ev,char *state)
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




void gui_debug_out(char *a)
{ 
    GtkTextIter iter;
    GtkTextMark *mark;

    gtk_text_buffer_get_end_iter(txt,&iter);
    gtk_text_buffer_place_cursor(txt,&iter);
    mark=gtk_text_buffer_get_insert(txt);    
    gtk_text_view_scroll_to_mark(txtv,mark,0.0,TRUE,1.0,1.0);
    gtk_text_buffer_insert_with_tags_by_name(txt,&iter,a,-1,"tt",NULL);
    if(DEB(z80_get_struct())->deb_stdout) puts(a);
}

void gui_debug_in(char *a)
{
    strcpy(a,cmd_buffer);    
    memset(cmd_buffer,0,sizeof(cmd_buffer));
}

void ed_ed(GtkWidget *wg)
{
    strcpy(cmd_buffer,gtk_entry_get_text(GTK_ENTRY(wg)));
    gtk_entry_set_text(GTK_ENTRY(wg),"");
}

void debugger_destroy(GtkWidget *wg)
{
    gtk_widget_destroy(wg);
    DEBUGGER_RESET(z80_get_struct());
}

void debugger()
{
    GtkWidget *wg,*vb, *vv,*ed,*sw;

    memset(cmd_buffer,0,sizeof(cmd_buffer));
    DEBUGGER_RESET(z80_get_struct());
    
    wg=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_signal_connect(GTK_OBJECT(wg),"destroy",GTK_SIGNAL_FUNC(debugger_destroy),NULL);

    gtk_window_set_title(GTK_WINDOW(wg),"Z80 debugger");
    gtk_widget_set_usize(wg,256,256);
    vb=gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(wg),vb);
    
    sw=gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(vb),sw);    

    vv=gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(sw),vv);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(vv),FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(vv),FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(vv),FALSE);

    txt=gtk_text_view_get_buffer(GTK_TEXT_VIEW(vv));
    txtv=GTK_TEXT_VIEW(vv);

    ed=gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vb),ed,FALSE,FALSE,0);    
    gtk_entry_set_editable(GTK_ENTRY(ed),TRUE);
    gtk_signal_connect(GTK_OBJECT(ed),"activate",GTK_SIGNAL_FUNC(ed_ed),NULL);

    gtk_text_buffer_create_tag(txt,"tt","family","monospace","foreground","blue",NULL);
    
    gtk_widget_show_all(wg);
    z80_set_run(NULL,1);
    z80_set_debug(cfg.ptr,1);
}

void gui_setup(void *ptr, void *argc, void *argv)
{
    int i;
    GtkWidget *wg, *tt,*tk,*k;
    
    cfg.zoom_x = cfg.zoom_y = 1;
    cfg.pix = NULL;
    cfg.draw_area = NULL;


    cfg.ptr=ptr;
    gtk_init(argc, argv);
    for(i=0; i<16; i++) gdk_color_alloc(gdk_colormap_get_system(),&gcc[i]);


    wg=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_signal_connect(GTK_OBJECT(wg),"destroy",GTK_SIGNAL_FUNC(kill_me),NULL);
    gtk_window_set_title(GTK_WINDOW(wg),"Elwro 800 Junior Emulator");
    gtk_widget_set_usize(wg,256,256);


    gtk_widget_set_events(wg,GDK_KEY_RELEASE_MASK);
    
    gtk_signal_connect(GTK_OBJECT(wg),"key_press_event",GTK_SIGNAL_FUNC(gui_key_pressed),"1");
    gtk_signal_connect(GTK_OBJECT(wg),"key_release_event",GTK_SIGNAL_FUNC(gui_key_pressed),"0");

    tt=gtk_vbox_new(FALSE,1);
    gtk_container_add(GTK_CONTAINER(wg),tt);

    tk=gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(tt),tk,FALSE,FALSE,0);

    gtk_toolbar_append_item(GTK_TOOLBAR(tk),"LD","Wczytaj",NULL,NULL,GTK_SIGNAL_FUNC(load_file),NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(tk),"RUN","Start",NULL,NULL,GTK_SIGNAL_FUNC(run_cpu),NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(tk),"RST","Start",NULL,NULL,GTK_SIGNAL_FUNC(reset),NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(tk),"Debug","Start",NULL,NULL,GTK_SIGNAL_FUNC(debugger),NULL);

/* pasek opisu klawiszy funkcyjnych */
    k = gtk_frame_new(NULL); 
    gtk_container_add(GTK_CONTAINER(tt),k);
    tk = gtk_label_new("F1 - CP/J  F2 - DIR  F3 - LOAD  F4 - SAVE  F5 - ERASE");
    gtk_container_add(GTK_CONTAINER(k),tk);    

    k = gtk_drawing_area_new();
    gtk_drawing_area_size(GTK_DRAWING_AREA(k),256*cfg.zoom_x,192*cfg.zoom_y);
    gtk_container_add(GTK_CONTAINER(tt),k);

    gtk_signal_connect(GTK_OBJECT(k),"expose_event",GTK_SIGNAL_FUNC(expose_dr),&cfg);    
    gtk_signal_connect(GTK_OBJECT(k),"configure_event",GTK_SIGNAL_FUNC(configure_dr),&cfg);    
    cfg.draw_area=k;
    gtk_widget_show_all(wg);

}

void gui_timer(void *func,long time,void *ptr)
{ 
    gtk_timeout_add(time, func,ptr); 
}
void gui_start()
{ 
    atexit(gtk_main_quit); 
    gtk_main(); 
}

