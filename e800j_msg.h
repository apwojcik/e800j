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

#pragma once

/*
 zmienna globalna definiująca gdzie komunikaty mają zostać wysyłane
 wartość 0 (domyślna) to stdout
*/
extern char msg_stream;

/*
 zmienna globalna definiująca poziom wysyłanych komunikatów
 domyslną wartością jest 0, czyli jedynie błędy krytyczne będą wysyłane
*/
extern char msg_level;

/*
    wysłanie komunikatu
*/
extern void msg_printf(const char level, const char* fmt, ...);

/* poziomy komunikatów */
enum {
    MSG_CRITICAL_ERROR, /* Błędy krytyczne dla działania programu */
    MSG_ERROR, /* Błędy, które pozwalają emulacji mimo to działać dalej */
    MSG_WARN, /* Ostrzeżenia */
    MSG_MSG, /* Komunikaty poziom 0 */
    MSG_VERB_MSG, /* Komunikaty poziom 1 */
    MSG_NVERB_MSG, /* Komunikaty poziom 2 */
    MSG_UNVERB_MSG /* Komunikaty poziom 3 */
};