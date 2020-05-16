// Generate lcd init code from TFT_eSPI library

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

typedef uint8_t u8;
u8 dataBuf[1024];
int dataBufPos;
u8 lastCmd;
int hasLastCmd = 0;

void commitLastCmd() {
    if (!hasLastCmd) {
        return;
    }
    hasLastCmd = 0;
    assert(dataBufPos < 0xFF);
    printf("0x%02X, 0x%02X, ", lastCmd, dataBufPos);
    int i = 0;
    for (i = 0; i < dataBufPos; i++) {
        printf("0x%02X, ", dataBuf[i]);
    }
    printf("//cmd\n");
    dataBufPos = 0;
}

void writecommand(u8 cmd) {
    assert(cmd != 0xFF);
    commitLastCmd();
    lastCmd = cmd;
    hasLastCmd = 1;
}

void writedata(u8 dat) {
    
    dataBuf[dataBufPos] = dat;
    dataBufPos ++;
}

void delay(int ms) {
    commitLastCmd();
    assert(ms < 0xFF);
    printf("0xFF, %d, //delay \n", ms);
}

void spi_begin() {

}

void spi_end() {

}

#define TFT_WIDTH  240
#define TFT_HEIGHT 240
#include "ST7789_Defines.h"
void init()
#include "ST7789_Init.h"

int main() {
    init();
    commitLastCmd();
    printf("0xFF, 0xFF //FIN\n");
}
