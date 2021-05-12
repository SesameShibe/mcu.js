#pragma once
#include "global.h"


uint16_t halUtfReadUtf8Char(char** pstring) {
	size_t byteCount = 0;
	uint16_t unicode = 0;
	char* string = *pstring;

	if ((string[0] & 0x80) == 0) {
		unicode = (uint16_t) string[0];
		byteCount = 1;
	} else if ((string[0] & 0xE0) == 0xC0) {
		unicode = ((string[0] & 0x1F) << 6) | (string[1] & 0x3F);
		byteCount = 2;
	} else if ((string[0] & 0xF0) == 0xE0) {
		unicode = ((string[0] & 0x0F) << 12) | ((string[1] & 0x3F) << 6) | (string[2] & 0x3F);
		byteCount = 3;
	}

	*pstring = &string[byteCount];
	return unicode;
}