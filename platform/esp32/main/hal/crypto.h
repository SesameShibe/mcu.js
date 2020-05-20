#include "global.h"
#include <mbedtls/sha1.h>

int halCryptoHashBuf(const char *algorithm, JS_BUFFER src, JS_BUFFER dst)
{
    CHECK_JSBUF_NOTNULL(src);
    if (strcmp(algorithm, "sha1") == 0)
    {
        CHECK_JSBUF_SIZE(dst, 20);
        mbedtls_sha1(src.buf, src.size, dst.buf);
        return 0;
    }

    return -1;
}