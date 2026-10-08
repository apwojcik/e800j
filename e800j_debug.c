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

#ifndef NO_SDL
#include <SDL/SDL.h>
#endif

#include "e800j_kbrd.h"
#include "e800j_file.h"
#include "e800j_main.h"
#include "z80_debug.h"
#include "e800j_scrn.h"
#include "e800j_gui.h"
#include "e800j_debug.h"

enum{
    DEBUGGER_WMAIN,
    DEBUGGER_TEXT_BUFFER,
    DEBUGGER_TEXT_VIEW,
    DEBUGGER_VIEW,
    DEBUGGER_REG_PC,
    DEBUGGER_REG_SP,
    DEBUGGER_REG_AF,
    DEBUGGER_REG_AF_,
    DEBUGGER_REG_BC,
    DEBUGGER_REG_BC_,
    DEBUGGER_REG_DE,
    DEBUGGER_REG_DE_,
    DEBUGGER_REG_HL,
    DEBUGGER_REG_HL_,
    DEBUGGER_REG_IX,
    DEBUGGER_REG_IY,
    DEBUGGER_REG_I,    
    DEBUGGER_REG_R,    
    DEBUGGER_IM,
    DEBUGGER_FE,
    DEBUGGER_FLAGS,
    DEBUGGER_SF,
    DEBUGGER_ZF,
    DEBUGGER_5F,
    DEBUGGER_HF,
    DEBUGGER_3F,
    DEBUGGER_PF,
    DEBUGGER_NF,
    DEBUGGER_CF,
    DEBUGGER_DEASM,
    DEBUGGER_BP
};


void gui_gtk_set_regs(Z80 *z);
char *gui_hex2(int value, char *buffer);
void *gtk_widgets[256];

char trap_type=0;
int  trap_addr=0;
int deasm_row_selected = 0;
int bp_row_selected = 0;
char cmd_buffer[1024];



void gui_gtk_select_row_deasm(GtkWidget *wg, int row, int column, GdkEventButton *ev, void *pt)
    { deasm_row_selected = row; }

void gui_gtk_bp_row_deasm(GtkWidget *wg, int row, int column, GdkEventButton *ev, void *pt)
    { bp_row_selected = row; }


void gui_chop(char *txt)
{
    while(*txt && *txt!='\n') txt++;
    *txt = '\0';
}

char deasm_mode=0;

void gui_send_cmd(char *cmd)
    { strcpy(cmd_buffer, cmd); }

void gui_gtk_debug_deasm( char *txt_buffer)
{
    char *clist[2];
    char tmpstr[10];

    if(deasm_mode){
        if(*txt_buffer == '.'){
	    gtk_clist_thaw(gtk_widgets[DEBUGGER_DEASM]);
	    deasm_mode = 0;
	    return;
	}
	if(txt_buffer)
	{
	    if(*txt_buffer == '>') return;
	    gui_chop(txt_buffer);
	    *(txt_buffer + 4) = '\0';
	    clist[0] = txt_buffer;
	    clist[1] = txt_buffer + 5;	
	    gtk_clist_append(GTK_CLIST(gtk_widgets[DEBUGGER_DEASM]),clist);

	}
    } else {
	if(*txt_buffer == '.'){
	    gtk_clist_freeze(gtk_widgets[DEBUGGER_DEASM]);
	    gtk_clist_clear(GTK_CLIST(gtk_widgets[DEBUGGER_DEASM]));
	    gui_hex2(z80_get_struct()->PCp,tmpstr+1);
	    *(tmpstr+0) = 'w';
	    *(tmpstr+1) = 'c';
	    *(tmpstr+2) = ' ';
	    *(tmpstr+7) = '\n';
	    *(tmpstr+8) = '\0';	    
	    gui_send_cmd(tmpstr);
	    deasm_mode = 1;
	}
    }
}

