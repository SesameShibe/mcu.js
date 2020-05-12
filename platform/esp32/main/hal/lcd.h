#pragma once
#include "driver/spi_master.h"
#include "global.h"
#include "gpio.h"

#define LCD_PIN_DC (27)
#define LCD_PIN_CS (5)
#define LCD_PIN_SCLK (18)
#define LCD_PIN_MOSI (19)
#define LCD_PIN_BL (12)
#define LCD_WIDTH (240)
#define LCD_HEIGHT (240)
#define LCD_CMD_CASET 0x2A
#define LCD_CMD_PASET 0x2B
#define LCD_CMD_RAMWR 0x2C

static const u8 lcdInitCode[] = {
	0x11, 0x00, // cmd
	0xFF, 120, // delay
	0x13, 0x00, // cmd
	0x36, 0x01, 0x08, // cmd
	0xB6, 0x02, 0x0A, 0x82, // cmd
	0x3A, 0x01, 0x55, // cmd
	0xFF, 10, // delay
	0xB2, 0x05, 0x0C, 0x0C, 0x00, 0x33, 0x33, // cmd
	0xB7, 0x01, 0x35, // cmd
	0xBB, 0x01, 0x28, // cmd
	0xC0, 0x01, 0x0C, // cmd
	0xC2, 0x02, 0x01, 0xFF, // cmd
	0xC3, 0x01, 0x10, // cmd
	0xC4, 0x01, 0x20, // cmd
	0xC6, 0x01, 0x0F, // cmd
	0xD0, 0x02, 0xA4, 0xA1, // cmd
	0xE0, 0x0E, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17, // cmd
	0xE1, 0x0E, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E, // cmd
	0x21, 0x00, // cmd
	0x2A, 0x04, 0x00, 0x00, 0x00, 0xE5, // cmd
	0x2B, 0x04, 0x00, 0x00, 0x01, 0x3F, // cmd
	0xFF, 120, // delay
	0x29, 0x00, // cmd
	0xFF, 120, // delay
	0xFF, 0xFF // FIN
};

#define LCD_SET_CS(v) halGpioWrite(LCD_PIN_CS, (v))
#define LCD_SET_DC(v) halGpioWrite(LCD_PIN_DC, (v))

#define LCD_SPI_BUS SPI3_HOST
#define LCD_SPI_HW SPI3
spi_device_handle_t lcdSpiDev;

static uint16_t lcdFB[LCD_WIDTH * LCD_HEIGHT];
u16 lcdRowOffset, lcdColOffset;

static inline void halLcdSpiWrite(u32 dat, u8 bitlen) {
	LCD_SPI_HW.mosi_dlen.val = bitlen - 1;
	LCD_SPI_HW.miso_dlen.val = 0;
	LCD_SPI_HW.data_buf[0] = dat;
	LCD_SPI_HW.cmd.usr = 1;
	while (LCD_SPI_HW.cmd.usr)
		;
}

void halLcdWriteCmd8(u8 dat) {
	LCD_SET_DC(0);
	LCD_SET_CS(0);
	halLcdSpiWrite(dat, 8);
	LCD_SET_CS(1);
}

void halLcdWriteDat8(u8 dat) {
	LCD_SET_DC(1);
	LCD_SET_CS(0);
	halLcdSpiWrite(dat, 8);
	LCD_SET_CS(1);
}

static inline void halLcdWritePixels(u32* buf, u32 lenInU32) {
	const int maxSendU32ForOnce = 16;
	int i;

	LCD_SPI_HW.mosi_dlen.val = (maxSendU32ForOnce * 32) - 1;
	LCD_SPI_HW.miso_dlen.val = 0;
	while (lenInU32 >= maxSendU32ForOnce) {
		for (i = 0; i < maxSendU32ForOnce; i++) {
			LCD_SPI_HW.data_buf[i] = buf[i];
		}
		LCD_SPI_HW.cmd.usr = 1;
		lenInU32 -= maxSendU32ForOnce;
		buf += maxSendU32ForOnce;
		while (LCD_SPI_HW.cmd.usr)
			;
	}
	for (i = 0; i < lenInU32; i++) {
		LCD_SPI_HW.mosi_dlen.val = (lenInU32 * 32) - 1;
		LCD_SPI_HW.data_buf[i] = buf[i];
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

void halLcdUpdate() {
	u16 c1 = 0, c2 = LCD_WIDTH - 1;
	u16 r1 = 0, r2 = LCD_HEIGHT - 1;

	halLcdWriteCmd8(LCD_CMD_CASET); // Column address set
	halLcdWriteDat8(c1 >> 8);
	halLcdWriteDat8(c1);
	halLcdWriteDat8(c2 >> 8);
	halLcdWriteDat8(c2);

	halLcdWriteCmd8(LCD_CMD_PASET); // Row address set
	halLcdWriteDat8(r1 >> 8);
	halLcdWriteDat8(r1);
	halLcdWriteDat8(r2 >> 8);
	halLcdWriteDat8(r2);

	halLcdWriteCmd8(LCD_CMD_RAMWR);
	LCD_SET_DC(1);
	LCD_SET_CS(0);
	halLcdWritePixels((u32*) lcdFB, sizeof(lcdFB) / 4);
	LCD_SET_CS(1);
}

void halLcdInit() {
	spi_bus_config_t cfg = { .mosi_io_num = LCD_PIN_MOSI,
		                     .miso_io_num = -1,
		                     .sclk_io_num = LCD_PIN_SCLK,
		                     .quadwp_io_num = -1,
		                     .quadhd_io_num = -1,
		                     .max_transfer_sz = 0,
		                     .flags = 0,
		                     .intr_flags = 0 };
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz = 20000000, // Clock
		.mode = 0, // SPI mode 0
		.spics_io_num = -1, // CS pin
		.queue_size = 7, // We want to be able to queue 7 transactions at a time
		.pre_cb = NULL, // Specify pre-transfer callback to handle D/C line
	};
	esp_err_t ret = spi_bus_initialize(LCD_SPI_BUS, &cfg, 1);
	ESP_ERROR_CHECK(ret);
	ret = spi_bus_add_device(LCD_SPI_BUS, &devcfg, &lcdSpiDev);
	ESP_ERROR_CHECK(ret);

	LCD_SET_CS(1);
	LCD_SET_DC(1);
	halGpioConfig(LCD_PIN_CS, GPIO_MODE_OUTPUT, 0);
	halGpioConfig(LCD_PIN_DC, GPIO_MODE_OUTPUT, 0);
	halGpioConfig(LCD_PIN_BL, GPIO_MODE_OUTPUT, 0);

	/* Do a dummy transmission to config the freq settings */
	spi_transaction_t t;
	memset(&t, 0, sizeof(t)); // Zero out the transaction
	t.length = 8;
	t.flags = SPI_TRANS_USE_TXDATA;
	t.tx_data[0] = 0;
	ret = spi_device_polling_transmit(lcdSpiDev, &t); // Transmit!
	ESP_ERROR_CHECK(ret);

	halLcdExecuteInitCode(lcdInitCode);
	memset(lcdFB, 0x0f, sizeof(lcdFB));
	halLcdUpdate();
	halGpioWrite(LCD_PIN_BL, 1);
}

JS_BUFFER halLcdGetFB() {
	JS_BUFFER buf = { .buf = (u8*) lcdFB, .size = sizeof(lcdFB) };
	return buf;
}