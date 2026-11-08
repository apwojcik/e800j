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

struct elwro;

unsigned char mem_zx48_read(const void*, unsigned short addr);
void mem_zx48_write(const void*, unsigned short addr, unsigned char data);
unsigned char mem_zx128_read(const void*, unsigned short addr);
void mem_zx128_write(const void*, unsigned short addr, unsigned char data);
unsigned char mem_e800j_read(const void*, unsigned short addr);
void mem_e800j_write(const void*, unsigned short addr, unsigned char data);