void gui_debug_out(char *a)
{ 
    GtkTextIter iter;
    GtkTextMark *mark;

    if(!deasm_mode)
    {
	if( *a != '.' ){
	    gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(gtk_widgets[DEBUGGER_TEXT_BUFFER]),&iter);
	    gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(gtk_widgets[DEBUGGER_TEXT_BUFFER]),&iter);
	    mark=gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(gtk_widgets[DEBUGGER_TEXT_BUFFER]));    
	    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(gtk_widgets[DEBUGGER_TEXT_VIEW]),mark,0.0,TRUE,1.0,1.0);
	    gtk_text_buffer_insert_with_tags_by_name(GTK_TEXT_BUFFER(gtk_widgets[DEBUGGER_TEXT_BUFFER]),&iter,a,-1,"tt",NULL);
	    if(DEB(z80_get_struct())->deb_stdout) puts(a);
	}
	gui_gtk_set_regs(z80_get_struct());
    };
    gui_gtk_debug_deasm(a);
}


void gui_debug_in(char *a)
{
    strcpy(a,cmd_buffer);    
    memset(cmd_buffer,0,sizeof(cmd_buffer));
}


void gui_gtk_ed_ed(GtkWidget *wg)
{
    strcpy(cmd_buffer,gtk_entry_get_text(GTK_ENTRY(wg)));
    gtk_entry_set_text(GTK_ENTRY(wg),"");
}

void gui_gtk_debugger_destroy(GtkWidget *wg)
{
    gtk_widget_destroy(wg);
    DEBUGGER_RESET(z80_get_struct());
    z80_set_run(NULL, 1);
    z80_set_debug(z80_get_struct(),0);
}

char gui_chex(char ch)
    { return ch < 10 ? '0' + ch : 'a' + ch - 10; }

char *gui_hex2(int value, char *buffer)
{
    *(buffer + 0) = '0';
    *(buffer + 1) = 'x';
    *(buffer + 2) = gui_chex( value / 0x1000); value %= 0x1000;
    *(buffer + 3) = gui_chex(value  / 0x100 ); value %= 0x100;
    *(buffer + 4) = gui_chex(value  / 0x10  ); value %= 0x10;
    *(buffer + 5) = gui_chex(value);
    *(buffer + 6) = '\0';    
    return buffer;
}

char *gui_hex(int value, char *buffer)
{
    *(buffer + 0) = '0';
    *(buffer + 1) = 'x';
    *(buffer + 2) = gui_chex(value  / 0x10  ); value %= 0x10;
    *(buffer + 3) = gui_chex(value);
    *(buffer + 4) = '\0';    
    return buffer;
}

char *gui_deb_test(Z80 *z,unsigned char mask)
{
    return (*z->Fp & mask) ? "1":"0";
}

void gui_gtk_set_regs(Z80 *z)
{
    char bf[10];

    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_PC]), gui_hex2(z->PCp,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_SP]), gui_hex2(z->SPp,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_AF]), gui_hex2(*z->AFp,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_AF_]),gui_hex2(z->AltAF ? z->AFr : z->AFr_,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_BC]), gui_hex2(*z->BCp,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_BC_]),gui_hex2(z->Alt ? z->BCr : z->BCr_,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_DE]), gui_hex2(*z->DEp,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_DE_]),gui_hex2(z->Alt ? z->DEr : z->DEr_,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_HL]), gui_hex2(z->Alt ? z->HLr_ : z->HLr,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_HL_]),gui_hex2(z->Alt ? z->HLr : z->HLr_,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_IX]), gui_hex2(z->IXp,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_IY]), gui_hex2(z->IYp,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_I]),  gui_hex(z->Ip,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_REG_R]),  gui_hex(z->Rp,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_IM]),     gui_hex(z->IMr,bf));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_FE]),     gui_hex(((e800j*)z->cust)->reg373,bf));

    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_SF]), 	   gui_deb_test(z,0x80));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_ZF]),     gui_deb_test(z,0x40));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_5F]),     gui_deb_test(z,0x20));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_HF]),     gui_deb_test(z,0x10));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_3F]),     gui_deb_test(z,0x08));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_PF]),     gui_deb_test(z,0x04));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_NF]),     gui_deb_test(z,0x02));
    gtk_label_set(GTK_LABEL(gtk_widgets[DEBUGGER_CF]),     gui_deb_test(z,0x01));
}

