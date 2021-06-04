#pragma once
#include "driver/spi_master.h"
#include "esp_partition.h"
#include "font.h"
#include "global.h"
#include "gpio.h"
#include "lcd-driver.h"

#define CIRCLE_CORNER_TL 1 << 0
#define CIRCLE_CORNER_TR 1 << 1
#define CIRCLE_CORNER_BL 1 << 2
#define CIRCLE_CORNER_BR 1 << 3

#define COLOR_TRANSPARENT 0xFFFF00

typedef struct hal_rect_t {
  int16_t left;
  int16_t top;
  int16_t right;
  int16_t bottom;
} hal_rect_t;

static hal_font_t* font;
static hal_font_section_info_t* sections;

static hal_icon_font_t* iconFont;
static uint32_t* iconIds;

uint16_t lcdRowOffset, lcdColOffset;

static uint32_t penColor = 0x1f;
static hal_rect_t renderRect = {0, 0, LCD_WIDTH, LCD_HEIGHT};

void halLcbPrintBuf(JS_BUFFER buf, int32_t len) {
  if (len == -1) len = buf.size;

  int8_t column = 0;
  printf("\n");
  for (int i = 0; i < len; i++) {
    printf("%02x ", buf.buf[i]);

    column++;
    if (column == 16) {
      printf("\n");
      column = 0;
    }
  }
}

JS_BUFFER halLcdCopyFB(int16_t left, int16_t top, int16_t right,
                       int16_t bottom) {
  JS_BUFFER buf;
  uint16_t* lcdFBCopy =
      mcujs_alloc_function(0, (right - left) * (bottom - top) * 2);
  if (left == 0 && top == 0 && right == LCD_WIDTH && bottom == LCD_HEIGHT) {
    // Should be faster when copy the whole screen.
    memcpy(lcdFBCopy, lcdFB, LCD_WIDTH * LCD_HEIGHT * 2);
    buf.buf = (uint8_t*)lcdFBCopy;
    buf.size = LCD_WIDTH * LCD_HEIGHT * 2;
  } else {
    int16_t index = 0;

    memset(lcdFBCopy, 0, LCD_WIDTH * LCD_HEIGHT * 2);
    do {
      do {
        lcdFBCopy[index] = lcdFB[(top * LCD_WIDTH) + left];
        index++;
      } while (++left < right);
    } while (++top < bottom);
    buf.buf = (uint8_t*)lcdFBCopy;
    buf.size = index;
  }

  return buf;
}

void halLcbReleaseCopy(JS_BUFFER buf) { mcujs_free_function(&buf, buf.buf); }

JS_BUFFER halLcdGetFB() {
  JS_BUFFER buf = {.buf = (u8*)lcdFB, .size = LCD_WIDTH * LCD_HEIGHT * 2};
  return buf;
}

void halLcdSetFB(JS_BUFFER fb) {
  if (fb.size >= LCD_WIDTH * LCD_HEIGHT * 2) {
    memcpy(lcdFB, fb.buf, LCD_WIDTH * LCD_HEIGHT * 2);
  }
}

void halLcdPutPixels(int16_t x, int16_t y, int16_t width, int16_t height,
                     JS_BUFFER buf) {
  int16_t right = x + width;
  int16_t bottom = y + height;
  size_t index = 0;
  uint16_t* pixels = (uint16_t*)buf.buf;

  int16_t yi = y;
  do {
    int16_t xi = x;
    do {
      if (index >= buf.size) break;

      if (xi < renderRect.left || xi >= renderRect.right ||
          yi < renderRect.top || yi >= renderRect.bottom)
        continue;

      uint16_t pixel = pixels[index];
      lcdFB[(yi * LCD_WIDTH) + xi] = pixel;

      index++;
    } while (++xi < right);
  } while (++yi < bottom);
}

void halLcdClearScreen() { memset(lcdFB, 0, LCD_WIDTH * LCD_HEIGHT * 2); }

void swapInt(int16_t* a, int16_t* b) {
  *a ^= *b;
  *b ^= *a;
  *a ^= *b;
}

