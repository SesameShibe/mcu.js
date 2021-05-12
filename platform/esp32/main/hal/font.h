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

typedef struct hal_font_glyph_t {
	uint16_t width;
	uint16_t height;
	uint8_t* data;
} hal_font_glyph_t;

static hal_font_t* font;
static hal_font_section_info_t* sections;

static hal_icon_font_t* iconFont;
static uint32_t* iconIds;

#define TEXT_LINE_HEIGHT 16

static hal_font_section_info_t* halFontGetSection(uint16_t c) {
	hal_font_section_info_t* section = sections;

	for (int i = 0; i < font->sectionCount; i++) {
		if (c >= section->codeStart && c <= section->codeEnd)
			return section;
		section = &sections[i];
	}

	return NULL;
}

hal_font_glyph_t halFontGetGlyph(uint16_t c) {
	hal_font_glyph_t glyph;
	hal_font_section_info_t* section = halFontGetSection(c);

	memset(&glyph, 0, sizeof(hal_font_glyph_t));

	if (section != NULL) {
		glyph.data = (uint8_t*) (&((uint8_t*) font)[0] +
		                         (section->dataOffset + (section->glyphEntrySize * (c - section->codeStart))));
		glyph.width = section->charWidth;
		glyph.height = section->charHeight;
	}

	return glyph;
}

hal_font_glyph_t halFontGetIcon(uint32_t id) {
	int32_t low = 0, high = iconFont->entryCount - 1;
	int32_t iconIndex = -1;
	hal_font_glyph_t glyph;

	memset(&glyph, 0, sizeof(hal_font_glyph_t));

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

	if (low <= high) {
		glyph.data = (((uint8_t*) iconFont) + sizeof(hal_icon_font_t) + (iconFont->entryCount * 4) +
		              (iconFont->entrySize * iconIndex));
		glyph.width = iconFont->width;
		glyph.height = iconFont->height;
	}

	return glyph;
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