void gui_gtk_bt_remove(GtkWidget *wg, GtkWidget *tg)
{
    char *txt;
    
    gtk_clist_get_text(GTK_CLIST(gtk_widgets[DEBUGGER_BP]),bp_row_selected,0,&txt);
    if(txt)
	gtk_clist_remove(GTK_CLIST(gtk_widgets[DEBUGGER_BP]),bp_row_selected);
}


void gui_gtk_eqsel(GtkWidget *wg, char *ptr)
{
    char *clist[2];
    char *ttype[3] = {"==",">=","<="};
    char tmpstr[8];
    
    switch(*ptr){
	case '0': trap_type = 0; break;
	case '1': trap_type = 1; break;
	case '2': trap_type = 2; break;
	case '3': trap_addr = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wg)); break;
	case '4': clist[1] = ttype[(int)trap_type]; gui_hex2(trap_addr,tmpstr); clist[0] = tmpstr;
		  gtk_clist_append(GTK_CLIST(gtk_widgets[DEBUGGER_BP]), clist); break;
    }
}



void gui_gtk_bt_add(GtkWidget *wg, GtkWidget *tg)
{
    GtkWidget *win,*hbox, *vbox, *menu, *omenu, *el;
    void *gr = NULL;
    GtkAdjustment *adj;
    
    trap_type=0; trap_addr=0;
    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win),"Dodaj punkt zatrzymania");    
    gtk_signal_connect(GTK_OBJECT(win),"destroy",GTK_SIGNAL_FUNC(gtk_widget_destroy),NULL);
    vbox = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(win),vbox);
    hbox = gtk_hbox_new(FALSE,5);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,10);
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("PC "));
    omenu = gtk_option_menu_new();
    gtk_container_add(GTK_CONTAINER(hbox),omenu);
    menu = gtk_menu_new();
    el = gtk_radio_menu_item_new_with_label(gr,"==");    
    gr = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(el));
    gtk_menu_append(GTK_MENU(menu),el);
    gtk_signal_connect(GTK_OBJECT(el),"activate",GTK_SIGNAL_FUNC(gui_gtk_eqsel),"0");
    el = gtk_radio_menu_item_new_with_label(gr,">=");    
    gr = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(el));
    gtk_menu_append(GTK_MENU(menu),el);
    gtk_signal_connect(GTK_OBJECT(el),"activate",GTK_SIGNAL_FUNC(gui_gtk_eqsel),"1");
    el = gtk_radio_menu_item_new_with_label(gr,"<=");    
    gr = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(el));
    gtk_menu_append(GTK_MENU(menu),el);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(omenu), menu);
    gtk_signal_connect(GTK_OBJECT(el),"activate",GTK_SIGNAL_FUNC(gui_gtk_eqsel),"2");
    adj = GTK_ADJUSTMENT(gtk_adjustment_new( 0,0,0xffff,1,16,16));
    el = gtk_spin_button_new(adj,1,0);
    gtk_signal_connect(GTK_OBJECT(el),"value_changed",GTK_SIGNAL_FUNC(gui_gtk_eqsel),"3");
    gtk_container_add(GTK_CONTAINER(hbox),el);
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,3);
    el = gtk_button_new_with_label("Rezygnuj");
    gtk_box_pack_end(GTK_BOX(hbox),el,FALSE,FALSE,3);
    gtk_signal_connect_object(GTK_OBJECT(el),"clicked",GTK_SIGNAL_FUNC(gtk_widget_destroy),win);
    el = gtk_button_new_with_label("OK");
    gtk_box_pack_end(GTK_BOX(hbox),el,FALSE,FALSE,3);
    gtk_signal_connect_object_after(GTK_OBJECT(el),"clicked",GTK_SIGNAL_FUNC(gtk_widget_destroy),win);
    gtk_signal_connect(GTK_OBJECT(el),"clicked",GTK_SIGNAL_FUNC(gui_gtk_eqsel),"4");
    gtk_widget_show_all(win);    
}

