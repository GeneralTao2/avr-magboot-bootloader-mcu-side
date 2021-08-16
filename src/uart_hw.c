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

#include <avr/io.h>
#include "uart.h" 

#define UART_UBRR (uint16_t)( ( (F_CPU / 16 / BAUD_RATE) ) - 1)

void UART_WriteChar(uint8_t ch)
{ 
	loop_until_bit_is_set(UCSRA, UDRE);   
	UDR = ch;
}

uint8_t UART_ReadChar(void)
{
	uint8_t ch;   

	loop_until_bit_is_set(UCSRA, RXC);
	ch = UDR;

	return ch;
}

void UART_Init(void)
{
	UCSRB = _BV(RXEN) | _BV(TXEN);
	UCSRC = _BV(UCSZ0) | _BV(UCSZ1);
	UBRRH = (uint8_t)(UART_UBRR >> 8);
	UBRRL = (uint8_t)(UART_UBRR); 
}
