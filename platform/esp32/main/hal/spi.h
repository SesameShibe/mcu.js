#include <SPI.h>
#include "global.h"


SPISettings spiSet;

void halSpiWrite8(u8 v)
{
    SPI.beginTransaction(spiSet);
    SPI.write(v);
    SPI.endTransaction();
}

void halSpiWrite16(u16 v)
{
    SPI.beginTransaction(spiSet);
    SPI.write16(v);
    SPI.endTransaction();
}

void halSpiWrite32(u32 v)
{
    SPI.beginTransaction(spiSet);
    SPI.write32(v);
    SPI.endTransaction();
}


void halSpiWriteBuf(JS_BUFFER buf, u32 offset, u32 len)
{
    if ((len <= buf.size) && (offset + len <= buf.size) && (offset < buf.size))
    {
        SPI.beginTransaction(spiSet);
        SPI.writeBytes(buf.buf + offset, len);
        SPI.endTransaction();
    }
}