void gui_gtk_break_points(GtkWidget *wg)
{
    GtkWidget *cl,*hb,*bt,*select;
    char *ctitles[] = {"Adres","Warunek"};
    
    select = gtk_scrolled_window_new(FALSE,FALSE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(select),GTK_POLICY_AUTOMATIC,GTK_POLICY_ALWAYS);
    gtk_box_pack_start(GTK_BOX(wg),select, TRUE,TRUE,0);
    cl = gtk_clist_new_with_titles(2, ctitles);
    gtk_clist_set_column_width(GTK_CLIST(cl),0,60);
    gtk_container_add(GTK_CONTAINER(select),cl);
    gtk_signal_connect(GTK_OBJECT(cl),"select_row",GTK_SIGNAL_FUNC(gui_gtk_bp_row_deasm),NULL);
    gtk_widgets[DEBUGGER_BP]=cl;
    hb = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(wg),hb,FALSE,FALSE,0);    
    bt = gtk_button_new_with_label("Usun");
    gtk_container_add(GTK_CONTAINER(hb),bt);
    gtk_signal_connect(GTK_OBJECT(bt),"clicked",GTK_SIGNAL_FUNC(gui_gtk_bt_remove),wg);
    bt = gtk_button_new_with_label("Dodaj");
    gtk_container_add(GTK_CONTAINER(hb),bt);
    gtk_signal_connect(GTK_OBJECT(bt),"clicked",GTK_SIGNAL_FUNC(gui_gtk_bt_add),wg);
}

void gui_gtk_regstat(GtkWidget *wg, Z80 *z)
{
    GtkWidget *vbox, *hbox, *vv,*mv,*zv;
    char bf[10];
    
    mv = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(wg),mv,FALSE,FALSE,5);    

    hbox = gtk_frame_new("Stan rejestrow");
    gtk_box_pack_start(GTK_BOX(mv),hbox,FALSE,FALSE,10);
    vbox = gtk_vbox_new(FALSE,5);
    gtk_container_add(GTK_CONTAINER(hbox),vbox);    

    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("PC "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_PC] = gtk_label_new(gui_hex2(0,bf)));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("  SP "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_SP] = gtk_label_new(gui_hex2(0,bf)));
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("AF "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_AF] = gtk_label_new(gui_hex2(0,bf)));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("  AF' "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_AF_] = gtk_label_new(gui_hex2(0,bf)));
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("BC "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_BC] = gtk_label_new(gui_hex2(0,bf)));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("  BC' "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_BC_] = gtk_label_new(gui_hex2(0,bf)));
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("DE "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_DE] = gtk_label_new(gui_hex2(0,bf)));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("  DE' "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_DE_] = gtk_label_new(gui_hex2(0,bf)));
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("HL "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_HL] = gtk_label_new(gui_hex2(0,bf)));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("  HL' "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_HL_] = gtk_label_new(gui_hex2(0,bf)));
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("IX "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_IX] = gtk_label_new(gui_hex2(0,bf)));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("   IY "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_IY] = gtk_label_new(gui_hex2(0,bf)));
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new(" I "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_I] = gtk_label_new(gui_hex(0,bf)));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("    R "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_REG_R] = gtk_label_new(gui_hex(0,bf)));
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("IM "));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_IM] = gtk_label_new(gui_hex(0,bf)));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_label_new("  OUT#FE ="));
    gtk_container_add(GTK_CONTAINER(hbox),gtk_widgets[DEBUGGER_FE] = gtk_label_new(gui_hex(0,bf)));
    
