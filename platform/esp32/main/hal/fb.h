#include "TTGO.h"
#include "esp_partition.h"
#include "global.h"
// #include "../__generated/gen_font_ascii.h"
// #include "../__generated/gen_font_unicode.h"

// #define ASCII_FONT font_8
// #define ASCII_FONT_CELL_WIDTH font_8_cell_width
// #define ASCII_FONT_CELL_HEIGHT font_8_cell_height
// #define ASCII_FONT_CHARS chars_8
// #define ASCII_FONT_CHAR_COUNT font_8_char_count

// #define UNICODE_FONT font_16
// #define UNICODE_FONT_CELL_WIDTH font_16_cell_width
// #define UNICODE_FONT_CELL_HEIGHT font_16_cell_height
// #define UNICODE_FONT_CHARS chars_16
// #define UNICODE_FONT_CHAR_COUNT font_16_char_count

#define TEXT_LINE_HEIGHT 16

typedef struct hal_font_section_info_t {
	uint16_t codeStart;
	uint16_t codeEnd;
	uint16_t charWidth;
	uint16_t glyphEntrySize;
	uint32_t dataOffset;
} hal_font_section_info_t;

typedef struct hal_font_t {
	uint32_t magic; // "FONT"
	uint16_t sectionCount;
} hal_font_t;

static hal_font_t* font;
static hal_font_section_info_t* sections;
static uint16_t* FrameBuffer;
static uint32_t PenColor = 0;

TTGOClass* ttgo;
TFT_eSPI tft;

void halFbClearScreen();

void swapInt(int32_t* a, int32_t* b) {
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}

float_t triangleArea(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
	return (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
}

void initFont() {
	const void* font_data;
	spi_flash_mmap_handle_t handle;
	const esp_partition_t* part =
	    esp_partition_find_first((esp_partition_type_t) 64, (esp_partition_subtype_t) 0, "font");
	esp_err_t error = esp_partition_mmap(part, 0, part->size, SPI_FLASH_MMAP_DATA, &font_data, &handle);

	ESP_ERROR_CHECK(error);

	font = (hal_font_t*) font_data;
	sections = (hal_font_section_info_t*) (((uint8_t*) font_data) + 6);

	printf("Font partition address > 0x%x\n", part->address);
	printf("Font partition size > 0x%x\n", part->size);
	printf("Magic %x\n", font->magic);
	printf("First section info:\n    code start=0x%x\n    code end=0x%x\n    char width=0x%x\n    data offset=0x%x\n   "
	       " entry size=0x%x\n",
	       sections->codeStart, sections->codeEnd, sections->charWidth, sections->dataOffset, sections->glyphEntrySize);
}

void halFbInit() {
	initFont();

	ttgo = TTGOClass::getWatch();
	tft = *ttgo->eTFT;

	ttgo->begin();
	ttgo->openBL();

	tft.setRotation(0);
	tft.fillScreen(TFT_BLACK);

	FrameBuffer = (uint16_t*) malloc(TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t));
	halFbClearScreen();
}

void halFbUpdate() {
	tft.setSwapBytes(true);
	tft.pushImage(0, 0, TFT_WIDTH, TFT_HEIGHT, FrameBuffer);
}

void halFbSetPenColor(uint32_t color) {
	PenColor = color;
}

uint32_t halFbGetPenColor() {
	return PenColor;
}

void halFbClearScreen() {
	memset(FrameBuffer, 0, TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t));
}

void IRAM_ATTR halFbDrawDot(int32_t x, int32_t y, int32_t color = -1) {
	if (x < 0 || x >= TFT_WIDTH || y < 0 || y >= TFT_HEIGHT)
		return;

	if (color == -1)
		color = PenColor;

	FrameBuffer[(y * TFT_WIDTH) + x] = (uint16_t) color;
}

void halFbDrawHLine(int32_t x0, int32_t x1, int32_t y) {
	int32_t a, b;
	a = min(x0, x1);
	b = max(x0, x1);

	if (b < 0 || a > TFT_WIDTH || y < 0 || y > TFT_HEIGHT)
		return;

	if (a < 0)
		a = 0;

	a += (y * TFT_WIDTH);
	b += (y * TFT_WIDTH);

	for (; a < b; a++) {
		FrameBuffer[a] = (uint16_t) PenColor;
	}
}

void halFbDrawVLine(int32_t y0, int32_t y1, int32_t x) {
	int32_t a, b;
	a = min(y0, y1);
	b = max(y0, y1);

	if (b < 0 || a > TFT_HEIGHT || x < 0 || x > TFT_WHITE)
		return;

	if (a < 0)
		a = 0;

	for (; a < b; a++) {
		FrameBuffer[a * TFT_WIDTH + x] = (uint16_t) PenColor;
	}
}

void halFbDrawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
	if (y0 == y1) {
		halFbDrawHLine(x0, x1, y0);
		return;
	} else if (x0 == x1) {
		halFbDrawVLine(y0, y1, x0);
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
				halFbDrawDot(curY, curX);
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
				halFbDrawDot(curX, curY);
				curX += stepX;
			}
		}
	}
}

