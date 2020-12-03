void halLcdDrawDot(int16_t, int16_t);

typedef struct hal_font_section_info_t {
	uint16_t codeStart;
	uint16_t codeEnd;
	uint8_t charWidth;
	uint8_t charHeight;
	uint16_t glyphEntrySize;
	uint32_t dataOffset;
} hal_font_section_info_t;

typedef struct hal_font_t {
	uint32_t magic; // "FONT"
	uint16_t sectionCount;
	uint16_t version;
} hal_font_t;

typedef struct hal_icon_font_t {
	int32_t magic; // "ICON"
	int32_t sectionSize;
	int16_t entryCount;
	int8_t width;
	int8_t height;
	int32_t entrySize;
} hal_icon_font_t;

static hal_font_t* font;
static hal_font_section_info_t* sections;

static hal_icon_font_t* iconFont;
static uint32_t* iconIds;

#define TEXT_LINE_HEIGHT 16

size_t halFontReadUtf8Char(uint16_t* readChar, int32_t* ppos, const char* string) {
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

static hal_font_section_info_t* halFontGetSection(uint16_t c) {
	hal_font_section_info_t* section = sections;

	for (int i = 0; i < font->sectionCount; i++) {
		if (c >= section->codeStart && c <= section->codeEnd)
			return section;
		section = &sections[i];
	}

	return NULL;
}

int16_t halFontMeasureTextWidth(const char* string) {
	uint16_t unicode = 0;
	int16_t width = 0;
	int32_t currPos = 0;
	halFontReadUtf8Char(&unicode, &currPos, string);

	while (unicode) {
		hal_font_section_info_t* section = halFontGetSection(unicode);
		halFontReadUtf8Char(&unicode, &currPos, string);

		if (section == NULL)
			continue;

		width += section->charWidth;
	}

	return width;
}

int16_t halLcdMeasureTextHeight(const char* string) {
	return 16;
}

void halFontDrawGlyph(const uint8_t* glyphData, uint16_t width, uint16_t height, int16_t x, int16_t y) {
	for (int16_t yi = 0; yi < height; yi++) {
		for (int16_t xi = 0; xi < width; xi += 8) {
			uint8_t b = *glyphData;
			glyphData++;

			if ((b & 0x01) == 0x01) halLcdDrawDot(x + xi    , y + yi);
			if ((b & 0x02) == 0x02) halLcdDrawDot(x + xi + 1, y + yi);
			if ((b & 0x04) == 0x04) halLcdDrawDot(x + xi + 2, y + yi);
			if ((b & 0x08) == 0x08) halLcdDrawDot(x + xi + 3, y + yi);
			if ((b & 0x10) == 0x10) halLcdDrawDot(x + xi + 4, y + yi);
			if ((b & 0x20) == 0x20) halLcdDrawDot(x + xi + 5, y + yi);
			if ((b & 0x40) == 0x40) halLcdDrawDot(x + xi + 6, y + yi);
			if ((b & 0x80) == 0x80) halLcdDrawDot(x + xi + 7, y + yi);
		}
	}
}

uint16_t halFontDrawChar(uint16_t c, int16_t x, int16_t y) {
	const uint8_t* glyphData = NULL;
	hal_font_section_info_t* section = halFontGetSection(c);

	if (section == NULL)
		return 0;

	glyphData = (const uint8_t*) (&((uint8_t*) font)[0] +
	                              (section->dataOffset + (section->glyphEntrySize * (c - section->codeStart))));
	halFontDrawGlyph(glyphData, section->charWidth, section->charHeight, x, y + (16 - section->charHeight));
	return section->charWidth;
}

void halFontDrawText(const char* string, int16_t x, int16_t y) {
	uint16_t unicode = 0;
	int32_t currPos = 0;
	int16_t dx = x, dy = y;
	halFontReadUtf8Char(&unicode, &currPos, string);

	while (unicode) {
		if (unicode == 0xd || unicode == 0xa) {
			dx = x;
			dy += TEXT_LINE_HEIGHT;
		} else {
			dx += halFontDrawChar(unicode, dx, dy);
		}

		halFontReadUtf8Char(&unicode, &currPos, string);
	}
}

void halFontDrawIcon(uint32_t id, int16_t x, int16_t y) {
	int32_t low = 0, high = iconFont->entryCount - 1;
	int32_t iconIndex = -1;

	while (low <= high) {
		iconIndex = (low + high) / 2;
		if (iconIds[iconIndex] > id) {
			high = iconIndex - 1;
		} else if (iconIds[iconIndex] < id) {
			low = iconIndex + 1;
		} else {
			break;
		}
	}

	if (iconIndex == -1) {
		return;
	}

	uint8_t* iconData = (((uint8_t*) iconFont) + sizeof(hal_icon_font_t) + (iconFont->entryCount * 4) +
	                     (iconFont->entrySize * iconIndex));

	uint8_t b = 0;
	uint16_t mask = 0x100;
	for (int16_t yi = 0; yi < iconFont->height; yi++) {
		for (int16_t xi = 0; xi < iconFont->width; xi++) {
			if (mask == 0x100) {
				b = *iconData;
				iconData++;
				mask = 1;
			}

			if ((b & mask) == mask) {
				halLcdDrawDot(x + xi, y + yi);
			}
			mask <<= 1;
		}
	}
}

void halFontInit() {
	const void* font_partition;
	spi_flash_mmap_handle_t handle;
	const esp_partition_t* part =
	    esp_partition_find_first((esp_partition_type_t) 64, (esp_partition_subtype_t) 0, "font");
	esp_err_t error = esp_partition_mmap(part, 0, part->size, SPI_FLASH_MMAP_DATA, &font_partition, &handle);

	ESP_ERROR_CHECK(error);

	iconFont = (hal_icon_font_t*) font_partition;
	iconIds = (uint32_t*) (((uint8_t*) font_partition) + sizeof(hal_icon_font_t));
	printf("icon magic: %x\n"
	       "icon size: 0x%x\n",
	       iconFont->magic, iconFont->sectionSize);

	font = (hal_font_t*) (((uint8_t*) font_partition) + iconFont->sectionSize);
	sections = (hal_font_section_info_t*) (((uint8_t*) font_partition) + iconFont->sectionSize + 8);

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