/* znaczniki */
    vv = gtk_frame_new("Znaczniki");
    gtk_box_pack_start(GTK_BOX(mv),vv,FALSE,FALSE,0);
    hbox = gtk_hbox_new(FALSE,0);
    zv = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(vv),zv);
    gtk_box_pack_start(GTK_BOX(zv),hbox,FALSE,FALSE,0);
    vv = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),vv,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(vv),gtk_label_new("S"));
    gtk_container_add(GTK_CONTAINER(vv),gtk_widgets[DEBUGGER_SF] = gtk_label_new("0"));
    vv = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),vv,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(vv),gtk_label_new("Z"));
    gtk_container_add(GTK_CONTAINER(vv),gtk_widgets[DEBUGGER_ZF] = gtk_label_new("0"));
    vv = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),vv,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(vv),gtk_label_new("5"));
    gtk_container_add(GTK_CONTAINER(vv),gtk_widgets[DEBUGGER_5F] = gtk_label_new("0"));
    vv = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),vv,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(vv),gtk_label_new("H"));
    gtk_container_add(GTK_CONTAINER(vv),gtk_widgets[DEBUGGER_HF] = gtk_label_new("0"));
    vv = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),vv,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(vv),gtk_label_new("3"));
    gtk_container_add(GTK_CONTAINER(vv),gtk_widgets[DEBUGGER_3F] = gtk_label_new("0"));
    vv = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),vv,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(vv),gtk_label_new("P"));
    gtk_container_add(GTK_CONTAINER(vv),gtk_widgets[DEBUGGER_PF] = gtk_label_new("0"));
    vv = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),vv,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(vv),gtk_label_new("N"));
    gtk_container_add(GTK_CONTAINER(vv),gtk_widgets[DEBUGGER_NF] = gtk_label_new("0"));
    vv = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),vv,FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(vv),gtk_label_new("C"));
    gtk_container_add(GTK_CONTAINER(vv),gtk_widgets[DEBUGGER_CF] = gtk_label_new("0"));
    gui_gtk_set_regs(z);
    gui_gtk_break_points(mv);
}

void gui_gtk_save_console(GtkWidget *sel)
{
    char *buffer;
    GtkTextIter start, end;
    FILE *f;
    const char *fname;

    fname = gtk_file_selection_get_filename(GTK_FILE_SELECTION(sel));

    if(!(f = fopen(fname,"w"))){
	gui_gtk_msgbox("Nie mozna zapisac pliku:",fname);
	return;
    }
    
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(gtk_widgets[DEBUGGER_TEXT_BUFFER]), &start, &end);
    buffer = gtk_text_iter_get_text(&start, &end);
    if(!buffer) return;
    fputs(buffer, f);
    g_free(buffer);
    fclose(f);
}

void gui_debug_setconsole(GtkWidget *wg, char *ptr)
{
    GtkWidget *sel;
    GtkTextIter start, end;    

    switch(*ptr){
	case '0': gui_send_cmd(GTK_TOGGLE_BUTTON(wg)->active ? "wpout 1\n":"wpout 0\n"); break;
	case '1': gui_send_cmd(GTK_TOGGLE_BUTTON(wg)->active ? "wstdout 1\n":"wstdout 0\n"); break;
	case '2': sel = gtk_file_selection_new("Zapisz jako"); gtk_widget_show(sel);
		  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(sel)->cancel_button),
		  "clicked",GTK_SIGNAL_FUNC(gtk_widget_destroy),sel);
		  gtk_signal_connect_object_after(GTK_OBJECT(GTK_FILE_SELECTION(sel)->ok_button),
		  "clicked",GTK_SIGNAL_FUNC(gtk_widget_destroy),sel);
		  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(sel)->ok_button),
		  "clicked",GTK_SIGNAL_FUNC(gui_gtk_save_console),sel);
		  break;
	case '3': gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(gtk_widgets[DEBUGGER_TEXT_BUFFER]), &start, &end);
		  gtk_text_buffer_delete(GTK_TEXT_BUFFER(gtk_widgets[DEBUGGER_TEXT_BUFFER]), &start, &end);
    }
}