void halFbDrawRectangle(int32_t left, int32_t top, int32_t right, int32_t bottom) {
	halFbDrawLine(left, top, right, top);
	halFbDrawLine(right, top, right, bottom);
	halFbDrawLine(left, top, left, bottom);
	halFbDrawLine(left, bottom, right, bottom);
}

void halFbFillRectangle(int32_t left, int32_t top, int32_t right, int32_t bottom) {
	while (top <= bottom) {
		halFbDrawLine(left, top, right, top);
		top++;
	}
}

void halFbDrawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
	if (triangleArea(x0, y0, x1, y1, x2, y2) == 0)
		return;

	halFbDrawLine(x0, y0, x1, y1);
	halFbDrawLine(x0, y0, x2, y2);
	halFbDrawLine(x1, y1, x2, y2);
}

void halFbFillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
	float_t area = triangleArea(x0, y0, x1, y1, x2, y2);
	if (area == 0)
		return;

	int32_t minX, minY, maxX, maxY;
	minX = max(min(min(x0, x1), x2), 0);
	minY = max(min(min(y0, y1), y2), 0);
	maxX = min(max(max(x0, x1), x2), TFT_WIDTH - 1);
	maxY = min(max(max(y0, y1), y2), TFT_HEIGHT - 1);

	if (minX > maxX || minY > maxY)
		return;

	for (int32_t y = minY; y < maxY; y++) {
		for (int32_t x = minX; x < maxX; x++) {
			float_t e0 = triangleArea(x1, y1, x2, y2, x, y) / area;
			float_t e1 = triangleArea(x2, y2, x0, y0, x, y) / area;
			float_t e2 = triangleArea(x0, y0, x1, y1, x, y) / area;

			if (e0 >= 0 && e1 >= 0 && e2 >= 0) {
				halFbDrawDot(x, y);
			}
		}
	}
}

void halFbDrawCircle(int32_t px, int32_t py, int32_t r) {
	int32_t x = 0, y = r, d = 1 - r;

	while (y >= x) {
		halFbDrawDot(x + px, y + py);
		halFbDrawDot(y + px, x + py);
		halFbDrawDot(-x + px, y + py);
		halFbDrawDot(-y + px, x + py);

		halFbDrawDot(-x + px, -y + py);
		halFbDrawDot(-y + px, -x + py);
		halFbDrawDot(x + px, -y + py);
		halFbDrawDot(y + px, -x + py);

		if (d <= 0) {
			d += (x << 2) + 6;
		} else {
			d += ((x - y) << 2) + 10;
			y--;
		}
		x++;
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

	return nullptr;
}

int32_t halFbMeasureTextWidth(const char* string) {
	uint16_t unicode = 0;
	int32_t currPos = 0, width = 0;
	readUtf8Char(&unicode, &currPos, string);

	while (unicode) {
		hal_font_section_info_t* section = halGetFontSection(unicode);
		readUtf8Char(&unicode, &currPos, string);

		if (section == nullptr)
			continue;

		width += section->charWidth;
	}

	return width;
}

int32_t halFbMeasureTextHeight(const char* string) {
	return 16;
}

void IRAM_ATTR drawGlyph(const uint8_t* glyph, uint16_t width, uint16_t height, int32_t x, int32_t y) {
	uint16_t mask = 1;
	uint32_t bitsIndex = 0;

	for (int32_t yi = 0; yi < height; yi++) {
		for (int32_t xi = 0; xi < width; xi++) {
			uint8_t bit = readBit(glyph, yi * width + xi);
			uint8_t c = bit == 0 ? 0 : 255;

			uint16_t b = (c >> 3) & 0x1f;
			uint16_t g = ((c >> 2) & 0x3f) << 5;
			uint16_t r = ((c >> 3) & 0x1f) << 11;
			uint16_t c565 = r | g | b;
			if (c565 != 0)
				halFbDrawDot(x + xi, y + yi, c565);

			mask = mask << 1;
			if (mask == 256) {
				mask = 1;
				bitsIndex++;
			}
		}
	}
}

int32_t IRAM_ATTR halFbDrawChar(uint16_t c, int32_t x, int32_t y) {
	int32_t charIndex = -1;
	const uint8_t* glyph = nullptr;
	hal_font_section_info_t* section = halGetFontSection(c);

	if (section == nullptr)
		return 0;

	glyph = (const uint8_t*) (&((uint8_t*) font)[0] +
	                          (section->dataOffset + (section->glyphEntrySize * (c - section->codeStart))));
	drawGlyph(glyph, section->charWidth, 16, x, y);
	return section->charWidth;
}

void halFbDrawText(const char* string, int32_t x, int32_t y) {
	uint16_t unicode = 0;
	int32_t currPos = 0;
	readUtf8Char(&unicode, &currPos, string);

	while (unicode) {
		if (unicode == 0xd || unicode == 0xa) {
			x = 0;
			y += TEXT_LINE_HEIGHT;
		} else {
			x += halFbDrawChar(unicode, x, y);
		}

		readUtf8Char(&unicode, &currPos, string);
	}
}