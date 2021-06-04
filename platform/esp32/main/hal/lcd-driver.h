#pragma once

#include "driver/spi_master.h"
#include "esp_partition.h"
#include "font.h"
#include "global.h"
#include "gpio.h"
#include "hal/spi_ll.h"

#define LCD_SPI_CLOCK (20000000)
#define LCD_XPT2046_CLOCK (1000)
spi_ll_clock_val_t lcdSpiClockReg;
spi_ll_clock_val_t lcdXpt2046ClockReg;

const u8 lcdInitCode[] = {
    0x1A, 0x01, 0x02,  // cmd
    0x1B, 0x01, 0x88,  // cmd
    0x23, 0x01, 0x00,  // cmd
    0x24, 0x01, 0xEE,  // cmd
    0x25, 0x01, 0x15,  // cmd
    0x2D, 0x01, 0x03,  // cmd
    0x18, 0x01, 0x1E,  // cmd
    0x19, 0x01, 0x01,  // cmd
    0x01, 0x01, 0x00,  // cmd
    0x1F, 0x01, 0x88,  // cmd
    0xFF, 5,           // delay
    0x1F, 0x01, 0x80,  // cmd
    0xFF, 5,           // delay
    0x1F, 0x01, 0x90,  // cmd
    0xFF, 5,           // delay
    0x1F, 0x01, 0xD0,  // cmd
    0xFF, 5,           // delay
    0x2F, 0x01, 0x00,  // cmd
    0x17, 0x01, 0x05,  // cmd
    0x16, 0x01, 0x70,  // cmd
    0x36, 0x01, 0x09,  // cmd
    0x29, 0x01, 0x31,  // cmd
    0x71, 0x01, 0x1A,  // cmd
    0x40, 0x01, 0x01,  // cmd
    0x41, 0x01, 0x08,  // cmd
    0x42, 0x01, 0x04,  // cmd
    0x43, 0x01, 0x2D,  // cmd
    0x44, 0x01, 0x30,  // cmd
    0x45, 0x01, 0x3E,  // cmd
    0x46, 0x01, 0x02,  // cmd
    0x47, 0x01, 0x69,  // cmd
    0x48, 0x01, 0x07,  // cmd
    0x49, 0x01, 0x0E,  // cmd
    0x4A, 0x01, 0x12,  // cmd
    0x4B, 0x01, 0x14,  // cmd
    0x4C, 0x01, 0x17,  // cmd
    0x50, 0x01, 0x01,  // cmd
    0x51, 0x01, 0x0F,  // cmd
    0x52, 0x01, 0x12,  // cmd
    0x53, 0x01, 0x3B,  // cmd
    0x54, 0x01, 0x37,  // cmd
    0x55, 0x01, 0x3E,  // cmd
    0x56, 0x01, 0x16,  // cmd
    0x57, 0x01, 0x7D,  // cmd
    0x58, 0x01, 0x08,  // cmd
    0x59, 0x01, 0x0B,  // cmd
    0x5A, 0x01, 0x0D,  // cmd
    0x5B, 0x01, 0x11,  // cmd
    0x5C, 0x01, 0x18,  // cmd
    0x5D, 0x01, 0xFF,  // cmd
    0x1B, 0x01, 0x1A,  // cmd
    0x1A, 0x01, 0x55,  // cmd
    0x24, 0x01, 0x10,  // cmd
    0x25, 0x01, 0x38,  // cmd
    0x28, 0x01, 0x38,  // cmd
    0xFF, 40,          // delay
    0x28, 0x01, 0x3C,  // cmd
    0x22, 0x00,        // cmd
    0xFF, 0xFF         // FIN

};

#define LCD_PIN_DC (12)
#define LCD_PIN_CS (5)
#define LCD_PIN_SCLK (18)
#define LCD_PIN_MOSI (23)
#define LCD_PIN_BL (13)
#define LCD_PIN_XPT2046_CS (4)
#define LCD_PIN_MISO (19)

#define LCD_WIDTH (320)
#define LCD_HEIGHT (240)
#define LCD_CMD_CASET 0x02
#define LCD_CMD_PASET 0x06
#define LCD_CMD_RAMWR 0x22

EXT_RAM_ATTR u16 lcdFB[320 * 240];

#define LCD_SET_CS(v) halGpioWrite(LCD_PIN_CS, (v))
#define LCD_SET_DC(v) halGpioWrite(LCD_PIN_DC, (v))
#define LCD_SET_XPT2046_CS(v) halGpioWrite(LCD_PIN_XPT2046_CS, (v))

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

static void halLcdWrite8(u8 dat) { halLcdSpiWrite(dat, 8); }

