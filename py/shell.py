from __future__ import unicode_literals

import binascii
import codecs
import serial
import sys
import time
import threading
from prompt_toolkit import PromptSession
from prompt_toolkit.history import FileHistory

COM_PORT = 'COM6'
ser = ''


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
        except UnicodeDecodeError, err:
            sys.stderr.write('['+binascii.b2a_hex(err.object[err.start:err.end])+']')


def runfile(fn):
    with codecs.open(fn, 'rb', getFileEncoding(fn)) as f:
        data = f.read()
    writer = codecs.getwriter('utf-8')(ser)
    writer.write(data)
    ser.write(b'\xF8')


def start():
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