float_t triangleArea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                     uint16_t x2, uint16_t y2) {
  return (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
}

int16_t min(int16_t a, int16_t b) { return a > b ? b : a; }

int16_t max(int16_t a, int16_t b) { return a > b ? a : b; }

void halLcdSetPenColor(uint32_t color) { penColor = color; }

uint16_t halLcdGetPenColor() { return penColor; }

void halLcdSetRenderRect(int16_t left, int16_t top, int16_t right,
                         int16_t bottom) {
  renderRect.left = left < 0 ? 0 : left;
  renderRect.top = top < 0 ? 0 : top;
  renderRect.right = right > LCD_WIDTH ? LCD_WIDTH : right;
  renderRect.bottom = bottom > LCD_HEIGHT ? LCD_HEIGHT : bottom;
}

void IRAM_ATTR halLcdDrawDot(int16_t x, int16_t y) {
  // Ignore outside screen pixels
  if (x < renderRect.left || x >= renderRect.right || y < renderRect.top ||
      y >= renderRect.bottom)
    return;

  if (penColor == COLOR_TRANSPARENT) return;

  lcdFB[(y * LCD_WIDTH) + x] = (uint16_t)penColor;
}

void halLcdDrawHLine(int16_t x0, int16_t x1, int16_t y) {
  int16_t a, b;
  a = min(x0, x1);
  b = max(x0, x1);

  if (a < 0) a = 0;

  for (; a < b; a++) {
    halLcdDrawDot(a, y);
  }
}

void halLcdDrawVLine(int16_t y0, int16_t y1, int16_t x) {
  int16_t a, b;
  a = min(y0, y1);
  b = max(y0, y1);

  if (a < 0) a = 0;

  for (; a < b; a++) {
    halLcdDrawDot(x, a);
  }
}

void halLcdDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  if (y0 == y1) {
    halLcdDrawHLine(x0, x1, y0);
    return;
  } else if (x0 == x1) {
    halLcdDrawVLine(y0, y1, x0);
    return;
  } else {
    int16_t dx = abs(x1 - x0), dy = abs(y1 - y0);
    bool steep = dx < dy;

    if (steep) {
      swapInt(&x0, &y0);
      swapInt(&x1, &y1);
      swapInt(&dx, &dy);
    }

    int16_t stepX = (x1 - x0) > 0 ? 1 : -1, stepY = (y1 - y0) > 0 ? 1 : -1,
            curX = x0, curY = y0, n2dy = dy << 1, n2dydx = (dy - dx) << 1,
            d = (dy << 1) - dx;

    if (steep) {
      while (curX != x1) {
        if (d < 0) {
          d += n2dy;
        } else {
          curY += stepY;
          d += n2dydx;
        }
        halLcdDrawDot(curY, curX);
        curX += stepX;
      }
    } else {
      while (curX != x1) {
        if (d < 0) {
          d += n2dy;
        } else {
          curY += stepY;
          d += n2dydx;
        }
        halLcdDrawDot(curX, curY);
        curX += stepX;
      }
    }
  }
}

void halLcdDrawCircle(int16_t px, int16_t py, int16_t r, int32_t cornerMask) {
  int16_t x = 0, y = r, d = 1 - r;

  while (y >= x) {
    // Bottom right
    if ((cornerMask & CIRCLE_CORNER_BR) == CIRCLE_CORNER_BR) {
      halLcdDrawDot(x + px, y + py);
      halLcdDrawDot(y + px, x + py);
    }
    // Bottom left
    if ((cornerMask & CIRCLE_CORNER_BL) == CIRCLE_CORNER_BL) {
      halLcdDrawDot(-x + px, y + py);
      halLcdDrawDot(-y + px, x + py);
    }
    // Top left
    if ((cornerMask & CIRCLE_CORNER_TL) == CIRCLE_CORNER_TL) {
      halLcdDrawDot(-x + px, -y + py);
      halLcdDrawDot(-y + px, -x + py);
    }
    // Top right
    if ((cornerMask & CIRCLE_CORNER_TR) == CIRCLE_CORNER_TR) {
      halLcdDrawDot(x + px, -y + py);
      halLcdDrawDot(y + px, -x + py);
    }

    if (d <= 0) {
      d += (x << 2) + 6;
    } else {
      d += ((x - y) << 2) + 10;
      y--;
    }
    x++;
  }
}

void halLcdFillCircle(int16_t px, int16_t py, int16_t r, int32_t cornerMask) {
  int16_t x = 0, y = r, d = 1 - r;

  while (y >= x) {
    // Bottom right
    if ((cornerMask & CIRCLE_CORNER_BR) == CIRCLE_CORNER_BR) {
      halLcdDrawHLine(px, x + px, y + py);
      halLcdDrawHLine(px, y + px, x + py);
    }
    // Bottom left
    if ((cornerMask & CIRCLE_CORNER_BL) == CIRCLE_CORNER_BL) {
      halLcdDrawHLine(px, -x + px, y + py);
      halLcdDrawHLine(px, -y + px, x + py);
    }
    // Top left
    if ((cornerMask & CIRCLE_CORNER_TL) == CIRCLE_CORNER_TL) {
      halLcdDrawHLine(px, -x + px, -y + py);
      halLcdDrawHLine(px, -y + px, -x + py);
    }
    // Top right
    if ((cornerMask & CIRCLE_CORNER_TR) == CIRCLE_CORNER_TR) {
      halLcdDrawHLine(px, x + px, -y + py);
      halLcdDrawHLine(px, y + px, -x + py);
    }

    if (d <= 0) {
      d += (x << 2) + 6;
    } else {
      d += ((x - y) << 2) + 10;
      y--;
    }
    x++;
  }
}

