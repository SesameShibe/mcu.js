#include "TFT_eSPI.h"
#include "global.h"
#include "../__generated/test.h"

static uint16_t *FrameBuffer;
static uint32_t PenColor = 0;

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);

void halFbClearScreen();

void swapInt(int32_t *a, int32_t *b)
{
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

float_t triangleArea(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    return (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
}

void halFbInit()
{
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    FrameBuffer = (uint16_t *)malloc(TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t));
    halFbClearScreen();
}

void halFbUpdate()
{
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, TFT_WIDTH, TFT_HEIGHT, FrameBuffer);
}

void halFbSetPenColor(uint32_t color)
{
    PenColor = color;
}

uint32_t halFbGetPenColor()
{
    return PenColor;
}

void halFbClearScreen()
{
    memset(FrameBuffer, 0, TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t));
}

void halFbDrawDot(int32_t x, int32_t y, int32_t color = -1)
{
    if (x < 0 || x >= TFT_WIDTH || y < 0 || y >= TFT_HEIGHT)
        return;

    if (color == -1)
        color = PenColor;

    FrameBuffer[(y * TFT_WIDTH) + x] = (uint16_t)color;
}

void halFbDrawHLine(int32_t x0, int32_t x1, int32_t y)
{
    int32_t a, b;
    a = min(x0, x1);
    b = max(x0, x1);

    if (b < 0 || a > TFT_WIDTH || y < 0 || y > TFT_HEIGHT)
        return;

    if (a < 0)
        a = 0;

    a += (y * TFT_WIDTH);
    b += (y * TFT_WIDTH);

    for (; a < b; a++)
    {
        FrameBuffer[a] = (uint16_t)PenColor;
    }
}

void halFbDrawVLine(int32_t y0, int32_t y1, int32_t x)
{
    int32_t a, b;
    a = min(y0, y1);
    b = max(y0, y1);

    if (b < 0 || a > TFT_HEIGHT || x < 0 || x > TFT_WHITE)
        return;

    if (a < 0)
        a = 0;

    for (; a < b; a++)
    {
        FrameBuffer[a * TFT_WIDTH + x] = (uint16_t)PenColor;
    }
}

void halFbDrawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    if (y0 == y1)
    {
        halFbDrawHLine(x0, x1, y0);
        return;
    }
    else if (x0 == x1)
    {
        halFbDrawVLine(y0, y1, x0);
        return;
    }
    else
    {
        int32_t dx = abs(x1 - x0),
                dy = abs(y1 - y0);
        bool steep = dx < dy;

        if (steep)
        {
            swapInt(&x0, &y0);
            swapInt(&x1, &y1);
            swapInt(&dx, &dy);
        }

        int32_t stepX = (x1 - x0) > 0 ? 1 : -1,
                stepY = (y1 - y0) > 0 ? 1 : -1,
                curX = x0,
                curY = y0,
                n2dy = dy << 1,
                n2dydx = (dy - dx) << 1,
                d = (dy << 1) - dx;

        if (steep)
        {
            while (curX != x1)
            {
                if (d < 0)
                {
                    d += n2dy;
                }
                else
                {
                    curY += stepY;
                    d += n2dydx;
                }
                halFbDrawDot(curY, curX);
                curX += stepX;
            }
        }
        else
        {
            while (curX != x1)
            {
                if (d < 0)
                {
                    d += n2dy;
                }
                else
                {
                    curY += stepY;
                    d += n2dydx;
                }
                halFbDrawDot(curX, curY);
                curX += stepX;
            }
        }
    }
}

void halFbDrawRectangle(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    halFbDrawLine(left, top, right, top);
    halFbDrawLine(right, top, right, bottom);
    halFbDrawLine(left, top, left, bottom);
    halFbDrawLine(left, bottom, right, bottom);
}

void halFbFillRectangle(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    while (top <= bottom)
    {
        halFbDrawLine(left, top, right, top);
        top++;
    }
}

void halFbDrawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    if (triangleArea(x0, y0, x1, y1, x2, y2) == 0)
        return;

    halFbDrawLine(x0, y0, x1, y1);
    halFbDrawLine(x0, y0, x2, y2);
    halFbDrawLine(x1, y1, x2, y2);
}

void halFbFillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
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

    for (int32_t y = minY; y < maxY; y++)
    {
        for (int32_t x = minX; x < maxX; x++)
        {
            float_t e0 = triangleArea(x1, y1, x2, y2, x, y) / area;
            float_t e1 = triangleArea(x2, y2, x0, y0, x, y) / area;
            float_t e2 = triangleArea(x0, y0, x1, y1, x, y) / area;

            if (e0 >= 0 && e1 >= 0 && e2 >= 0)
            {
                halFbDrawDot(x, y);
            }
        }
    }
}

void halFbDrawCircle(int32_t px, int32_t py, int32_t r)
{
    int32_t x = 0,
            y = r,
            d = 1 - r;

    while (y >= x)
    {
        halFbDrawDot(x + px, y + py);
        halFbDrawDot(y + px, x + py);
        halFbDrawDot(-x + px, y + py);
        halFbDrawDot(-y + px, x + py);

        halFbDrawDot(-x + px, -y + py);
        halFbDrawDot(-y + px, -x + py);
        halFbDrawDot(x + px, -y + py);
        halFbDrawDot(y + px, -x + py);

        if (d <= 0)
        {
            d += (x << 2) + 6;
        }
        else
        {
            d += ((x - y) << 2) + 10;
            y--;
        }
        x++;
    }
}

uint16_t *decodeUtf8(size_t *strSize, const char *string)
{
    size_t len = strlen(string);
    size_t max_len = len * 3;
    uint16_t *decodeBuff = (uint16_t *)malloc(max_len * sizeof(uint16_t));
    int32_t decodeState = 0;
    *strSize = 0;

    for (int32_t i = 0; i < len; i++)
    {
        if ((string[i] & 0x80) == 0)
        {
            decodeBuff[*strSize] = (uint16_t)string[i];
            *strSize = *strSize + 1;
            continue;
        }
        if (decodeState == 0)
        {
            if ((string[i] & 0xE0) == 0xC0)
            {
                decodeBuff[*strSize] = ((string[i] & 0x1F) << 6);
                decodeState = 1;
                continue;
            }
            if ((string[i] & 0xF0) == 0xE0)
            {
                decodeBuff[*strSize] = ((string[i] & 0x0F) << 12);
                decodeState = 2;
                continue;
            }
        }
        else
        {
            if (decodeState == 2)
            {
                decodeBuff[*strSize] |= ((string[i] & 0x3F) << 6);
                decodeState--;
                continue;
            }
            else
            {
                decodeBuff[*strSize] |= (string[i] & 0x3F);
                decodeState = 0;
                *strSize = *strSize + 1;
                continue;
            }
        }
    }

    uint16_t *retv = (uint16_t *)malloc(*strSize * sizeof(uint16_t));
    memcpy(retv, decodeBuff, *strSize * sizeof(uint16_t));
    free(decodeBuff);
    return retv;
}

void halFbDrawChar(uint16_t c, int32_t x, int32_t y)
{
    for (int32_t i = 0; i < font_16_count; i++)
    {
        if (chars_16[i] == c)
        {
            const uint8_t *glyphData = &font_16[i * 16 * 16];
            for (int32_t yi = 0; yi < 16; yi++)
            {
                for (int32_t xi = 0; xi < 16; xi++)
                {
                    uint8_t c = glyphData[yi * 16 + xi];
                    uint16_t b = (c >> 3) & 0x1f;
                    uint16_t g = ((c >> 2) & 0x3f) << 5;
                    uint16_t r = ((c >> 3) & 0x1f) << 11;
                    uint16_t c565 = r | g | b;
                    if (c565 != 0)
                        halFbDrawDot(x + xi, y + yi, c565);
                }
            }
            return;
        }
    }
}

void halFbDrawText(const char *string, int32_t x, int32_t y)
{
    size_t strSize = 0;
    uint16_t *unicodes = decodeUtf8(&strSize, string);

    for (int i = 0; i < strSize; i++)
    {
        halFbDrawChar(unicodes[i], x, y);
        x += 16;
    }
}