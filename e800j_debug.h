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

#ifndef __e800j_debug_h__
#define __e800j_debug_h__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include "e800j_kbrd.h"
#include "e800j_file.h"
#include "e800j_main.h"
#include "z80_debug.h"
#include "e800j_scrn.h"
#include "e800j_gui.h"

void gui_debug_out(char *a);
void gui_debug_in(char *a);
void ed_ed(GtkWidget *wg);
void debugger_destroy(GtkWidget *wg);
void gui_gtk_debugger(GtkWidget *parwg, e8_gui *gui);

#endif
