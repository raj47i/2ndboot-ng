/*
 *  A module for creating rebootless custom image boot support.
 *
 *  Copyright (C) 2010 XVilka <xvilka at gmail.com>
 *
 *  Inspired by 2ndboot by dimich: http://hg.ezxdev.org/2ndboot/
 * 
 *  This file is part of 2ndboot-ng.
 *
 *  2ndboot-ng is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Foobar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *   along with 2ndboot-ng.  If not, see <http://www.gnu.org/licenses/>.
 * 
*/

#ifndef __CRC32_H__
#define __CRC32_H__

void crc32_init_ctx(uint32_t *ctx);
void crc32_update(uint32_t *ctx, const uint8_t *data, size_t size);
void crc32_final(uint32_t *ctx);
uint32_t crc32(const uint8_t *data, size_t size);

#endif // __CRC32_H__
