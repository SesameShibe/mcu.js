#pragma once
#include "driver/spi_master.h"
#include "global.h"
#include "gpio.h"

#define SPI_BUS SPI2_HOST
spi_device_handle_t spiDev;

void halSpiBegin(u32 clkSpeed, u32 sck, u32 miso, u32 mosi) {
	spi_bus_config_t cfg = { .mosi_io_num = mosi,
		                     .miso_io_num = miso,
		                     .sclk_io_num = sck,
		                     .quadwp_io_num = -1,
		                     .quadhd_io_num = -1,
		                     .max_transfer_sz = 0,
		                     .flags = 0,
		                     .intr_flags = 0 };
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz = clkSpeed, // Clock
		.mode = 0, // SPI mode 0
		.spics_io_num = -1, // CS pin
		.queue_size = 7, // We want to be able to queue 7 transactions at a time
		.pre_cb = NULL, // Specify pre-transfer callback to handle D/C line
	};
	esp_err_t ret = spi_bus_initialize(SPI_BUS, &cfg, 1);
	ESP_ERROR_CHECK(ret);
	ret = spi_bus_add_device(SPI_BUS, &devcfg, &spiDev);
	ESP_ERROR_CHECK(ret);
}

static void halSpiWriteBits(u32 v, u32 bits) {
	spi_transaction_t t;
	memset(&t, 0, sizeof(t)); // Zero out the transaction
	t.length = bits; //
	t.tx_buffer = &v; // The data is the cmd itself
	spi_device_polling_transmit(spiDev, &t); // Transmit!
}

static inline u32 halSpiConvertPixels(u32 src) {
	return src;
}

void halSpiWrite8(u8 v) {
	halSpiWriteBits(v, 8);
}

void halSpiWrite16(u16 v) {
	halSpiWriteBits(v, 16);
}

void halSpiWrite32(u32 v) {
	halSpiWriteBits(v, 32);
}

void halSpiWriteBuf(JS_BUFFER buf, u32 offset, u32 len) {
	if ((len <= buf.size) && (offset + len <= buf.size) && (offset < buf.size)) {
		spi_transaction_t t;
		memset(&t, 0, sizeof(t)); // Zero out the transaction
		t.length = 8 * len; // Command is 8 bits
		t.tx_buffer = buf.buf + offset; // The data is the cmd itself
		t.user = (void*) 0; // D/C needs to be set to 0
		spi_device_polling_transmit(spiDev, &t); // Transmit!
	}
}
