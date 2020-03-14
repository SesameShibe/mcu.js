# coding: utf-8
import freetype
from freetype import (FT_LOAD_DEFAULT, FT_LOAD_NO_BITMAP, FT_LOAD_NO_HINTING,
                      FT_LOAD_RENDER, FT_RENDER_MODE_NORMAL, Face, Vector)
from io import BytesIO


def f26d6_to_int(val):
    ret = (abs(val) & 0x7FFFFFC0) >> 6
    if val < 0:
        return -ret
    else:
        return ret


def f16d16_to_int(val):
    ret = (abs(val) & 0x3FFFC0) >> 16
    if val < 0:
        return -ret
    else:
        return ret


class Font(object):
    def __init__(self, ttfPath, pxSize):
        self.FtFont = freetype.Face(ttfPath)
        self.FtFont.set_pixel_sizes(pxSize, pxSize)
        self.FontSize = pxSize
        self.CellWidth = pxSize
        self.CellHeight = pxSize
        self.BaseLine = pxSize-3
        self.Chars = {}

    def addChars(self, chars):
        for c in chars:
            if not c in self.Chars.keys():
                self.addChar(c)

    def addChar(self, c):
        flags = FT_LOAD_RENDER | FT_LOAD_NO_HINTING
        self.FtFont.load_char(c, flags)

        glyphslot = self.FtFont.glyph
        bitmap = glyphslot.bitmap

        adv = f26d6_to_int(glyphslot.metrics.horiAdvance)
        horiBearingX = f26d6_to_int(glyphslot.metrics.horiBearingX)
        horiBearingY = f26d6_to_int(glyphslot.metrics.horiBearingY)

        dxo = ((self.CellWidth - adv) / 2)
        dy = self.BaseLine - horiBearingY - 1

        b = bytearray('\x00'*(self.CellWidth*self.CellHeight))
        dxo += horiBearingX
        for y in range(bitmap.rows):
            dx = dxo
            for x in range(bitmap.width):
                pos = y * bitmap.width + x
                a = ((bitmap.buffer[pos]) & 0xFF)
                b[self.CellWidth*dy+dx] = a
                dx += 1
            dy += 1
            if(dy >= self.CellHeight):
                break

        self.Chars[c] = b

    def saveHeader(self, path):
        with open(path, 'w')as hdr:
            s = ''
            m = ''
            i = 0
            for k in sorted(self.Chars.keys()):
                v = self.Chars[k]
                m += '0x%04x, ' % ord(k)
                for c in v:
                    s += '0x%02x, ' % (c)
                    i += 1
                    if(i == 16):
                        s += '\n'
                        i = 0
            hdr.write('const uint8_t font_%d[] =  {\n' % self.FontSize)
            hdr.write(s)
            hdr.write('};\n\n')

            hdr.write('const uint16_t chars_%d[] =  {\n' % self.FontSize)
            hdr.write(m)
            hdr.write('};\n\n')

            hdr.write('const size_t font_%d_count =  %d;' %
                      (self.FontSize, len(self.Chars)))

    def saveBin(self, path):
        with open(path, 'wb') as fnt:
            for k in sorted(self.Chars.keys()):
                fnt.write(self.Chars[k])


def generateFont(ttfPath, size, chars, savePath):
    font = Font(ttfPath, size)
    font.addChars(chars)
    font.saveHeader(savePath)