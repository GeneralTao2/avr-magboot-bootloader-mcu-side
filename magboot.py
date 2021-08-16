#!python3
# Copyright (c) 2010-2011 Magnus Olsson (magnus@minimum.se)
# See LICENSE for details

"""magboot - AVR serial bootloader

usage: magboot <port> <device> <[command1] [command2] ... [commandN]>

commands:
-d      Show more informative messages
-w <file>	Write contents of <file> to beginning of flash (use - for stdin)
-i		Verify device signature
-r		Reset device (will bypass bootloader on next boot)
-z		Wait for device to appear on <port>

supported devices:
atmega32
atmega328p
atmega168p
"""

import serial
import getopt
import sys
from struct import *
from array import array
import time
from struct import unpack
import os
import math

global ser
global devicelist
global dev
global debug_mode

atmega328p = {
    'signature': b"\x1E\x95\x0F",
    'pagesize': 128,
}

atmega32 = {
    'signature': b"\x1E\x95\x02",
    'pagesize': 128,
}

atmega168p = {
    'signature': b"\x1E\x94\x0B",
    'pagesize': 128,
}

devicelist = {
    'atmega328p': atmega328p,
    'atmega32': atmega32,
    'atmega168p': atmega168p,
}

# Print iterations progress


def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '=', printEnd = "\r"):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print(f'\r{prefix} |{bar}| {percent}% {suffix}', end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()


def usage(*args):
    sys.stdout = sys.stderr
    print(__doc__)
    for msg in args:
        print(msg)
    sys.exit(2)


def do_cmd(str, expect_reply=True):
    ser.write(str)
    if not expect_reply:
        if debug_mode:
            print("> OK")
        return

    reply = ser.read()
    
    if (len(reply) == 0):
        print("> FAILED (TIMEOUT)")
        sys.exit(1)

    if (reply == b'Y'):
        if debug_mode:
            print("> OK")
        return

    if (reply == b'N'):
        print("> FAILED")
    else:
        print("> FAILED (UNKNOWN):")
        while (len(reply) == 1):
            print(reply)
            reply = ser.read()
    sys.exit(1)


def cmd_device_id():
    if debug_mode:
        print("ID")
    do_cmd(b'I' + dev['signature'])


def cmd_load_addr(addr):
    if debug_mode:
        print("LOAD_ADDR")
    # Little-endian, 16-bit uint load address
    load_addr = pack('<H', addr)
    do_cmd(b'A' + load_addr)


def checksum(data):
    csum = 0
    words = unpack('H'*(len(data)//2),data)
    for w in words:
        csum = csum + w

    # Fold overflow bits
    while (csum >> 16):
        csum = (csum & 0xFFFF) + (csum >> 16)

    return pack('<H', csum)


def cmd_write_file(fname):
    binsize = os.path.getsize(fname)
    items = list(range(0, math.ceil(binsize / 128)*128))
    l = len(items)
    i = 0
    if (fname == "-"):
        f = sys.stdin
    else:
        f = open(fname, "rb")

    eof = False

    cmd_load_addr(0)
    if not debug_mode:
        printProgressBar(0, l, prefix = 'Progress:', suffix = 'Complete', length = 50)
    while (not eof):
        buf = bytearray()
        bytecount = 0
        while (bytecount < dev['pagesize']):
            data = f.read(1)
            if (len(data) == 0):
                eof = True
                break
            buf += data
            bytecount = bytecount + 1

        # Zerofill remaining part of page
        if (bytecount != dev['pagesize']):
            while (bytecount < dev['pagesize']):
                buf += b'\x00'
                bytecount = bytecount + 1

        csum = checksum(buf)
        do_cmd(b'W' + csum + buf)
        if not debug_mode:
            printProgressBar(i + 1, l, prefix = 'Progress:', suffix = 'Complete', length = 50)
            i += 128
    if not debug_mode:
        printProgressBar(i + 1, l, prefix = 'Progress:', suffix = 'Complete', length = 50, printEnd="\n")
    


def cmd_reset():
    if debug_mode:
        print("RESET")
    do_cmd(b'R', False)


def cmd_wait():
    print("WAIT")
    while True:
        sys.stdout.flush()
        ser.write(b'I' + dev['signature'])
        data = ser.read()
        if (len(data) == 1 and data == b'Y'):
            break
        sys.stdout.write('.')
    sys.stdout.write('\n')


if __name__ == "__main__":
    if len(sys.argv) < 4:
        usage()

    try:
        dev = devicelist[sys.argv[2]]
    except KeyError:
        print("Unsupported device '" + sys.argv[2] + "'")
        sys.exit(3)

    ser = serial.Serial(port=sys.argv[1], baudrate=250000, timeout=1)
    time.sleep(0.1) 

    try:
        opts, args = getopt.getopt(sys.argv[3:], 'a:w:ijrzd')
    except getopt.error as msg:
        usage(msg)

    debug_mode = False
    for o, a in opts:
        if o == '-d':
            debug_mode = True
        if o == '-w':
            cmd_write_file(a)
        if o == '-i':
            cmd_device_id()
        if o == '-r':
            cmd_reset()
        if o == '-z':
            cmd_wait()

    ser.close()

