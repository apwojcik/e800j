/* Elwro 800 Junior emulator v0.3
 * Copyright (C) 2006 Krzysztof Komarnicki
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

#pragma once

#include <stdbool.h>

struct config;

/*
 Przetwarzanie linii poleceń w dwóch przebiegach nad tym samym kontekstem (struct config):

 1. cmdline_preparse() - przed wczytaniem pliku konfiguracji: wybiera tylko to, co jest potrzebne
    do jego wczytania (--config, --profile, --log-level, --help). Pozostałe opcje pomija
    bez komunikatów. Zwraca false tylko przy brakującej/zbyt długiej wartości --config/--profile.
 2. config_load() (e800j_cfg.h) wypełnia kontekst z pliku wskazanego przez --config.
 3. cmdline_parse() - pełny przebieg po wczytaniu konfiguracji: sprawdza wszystkie opcje
    i argumenty (snapshot) i nadpisuje nimi wartości z pliku konfiguracji.

 Zwracają false i wypisują komunikat (msg_printf) przy nieznanej opcji, braku lub błędnej
 wartości opcji albo nadmiarowym argumencie. Po napotkaniu opcji pomocy ustawiają 'help'
 i kończą przetwarzanie.
*/
bool cmdline_preparse(struct config* cfg, int argc, const char** argv);
bool cmdline_parse(struct config* cfg, int argc, const char** argv);
