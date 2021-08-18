#pragma once

#include "driver/spi_master.h"
#include "esp_partition.h"
#include "global.h"
#include "gpio.h"
#include "hal/spi_ll.h"

#define LCD_SPI_CLOCK (1000000)
#define LCD_COLOR_WHITE (1)
#define LCD_COLOR_BLACK (0)
#define LCD_COLOR_INVERSE (2)

#define LCD_PIN_RSTN (27)
#define LCD_PIN_DC (26)
#define LCD_PIN_CS (15)
#define LCD_PIN_SCLK (18)
#define LCD_PIN_MOSI (23)
#define LCD_PIN_MISO (19)

#define LCD_WIDTH (128)
#define LCD_HEIGHT (64)

u8 lcdFB[(LCD_WIDTH * LCD_HEIGHT) / 8];

#define LCD_SET_CS(v) halGpioWrite(LCD_PIN_CS, (v))
#define LCD_SET_DC(v) halGpioWrite(LCD_PIN_DC, (v))
#define LCD_SET_RSTN(v) halGpioWrite(LCD_PIN_RSTN, (v))

#define LCD_SPI_BUS SPI3_HOST
#ifdef CONFIG_IDF_TARGET_ESP32S2
#define LCD_SPI_HW GPSPI3
#else
#define LCD_SPI_HW SPI3
#endif

spi_device_handle_t lcdSpiDev;

