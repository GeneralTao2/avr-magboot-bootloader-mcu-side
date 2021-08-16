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

/* !!! Works only with boud rate 9600 and less */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"

#define UART_START_BIT 0u
#define UART_STOP_BIT 1u

#define UART_BIT_MASK 0x01
#define UART_BITS_IN_BYTE 8u
#define UART_BIT_STEP 1u
#define UART_MSB_MASK (1 << 7)

#define UART_PRESCALER 8u

/* Calculate UART_BAUD_TIME using timer pre-scaler 64 */
#define UART_BAUD_TIME ( (F_CPU / UART_PRESCALER) / BAUD_RATE)

#if (UART_BAUD_TIME > 255)
#error Baudrate is too low!
#endif

#if (UART_BAUD_TIME < 1)
#error Baudrate is too high!
#endif

static void UART_BitDelayFromCurrentTCNT(void);
static void UART_BitDelayFromErasedTCNT(void);
static void UART_BitHalfDelayFromErasedTCNT(void);
static uint8_t UART_Read_RX_Pin(void);
static void UART_Read_TX_Pin(uint8_t state);
static int16_t UART_Read_TX_CharBody(void);

static void UART_BitDelayFromCurrentTCNT(void)
{
    /* Timer pre-scaler 8 on */
	TCCR0 |= _BV(CS01); 
	while ( (TCNT0 < UART_BAUD_TIME) ) 
    { 
        /* Nothing to do */ 
    }
    /* off */
	TCCR0 &= ~_BV(CS01);
}

static void UART_BitDelayFromErasedTCNT(void)
{
	TCNT0 = 0u;
	UART_BitDelayFromCurrentTCNT();
}

static void UART_BitHalfDelayFromErasedTCNT(void)
{
	TCNT0 = UART_BAUD_TIME / 2;
	UART_BitDelayFromCurrentTCNT();
}

static uint8_t UART_Read_RX_Pin(void) 
{
    uint8_t retVal = 0u;
    retVal = (CFG_SWUART_RX_PIN & _BV(CFG_SWUART_RX_BIT)) != 0;
	return retVal;
}  

static void UART_Read_TX_Pin(uint8_t state)
{
	if (state == 1u)
    {
		CFG_SWUART_TX_PORT |= _BV(CFG_SWUART_TX_BIT);
    }
	else
    {
		CFG_SWUART_TX_PORT &= ~(_BV(CFG_SWUART_TX_BIT));
    }
}

/* 8N1 transmit */
void UART_WriteChar(uint8_t ch)
{
	uint8_t bitIdx = 0u;

	UART_Read_TX_Pin(UART_START_BIT);
	UART_BitDelayFromErasedTCNT();
	for (bitIdx = 0u; bitIdx < UART_BITS_IN_BYTE; bitIdx++) 
    {
		UART_Read_TX_Pin(ch & UART_BIT_MASK);
		ch >>= UART_BIT_STEP;
		UART_BitDelayFromErasedTCNT();
	}
	UART_Read_TX_Pin(UART_STOP_BIT);
	UART_BitDelayFromErasedTCNT();
}

static int16_t UART_Read_TX_CharBody(void)
{
    int16_t retVal = 0;
	uint8_t bitIdx = 0u;
	uint8_t ch = 0;

	UART_BitHalfDelayFromErasedTCNT();
	if (UART_Read_RX_Pin() != UART_START_BIT)
    { 
        /* Glitch */
		retVal = -1; 
    }
    if(retVal == 0)
    {
        UART_BitDelayFromErasedTCNT();
        for (bitIdx = 0; bitIdx < 8; bitIdx++) 
        {
            ch >>= UART_BIT_STEP;
            if ( UART_Read_RX_Pin() )
            {
                ch |= UART_MSB_MASK;
            }
            UART_BitDelayFromErasedTCNT();
        }

        if (UART_Read_RX_Pin() != UART_STOP_BIT)
        {
            /* Frame error */
            retVal = -2; 
        }
    }
	return (int16_t)ch;
}

/* 8N1 receive */
uint8_t UART_ReadChar(void)
{
	while (1) 
    {
		int16_t ch = 0;
		while (UART_Read_RX_Pin() != UART_START_BIT) 
        { 
            /* Nothing to do */ 
        }

		ch = UART_Read_TX_CharBody();
		if (ch >= 0)
        {   
			return (uint8_t)ch;
        }
	}
}

void UART_Init(void)
{
    TCCR0 = 0u;
    OCR0 = 0u;
    TIMSK = 0u;
    TCNT0 = 0u;
    OCR0 = 0u;
    TIFR = 0u;

	CFG_SWUART_RX_DIR  &= ~(_BV(CFG_SWUART_RX_BIT));
	CFG_SWUART_RX_PORT |= _BV(CFG_SWUART_RX_BIT);
	CFG_SWUART_TX_DIR  |= _BV(CFG_SWUART_TX_BIT);

	UART_Read_TX_Pin(UART_STOP_BIT);
}