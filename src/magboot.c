/* Copyright (C) 2010-2011 Magnus Olsson
 *
 * This file is part of magboot
 * magboot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * magboot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with magboot.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h> 
#include <avr/boot.h>
#include <stdbool.h>    
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>

#include "uart.h"

#define MB_WORD_MASK 0xFFFF
#define MB_BITS_IN_WORD 16

#define CMD_OK() UART_WriteChar('Y')
#define CMD_FAIL() UART_WriteChar('N') 
   
typedef void (*tv_JumpFunc)(void);

static uint16_t MB_CheckSum(uint8_t *data, size_t size);
static void MB_Jump(uint16_t addr);

void MB_CheckResetReason(void);
bool MB_CMD_Reset(void);
bool MB_CMD_LoadAddr(uint16_t *addr);
bool MB_CMD_WritePage(uint16_t *addr);
bool MB_CMD_DeviceId(void);

static uint16_t MB_CheckSum(uint8_t *data, size_t size)
{
	uint16_t *words = (uint16_t *) data;
	size_t wordsQuantity = size / 2;
	uint32_t sum = 0u;
	uint8_t idx = 0u;

	for (idx = 0u; idx < wordsQuantity; idx++)
	{
		sum += words[idx];
	}
	/* Fold */
	while (sum >> MB_BITS_IN_WORD)
	{	
		sum = (sum & MB_WORD_MASK) + (sum >> MB_BITS_IN_WORD);
	}

	return (uint16_t) sum;
}

static void MB_Jump(uint16_t addr)
{
	tv_JumpFunc func = 0;

	/* Clear WDRF to prevent "infinite reset loop". Do not rely on application
	 * to clear it, since it may not be WDT-aware and cannot account for the WDT
	 * usage of Magboot. The downside of this is however that an application can
	 * never detect WDT reset during initialization.
	 */
	MCUSR &= ~(_BV(WDRF));
	wdt_disable();
	func = (tv_JumpFunc) addr;
	func();
}

void MB_CheckResetReason(void)
{
	if ( bit_is_clear(MCUSR, EXTRF) ) 
	{
		/* Bypass magboot if reset caused by watchdog, power-on or brown-out */
		MB_Jump(0);
	} else
	{
		MCUSR &= ~(_BV(EXTRF));
	}  
}

bool MB_CMD_Reset(void)
{
	wdt_enable(WDTO_15MS);
	while (1);

	return false; /* Unreachable */
}

bool MB_CMD_LoadAddr(uint16_t *addr)
{
	/* 16-bit addr, little-endian */
	(*addr) = UART_ReadChar();
	(*addr) += UART_ReadChar() << 8;
	return false;
}

bool MB_CMD_WritePage(uint16_t *addr)
{
	bool retVal = false;
	uint16_t idx = 0u;
	uint16_t page = (*addr);
	/* Pagesize + 16-bit MB_CheckSum */
	uint8_t buf[SPM_PAGESIZE] = {0};
	uint16_t expectedCheckSum = 0u;
	uint16_t actualCheckSum = 0u;

	boot_page_erase(page);

	expectedCheckSum = UART_ReadChar();
	expectedCheckSum += UART_ReadChar() << 8;

	for(idx = 0; idx < SPM_PAGESIZE; idx++)
	{
		buf[idx] = UART_ReadChar();
	}

	actualCheckSum = MB_CheckSum(buf, sizeof(buf));

	if (expectedCheckSum == actualCheckSum) 
	{
		boot_spm_busy_wait();
		for (idx = 0; idx < SPM_PAGESIZE; idx += 2) 
		{
			uint16_t w = buf[idx];
			w += buf[idx+1] << 8;
			boot_page_fill(page + idx, w);
		}

		boot_page_write(page);
		boot_spm_busy_wait();

		boot_rww_enable();

		/* Auto-increment address */
		(*addr) += SPM_PAGESIZE;
	} else
	{
		retVal = true;
	}

	return retVal;
}

bool MB_CMD_DeviceId(void)
{
	bool fail = false;

	/* Always read full signature, even if fail is detected */
	if (UART_ReadChar() != SIGNATURE_0)
	{	
		fail = true;
	}
	if (UART_ReadChar() != SIGNATURE_1)
	{	
		fail = true;
	}
	if (UART_ReadChar() != SIGNATURE_2)
	{	
		fail = true;
	}

	return fail;
}


int main(void) {
	uint16_t addr = 0;
	bool fail = true;

	LED_DIR |= _BV(LED_BIT);
	LED_PORT ^= _BV(LED_BIT);

	MB_CheckResetReason();

	wdt_enable(WDTO_250MS);

	UART_Init();
	
	while(1) 
	{
		switch ( UART_ReadChar() ) 
		{
			/* Device ID */
			case 'I':
				fail = MB_CMD_DeviceId();
				break;

			/* Address */
			case 'A':
				fail = MB_CMD_LoadAddr(&addr);
				break;

			/* Write page */
			case 'W':
				fail = MB_CMD_WritePage(&addr);
				break;

			/* Reset */
			case 'R':
				fail = MB_CMD_Reset();
				break;

			default:
				fail = true;
				break;
		}

		if (fail) 
		{	
			CMD_FAIL();
		} else
		{
			CMD_OK();
		}

		wdt_reset();
	}
}
