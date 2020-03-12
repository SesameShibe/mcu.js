#include "global.h"
#include <mbedtls/sha1.h>

int halCryptoHashBuf(const char *algorithm, JS_BUFFER src, JS_BUFFER dst)
{
    size_t dstLen = 0;
    if (strcmp(algorithm, "sha1") == 0)
    {
        if (20 > dst.size)
        {
            return -2;
        }
        mbedtls_sha1(src.buf, src.size, dst.buf);
        return 0;
    }

    return -1;
}