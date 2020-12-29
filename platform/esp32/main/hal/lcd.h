#pragma once
#include "driver/spi_master.h"
#include "esp_partition.h"
#include "font.h"
#include "global.h"
#include "gpio.h"

#define LCD_PIN_DC (27)
#define LCD_PIN_CS (5)
#define LCD_PIN_SCLK (18)
#define LCD_PIN_MOSI (19)
#define LCD_PIN_BL (12)
#define LCD_WIDTH (240)
#define LCD_HEIGHT (240)
#define LCD_CMD_CASET 0x2A
#define LCD_CMD_PASET 0x2B
#define LCD_CMD_RAMWR 0x2C

static const uint8_t lcdInitCode[] = {
	0x11, 0x00, // cmd
	0xFF, 120, // delay
	0x13, 0x00, // cmd
	0x36, 0x01, 0x08, // cmd
	0xB6, 0x02, 0x0A, 0x82, // cmd
	0x3A, 0x01, 0x55, // cmd
	0xFF, 10, // delay
	0xB2, 0x05, 0x0C, 0x0C, 0x00, 0x33, 0x33, // cmd
	0xB7, 0x01, 0x35, // cmd
	0xBB, 0x01, 0x28, // cmd
	0xC0, 0x01, 0x0C, // cmd
	0xC2, 0x02, 0x01, 0xFF, // cmd
	0xC3, 0x01, 0x10, // cmd
	0xC4, 0x01, 0x20, // cmd
	0xC6, 0x01, 0x0F, // cmd
	0xD0, 0x02, 0xA4, 0xA1, // cmd
	0xE0, 0x0E, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17, // cmd
	0xE1, 0x0E, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E, // cmd
	0x21, 0x00, // cmd
	0x2A, 0x04, 0x00, 0x00, 0x00, 0xE5, // cmd
	0x2B, 0x04, 0x00, 0x00, 0x01, 0x3F, // cmd
	0xFF, 120, // delay
	0x29, 0x00, // cmd
	0xFF, 120, // delay
	0xFF, 0xFF // FIN
};

#define LCD_SET_CS(v) halGpioWrite(LCD_PIN_CS, (v))
#define LCD_SET_DC(v) halGpioWrite(LCD_PIN_DC, (v))

#define LCD_SPI_BUS SPI3_HOST
#ifdef CONFIG_IDF_TARGET_ESP32S2
#define LCD_SPI_HW GPSPI3
#else
#define LCD_SPI_HW SPI3
#endif

spi_device_handle_t lcdSpiDev;

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

static uint16_t* lcdFB;
uint16_t lcdRowOffset, lcdColOffset;

static uint32_t penColor = 0;
static hal_rect_t renderRect = { 0, 0, 0, 0 };

static ALWAYS_INLINE void halLcdSpiWrite(u32 dat, u8 bitlen) {
	LCD_SPI_HW.mosi_dlen.val = bitlen - 1;
	LCD_SPI_HW.miso_dlen.val = 0;
	LCD_SPI_HW.data_buf[0] = dat;
	LCD_SPI_HW.cmd.usr = 1;
	while (LCD_SPI_HW.cmd.usr)
		;
}

void IRAM_ATTR halLcdWriteCmd8(u8 dat) {
	LCD_SET_DC(0);
	LCD_SET_CS(0);
	halLcdSpiWrite(dat, 8);
	LCD_SET_CS(1);
}

void IRAM_ATTR halLcdWriteDat8(u8 dat) {
	LCD_SET_DC(1);
	LCD_SET_CS(0);
	halLcdSpiWrite(dat, 8);
	LCD_SET_CS(1);
}

static ALWAYS_INLINE void halLcdWritePixels(u32* buf, u32 lenInU32) {
	const int maxSendU32ForOnce = 16;
	int i;

	LCD_SPI_HW.mosi_dlen.val = (maxSendU32ForOnce * 32) - 1;
	LCD_SPI_HW.miso_dlen.val = 0;
	while (lenInU32 >= maxSendU32ForOnce) {
		for (i = 0; i < maxSendU32ForOnce; i++) {
			LCD_SPI_HW.data_buf[i] = buf[i];
		}
		LCD_SPI_HW.cmd.usr = 1;
		lenInU32 -= maxSendU32ForOnce;
		buf += maxSendU32ForOnce;
		while (LCD_SPI_HW.cmd.usr)
			;
	}
	for (i = 0; i < lenInU32; i++) {
		LCD_SPI_HW.mosi_dlen.val = (lenInU32 * 32) - 1;
		LCD_SPI_HW.data_buf[i] = buf[i];
		LCD_SPI_HW.cmd.usr = 1;
		while (LCD_SPI_HW.cmd.usr)
			;
	}
}

