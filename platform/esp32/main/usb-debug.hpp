// USB Debug Interface

#include "global.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "sdkconfig.h"
#include "hal/usb_serial_jtag_ll.h"

QueueHandle_t udRxQueue, udTxQueue;
TaskHandle_t udTask;

#define UD_RX_BUF_SIZE (20000)
#define UD_TX_BUF_SIZE (20000)
#define UD_LINE_BUF_SIZE (20000)

u8* udLineBuf;
int udLineBufPtr = 0;

void udTryTxData() {
    size_t bytesSent = 0;
    while(1) {
        if (!usb_serial_jtag_ll_txfifo_writable()) {
            break;
        }
        uint8_t data;
        if (xQueueReceive(udTxQueue, &data, 0)) {
            usb_serial_jtag_ll_write_txfifo(&data, 1);
            bytesSent++;
        } else {
            break;
        }
    }
    if (bytesSent > 0) {
        usb_serial_jtag_ll_txfifo_flush();
    }
}

void udTryRxData() {
    while(1) {
        u8 data;
        u32 bytesRead = usb_serial_jtag_ll_read_rxfifo(&data, 1);
        if (bytesRead > 0) {
            xQueueSend(udRxQueue, &data, 0);
        } else {
            break;
        }
    }
}

// Define udUsbSerialTask
void udUsbSerialTask(void *pvParameters)
{
    while(1) {
        udTryTxData();
        udTryRxData();
        // Delay 20ms
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

int udInit() {
    // Initialize the ring buffers
    udRxQueue = xQueueCreate(UD_RX_BUF_SIZE, sizeof(uint8_t));
    udTxQueue = xQueueCreate(UD_TX_BUF_SIZE, sizeof(uint8_t));
    MCUJS_ASSERT(udRxQueue);
    MCUJS_ASSERT(udTxQueue);
    udLineBuf = (u8*)malloc(UD_LINE_BUF_SIZE);
    MCUJS_ASSERT(udLineBuf);
    // Start the USB serial task
    xTaskCreatePinnedToCore(udUsbSerialTask, "udUsbSerialTask", 4096, NULL, 5, &udTask, 0);
    return 0;
}

int udReadChar(u8* c) {
    int ret = xQueueReceive(udRxQueue, c, 0);
    if (ret > 0) {
        return 1;
    }
    ret = uart_read_bytes(UART_NUM_0, c, 1, 0);
    if (ret > 0) {
        return 1;
    }
    return 0;
}

const char* udReadLine() {
    while(1) {
        u8 data;
        if (udReadChar(&data)) {
            if (data == 0xF8) {
                udLineBuf[udLineBufPtr] = 0;
                udLineBufPtr = 0;
                return (const char*)udLineBuf;
            }
            udLineBuf[udLineBufPtr++] = data;
            if (udLineBufPtr >= UD_LINE_BUF_SIZE) {
                printf("udReadLine: Line buffer overflow\n");
                udLineBufPtr = 0;
                return NULL;
            }
        } else {
            return NULL;
        }
    }
}

void udPutChar(char data) {
    xQueueSend(udTxQueue, (u8*) &data, 0);
    uart_write_bytes(UART_NUM_0, &data, 1);
}


void udPutString(const char* data) {
    while(*data) {
        udPutChar(*data);
        data++;
    }
}


void udPutBuffer(const u8* data, size_t len) {
    for(size_t i = 0; i < len; i++) {
        udPutChar((char)data[i]);
    }
}


extern "C" {


void mjsPrintf(const char *fmt, ...) {
    char buf[512];
    // Print to buffer
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    udPutString(buf);
}


void mjsPrintBuf(const uint8_t *buf, size_t len) {
    udPutBuffer(buf, len);
}


void mjsPrintString(const char *str) {
    udPutString(str);
}

void mjsPrintBufHex(const uint8_t* buf, size_t len) {
    const char* hexDigits = "0123456789abcdef";
    
    for(size_t i = 0; i < len; i++) {
        udPutChar(hexDigits[(buf[i] >> 4) & 0xF]);
        udPutChar(hexDigits[buf[i] & 0xF]);
    }
}

}