static u16 halLcdXchg16(u16 dat) {
  LCD_SPI_HW.mosi_dlen.usr_mosi_dbitlen = 15;
  LCD_SPI_HW.miso_dlen.usr_miso_dbitlen = 15;
  LCD_SPI_HW.data_buf[0] = dat;

  LCD_SPI_HW.cmd.usr = 1;
  while (LCD_SPI_HW.cmd.usr)
    ;
  dat = LCD_SPI_HW.data_buf[0];
  /*
  if (!LCD_SPI_HW.ctrl.rd_bit_order) {
    dat = (dat >> 8) | (dat << 8);
  }*/
  return dat;
}

void IRAM_ATTR halLcdWriteCmd8(u8 dat) {
  LCD_SET_DC(0);
  LCD_SET_CS(0);
  halLcdSpiWrite(dat, 8);
  LCD_SET_CS(1);
}

void IRAM_ATTR halLcdWriteDat8(u8 dat) {
  LCD_SET_DC(1);
  LCD_SET_CS(0);
  halLcdSpiWrite(dat, 8);
  LCD_SET_CS(1);
}

static ALWAYS_INLINE u32 convertPixelByteOrder(u32 src) {
  uint8_t* d = (uint8_t*)&(src);
  return d[1] | (d[0] << 8) | (d[3] << 16) | (d[2] << 24);
}

static ALWAYS_INLINE  void halLcdWritePixels(u32* buf, u32 lenInU32) {
  const int maxSendU32ForOnce = 16;
  int i;

  LCD_SPI_HW.mosi_dlen.val = (maxSendU32ForOnce * 32) - 1;
  LCD_SPI_HW.miso_dlen.val = 0;
  while (lenInU32 >= maxSendU32ForOnce) {
    for (i = 0; i < maxSendU32ForOnce; i++) {
      LCD_SPI_HW.data_buf[i] = convertPixelByteOrder(buf[i]);
    }
    LCD_SPI_HW.cmd.usr = 1;
    lenInU32 -= maxSendU32ForOnce;
    buf += maxSendU32ForOnce;
    while (LCD_SPI_HW.cmd.usr)
      ;
  }
  for (i = 0; i < lenInU32; i++) {
    LCD_SPI_HW.mosi_dlen.val = (lenInU32 * 32) - 1;
    LCD_SPI_HW.data_buf[i] = convertPixelByteOrder(buf[i]);
    LCD_SPI_HW.cmd.usr = 1;
    while (LCD_SPI_HW.cmd.usr)
      ;
  }
}

void halLcdExecuteInitCode(const u8* code) {
  int i;

  while (1) {
    u8 opCode = code[0];
    u8 datLen = code[1];
    if (opCode == 0xFF) {
      if (datLen == 0xFF) {
        break;
      } else {
        halOsSleepMs(datLen);
      }
      code += 2;
    } else {
      halLcdWriteCmd8(opCode);
      for (i = 0; i < datLen; i++) {
        halLcdWriteDat8(code[2 + i]);
      }
      code += 2 + datLen;
    }
  }
}

static void halLcdWriteReg(u8 cmd, u8 data) {
  halLcdWriteCmd8(cmd);
  halLcdWriteDat8(data);
}

void halLcdSetReg(u8 cmd, u8 data) { halLcdWriteReg(cmd, data); }

void IRAM_ATTR halLcdUpdate() {
  u16 x1 = 0, x2 = x1 + LCD_WIDTH - 1;
  u16 y1 = 0, y2 = y1 + LCD_HEIGHT - 1;

  halLcdWriteReg(0x02, x1 >> 8);  // Column address set
  halLcdWriteReg(0x03, x1 & 0xff);
  halLcdWriteReg(0x04, x2 >> 8);
  halLcdWriteReg(0x05, x2 & 0xff);

  halLcdWriteReg(0x06, y1 >> 8);
  halLcdWriteReg(0x07, y1 & 0xff);
  halLcdWriteReg(0x08, y2 >> 8);
  halLcdWriteReg(0x09, y2 & 0xff);

  halLcdWriteCmd8(0x22);

  LCD_SET_DC(1);
  LCD_SET_CS(0);
  halLcdWritePixels((u32*)lcdFB, LCD_WIDTH * LCD_HEIGHT * 2 / 4);
  LCD_SET_CS(1);
}

static int32_t touchBuffer[4];

JS_BUFFER halLcdGetTouchBuffer() {
  JS_BUFFER ret = {(u8*)touchBuffer, sizeof(touchBuffer)};
  return ret;
}