void halLcdDrawRectangle(int16_t left, int16_t top, int16_t right,
                         int16_t bottom, int16_t cornerRadius) {
  if ((left == right) || (top == bottom)) {
    // Draw nothing.
    return;
  }

  halLcdDrawLine(left + cornerRadius, top, right - cornerRadius, top);
  halLcdDrawLine(right, top + cornerRadius, right, bottom - cornerRadius);
  halLcdDrawLine(left, top + cornerRadius, left, bottom - cornerRadius);
  halLcdDrawLine(left + cornerRadius, bottom, right - cornerRadius, bottom);

  halLcdDrawCircle(left + cornerRadius, top + cornerRadius, cornerRadius,
                   CIRCLE_CORNER_TL);
  halLcdDrawCircle(right - cornerRadius, top + cornerRadius, cornerRadius,
                   CIRCLE_CORNER_TR);
  halLcdDrawCircle(left + cornerRadius, bottom - cornerRadius, cornerRadius,
                   CIRCLE_CORNER_BL);
  halLcdDrawCircle(right - cornerRadius, bottom - cornerRadius, cornerRadius,
                   CIRCLE_CORNER_BR);
}

void halLcdFillRectangle(int16_t left, int16_t top, int16_t right,
                         int16_t bottom, int16_t cornerRadius) {
  if ((left >= right) || (top >= bottom)) {
    // Draw nothing.
    return;
  }

  int16_t y = 0;

  y = top + cornerRadius;
  while (y <= bottom - cornerRadius) {
    halLcdDrawLine(left, y, right, y);
    y++;
  }

  y = top;
  while (y < top + cornerRadius) {
    halLcdDrawLine(left + cornerRadius, y, right - cornerRadius, y);
    y++;
  }

  y = bottom - cornerRadius;
  while (y <= bottom) {
    halLcdDrawLine(left + cornerRadius, y, right - cornerRadius, y);
    y++;
  }

  halLcdFillCircle(left + cornerRadius, top + cornerRadius, cornerRadius,
                   CIRCLE_CORNER_TL);
  halLcdFillCircle(right - cornerRadius, top + cornerRadius, cornerRadius,
                   CIRCLE_CORNER_TR);
  halLcdFillCircle(left + cornerRadius, bottom - cornerRadius, cornerRadius,
                   CIRCLE_CORNER_BL);
  halLcdFillCircle(right - cornerRadius, bottom - cornerRadius, cornerRadius,
                   CIRCLE_CORNER_BR);
}

void halLcdDrawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2) {
  if (triangleArea(x0, y0, x1, y1, x2, y2) == 0) return;

  halLcdDrawLine(x0, y0, x1, y1);
  halLcdDrawLine(x0, y0, x2, y2);
  halLcdDrawLine(x1, y1, x2, y2);
}

void halLcdFillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2) {
  float_t area = triangleArea(x0, y0, x1, y1, x2, y2);
  if (area == 0) return;

  int16_t minX, minY, maxX, maxY;
  minX = max(min(min(x0, x1), x2), 0);
  minY = max(min(min(y0, y1), y2), 0);
  maxX = min(max(max(x0, x1), x2), LCD_WIDTH - 1);
  maxY = min(max(max(y0, y1), y2), LCD_HEIGHT - 1);

  if (minX > maxX || minY > maxY) return;

  for (int16_t y = minY; y < maxY; y++) {
    for (int16_t x = minX; x < maxX; x++) {
      float_t e0 = triangleArea(x1, y1, x2, y2, x, y) / area;
      float_t e1 = triangleArea(x2, y2, x0, y0, x, y) / area;
      float_t e2 = triangleArea(x0, y0, x1, y1, x, y) / area;

      if (e0 >= 0 && e1 >= 0 && e2 >= 0) {
        halLcdDrawDot(x, y);
      }
    }
  }
}

void halLcdInit() {
  halLcdDriverInit();
  halFontInit();
}