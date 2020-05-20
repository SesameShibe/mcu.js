#include <driver/uart.h>

static const uart_config_t uart_config = { .baud_rate = 115200,
	                                       .data_bits = UART_DATA_8_BITS,
	                                       .parity = UART_PARITY_DISABLE,
	                                       .stop_bits = UART_STOP_BITS_1,
	                                       .flow_ctrl = UART_HW_FLOWCTRL_DISABLE };

static const int RX_BUF_SIZE = 16384, TX_BUF_SIZE = 1024;

int halUartInitPort(int32_t uart_num, int32_t tx_pin, int32_t rx_pin, int32_t rts_pin, int32_t cts_pin) {
	uart_param_config((uart_port_t) uart_num, &uart_config);
	uart_set_pin((uart_port_t) uart_num, tx_pin, rx_pin, rts_pin, cts_pin);
	uart_driver_install((uart_port_t) uart_num, RX_BUF_SIZE, TX_BUF_SIZE, 0, NULL, 0);

	return 1;
}

int32_t halUartReadByte(int32_t uart_num) {
	uint8_t buf[1];
	uart_read_bytes((uart_port_t) uart_num, &buf[0], 1, 100);

	return (int32_t)(buf[0] & 0xFF);
}

void halUartWriteString(int32_t uart_num, const char* str) {
	size_t len = strlen(str);
	uart_write_bytes((uart_port_t) uart_num, str, len);
}

void halUartWriteByte(int32_t uart_num, int32_t c) {
	char s[2] = { 0, 0 };
	s[0] = (char) c;
	// putchar(c);
	uart_write_bytes((uart_port_t) uart_num, s, 1);
}

int32_t halUartAvailableBytes(int32_t uart_num) {
	int32_t len = 0;
	uart_get_buffered_data_len((uart_port_t) uart_num, (size_t*) &len);
	return len;
}
