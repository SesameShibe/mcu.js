#pragma once

int mcujsHandleAssertFailed(const char* expr, const char* file, int line);
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define MCUJS_ASSERT(expression) \
	(void) ((!!(expression)) || (mcujsHandleAssertFailed((#expression), (__FILE__), (__LINE__)), 0))

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;

typedef struct _JS_BUFFER {
	uint8_t* buf;
	size_t size;
} JS_BUFFER;
