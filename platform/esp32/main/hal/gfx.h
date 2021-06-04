#pragma once
#include "font.h"
#include "global.h"
#include "utf.h"

typedef enum {
	CIRCLE_CORNER_TL = 1 << 0,
	CIRCLE_CORNER_TR = 1 << 1,
	CIRCLE_CORNER_BL = 1 << 2,
	CIRCLE_CORNER_BR = 1 << 3,
} CircleCorner;

#define COLOR_TRANSPARENT 0xFFFF00

typedef struct gfx_rect_t {
	int16_t left;
	int16_t top;
	int16_t right;
	int16_t bottom;
} gfx_rect_t;

typedef struct gfx_size_t {
	uint16_t width;
	uint16_t height;
} gfx_size_t;

typedef union u_size_t {
	uint32_t val;
	gfx_size_t size;
} u_size_t;

typedef struct gfx_pixel_t {
	uint16_t b : 5;
	uint16_t g : 6;
	uint16_t r : 5;
} gfx_pixel_t;

void swapInt(int16_t* a, int16_t* b) {
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}

float_t triangleArea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	return (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
}

int16_t min(int16_t a, int16_t b) {
	return a > b ? b : a;
}

int16_t max(int16_t a, int16_t b) {
	return a > b ? a : b;
}

gfx_size_t halGfxGetBitmapSize(JS_BUFFER buf) {
	gfx_size_t size = { .width = 0, .height = 0 };
	if (buf.buf != NULL && buf.size >= sizeof(gfx_size_t)) {
		gfx_size_t* pSize = (gfx_size_t*) buf.buf;
		size.width = pSize->width;
		size.height = pSize->height;
	}
	return size;
}

uint16_t* halGfxGetBitmapData(JS_BUFFER buf) {
	uint16_t* data = NULL;
	if (buf.buf != NULL && buf.size >= sizeof(gfx_size_t)) {
		data = (uint16_t*) (buf.buf + sizeof(gfx_size_t));
	}
	return data;
}

void IRAM_ATTR halGfxDrawDot(JS_BUFFER buf, int16_t color, int16_t x, int16_t y) {
	gfx_size_t size = halGfxGetBitmapSize(buf);
	if (x >= 0 && x < size.width && y >= 0 && y < size.height) {
		uint16_t* textData = (uint16_t*) &buf.buf[sizeof(gfx_size_t)];
		textData[(y * size.width) + x] = (uint16_t) color;
	}
}

void halGfxDrawHLine(JS_BUFFER buf, int16_t color, int16_t x0, int16_t x1, int16_t y) {
	int16_t a, b;
	a = min(x0, x1);
	b = max(x0, x1);

	if (a < 0)
		a = 0;

	for (; a < b; a++) {
		halGfxDrawDot(buf, color, a, y);
	}
}

void halGfxDrawVLine(JS_BUFFER buf, int16_t color, int16_t y0, int16_t y1, int16_t x) {
	int16_t a, b;
	a = min(y0, y1);
	b = max(y0, y1);

	if (a < 0)
		a = 0;

	for (; a < b; a++) {
		halGfxDrawDot(buf, color, x, a);
	}
}

void halGfxDrawLine(JS_BUFFER buf, int16_t color, int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
	if (y0 == y1) {
		halGfxDrawHLine(buf, color, x0, x1, y0);
		return;
	} else if (x0 == x1) {
		halGfxDrawVLine(buf, color, y0, y1, x0);
		return;
	} else {
		int16_t dx = abs(x1 - x0), dy = abs(y1 - y0);
		bool steep = dx < dy;

		if (steep) {
			swapInt(&x0, &y0);
			swapInt(&x1, &y1);
			swapInt(&dx, &dy);
		}

		int16_t stepX = (x1 - x0) > 0 ? 1 : -1, stepY = (y1 - y0) > 0 ? 1 : -1, curX = x0, curY = y0, n2dy = dy << 1,
		        n2dydx = (dy - dx) << 1, d = (dy << 1) - dx;

		if (steep) {
			while (curX != x1) {
				if (d < 0) {
					d += n2dy;
				} else {
					curY += stepY;
					d += n2dydx;
				}
				halGfxDrawDot(buf, color, curY, curX);
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
				halGfxDrawDot(buf, color, curX, curY);
				curX += stepX;
			}
		}
	}
}

