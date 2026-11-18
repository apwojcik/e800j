/* Elwro 800 Junior emulator v0.3
 * Copyright (C) 2026 Artur Wojcik
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

/* Wymiana dyskietek w trakcie pracy emulatora (klawisze F5/F6) */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <SDL3/SDL.h>

struct elwro;

/*
 F5/F6 - wymiana dyskietki w napędzie A/B (okno wyboru pliku lub, gdy ustawiono diskdir,
 kolejny plik *.dsk z katalogu), Shift - wysunięcie, Alt (macOS: Opt) - przełączenie RO/RW,
 Win (macOS: Ctrl) - nowa pusta dyskietka: okno zapisu pliku pyta o nazwę i miejsce, powstaje obraz
 bez ścieżek (do sformatowania w emulowanym komputerze), wkładany w trybie RW.
 Zwraca true, jeśli klawisz został obsłużony.
*/
bool disk_hotkey(struct elwro* e, SDL_Keycode key, SDL_Keymod mod);

/* włożenie upuszczonego na okno pliku *.dsk do napędu A (0) lub B (1), tryb RO/RW jak ustawiony dla napędu */
void disk_drop(struct elwro* e, int drive, const char* path);

/* obsługa zdarzenia z okna wyboru pliku, zwraca true, jeśli zdarzenie było przeznaczone dla modułu */
bool disk_event(struct elwro* e, const SDL_Event* event);

/* stan napędów A i B ("<prefix>A: nazwa (RW)" / "<prefix>A: no disk"), w jednym wierszu */
void disk_status(const struct elwro* e, const char* prefix, char* buf, size_t size);

/* komunikat ze stanem obu napędów, pokazywany po uruchomieniu emulatora */
void disk_announce(struct elwro* e);
