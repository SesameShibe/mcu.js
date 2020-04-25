from __future__ import unicode_literals

import codecs
import serial
import sys
import time
import threading
from prompt_toolkit import PromptSession
from prompt_toolkit.history import FileHistory

from font import getFileEncoding

COM_PORT = 'COM6'
ser = ''

def recvLoop():
    reader = codecs.getreader('utf-8')(ser)
    while True:
        char = reader.read(1)
        sys.stdout.write(char)

def runfile(fn):
    with codecs.open(fn, 'rb', getFileEncoding(fn)) as f:
        data = f.read()
    writer = codecs.getwriter('utf-8')(ser)
    writer.write(data)
    ser.write(b'\xF8')

def start():
    global ser
    ser = serial.Serial(COM_PORT, 115200)

    recvThread = threading.Thread(target = recvLoop)
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