void halLcdExecuteInitCode(const u8* code) {
	int i;

	while (1) {
		u8 opCode = code[0];
		u8 datLen = code[1];
		if (opCode == 0xFF) {
			if (datLen == 0xFF) {
				break;
			} else {
				halOsSleepMs(datLen);
			}
			code += 2;
		} else {
			halLcdWriteCmd8(opCode);
			for (i = 0; i < datLen; i++) {
				halLcdWriteDat8(code[2 + i]);
			}
			code += 2 + datLen;
		}
	}
}

void IRAM_ATTR halLcdUpdate() {
	u16 c1 = 0, c2 = LCD_WIDTH - 1;
	u16 r1 = 0, r2 = LCD_HEIGHT - 1;

	halLcdWriteCmd8(LCD_CMD_CASET); // Column address set
	halLcdWriteDat8(c1 >> 8);
	halLcdWriteDat8(c1);
	halLcdWriteDat8(c2 >> 8);
	halLcdWriteDat8(c2);

	halLcdWriteCmd8(LCD_CMD_PASET); // Row address set
	halLcdWriteDat8(r1 >> 8);
	halLcdWriteDat8(r1);
	halLcdWriteDat8(r2 >> 8);
	halLcdWriteDat8(r2);

	halLcdWriteCmd8(LCD_CMD_RAMWR);
	LCD_SET_DC(1);
	LCD_SET_CS(0);
	halLcdWritePixels((u32*) lcdFB, LCD_WIDTH * LCD_HEIGHT * 2 / 4);
	LCD_SET_CS(1);
}

void halLcdInit() {
	lcdFB = mcujs_alloc_function(0, LCD_WIDTH * LCD_WIDTH * 2);
	spi_bus_config_t cfg = { .mosi_io_num = LCD_PIN_MOSI,
		                     .miso_io_num = -1,
		                     .sclk_io_num = LCD_PIN_SCLK,
		                     .quadwp_io_num = -1,
		                     .quadhd_io_num = -1,
		                     .max_transfer_sz = 0,
		                     .flags = 0,
		                     .intr_flags = 0 };
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz = 20000000, // Clock
		.mode = 0, // SPI mode 0
		.spics_io_num = -1, // CS pin
		.queue_size = 7, // We want to be able to queue 7 transactions at a time
		.pre_cb = NULL, // Specify pre-transfer callback to handle D/C line
	};
	esp_err_t ret = spi_bus_initialize(LCD_SPI_BUS, &cfg, 1);
	ESP_ERROR_CHECK(ret);
	ret = spi_bus_add_device(LCD_SPI_BUS, &devcfg, &lcdSpiDev);
	ESP_ERROR_CHECK(ret);

	LCD_SET_CS(1);
	LCD_SET_DC(1);
	halGpioConfig(LCD_PIN_CS, GPIO_MODE_OUTPUT, 0);
	halGpioConfig(LCD_PIN_DC, GPIO_MODE_OUTPUT, 0);
	halGpioConfig(LCD_PIN_BL, GPIO_MODE_OUTPUT, 0);

	/* Do a dummy transmission to config the freq settings */
	spi_transaction_t t;
	memset(&t, 0, sizeof(t)); // Zero out the transaction
	t.length = 8;
	t.flags = SPI_TRANS_USE_TXDATA;
	t.tx_data[0] = 0;
	ret = spi_device_polling_transmit(lcdSpiDev, &t); // Transmit!
	ESP_ERROR_CHECK(ret);

	halLcdExecuteInitCode(lcdInitCode);
	memset(lcdFB, 0x0f, LCD_WIDTH * LCD_HEIGHT * 2);
	halLcdUpdate();
	halGpioWrite(LCD_PIN_BL, 1);

	halFontInit();
}