void halGfxDrawCircle(JS_BUFFER buf, int16_t color, int16_t px, int16_t py, int16_t r, int32_t cornerMask) {
	int16_t x = 0, y = r, d = 1 - r;

	while (y >= x) {
		// Bottom right
		if ((cornerMask & CIRCLE_CORNER_BR) == CIRCLE_CORNER_BR) {
			halGfxDrawDot(buf, color, x + px, y + py);
			halGfxDrawDot(buf, color, y + px, x + py);
		}
		// Bottom left
		if ((cornerMask & CIRCLE_CORNER_BL) == CIRCLE_CORNER_BL) {
			halGfxDrawDot(buf, color, -x + px, y + py);
			halGfxDrawDot(buf, color, -y + px, x + py);
		}
		// Top left
		if ((cornerMask & CIRCLE_CORNER_TL) == CIRCLE_CORNER_TL) {
			halGfxDrawDot(buf, color, -x + px, -y + py);
			halGfxDrawDot(buf, color, -y + px, -x + py);
		}
		// Top right
		if ((cornerMask & CIRCLE_CORNER_TR) == CIRCLE_CORNER_TR) {
			halGfxDrawDot(buf, color, x + px, -y + py);
			halGfxDrawDot(buf, color, y + px, -x + py);
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

void halGfxFillCircle(JS_BUFFER buf, int16_t color, int16_t px, int16_t py, int16_t r, int32_t cornerMask) {
	int16_t x = 0, y = r, d = 1 - r;

	while (y >= x) {
		// Bottom right
		if ((cornerMask & CIRCLE_CORNER_BR) == CIRCLE_CORNER_BR) {
			halGfxDrawHLine(buf, color, px, x + px, y + py);
			halGfxDrawHLine(buf, color, px, y + px, x + py);
		}
		// Bottom left
		if ((cornerMask & CIRCLE_CORNER_BL) == CIRCLE_CORNER_BL) {
			halGfxDrawHLine(buf, color, px, -x + px, y + py);
			halGfxDrawHLine(buf, color, px, -y + px, x + py);
		}
		// Top left
		if ((cornerMask & CIRCLE_CORNER_TL) == CIRCLE_CORNER_TL) {
			halGfxDrawHLine(buf, color, px, -x + px, -y + py);
			halGfxDrawHLine(buf, color, px, -y + px, -x + py);
		}
		// Top right
		if ((cornerMask & CIRCLE_CORNER_TR) == CIRCLE_CORNER_TR) {
			halGfxDrawHLine(buf, color, px, x + px, -y + py);
			halGfxDrawHLine(buf, color, px, y + px, -x + py);
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

void halGfxDrawRectangle(JS_BUFFER buf, int16_t color, int16_t left, int16_t top, int16_t right, int16_t bottom,
                         int16_t cornerRadius) {
	if ((left == right) || (top == bottom)) {
		// Draw nothing.
		return;
	}

	halGfxDrawLine(buf, color, left + cornerRadius, top, right - cornerRadius, top);
	halGfxDrawLine(buf, color, right, top + cornerRadius, right, bottom - cornerRadius);
	halGfxDrawLine(buf, color, left, top + cornerRadius, left, bottom - cornerRadius);
	halGfxDrawLine(buf, color, left + cornerRadius, bottom, right - cornerRadius, bottom);

	halGfxDrawCircle(buf, color, left + cornerRadius, top + cornerRadius, cornerRadius, CIRCLE_CORNER_TL);
	halGfxDrawCircle(buf, color, right - cornerRadius, top + cornerRadius, cornerRadius, CIRCLE_CORNER_TR);
	halGfxDrawCircle(buf, color, left + cornerRadius, bottom - cornerRadius, cornerRadius, CIRCLE_CORNER_BL);
	halGfxDrawCircle(buf, color, right - cornerRadius, bottom - cornerRadius, cornerRadius, CIRCLE_CORNER_BR);
}

void halGfxFillRectangle(JS_BUFFER buf, int16_t color, int16_t left, int16_t top, int16_t right, int16_t bottom,
                         int16_t cornerRadius) {
	if ((left >= right) || (top >= bottom)) {
		// Draw nothing.
		return;
	}

	int16_t y = 0;

	y = top + cornerRadius;
	while (y <= bottom - cornerRadius) {
		halGfxDrawLine(buf, color, left, y, right, y);
		y++;
	}

	y = top;
	while (y < top + cornerRadius) {
		halGfxDrawLine(buf, color, left + cornerRadius, y, right - cornerRadius, y);
		y++;
	}

	y = bottom - cornerRadius;
	while (y <= bottom) {
		halGfxDrawLine(buf, color, left + cornerRadius, y, right - cornerRadius, y);
		y++;
	}

	halGfxFillCircle(buf, color, left + cornerRadius, top + cornerRadius, cornerRadius, CIRCLE_CORNER_TL);
	halGfxFillCircle(buf, color, right - cornerRadius, top + cornerRadius, cornerRadius, CIRCLE_CORNER_TR);
	halGfxFillCircle(buf, color, left + cornerRadius, bottom - cornerRadius, cornerRadius, CIRCLE_CORNER_BL);
	halGfxFillCircle(buf, color, right - cornerRadius, bottom - cornerRadius, cornerRadius, CIRCLE_CORNER_BR);
}

void halGfxDrawTriangle(JS_BUFFER buf, int16_t color, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                        int16_t y2) {
	if (triangleArea(x0, y0, x1, y1, x2, y2) == 0)
		return;

	halGfxDrawLine(buf, color, x0, y0, x1, y1);
	halGfxDrawLine(buf, color, x0, y0, x2, y2);
	halGfxDrawLine(buf, color, x1, y1, x2, y2);
}

void halGfxFillTriangle(JS_BUFFER buf, int16_t color, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                        int16_t y2) {
	float_t area = triangleArea(x0, y0, x1, y1, x2, y2);
	if (area == 0)
		return;

	int16_t minX, minY, maxX, maxY;
	gfx_size_t bmpSize = halGfxGetBitmapSize(buf);

	minX = max(min(min(x0, x1), x2), 0);
	minY = max(min(min(y0, y1), y2), 0);
	maxX = min(max(max(x0, x1), x2), bmpSize.width - 1);
	maxY = min(max(max(y0, y1), y2), bmpSize.height - 1);

	if (minX > maxX || minY > maxY)
		return;

	for (int16_t y = minY; y < maxY; y++) {
		for (int16_t x = minX; x < maxX; x++) {
			float_t e0 = triangleArea(x1, y1, x2, y2, x, y) / area;
			float_t e1 = triangleArea(x2, y2, x0, y0, x, y) / area;
			float_t e2 = triangleArea(x0, y0, x1, y1, x, y) / area;

			if (e0 >= 0 && e1 >= 0 && e2 >= 0) {
				halGfxDrawDot(buf, color, x, y);
			}
		}
	}
}

void halGfxDrawBitmap1Bpp(JS_BUFFER buf, int16_t color, const uint8_t* data, uint16_t width, uint16_t height, int16_t x,
                          int16_t y) {
	for (int16_t yi = 0; yi < height; yi++) {
		for (int16_t xi = 0; xi < width; xi += 8) {
			uint8_t b = *data;
			data++;

			if ((b & 0x01) == 0x01)
				halGfxDrawDot(buf, color, x + xi, y + yi);
			if ((b & 0x02) == 0x02)
				halGfxDrawDot(buf, color, x + xi + 1, y + yi);
			if ((b & 0x04) == 0x04)
				halGfxDrawDot(buf, color, x + xi + 2, y + yi);
			if ((b & 0x08) == 0x08)
				halGfxDrawDot(buf, color, x + xi + 3, y + yi);
			if ((b & 0x10) == 0x10)
				halGfxDrawDot(buf, color, x + xi + 4, y + yi);
			if ((b & 0x20) == 0x20)
				halGfxDrawDot(buf, color, x + xi + 5, y + yi);
			if ((b & 0x40) == 0x40)
				halGfxDrawDot(buf, color, x + xi + 6, y + yi);
			if ((b & 0x80) == 0x80)
				halGfxDrawDot(buf, color, x + xi + 7, y + yi);
		}
	}
}

void halGfxDrawBitmapRgb565(JS_BUFFER buf, const uint16_t* data, uint16_t width, int16_t height, int16_t x,
                            uint16_t y) {
	for (int16_t yi = 0; yi < height; yi++) {
		for (int16_t xi = 0; xi < width; xi++) {
			halGfxDrawDot(buf, data[yi * width + xi], x + xi, y + yi);
		}
	}
}

void halGfxDrawBitmap(JS_BUFFER buf, JS_BUFFER bmp, int16_t x, int16_t y) {
	gfx_size_t bmpSize = halGfxGetBitmapSize(bmp);

	halGfxDrawBitmapRgb565(buf, (const uint16_t*) halGfxGetBitmapData(bmp), bmpSize.width, bmpSize.height, x, y);
}

void halGfxDrawBitmapWithTransparent(JS_BUFFER buf, JS_BUFFER bmp, int16_t x, int16_t y, uint16_t transparentColor) {
	gfx_size_t bmpSize = halGfxGetBitmapSize(bmp);
	uint16_t* data = halGfxGetBitmapData(bmp);

	for (int16_t yi = 0; yi < bmpSize.height; yi++) {
		for (int16_t xi = 0; xi < bmpSize.width; xi++) {
			uint16_t pix = data[yi * bmpSize.width + xi];
			if (pix == transparentColor)
				continue;
			halGfxDrawDot(buf, pix, x + xi, y + yi);
		}
	}
}

u_size_t halGfxDrawChar(JS_BUFFER buf, int16_t color, uint16_t c, int16_t x, int16_t y) {
	u_size_t size;
	hal_font_glyph_t glyph = halFontGetGlyph(c);

	if (glyph.width > 0 && glyph.height > 0) {
		halGfxDrawBitmap1Bpp(buf, color, glyph.data, glyph.width, glyph.height, x, y);
		size.size.width = glyph.width;
		size.size.height = glyph.height;
	}

	return size;
}

void halGfxDrawText(JS_BUFFER buf, int16_t color, const char* string, int16_t x, int16_t y) {
	uint16_t unicode = 0;
	int16_t dx = x, dy = y;
	u_size_t char_size;
	char* s = (char*) string;

	while ((unicode = halUtfReadUtf8Char(&s)) != 0) {
		if (unicode == 0xd || unicode == 0xa) {
			dx = x;
			dy += TEXT_LINE_HEIGHT;
		} else {
			char_size = halGfxDrawChar(buf, color, unicode, dx, dy);
			dx += char_size.size.width;
		}
	}
}

uint32_t halGfxMeasureText(const char* string) {
	u_size_t size;
	uint16_t unicode = 0;
	uint16_t width = 0;
	char* s = (char*) string;

	size.size.height = TEXT_LINE_HEIGHT;
	while ((unicode = halUtfReadUtf8Char(&s)) != 0) {
		if (unicode == 0xd || unicode == 0xa) {
			size.size.width = (width > size.size.width) ? width : size.size.width;
			width = 0;
			size.size.height += TEXT_LINE_HEIGHT;
		} else {
			hal_font_glyph_t glyph = halFontGetGlyph(unicode);
			width += glyph.width;
		}
	}
	size.size.width = (width > size.size.width) ? width : size.size.width;
	return size.val;
}

void halGfxDrawIcon(JS_BUFFER buf, int16_t color, uint32_t id, int16_t x, int16_t y) {
	hal_font_glyph_t glyph = halFontGetIcon(id);

	if (glyph.width > 0 && glyph.height > 0) {
		halGfxDrawBitmap1Bpp(buf, color, glyph.data, glyph.width, glyph.height, x, y);
	}
}
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
JS_BUFFER halGfxNewBitmap(uint16_t width, uint16_t height, uint16_t color) {
	JS_BUFFER newBuffer;
	newBuffer.size = (width * height * 2) + sizeof(gfx_size_t);
	newBuffer.buf = mcujs_alloc_function(0, newBuffer.size);
	gfx_size_t* pSize = (gfx_size_t*) newBuffer.buf;
	pSize->width = width;
	pSize->height = height;
	uint16_t* texData = (uint16_t*) &newBuffer.buf[sizeof(gfx_size_t)];

	for (int32_t i = 0; i < (width * height); i++) {
		texData[i] = color;
	}

	return newBuffer;
}

void halGfxReleaseBitmap(JS_BUFFER buf) {
	mcujs_free_function(0, buf.buf);
}