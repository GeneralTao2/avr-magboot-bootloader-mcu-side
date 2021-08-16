# Copyright (C) 2010-2011 Magnus Olsson
#
# This file is part of magboot
# magboot is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# magboot is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with magboot.  If not, see <http://www.gnu.org/licenses/>.
#

CC=avr-gcc
OBJDUMP=avr-objdump
OBJCOPY=avr-objcopy
STRIP=avr-strip

PROGTYPE=usbasp
#PROGPORT=/dev/avrusb0
AVRDUDE=avrdude -p $(MCU) -c $(PROGTYPE) # -P $(PROGPORT)

BUILD_FOLDER_PATH=/Build/
DBAUD_RATE=250000

UART_HW=magboot_hw
UART_SW=magboot_sw
UART_HW_HEX=magboot_hw.hex
UART_SW_HEX=magboot_sw.hex
UART_HW_BIN=magboot_hw.bin
UART_SW_BIN=magboot_sw.bin

UART_SW_C=uart_sw.c
UART_HW_C=uart_hw.c
UART_H=uart.h

COMMON_OBJS=magboot.o
UART_HW_OBJS=uart_hw.o
UART_SW_OBJS=uart_sw.o 

#OPTIMIZE = -Os -fno-inline-small-functions -fno-split-wide-types -mshort-calls
OPTIMIZE = -Os
CFLAGS += -g -Wall $(OPTIMIZE) -mmcu=$(MCU) -include config.h -D BAUD_RATE=$(DBAUD_RATE)
LDFLAGS += -Wl,--section-start=.text=$(BOOTADDR)

# Target definition
MCU=atmega32
HFUSE=0xd0
BOOTADDR=0x7000

.PHONY: all
all: $(UART_HW_HEX) $(UART_SW_HEX) $(UART_SW_BIN) $(UART_HW_BIN)

$(UART_HW): $(COMMON_OBJS) $(UART_HW_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(UART_SW): $(COMMON_OBJS) $(UART_SW_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^



$(UART_HW_HEX): $(UART_HW)
	$(OBJCOPY) -j .text -j .data -S -O ihex $^ $@

$(UART_SW_HEX): $(UART_SW)
	$(OBJCOPY) -j .text -j .data -S -O ihex $^ $@

$(UART_HW_BIN): $(UART_HW)
	$(OBJCOPY) -j .text -j .data -S -O binary $^ $@

$(UART_SW_BIN): $(UART_SW)
	$(OBJCOPY) -j .text -j .data -S -O binary $^ $@

.PHONY: flash_sw
flash_sw: $(UART_SW_HEX)
	$(AVRDUDE) -e -U flash:w:$^

.PHONY: flash_hw
flash_hw: $(UART_HW_HEX)
	$(AVRDUDE) -e -U flash:w:$^

.PHONY: fusedump
fusedump:
	$(AVRDUDE) -U hfuse:r:-:h

.PHONY: fuseprog
fuseprog:
	$(AVRDUDE) -u -U hfuse:w:$(HFUSE):m

.PHONY: flashdump
flashdump:
	$(AVRDUDE) -U flash:r:flash.bin:r

.PHONY: clean
clean:
	rm -f $(COMMON_OBJS) $(UART_HW) $(UART_HW_HEX) $(UART_HW_OBJS) $(UART_HW_BIN) $(UART_SW) $(UART_SW_HEX) $(UART_SW_OBJS) $(UART_SW_BIN)