void halLcbPrintBuf(JS_BUFFER buf, int32_t len) {
	if (len == -1)
		len = buf.size;

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

JS_BUFFER halLcdCopyFB(int16_t left, int16_t top, int16_t right, int16_t bottom) {
	JS_BUFFER buf;
	uint16_t* lcdFBCopy = mcujs_alloc_function(0, (right - left) * (bottom - top) * 2);
	if (left == 0 && top == 0 && right == LCD_WIDTH && bottom == LCD_HEIGHT) {
		// Should be faster when copy the whole screen.
		memcpy(lcdFBCopy, lcdFB, LCD_WIDTH * LCD_HEIGHT * 2);
		buf.buf = (uint8_t*) lcdFBCopy;
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
		buf.buf = (uint8_t*) lcdFBCopy;
		buf.size = index;
	}

	return buf;
}

void halLcbReleaseCopy(JS_BUFFER buf) {
	mcujs_free_function(&buf, buf.buf);
}

JS_BUFFER halLcdGetFB() {
	JS_BUFFER buf = { .buf = (u8*) lcdFB, .size = LCD_WIDTH * LCD_HEIGHT * 2 };
	return buf;
}

void halLcdSetFB(JS_BUFFER fb) {
	if (fb.size >= LCD_WIDTH * LCD_HEIGHT * 2) {
		memcpy(lcdFB, fb.buf, LCD_WIDTH * LCD_HEIGHT * 2);
	}
}

void halLcdPutPixels(int16_t x, int16_t y, int16_t width, int16_t height, JS_BUFFER buf) {
	int16_t right = x + width;
	int16_t bottom = y + height;
	size_t index = 0;
	uint16_t* pixels = (uint16_t*) buf.buf;

	int16_t yi = y;
	do {
		int16_t xi = x;
		do {
			if (index >= buf.size)
				break;

			if (xi < renderRect.left || xi >= renderRect.right || yi < renderRect.top || yi >= renderRect.bottom)
				continue;

			uint16_t pixel = pixels[index];
			lcdFB[(yi * LCD_WIDTH) + xi] = pixel;

			index++;
		} while (++xi < right);
	} while (++yi < bottom);
}

void halLcdClearScreen() {
	memset(lcdFB, 0, LCD_WIDTH * LCD_HEIGHT * 2);
}

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

void halLcdSetPenColor(uint32_t color) {
	penColor = color;
}

uint16_t halLcdGetPenColor() {
	return penColor;
}

void halLcdSetRenderRect(int16_t left, int16_t top, int16_t right, int16_t bottom) {
	renderRect.left = left < 0 ? 0 : left;
	renderRect.top = top < 0 ? 0 : top;
	renderRect.right = right > LCD_WIDTH ? LCD_WIDTH : right;
	renderRect.bottom = bottom > LCD_HEIGHT ? LCD_HEIGHT : bottom;
}

void IRAM_ATTR halLcdDrawDot(int16_t x, int16_t y) {
	// Ignore outside screen pixels
	if (x < renderRect.left || x >= renderRect.right || y < renderRect.top || y >= renderRect.bottom)
		return;

	if (penColor == COLOR_TRANSPARENT)
		return;

	lcdFB[(y * LCD_WIDTH) + x] = (uint16_t) penColor;
}

void halLcdDrawHLine(int16_t x0, int16_t x1, int16_t y) {
	int16_t a, b;
	a = min(x0, x1);
	b = max(x0, x1);

	if (a < 0)
		a = 0;

	for (; a < b; a++) {
		halLcdDrawDot(a, y);
	}
}

void halLcdDrawVLine(int16_t y0, int16_t y1, int16_t x) {
	int16_t a, b;
	a = min(y0, y1);
	b = max(y0, y1);

	if (a < 0)
		a = 0;

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

void halLcdDrawRectangle(int16_t left, int16_t top, int16_t right, int16_t bottom, int16_t cornerRadius) {
	if ((left == right) || (top == bottom)) {
		// Draw nothing.
		return;
	}

	halLcdDrawLine(left + cornerRadius, top, right - cornerRadius, top);
	halLcdDrawLine(right, top + cornerRadius, right, bottom - cornerRadius);
	halLcdDrawLine(left, top + cornerRadius, left, bottom - cornerRadius);
	halLcdDrawLine(left + cornerRadius, bottom, right - cornerRadius, bottom);

	halLcdDrawCircle(left + cornerRadius, top + cornerRadius, cornerRadius, CIRCLE_CORNER_TL);
	halLcdDrawCircle(right - cornerRadius, top + cornerRadius, cornerRadius, CIRCLE_CORNER_TR);
	halLcdDrawCircle(left + cornerRadius, bottom - cornerRadius, cornerRadius, CIRCLE_CORNER_BL);
	halLcdDrawCircle(right - cornerRadius, bottom - cornerRadius, cornerRadius, CIRCLE_CORNER_BR);
}

void halLcdFillRectangle(int16_t left, int16_t top, int16_t right, int16_t bottom, int16_t cornerRadius) {
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

	halLcdFillCircle(left + cornerRadius, top + cornerRadius, cornerRadius, CIRCLE_CORNER_TL);
	halLcdFillCircle(right - cornerRadius, top + cornerRadius, cornerRadius, CIRCLE_CORNER_TR);
	halLcdFillCircle(left + cornerRadius, bottom - cornerRadius, cornerRadius, CIRCLE_CORNER_BL);
	halLcdFillCircle(right - cornerRadius, bottom - cornerRadius, cornerRadius, CIRCLE_CORNER_BR);
}

void halLcdDrawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
	if (triangleArea(x0, y0, x1, y1, x2, y2) == 0)
		return;

	halLcdDrawLine(x0, y0, x1, y1);
	halLcdDrawLine(x0, y0, x2, y2);
	halLcdDrawLine(x1, y1, x2, y2);
}

void halLcdFillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
	float_t area = triangleArea(x0, y0, x1, y1, x2, y2);
	if (area == 0)
		return;

	int16_t minX, minY, maxX, maxY;
	minX = max(min(min(x0, x1), x2), 0);
	minY = max(min(min(y0, y1), y2), 0);
	maxX = min(max(max(x0, x1), x2), LCD_WIDTH - 1);
	maxY = min(max(max(y0, y1), y2), LCD_HEIGHT - 1);

	if (minX > maxX || minY > maxY)
		return;

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