void gui_gtk_console(GtkWidget *wg)
{
    GtkWidget *vb, *vv,*ed,*sw, *fr,*hb;

    vb=gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(wg),vb);

    hb = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vb),hb,FALSE,FALSE,0);

    vv = gtk_check_button_new_with_label("Rejestruj");
    gtk_box_pack_start(GTK_BOX(hb),vv,FALSE,FALSE,5);
    gtk_signal_connect(GTK_OBJECT(vv),"toggled",GTK_SIGNAL_FUNC(gui_debug_setconsole),"0");
    vv = gtk_check_button_new_with_label("Kopia na stdout");
    gtk_box_pack_start(GTK_BOX(hb),vv,FALSE,FALSE,5);
    gtk_signal_connect(GTK_OBJECT(vv),"toggled",GTK_SIGNAL_FUNC(gui_debug_setconsole),"1");
    vv = gtk_button_new_with_label("Zapisz");
    gtk_box_pack_end(GTK_BOX(hb),vv,FALSE,FALSE,0);
    gtk_signal_connect(GTK_OBJECT(vv),"clicked",GTK_SIGNAL_FUNC(gui_debug_setconsole),"2");
    vv = gtk_button_new_with_label("Czysc");
    gtk_box_pack_end(GTK_BOX(hb),vv,FALSE,FALSE,0);
    gtk_signal_connect(GTK_OBJECT(vv),"clicked",GTK_SIGNAL_FUNC(gui_debug_setconsole),"3");

    fr = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(fr),GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(vb),fr); 
    sw=gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(fr),sw);    
    gtk_widgets[DEBUGGER_TEXT_VIEW] = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(sw),gtk_widgets[DEBUGGER_TEXT_VIEW]);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gtk_widgets[DEBUGGER_TEXT_VIEW]),FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(gtk_widgets[DEBUGGER_TEXT_VIEW]),FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(gtk_widgets[DEBUGGER_TEXT_VIEW]),FALSE);

    gtk_widgets[DEBUGGER_TEXT_BUFFER] = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_widgets[DEBUGGER_TEXT_VIEW]));

    ed=gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vb),ed,FALSE,FALSE,0);    
    gtk_entry_set_editable(GTK_ENTRY(ed),TRUE);
    gtk_signal_connect(GTK_OBJECT(ed),"activate",GTK_SIGNAL_FUNC(gui_gtk_ed_ed),NULL);
    gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(gtk_widgets[DEBUGGER_TEXT_BUFFER]),"tt","family",
							    "monospace", "foreground","blue",NULL);
}


void gui_gtk_add_bp(GtkWidget *wg)
{
    char *clist[3];
    char tmpstr[8];
    char *adr;

    gtk_clist_get_text(GTK_CLIST(gtk_widgets[DEBUGGER_DEASM]),deasm_row_selected,0,&adr);    
    strcpy(tmpstr,"0x");
    strcpy(tmpstr+2,adr);
    clist[0] = tmpstr;
    clist[1] = "==";
    gtk_clist_append(GTK_CLIST(gtk_widgets[DEBUGGER_BP]), clist);

}


void gui_gtk_deasm(GtkWidget *wg)
{
    GtkWidget *cl,*vb,*bt,*select;
    char *ctitles[] = {"Adres","Instrukcja"};
    
    vb = gtk_vbox_new(FALSE,10);
    gtk_box_pack_start(GTK_BOX(wg),vb,TRUE,TRUE,10);
    select = gtk_scrolled_window_new(FALSE,FALSE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(select),GTK_POLICY_AUTOMATIC,GTK_POLICY_ALWAYS);
    gtk_box_pack_start(GTK_BOX(vb),select, TRUE,TRUE,0);
    cl = gtk_clist_new_with_titles(2,ctitles);
    gtk_container_add(GTK_CONTAINER(select),cl);
    gtk_signal_connect(GTK_OBJECT(cl),"select_row",GTK_SIGNAL_FUNC(gui_gtk_select_row_deasm),NULL);
    gtk_widgets[DEBUGGER_DEASM] = cl;
    bt = gtk_button_new_with_label("<--- Dodaj ");
    gtk_box_pack_start(GTK_BOX(vb),bt,FALSE,FALSE,3);
    gtk_signal_connect(GTK_OBJECT(bt),"clicked",GTK_SIGNAL_FUNC(gui_gtk_add_bp),cl);

}

void gui_gtk_memo(GtkWidget *wg)
{

}

