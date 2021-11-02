from __future__ import unicode_literals

import os
import binascii
import codecs
import serial
import struct
import sys
import time
import threading
from prompt_toolkit import PromptSession
from prompt_toolkit.history import FileHistory

from PIL import Image

COM_PORT = 'COM6' if 'ESPPORT' not in os.environ else os.environ['ESPPORT']
ser = ''
shotpath = '1.png'


def getFileEncoding(path):
    with open(path, 'rb')as f:
        magic = f.read(4)
        # utf-8
        if(magic[:3] == '\xef\xbb\xbf'):
            return 'utf-8-sig'
        elif (magic[:2] == '\xff\xfe'):
            return 'utf-16le'
        elif (magic[:2] == '\xfe\xff'):
            return 'utf-16be'
        elif (magic == '\x00\x00\xff\xfe'):
            return 'utf-32le'
        elif (magic == '\xfe\xff\x00\x00'):
            return 'utf-32be'
        else:
            return 'utf-8'


def recvLoop():
    reader = codecs.getreader('utf-8')(ser)
    while True:
        try:
            char = reader.read(1)
            sys.stdout.write(char)
        except UnicodeDecodeError as uerr:
            if uerr.object == b'\xFB':
                print('Saving screenshot... ')
                head = ser.read(4)
                width, height = struct.unpack('<HH', head)
                buf = ser.read(width*height*2+4)
                bmp = (width, height, buf)
                savescreen(bmp)
            else:
                sys.stderr.write(
                    '['+binascii.b2a_hex(err.object[err.start:err.end])+']')


def runfile(fn):
    with codecs.open(fn, 'rb', getFileEncoding(fn)) as f:
        data = f.read()
    writer = codecs.getwriter('utf-8')(ser)
    writer.write(data)
    ser.write(b'\xF8')


def screenshot(path):
    global shotpath
    shotpath = path
    ser.write(b'\nsaveBitmap(lcd.getFB());\n\xF8')


def savescreen(bmp):
    global shotpath
    if not shotpath:
        shotpath = '1.png'
    w, h, buf = bmp
    img = Image.new('RGB', (w, h))

    for y in range(h):
        for x in range(w):
            index = (y * w + x) * 2
            b1, b2 = buf[index], buf[index+1]
            r = (((b1 & 0b11111000) >> 3) * 527 + 23) >> 6
            g = ((((b1 & 0b00000111) << 3) | (
                (b2 & 0b11100000) >> 5)) * 259 + 33) >> 6
            b = ((b2 & 0b00011111) * 527 + 23) >> 6

            img.putpixel((x, y), (r, g, b))

    img.save(shotpath)
    print(shotpath)


def start():
    COM_PORT = os.environ['ESPPORT']
    global ser
    ser = serial.Serial(COM_PORT, 115200)

    recvThread = threading.Thread(target=recvLoop)
    recvThread.daemon = True
    recvThread.start()

    time.sleep(2)
    ser.write(b'\nshellSetMode(1);\n')
    time.sleep(0.5)
    ser.write(b'\xF8\xF8')

    session = PromptSession(history=FileHistory('py/shell.history'))
    while True:
        cmd = session.prompt(': ')
        if cmd.startswith('.'):
            eval(cmd[1:])
        else:
            ser.write(cmd.encode('utf-8') + b'\xF8')