static ALWAYS_INLINE void halLcdSpiWrite(u32 dat, u8 bitlen) {
  LCD_SPI_HW.mosi_dlen.val = bitlen - 1;
  LCD_SPI_HW.miso_dlen.val = 0;
  LCD_SPI_HW.data_buf[0] = dat;
  LCD_SPI_HW.cmd.usr = 1;
  while (LCD_SPI_HW.cmd.usr)
    ;
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void spiWriteBuf(spi_device_handle_t spi, const uint8_t *data, int len) {
  esp_err_t ret;
  spi_transaction_t t;
  if (len == 0)
    return;                 // no need to send anything
  memset(&t, 0, sizeof(t)); // Zero out the transaction
  t.length = len * 8;       // Len is in bytes, transaction length is in bits.
  t.tx_buffer = data;       // Data
  t.user = (void *)1;       // D/C needs to be set to 1
  ret = spi_device_polling_transmit(spi, &t); // Transmit!
  assert(ret == ESP_OK);                      // Should have had no issues.
}

static void halLcdWrite8(u8 dat) { spiWriteBuf(lcdSpiDev, &dat, 1); }

void halLcdWriteCmd8(u8 dat) {
  LCD_SET_DC(0);
  LCD_SET_CS(0);
  halLcdWrite8(dat);
  LCD_SET_CS(1);
}

void halLcdWriteDat8(u8 dat) {
  LCD_SET_DC(1);
  LCD_SET_CS(0);
  halLcdWrite8(dat);
  LCD_SET_CS(1);
}

void halLcdUpdate() {
  const int bankSize = LCD_WIDTH * 8 / 8;

  for (int i = 0; i < 8; i++) {
    halLcdWriteCmd8(0xb0 + i);
    halLcdWriteCmd8(0x00);
    halLcdWriteCmd8(0x10);
    LCD_SET_DC(1);
    LCD_SET_CS(0);
    spiWriteBuf(lcdSpiDev, lcdFB + bankSize * i, bankSize);
    LCD_SET_CS(1);
  }
}

const u8 lcd_init_table[] = {

    0xFD, // Set Command Lock
    0X12, //   Default => 0x12
          //     0x12 => Driver IC interface is unlocked from entering command.
          //     0x16 => All Commands are locked except 0xFD.

    0XAE, // Set Display On/Off
          //   Default => 0xAE
          //     0xAE => Display Off
          //     0xAF => Display On

    0xD5, // Set Display Clock Divide Ratio / Oscillator Frequency
    0X30, // Set Clock as 116 Frames/Sec
          //   Default => 0x70
          //     D[3:0] => Display Clock Divider
          //     D[7:4] => Oscillator Frequency

    0xA8, // Set Multiplex Ratio
    0X37, //   Default => 0x3F (1/56 Duty)

    0xD3, // Set Display Offset
    0X08, //   Default => 0x00

    0x40, // Set Mapping RAM Display Start Line (0x00~0x3F)
          //   Default => 0x40 (0x00)

    0xA1, // Set SEG/Column Mapping (0xA0/0xA1)
          //   Default => 0xA0
          //     0xA0 => Column Address 0 Mapped to SEG0
          //     0xA1 => Column Address 0 Mapped to SEG127

    0xC8, // Set COM/Row Scan Direction (0xC0/0xC8)
          //   Default => 0xC0
          //     0xC0 => Scan from COM0 to 63
          //     0xC8 => Scan from COM63 to 0

    0xDA, // Set COM Pins Hardware Configuration
    0x12, //   Default => 0x12
          //     Alternative COM Pin Configuration
          //     Disable COM Left/Right Re-Map

    0x81, // Set SEG Output Current
    0x8F, // Set Contrast Control for Bank 0

    0xD9, // Set Pre-Charge as 2 Clocks & Discharge as 5 Clocks
    0x25, //   Default => 0x22 (2 Display Clocks [Phase 2] / 2 Display Clocks
          //   [Phase 1])
          //     D[3:0] => Phase 1 Period in 1~15 Display Clocks
          //     D[7:4] => Phase 2 Period in 1~15 Display Clocks

    0xDB, // Set VCOMH Deselect Level
    0x34, //   Default => 0x34 (0.78*VCC)

    0xA4, // Set Entire Display On / Off
          //   Default => 0xA4
          //     0xA4 => Normal Display
          //     0xA5 => Entire Display On

    0xA6, // Set Inverse Display On/Off
          //   Default => 0xA6
          //     0xA6 => Normal Display
          //     0xA7 => Inverse Display On

    0XAF, // Display On (0xAE/0xAF)
};

#include "Adafruit_GFX.h"
class MyLcdGfx : public Adafruit_GFX {
public:
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  MyLcdGfx() : Adafruit_GFX(LCD_WIDTH, LCD_HEIGHT) {}
};

void IRAM_ATTR MyLcdGfx::drawPixel(int16_t x, int16_t y, uint16_t color) {

  int16_t t;
  switch (rotation) {
  case 1:
    t = x;
    x = WIDTH - 1 - y;
    y = t;
    break;
  case 2:
    x = WIDTH - 1 - x;
    y = HEIGHT - 1 - y;
    break;
  case 3:
    t = x;
    x = y;
    y = HEIGHT - 1 - t;
    break;
  }

  if ((x < 0) || (y < 0) || (x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
    return;

  switch (color) {
  case LCD_COLOR_WHITE:
    lcdFB[x + (y / 8) * WIDTH] |= (1 << (y & 7));
    break;
  case LCD_COLOR_BLACK:
    lcdFB[x + (y / 8) * WIDTH] &= ~(1 << (y & 7));
    break;
  case LCD_COLOR_INVERSE:
    lcdFB[x + (y / 8) * WIDTH] ^= (1 << (y & 7));
    break;
  }
}

MyLcdGfx gfx;
#include "font.h"

void halLcdInit() {

  printf("lcd driver init...\n");
  halGpioWrite(LCD_PIN_RSTN, 0);
  halGpioConfig(LCD_PIN_RSTN, GPIO_MODE_OUTPUT, 0);
  halOsSleepMs(100);
  halGpioWrite(LCD_PIN_RSTN, 1);
  halOsSleepMs(100);

  spi_bus_config_t cfg = {.mosi_io_num = LCD_PIN_MOSI,
                          .miso_io_num = LCD_PIN_MISO,
                          .sclk_io_num = LCD_PIN_SCLK,
                          .quadwp_io_num = -1,
                          .quadhd_io_num = -1,
                          .max_transfer_sz = sizeof(lcdFB) + 32,
                          .flags = 0,
                          .intr_flags = 0};
  spi_device_interface_config_t devcfg;
  memset(&devcfg, 0, sizeof(devcfg));
  devcfg.clock_speed_hz = LCD_SPI_CLOCK; // Clock
  devcfg.mode = 0;                       // SPI mode 0
  devcfg.spics_io_num = -1;              // CS pin
  devcfg.queue_size = 7; // We want to be able to queue 7 transactions at a time
  esp_err_t ret = spi_bus_initialize(LCD_SPI_BUS, &cfg, SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(ret);
  ret = spi_bus_add_device(LCD_SPI_BUS, &devcfg, &lcdSpiDev);
  ESP_ERROR_CHECK(ret);

  LCD_SET_CS(1);
  LCD_SET_DC(1);
  halGpioConfig(LCD_PIN_CS, GPIO_MODE_OUTPUT, 0);
  halGpioConfig(LCD_PIN_DC, GPIO_MODE_OUTPUT, 0);
  for (int i = 0; i < sizeof(lcd_init_table); i++) {
    halLcdWriteCmd8(lcd_init_table[i]);
    halOsSleepMs(5);
  }
  gfx.drawCircle(35, 35, 10, 1);
  gfx.drawTriangle(60, 30, 50, 50, 70, 50, 1);
  gfx.drawRoundRect(90, 20, 20, 20, 4, 1);
  fbInitFont();
  fbDrawUtf8String("mcu.js 中文字体测试");
  halLcdUpdate();
}
