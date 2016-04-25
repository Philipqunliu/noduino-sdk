/*
 * Copyright (c) 2015 - 2025 MaiKe Labs
 * Copyright (c) 2015 Hristo Gochkov. All
 * rights reserved.
 * 
 * Software I2C library for esp8266
 * 
 * Porting from the esp8266 core for Arduino environment.
 * 
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __TWI_H__
#define __TWI_H__ 

#ifdef __cplusplus
extern "C" {
#endif

void wire_begin();
void wire_setClock(uint32_t clk);
void wire_beginTransmission(uint8_t addr);
size_t wire_write(uint8_t data);
uint8_t wire_endTransmission();
uint8_t wire_requestFrom(uint8_t addr, size_t len);
int wire_read();
int wire_available(void);

void twi_init (unsigned char sda, unsigned char scl);
void twi_stop (void);
void twi_setClock (unsigned int freq);
uint8_t	twi_writeTo (unsigned char address, unsigned char *buf, unsigned int len, unsigned char sendStop);
uint8_t	twi_readFrom (unsigned char address, unsigned char *buf, unsigned int len, unsigned char sendStop);

#ifdef __cplusplus
}
#endif

#endif