void gui_gtk_break(GtkWidget *wg)
{
    z80_set_run(NULL,0);
    z80_set_debug(z80_get_struct(),1);
}

void gui_gtk_continue(GtkWidget *wg)
{

    char *addr,*cond;
    char tmpstr[16];
    
    gtk_clist_get_text(GTK_CLIST(gtk_widgets[DEBUGGER_BP]),bp_row_selected,0,&addr);
    gtk_clist_get_text(GTK_CLIST(gtk_widgets[DEBUGGER_BP]),bp_row_selected,1,&cond);	

    if( *(cond + 1) == '=' ) {
	if( (*addr == '0') && (*(addr+1) == 'x') ) addr += 2;
	switch(*cond){
	    case '=' : sprintf(tmpstr,"g %s\n",addr);   break;
	    case '>' : sprintf(tmpstr,"ghe %s\n",addr); break;
	    case '<' : sprintf(tmpstr,"gle %s\n",addr); break;
	}
	gui_send_cmd(tmpstr);    
	gtk_clist_remove(GTK_CLIST(gtk_widgets[DEBUGGER_BP]),bp_row_selected);	
    } else {
	z80_set_run(NULL, 1);
	z80_set_debug(z80_get_struct(), 0);
    }
}


void gui_gtk_debugger(GtkWidget *parwg, e8_gui *gui)
{
    GtkWidget *wg, *vbox,*hbox, *bt, *nb;
    GtkWidget *page;

    memset(cmd_buffer,0,sizeof(cmd_buffer));
    DEBUGGER_RESET(z80_get_struct());

    deasm_row_selected = 0;
    bp_row_selected = 0;    

    wg=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_signal_connect(GTK_OBJECT(wg),"destroy",GTK_SIGNAL_FUNC(gui_gtk_debugger_destroy),NULL);
    gtk_window_set_title(GTK_WINDOW(wg),"Z80 debugger");
    gtk_widget_set_usize(wg,400,450);

    vbox = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(wg),vbox);

    nb = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(vbox),nb);
    page = gtk_vbox_new(FALSE,0);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),page,gtk_label_new("Trap"));
    hbox = gtk_hbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(page),hbox);
    gui_gtk_regstat(hbox,((e800j *)gui->e8)->cpu);
    gui_gtk_deasm(hbox); 
    page = gtk_vbox_new(FALSE,0);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),page,gtk_label_new("Konsola"));
    gui_gtk_console(page); 
//    page = gtk_vbox_new(FALSE,0);
//    gtk_notebook_append_page(GTK_NOTEBOOK(nb),page,gtk_label_new("Pamiec"));
//    gui_gtk_memo(page);    
     
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,5);
    bt = gtk_button_new_with_label("Zamknij");
    gtk_box_pack_end(GTK_BOX(hbox),bt,FALSE,FALSE,0);
    gtk_signal_connect_object(GTK_OBJECT(bt),"clicked",GTK_SIGNAL_FUNC(gui_gtk_debugger_destroy),wg);
    bt = gtk_button_new_with_label("Pauza");
    gtk_box_pack_end(GTK_BOX(hbox),bt,FALSE,FALSE,0);
    gtk_signal_connect(GTK_OBJECT(bt),"clicked",GTK_SIGNAL_FUNC(gui_gtk_break),gui);
    bt = gtk_button_new_with_label("Kontynuacja");
    gtk_box_pack_end(GTK_BOX(hbox),bt,FALSE,FALSE,0);
    gtk_signal_connect(GTK_OBJECT(bt),"clicked",GTK_SIGNAL_FUNC(gui_gtk_continue),gui);
    bt = gtk_button_new_with_label("Krok");
    gtk_box_pack_end(GTK_BOX(hbox),bt,FALSE,FALSE,0);
    gtk_signal_connect_object(GTK_OBJECT(bt),"clicked",GTK_SIGNAL_FUNC(gui_send_cmd),"wt\n");
    gtk_widget_show_all(wg);
    z80_set_run(NULL,1);
    z80_set_debug(((e800j *)gui->e8)->cpu,1);
    gui_send_cmd("dummy\n");
}

