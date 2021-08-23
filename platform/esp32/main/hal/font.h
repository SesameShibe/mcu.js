#pragma once
#include "esp_partition.h"
#include "global.h"
#include "lcd-driver.h"

spi_flash_mmap_handle_t fontMmapHandle;

u16 fbPosX;
u16 fbPosY;
u16 fbFgColor;

typedef struct _FB_FONT {
  int valid;
  int charWidth;
  int charHeight;
  int charDataSize;
  int pageSize;
  u8 *pData;
  u8 *pIndex;
  u8 *pCharData;
} FB_FONT;

FB_FONT fbFontAscii16;
FB_FONT fbFontCJK16;

FB_FONT *fbCurrentFont;

static inline void fbSetP(u16 x, u16 y, u16 v) { gfx.drawPixel(x, y, v); }

static void fbInitFont() {
  FB_FONT *pFont = &fbFontCJK16;
  const void *fontData = 0;

  memset(pFont, 0, sizeof(FB_FONT));
  auto part = esp_partition_find_first((esp_partition_type_t)0x40,
                                       (esp_partition_subtype_t)0, "font");
  auto ret = esp_partition_mmap(part, 0, part->size, SPI_FLASH_MMAP_DATA,
                                &fontData, &fontMmapHandle);
  if (ret != 0) {
    printf("mmap font failed...\n");
    return;
  }
  pFont->valid = 1;
  pFont->charWidth = *(u8 *)(fontData + 13);
  pFont->charHeight = *(u8 *)(fontData + 14);
  pFont->charDataSize = 1 + ((pFont->charWidth + 7) / 8) * pFont->charHeight;
  pFont->pageSize = pFont->charDataSize * 256;
  pFont->pData = (u8 *)fontData;
  pFont->pIndex = pFont->pData + 16;
  pFont->pCharData = pFont->pIndex + 256;

  fbCurrentFont = pFont;
}

static int fbDrawUnicodeRune(u32 rune) {
  if (!fbCurrentFont) {
    return 0;
  }
  int fontW = fbCurrentFont->charWidth;
  int fontH = fbCurrentFont->charHeight;
  int fontCharSize = fbCurrentFont->charDataSize;
  int fontPageSize = fbCurrentFont->pageSize;

  int screenW = gfx.width();
  int screenH = gfx.height();
  rune = (u16)(rune);
  if (rune == '\n') {
    fbPosY += fontH + 1;
    fbPosX = 0;
    return 0;
  }
  u8 pgOffset = fbCurrentFont->pIndex[rune >> 8];
  if (pgOffset == 0xFF) {
    return 0;
  }
  u8 *ptr = fbCurrentFont->pCharData + fontPageSize * pgOffset +
            fontCharSize * (rune & 0xff);
  u8 width = *ptr;
  ptr++;

  if (fbPosX + width >= screenW) {
    fbPosY += fontH + 1;
    fbPosX = 0;
  }
  if (fbPosY + fontH >= screenH) {
    return 0;
  }
  for (u8 y = 0; y < fontH; y++) {
    for (u8 x = 0; x < width; x++) {
      u8 pix = ptr[y * 2 + x / 8] & (1 << (x % 8));
      if (pix) {
        fbSetP(fbPosX + x, fbPosY + y, 1);
      }
    }
  }
  fbPosX += width + 1;
  if (fbPosX >= screenW) {
    fbPosY += fontH + 1;
    fbPosX = 0;
  }
  return width;
}

static void fbDrawUtf8String(const char *utf8Str) {
  u8 *p = (u8 *)utf8Str;
  u16 rune = 0;
  while (*p) {
    rune = 0;
    u8 byte1 = *p;
    p++;
    if ((byte1 & 0x80) == 0) {
      rune = byte1;
    } else {
      u8 byte2 = *p;
      p++;
      if (byte2 == 0) {
        break;
      }
      if ((byte1 & 0xE0) == 0xC0) {
        rune = ((byte1 & 0x1F) << 6) | (byte2 & 0x3F);
      } else {
        u8 byte3 = *p;
        p++;
        if (byte3 == 0) {
          break;
        }
        if ((byte1 & 0xf0) == 0xE0) {
          rune =
              ((byte1 & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F);
        } else {
          break;
        }
      }
    }
    fbDrawUnicodeRune(rune);
  }
}