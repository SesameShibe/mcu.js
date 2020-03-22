# coding: utf-8
import freetype
from freetype import (FT_LOAD_DEFAULT, FT_LOAD_NO_BITMAP, FT_LOAD_NO_HINTING,
                      FT_LOAD_RENDER, FT_RENDER_MODE_NORMAL, Face, Vector)
from io import BytesIO
import codecs
from PIL import Image


BLANK_CHARS = [u'\r', u'\n']


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
    def __init__(self, ttfPath, pxSize, fltr, padding, baseline):
        self.FtFont = freetype.Face(ttfPath)
        self.FtFont.set_pixel_sizes(pxSize, pxSize)
        self.FontSize = pxSize
        self.CellWidth = pxSize+padding
        self.CellHeight = pxSize+padding
        self.BaseLine = pxSize-baseline
        self.Filter = fltr
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

        dxo = ((self.CellWidth - bitmap.width) / 2)
        dy = self.BaseLine - horiBearingY

        b = bytearray('\x00'*(self.CellWidth*self.CellHeight))
        # dxo += horiBearingX
        if(dxo + bitmap.width) > self.CellWidth:
            dxo -= dxo+bitmap.width-self.CellWidth
        for y in range(bitmap.rows):
            if(dy >= self.CellHeight):
                break
            dx = dxo
            for x in range(bitmap.width):
                if(dx >= self.CellWidth):
                    break
                pos = y * bitmap.width + x
                a = ((bitmap.buffer[pos]) & 0xFF)
                b[self.CellWidth*dy+dx] = a
                dx += 1
            dy += 1

        self.Chars[c] = compressGlyphBmp(b, self.Filter)

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

            hdr.write('const size_t font_%d_char_count =  %d;\n\n' %
                      (self.FontSize, len(self.Chars)))

            hdr.write('const uint16_t font_%d_cell_width = %d;\n\n' %
                      (self.FontSize, self.CellWidth))

            hdr.write('const uint16_t font_%d_cell_height = %d;\n\n' %
                      (self.FontSize, self.CellHeight))

    def saveBin(self, path):
        with open(path, 'wb') as fnt:
            for k in sorted(self.Chars.keys()):
                fnt.write(self.Chars[k])

    def saveImage(self, path):
        glyphsToImg(path, [self.Chars[k]
                           for k in sorted(self.Chars.keys())], self.CellHeight, self.Filter)


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
        elif (magic[:2] == '\x00\x00\xff\xfe'):
            return 'utf-32le'
        elif (magic[:2] == '\xfe\xff\x00\x00'):
            return 'utf-32be'
        else:
            return 'utf-8'


def scanFiles(paths):
    chars = []
    for path in paths:
        encoding = getFileEncoding(path)
        with codecs.open(path, 'r', encoding) as f:
            print path
            text = f.read()
            for c in text:
                if c not in chars and c not in BLANK_CHARS:
                    chars.append(c)
    return sorted(chars)


def generateFont(ttfPath, size, chars, fltr, padding, baseline, savePath, imgSavePath):
    font = Font(ttfPath, size, fltr, padding, baseline)
    font.addChars(chars)
    font.saveHeader(savePath)
    if(imgSavePath):
        font.saveImage(imgSavePath)


def compressGlyphBmp(bs, fltr):
    bits = 0
    rotate = 0
    result = []

    for i in range(len(bs)):
        b = bs[i]
        if (b >= fltr):
            bits |= 1 << rotate
        else:
            pass

        rotate += 1
        if(rotate >= 8):
            result.append(bits)
            rotate = 0
            bits = 0

    return result


def toImg(pixels1bpp, size):
    img = Image.new('RGBA', (size, size))
    x = 0
    y = 0
    for bits in pixels1bpp:
        mask = 1
        while(mask < (1 << 8)):
            bit = bits & mask
            if(bit == mask):
                img.putpixel((x, y), (255, 255, 255, 255))
            mask <<= 1

            x += 1
            if(x == size):
                x = 0
                y += 1

    return img


def glyphsToImg(path, glyphs, glyphSize, fltr):
    rows = len(glyphs) / 16 + 1
    img = Image.new('RGBA', (16*glyphSize, rows*glyphSize))

    x = 0
    y = 0
    for g in glyphs:
        i = toImg(g, glyphSize)
        img.paste(i, (x*glyphSize, y*glyphSize))

        x += 1
        if(x == 16):
            x = 0
            y += 1

    img.save(path)


if __name__ == "__main__":
    import argparse

    def parse_options():
        parser = argparse.ArgumentParser(
            description="Nintendo CTR Font Converter Text Filter(xllt) Generator.")
        parser.add_argument('-t', '--ttf', help="Set TrueType font file path.")
        parser.add_argument('-s', '--size', help="Set font size.", type=int)
        parser.add_argument('-f', '--files', help="Files to scan.",
                            required=True, nargs='*', type=str)
        parser.add_argument(
            '-l', '--filter', help="Set bitmap grey filter.", default=90, type=int)
        parser.add_argument(
            '-p', '--padding', help="Set bitmap grey filter.", default=3, type=int)
        parser.add_argument(
            '-b', '--baseline', help="Set bitmap grey filter.", default=3, type=int)
        parser.add_argument('-o', '--output', help="Set output file path.")
        parser.add_argument(
            '-x', '--charset', help="Set raw charset path. If not set, the charset will not save.", default=None)
        parser.add_argument(
            '-i', '--image', help="Set image path. If not set, the image will not save.", default=None)
        return parser.parse_args()

    def main():
        options = parse_options()
        if not options:
            return False

        chars = scanFiles(options.files)
        if(options.charset):
            codecs.open(options.charset, 'w', 'utf-8').write(u''.join(chars))

        generateFont(options.ttf, options.size, chars, options.filter,
                     options.padding, options.baseline, options.output, options.image)

    main()
