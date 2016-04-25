/*
 * Copyright (c) 2015 - 2025 MaiKe Labs
 * Copyright (c) 2015 Hristo Gochkov. All rights reserved
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

#include "noduino.h"

uint8_t twi_dcount = 18;
static uint8_t twi_sda = 4, twi_scl = 5;
static uint32_t twi_clockStretchLimit;

//Enable SDA(becomes output and since GPO is 0 for the pin, it will pull the line low)
#define SDA_LOW()   (GPES = (1 << twi_sda))
//Disable SDA(becomes input and since it has pullup it will go high)
#define SDA_HIGH()  (GPEC = (1 << twi_sda))
#define SDA_READ()  ((GPI & (1 << twi_sda)) != 0)
#define SCL_LOW()   (GPES = (1 << twi_scl))
#define SCL_HIGH()  (GPEC = (1 << twi_scl))
#define SCL_READ()  ((GPI & (1 << twi_scl)) != 0)

#ifndef FCPU80
#define FCPU80 80000000L
#endif

#if F_CPU == FCPU80
#define TWI_CLOCK_STRETCH_MULTIPLIER 3
#else
#define TWI_CLOCK_STRETCH_MULTIPLIER 6
#endif

void ICACHE_FLASH_ATTR twi_setClock(unsigned int freq)
{
#if F_CPU == FCPU80
	//about 100 KHz
	if (freq <= 100000)
		twi_dcount = 19;
	//about 200 KHz
	else if (freq <= 200000)
		twi_dcount = 8;
	//about 300 KHz
	else if (freq <= 300000)
		twi_dcount = 3;
	//about 400 KHz
	else if (freq <= 400000)
		twi_dcount = 1;
	//about 400 KHz
	else
		twi_dcount = 1;
#else
	if (freq <= 100000)
		twi_dcount = 32;
	//about 100 KHz
	else if (freq <= 200000)
		twi_dcount = 14;
	//about 200 KHz
	else if (freq <= 300000)
		twi_dcount = 8;
	//about 300 KHz
	else if (freq <= 400000)
		twi_dcount = 5;
	//about 400 KHz
	else if (freq <= 500000)
		twi_dcount = 3;
	//about 500 KHz
	else if (freq <= 600000)
		twi_dcount = 2;
	//about 600 KHz
	else
		twi_dcount = 1;
	//about 700 KHz
#endif
}

void ICACHE_FLASH_ATTR twi_setClockStretchLimit(uint32_t limit){
	twi_clockStretchLimit = limit * TWI_CLOCK_STRETCH_MULTIPLIER;
}

void ICACHE_FLASH_ATTR twi_init(uint8_t pin_sda, uint8_t pin_scl)
{
	twi_sda = pin_sda;
	twi_scl = pin_scl;
	pinMode(twi_sda, INPUT_PULLUP);
	pinMode(twi_scl, INPUT_PULLUP);
	twi_setClock(100000);
	twi_setClockStretchLimit(230); // default value is 230 uS
}

void ICACHE_FLASH_ATTR twi_stop(void)
{
	pinMode(twi_sda, INPUT);
	pinMode(twi_scl, INPUT);
}

static void ICACHE_FLASH_ATTR twi_delay(uint8_t v)
{
	unsigned int i;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	unsigned int reg;
	for (i = 0; i < v; i++)
		reg = GPI;
#pragma GCC diagnostic pop
}

static bool ICACHE_FLASH_ATTR twi_write_start(void)
{
	SCL_HIGH();
	SDA_HIGH();
	if (SDA_READ() == 0) {
		serial_printf("twi write start sda read false\r\n");
		return false;
	}
	twi_delay(twi_dcount);
	SDA_LOW();
	twi_delay(twi_dcount);
	return true;
}

static bool ICACHE_FLASH_ATTR twi_write_stop(void)
{
	unsigned int i = 0;
	SCL_LOW();
	SDA_LOW();
	twi_delay(twi_dcount);
	SCL_HIGH();
	while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit);	// Clock stretching
	twi_delay(twi_dcount);
	SDA_HIGH();
	twi_delay(twi_dcount);

	return true;
}

static bool ICACHE_FLASH_ATTR twi_write_bit(bool bit)
{
	unsigned int i = 0;
	SCL_LOW();
	if (bit)
		SDA_HIGH();
	else
		SDA_LOW();
	twi_delay(twi_dcount + 1);
	SCL_HIGH();
	while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit);	// Clock stretching
	twi_delay(twi_dcount);
	return true;
}

static bool ICACHE_FLASH_ATTR twi_read_bit()
{
	unsigned int i = 0;
	SCL_LOW();
	SDA_HIGH();
	twi_delay(twi_dcount + 2);
	SCL_HIGH();
	while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit);	// Clock stretching
	bool bit = SDA_READ();
	twi_delay(twi_dcount);
	return bit;
}

static bool ICACHE_FLASH_ATTR twi_write_byte(uint8_t byte)
{
	uint8_t bit;
	for (bit = 0; bit < 8; bit++) {
		twi_write_bit(byte & 0x80);
		byte <<= 1;
	}
	return !twi_read_bit();
	//NACK, ACK
}

static uint8_t ICACHE_FLASH_ATTR twi_read_byte(bool nack)
{
	uint8_t byte = 0;
	uint8_t bit;
	for (bit = 0; bit < 8; bit++)
		byte = (byte << 1) | twi_read_bit();
	twi_write_bit(nack);
	return byte;
}

uint8_t ICACHE_FLASH_ATTR 
twi_writeTo(uint8_t address, uint8_t *buf, unsigned int len,
	    uint8_t sendStop)
{
	unsigned int i;
	if (!twi_write_start()) {
		serial_print("I2C: bus busy\r\n");
		return 4;
	}
	if (!twi_write_byte(((address << 1) | 0) & 0xFF)) {
		if (sendStop)
			twi_write_stop();
		serial_print("I2C: received NACK on transmit of address\r\n");
		return 2;
	}
	for (i = 0; i < len; i++) {
		if (!twi_write_byte(buf[i])) {
			if (sendStop)
				twi_write_stop();
			serial_print("I2C: received NACK on transmit of data\r\n");
			return 3;
		}
	}
	if (sendStop)
		twi_write_stop();
	i = 0;
	while (SDA_READ() == 0 && (i++) < 10) {
		SCL_LOW();
		twi_delay(twi_dcount);
		SCL_HIGH();
		twi_delay(twi_dcount);
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR 
twi_readFrom(uint8_t address, uint8_t *buf, unsigned int len,
	     uint8_t sendStop)
{
	unsigned int i;
	if (!twi_write_start()) {
		serial_print("I2C: bus busy\r\n");
		return 4;
	}
	if (!twi_write_byte(((address << 1) | 1) & 0xFF)) {
		if (sendStop)
			twi_write_stop();
		serial_print("I2C: received NACK on transmit of address\r\n");
		return 2;
	}
	for (i = 0; i < (len - 1); i++)
		buf[i] = twi_read_byte(false);
	buf[len - 1] = twi_read_byte(true);
	if (sendStop)
		twi_write_stop();
	i = 0;
	while (SDA_READ() == 0 && (i++) < 10) {
		SCL_LOW();
		twi_delay(twi_dcount);
		SCL_HIGH();
		twi_delay(twi_dcount);
	}
	return 0;
}

//////////////////////////////////////////////////////
#define WIRE_BUFFER_LEN		32
static uint8_t wire_rxBuffer[WIRE_BUFFER_LEN] = {0};
static uint8_t wire_rxBufferIndex = 0;
static uint8_t wire_rxBufferLength = 0;
static uint8_t wire_txAddress = 0;
static uint8_t wire_txBuffer[WIRE_BUFFER_LEN] = {0};
static uint8_t wire_txBufferIndex = 0;
static uint8_t wire_txBufferLength = 0;
static uint8_t wire_transmitting = 0;

void ICACHE_FLASH_ATTR wire_begin()
{
	twi_init(SDA, SCL);
}

void ICACHE_FLASH_ATTR wire_setClock(uint32_t clk)
{
	twi_setClock(clk);
}

void ICACHE_FLASH_ATTR wire_beginTransmission(uint8_t addr)
{
	wire_transmitting = 1;
	wire_txAddress = addr;
	wire_txBufferIndex = 0;
	wire_txBufferLength = 0;
}

size_t ICACHE_FLASH_ATTR wire_write(uint8_t data)
{
	if(wire_transmitting){
		if(wire_txBufferLength >= WIRE_BUFFER_LEN){
			return 0;
		}
		wire_txBuffer[wire_txBufferIndex] = data;
		++wire_txBufferIndex;
		wire_txBufferLength = wire_txBufferIndex;
	}
	return 1;
}

uint8_t ICACHE_FLASH_ATTR wire_endTransmission()
{
	int8_t ret;
	ret = twi_writeTo(wire_txAddress, wire_txBuffer, wire_txBufferLength, true);
	wire_txBufferIndex = 0;
	wire_txBufferLength = 0;
	wire_transmitting = 0;
	return ret;
}

uint8_t ICACHE_FLASH_ATTR wire_requestFrom(uint8_t addr, size_t len)
{
	if(len > WIRE_BUFFER_LEN) {
		len = WIRE_BUFFER_LEN;
	}
	size_t read = (twi_readFrom(addr, wire_rxBuffer, len, true) == 0) ? len : 0;
	wire_rxBufferIndex = 0;
	wire_rxBufferLength = read;
	return read;
}

int ICACHE_FLASH_ATTR wire_read()
{
	int value = -1;
	if(wire_rxBufferIndex < wire_rxBufferLength){
		value = wire_rxBuffer[wire_rxBufferIndex];
		++wire_rxBufferIndex;
	}
	return value;
}

int ICACHE_FLASH_ATTR wire_available(void)
{
	int result = wire_rxBufferLength - wire_rxBufferIndex;

	if (!result) {
		// yielding here will not make more data "available",
		// but it will prevent the system from going into WDT reset
		optimistic_yield(1000);
	}

	return result;
}
