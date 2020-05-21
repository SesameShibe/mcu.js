#pragma once
#include "driver/spi_master.h"
#include "esp_partition.h"
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
#define LCD_SPI_HW SPI3
spi_device_handle_t lcdSpiDev;

#define CIRCLE_CORNER_TL 1 << 0
#define CIRCLE_CORNER_TR 1 << 1
#define CIRCLE_CORNER_BL 1 << 2
#define CIRCLE_CORNER_BR 1 << 3
#define TEXT_LINE_HEIGHT 16

typedef struct hal_font_section_info_t {
	uint16_t codeStart;
	uint16_t codeEnd;
	uint16_t charWidth;
	uint16_t charHeight;
	uint16_t glyphEntrySize;
	uint32_t dataOffset;
} hal_font_section_info_t;

typedef struct hal_font_t {
	uint32_t magic; // "FONT"
	uint16_t sectionCount;
	uint16_t version;
} hal_font_t;

static hal_font_t* font;
static hal_font_section_info_t* sections;

static uint16_t lcdFB[LCD_WIDTH * LCD_HEIGHT];
uint16_t lcdRowOffset, lcdColOffset;
static uint32_t penColor = 0;

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
	halLcdWritePixels((u32*) lcdFB, sizeof(lcdFB) / 4);
	LCD_SET_CS(1);
}

void halLcdInitFont() {
	const void* font_data;
	spi_flash_mmap_handle_t handle;
	const esp_partition_t* part =
	    esp_partition_find_first((esp_partition_type_t) 64, (esp_partition_subtype_t) 0, "font");
	esp_err_t error = esp_partition_mmap(part, 0, part->size, SPI_FLASH_MMAP_DATA, &font_data, &handle);

	ESP_ERROR_CHECK(error);

	font = (hal_font_t*) font_data;
	sections = (hal_font_section_info_t*) (((uint8_t*) font_data) + 8);

	printf("Font partition address > 0x%x\n", part->address);
	printf("Font partition size > 0x%x\n", part->size);
	printf("Magic %x\nVersion:%x\n", font->magic, font->version);
	printf("First section info:\n"
	       "    code start=0x%x\n"
	       "    code end=0x%x\n"
	       "    char width=0x%x\n"
	       "    char height=0x%x\n"
	       "    entry size=0x%x\n"
	       "    data offset=0x%x\n",
	       sections->codeStart, sections->codeEnd, sections->charWidth, sections->charHeight, sections->glyphEntrySize,
	       sections->dataOffset);
}

void halLcdInit() {
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
	memset(lcdFB, 0x0f, sizeof(lcdFB));
	halLcdUpdate();
	halGpioWrite(LCD_PIN_BL, 1);

	halLcdInitFont();
}

JS_BUFFER halLcdGetFB() {
	JS_BUFFER buf = { .buf = (u8*) lcdFB, .size = sizeof(lcdFB) };
	return buf;
}

void swapInt(int32_t* a, int32_t* b) {
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}

float_t triangleArea(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
	return (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
}

int32_t min(int32_t a, int32_t b) {
	return a > b ? b : a;
}

int32_t max(int32_t a, int32_t b) {
	return a > b ? a : b;
}

void halLcdSetpenColor(uint32_t color) {
	penColor = color;
}

uint32_t halLcdGetpenColor() {
	return penColor;
}

void IRAM_ATTR halLcdDrawDot(int32_t x, int32_t y, int32_t color) {
	if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT)
		return;

	if (color == -1)
		color = penColor;

	lcdFB[(y * LCD_WIDTH) + x] = (uint16_t) color;
}

void halLcdDrawHLine(int32_t x0, int32_t x1, int32_t y) {
	int32_t a, b;
	a = min(x0, x1);
	b = max(x0, x1);

	if (b < 0 || a > LCD_WIDTH || y < 0 || y > LCD_HEIGHT)
		return;

	if (a < 0)
		a = 0;

	a += (y * LCD_WIDTH);
	b += (y * LCD_WIDTH);

	for (; a < b; a++) {
		lcdFB[a] = (uint16_t) penColor;
	}
}

void halLcdDrawVLine(int32_t y0, int32_t y1, int32_t x) {
	int32_t a, b;
	a = min(y0, y1);
	b = max(y0, y1);

	if (b < 0 || a > LCD_HEIGHT || x < 0 || x > LCD_WIDTH)
		return;

	if (a < 0)
		a = 0;

	for (; a < b; a++) {
		lcdFB[a * LCD_WIDTH + x] = (uint16_t) penColor;
	}
}

void halLcdDrawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
	if (y0 == y1) {
		halLcdDrawHLine(x0, x1, y0);
		return;
	} else if (x0 == x1) {
		halLcdDrawVLine(y0, y1, x0);
		return;
	} else {
		int32_t dx = abs(x1 - x0), dy = abs(y1 - y0);
		bool steep = dx < dy;

		if (steep) {
			swapInt(&x0, &y0);
			swapInt(&x1, &y1);
			swapInt(&dx, &dy);
		}

		int32_t stepX = (x1 - x0) > 0 ? 1 : -1, stepY = (y1 - y0) > 0 ? 1 : -1, curX = x0, curY = y0, n2dy = dy << 1,
		        n2dydx = (dy - dx) << 1, d = (dy << 1) - dx;

		if (steep) {
			while (curX != x1) {
				if (d < 0) {
					d += n2dy;
				} else {
					curY += stepY;
					d += n2dydx;
				}
				halLcdDrawDot(curY, curX, -1);
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
				halLcdDrawDot(curX, curY, -1);
				curX += stepX;
			}
		}
	}
}

void halLcdDrawCircle(int32_t px, int32_t py, int32_t r, int32_t cornerMask) {
	int32_t x = 0, y = r, d = 1 - r;

	while (y >= x) {
		// Bottom right
		if ((cornerMask & CIRCLE_CORNER_BR) == CIRCLE_CORNER_BR) {
			halLcdDrawDot(x + px, y + py, -1);
			halLcdDrawDot(y + px, x + py, -1);
		}
		// Bottom left
		if ((cornerMask & CIRCLE_CORNER_BL) == CIRCLE_CORNER_BL) {
			halLcdDrawDot(-x + px, y + py, -1);
			halLcdDrawDot(-y + px, x + py, -1);
		}
		// Top left
		if ((cornerMask & CIRCLE_CORNER_TL) == CIRCLE_CORNER_TL) {
			halLcdDrawDot(-x + px, -y + py, -1);
			halLcdDrawDot(-y + px, -x + py, -1);
		}
		// Top right
		if ((cornerMask & CIRCLE_CORNER_TR) == CIRCLE_CORNER_TR) {
			halLcdDrawDot(x + px, -y + py, -1);
			halLcdDrawDot(y + px, -x + py, -1);
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

void halLcdFillCircle(int32_t px, int32_t py, int32_t r, int32_t cornerMask) {
	int32_t x = 0, y = r, d = 1 - r;

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

void halLcdDrawRectangle(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t cornerRadius) {
	halLcdDrawLine(left + cornerRadius, top, right - cornerRadius, top);
	halLcdDrawLine(right, top + cornerRadius, right, bottom - cornerRadius);
	halLcdDrawLine(left, top + cornerRadius, left, bottom - cornerRadius);
	halLcdDrawLine(left + cornerRadius, bottom, right - cornerRadius, bottom);

	halLcdDrawCircle(left + cornerRadius, top + cornerRadius, cornerRadius, CIRCLE_CORNER_TL);
	halLcdDrawCircle(right - cornerRadius, top + cornerRadius, cornerRadius, CIRCLE_CORNER_TR);
	halLcdDrawCircle(left + cornerRadius, bottom - cornerRadius, cornerRadius, CIRCLE_CORNER_BL);
	halLcdDrawCircle(right - cornerRadius, bottom - cornerRadius, cornerRadius, CIRCLE_CORNER_BR);
}

void halLcdFillRectangle(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t cornerRadius) {
	int32_t y = 0;

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

void halLcdDrawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
	if (triangleArea(x0, y0, x1, y1, x2, y2) == 0)
		return;

	halLcdDrawLine(x0, y0, x1, y1);
	halLcdDrawLine(x0, y0, x2, y2);
	halLcdDrawLine(x1, y1, x2, y2);
}

void halLcdFillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
	float_t area = triangleArea(x0, y0, x1, y1, x2, y2);
	if (area == 0)
		return;

	int32_t minX, minY, maxX, maxY;
	minX = max(min(min(x0, x1), x2), 0);
	minY = max(min(min(y0, y1), y2), 0);
	maxX = min(max(max(x0, x1), x2), LCD_WIDTH - 1);
	maxY = min(max(max(y0, y1), y2), LCD_HEIGHT - 1);

	if (minX > maxX || minY > maxY)
		return;

	for (int32_t y = minY; y < maxY; y++) {
		for (int32_t x = minX; x < maxX; x++) {
			float_t e0 = triangleArea(x1, y1, x2, y2, x, y) / area;
			float_t e1 = triangleArea(x2, y2, x0, y0, x, y) / area;
			float_t e2 = triangleArea(x0, y0, x1, y1, x, y) / area;

			if (e0 >= 0 && e1 >= 0 && e2 >= 0) {
				halLcdDrawDot(x, y, -1);
			}
		}
	}
}

size_t IRAM_ATTR readUtf8Char(uint16_t* readChar, int32_t* ppos, const char* string) {
	size_t byteCount = 0;
	uint16_t unicode = 0;
	uint32_t pos = *ppos;

	if ((string[pos] & 0x80) == 0) {
		unicode = (uint16_t) string[pos];
		byteCount = 1;
	} else if ((string[pos] & 0xE0) == 0xC0) {
		unicode = ((string[pos] & 0x1F) << 6) | (string[pos + 1] & 0x3F);
		byteCount = 2;
	} else if ((string[pos] & 0xF0) == 0xE0) {
		unicode = ((string[pos] & 0x0F) << 12) | ((string[pos + 1] & 0x3F) << 6) | (string[pos + 2] & 0x3F);
		byteCount = 3;
	}

	*readChar = unicode;
	*ppos = pos + byteCount;
	return byteCount;
}

static inline uint8_t IRAM_ATTR readBit(const uint8_t* data, int32_t index) {
	uint8_t retv = 0;

	// printf("Reading bit at: %d\n", index);
	uint8_t bits = data[(index >> 3)];
	retv = (bits >> (index & 7)) & 1;

	return retv;
}

static inline hal_font_section_info_t* halGetFontSection(uint16_t c) {
	hal_font_section_info_t* section = sections;

	for (int i = 0; i < font->sectionCount; i++) {
		if (c >= section->codeStart && c <= section->codeEnd)
			return section;
		section = &sections[i];
	}

	return NULL;
}

int32_t halLcdMeasureTextWidth(const char* string) {
	uint16_t unicode = 0;
	int32_t currPos = 0, width = 0;
	readUtf8Char(&unicode, &currPos, string);

	while (unicode) {
		hal_font_section_info_t* section = halGetFontSection(unicode);
		readUtf8Char(&unicode, &currPos, string);

		if (section == NULL)
			continue;

		width += section->charWidth;
	}

	return width;
}

int32_t halLcdMeasureTextHeight(const char* string) {
	return 16;
}

void IRAM_ATTR drawGlyph(const uint8_t* glyphData, uint16_t width, uint16_t height, int32_t x, int32_t y, int32_t bLeft,
                         int32_t bTop, int32_t bRight, int32_t bBottom) {
	uint16_t mask = 1;
	uint32_t bitsIndex = 0;

	for (int32_t yi = 0; yi < height; yi++) {
		for (int32_t xi = 0; xi < width; xi++) {
			// Don't draw outside bbox
			if ((x + xi < bLeft) || (x + xi > bRight) || (y + yi < bTop) || (y + yi > bBottom)) {
				continue;
			}

			uint8_t bit = readBit(glyphData, yi * width + xi);

			if (bit != 0)
				halLcdDrawDot(x + xi, y + yi, -1);

			mask = mask << 1;
			if (mask == 256) {
				mask = 1;
				bitsIndex++;
			}
		}
	}
}

int32_t IRAM_ATTR halLcdDrawChar(uint16_t c, int32_t x, int32_t y, int32_t bLeft, int32_t bTop, int32_t bRight,
                                 int32_t bBottom) {
	const uint8_t* glyphData = NULL;
	hal_font_section_info_t* section = halGetFontSection(c);

	if (section == NULL)
		return 0;

	glyphData = (const uint8_t*) (&((uint8_t*) font)[0] +
	                              (section->dataOffset + (section->glyphEntrySize * (c - section->codeStart))));
	drawGlyph(glyphData, section->charWidth, section->charHeight, x, y + (16 - section->charHeight), bLeft, bTop,
	          bRight, bBottom);
	return section->charWidth;
}

void halLcdDrawText(const char* string, int32_t x, int32_t y, int32_t bLeft, int32_t bTop, int32_t bRight,
                    int32_t bBottom) {
	uint16_t unicode = 0;
	int32_t currPos = 0;
	int32_t dx = x, dy = y;
	readUtf8Char(&unicode, &currPos, string);

	while (unicode) {
		if (unicode == 0xd || unicode == 0xa) {
			dx = x;
			dy += TEXT_LINE_HEIGHT;
		} else {
			dx += halLcdDrawChar(unicode, dx, dy, bLeft, bTop, bRight, bBottom);
		}

		readUtf8Char(&unicode, &currPos, string);
	}
}