int32_t halLcdReadTouch() {
  int isTouched = 0;
  int x[3], y[3];
  float mappedX = 0, mappedY = 0;

  LCD_SPI_HW.clock.val = lcdXpt2046ClockReg;
  LCD_SET_XPT2046_CS(0);
  halLcdWrite8(0xB3 /* Z1 */);
  halLcdWrite8(0xB3 /* Z1 */);
  halLcdWrite8(0xB3 /* Z1 */);
  int z1 = halLcdXchg16(0xC3 /* Z2 */) >> 3;
  int z2 = halLcdXchg16(0x93 /* X */) >> 3;
  halLcdXchg16(0x93 /* X */);  // dummy X measure, 1st is always noisy
  x[0] = halLcdXchg16(0xD3 /* Y */) >> 3;
  y[0] = halLcdXchg16(0x93 /* X */) >> 3;  // make 3 x-y measurements
  x[1] = halLcdXchg16(0xD3 /* Y */) >> 3;
  y[1] = halLcdXchg16(0x93 /* X */) >> 3;
  x[2] = halLcdXchg16(0xD3 /* Y */) >> 3;
  y[2] = halLcdXchg16(0xD3) >> 3;
  halLcdXchg16(0);
  LCD_SET_XPT2046_CS(1);
  LCD_SPI_HW.clock.val = lcdSpiClockReg;

  if ((z1 - z2) > 100) {
    isTouched = 1;
  }
  if (isTouched) {
    mappedY = 1.0 - ((x[0] + x[1] + x[2]) / 3.0) / 4096.0;
    mappedX = 1.0 - ((y[0] + y[1] + y[2]) / 3.0) / 4096.0;
  }
  touchBuffer[0] = mappedX * 320;
  touchBuffer[1] = mappedY * 240;
  return isTouched;
}

void halLcdDriverInit() {
  printf("lcd driver init...\n");
  spi_bus_config_t cfg = {.mosi_io_num = LCD_PIN_MOSI,
                          .miso_io_num = -1,
                          .sclk_io_num = LCD_PIN_SCLK,
                          .quadwp_io_num = -1,
                          .quadhd_io_num = -1,
                          .max_transfer_sz = 0,
                          .flags = 0,
                          .intr_flags = 0};
  spi_device_interface_config_t devcfg = {
      .clock_speed_hz = LCD_SPI_CLOCK,  // Clock
      .mode = 0,                        // SPI mode 0
      .spics_io_num = -1,               // CS pin
      .queue_size = 7,  // We want to be able to queue 7 transactions at a time
      .pre_cb = NULL,   // Specify pre-transfer callback to handle D/C line
  };
  esp_err_t ret = spi_bus_initialize(LCD_SPI_BUS, &cfg, 1);
  ESP_ERROR_CHECK(ret);
  ret = spi_bus_add_device(LCD_SPI_BUS, &devcfg, &lcdSpiDev);
  ESP_ERROR_CHECK(ret);

  LCD_SET_CS(1);
  LCD_SET_DC(1);
  halGpioWrite(LCD_PIN_XPT2046_CS, 1);
  halGpioWrite(LCD_PIN_BL, 1);
  halGpioConfig(LCD_PIN_CS, GPIO_MODE_OUTPUT, 0);
  halGpioConfig(LCD_PIN_DC, GPIO_MODE_OUTPUT, 0);
  halGpioConfig(LCD_PIN_BL, GPIO_MODE_OUTPUT, 0);
  halGpioConfig(LCD_PIN_XPT2046_CS, GPIO_MODE_OUTPUT, 0);
  halGpioConfig(LCD_PIN_MISO, GPIO_MODE_INPUT, 0);

  /* Do a dummy transmission to config the freq settings */
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));  // Zero out the transaction
  t.length = 8;
  t.flags = SPI_TRANS_USE_TXDATA;
  t.tx_data[0] = 0;
  ret = spi_device_polling_transmit(lcdSpiDev, &t);  // Transmit!
  ESP_ERROR_CHECK(ret);

  spi_ll_master_cal_clock(SPI_LL_PERIPH_CLK_FREQ, LCD_SPI_CLOCK, 128,
                          &lcdSpiClockReg);
  spi_ll_master_cal_clock(SPI_LL_PERIPH_CLK_FREQ, LCD_XPT2046_CLOCK, 128,
                          &lcdXpt2046ClockReg);
  printf("%08x %08x\n", lcdSpiClockReg, lcdXpt2046ClockReg);
  LCD_SPI_HW.clock.val = lcdSpiClockReg;

  LCD_SET_CS(1);
  halOsSleepMs(120);
  LCD_SET_CS(0);

  halLcdExecuteInitCode(lcdInitCode);
  memset(lcdFB, 0x0f, LCD_WIDTH * LCD_HEIGHT * 2);
  halLcdUpdate();
  halGpioWrite(LCD_PIN_BL, 1);
}
