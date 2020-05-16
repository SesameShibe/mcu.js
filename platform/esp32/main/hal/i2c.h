
#pragma once
#include "global.h"
#include "gpio.h"

typedef struct _HAL_SWI2C_BUS {
	u32 pinSDA;
	u32 pinSCL;
	u8 lastSDA;
	u8 lastSCL;
	u32 extraDelay;
} HAL_SWI2C_BUS;
#define HAL_SWI2C_MAX_BUS (8)
static HAL_SWI2C_BUS i2cBus[HAL_SWI2C_MAX_BUS];

#define SET_SDA(v) gpio_set_level(i2cBus[busID].pinSDA, (v))
#define SET_SCL(v) gpio_set_level(i2cBus[busID].pinSCL, (v))
#define GET_SDA() gpio_get_level(i2cBus[busID].pinSDA)
#define SWI2C_DELAY halOsDelayUs(10)
#define CHECK_BUSID MCUJS_ASSERT(busID <= HAL_SWI2C_MAX_BUS)


void halI2cStart(u8 busID) {
    SWI2C_DELAY;
	SET_SDA(1);
	SET_SDA(0);
	SWI2C_DELAY;
	SET_SCL(0);
}

void halI2cStop(u8 busID) {
	SET_SDA(0);
	SWI2C_DELAY;
	SET_SCL(1);
	SWI2C_DELAY;
	SET_SDA(1);
	SWI2C_DELAY;
}

static void halI2cWriteBit(u8 busID, u8 bit) {
	SET_SDA(bit);
	SWI2C_DELAY;
	SET_SCL(1);
	SWI2C_DELAY;
	SET_SCL(0);
}

static u8 halI2cReadBit(u8 busID) {
	SET_SDA(1);
	SWI2C_DELAY;
	SET_SCL(1);
	SWI2C_DELAY;
	u8 ret = GET_SDA();
	SWI2C_DELAY;
	SET_SCL(0);
    return ret;
}

u8 halI2cWrite(u8 busID, u8 dat) {
    CHECK_BUSID;
    
	int i;
	for (i = 0; i < 8; i++) {
		halI2cWriteBit(busID, dat & 0x80);
		dat <<= 1;
	}
    // return 1 if ACK received
	return halI2cReadBit(busID) == 0;
}

u8 halI2cAddr(u8 busID, u8 addr, u8 isTx) {
    CHECK_BUSID;

	return halI2cWrite(busID, (addr << 1) + (isTx ? 0 : 1));
}

// ack should be 0 for last byte, otherwise ack should be 1
u8 halI2cRead(u8 busID, u8 ack) {
    CHECK_BUSID;

    u8 dat = 0;
    int i;

    SET_SDA(1);
    for (i = 0; i < 8; i++) {
        dat <<= 1;
		dat |= halI2cReadBit(busID);
	}
    halI2cWriteBit(busID, ack ? 0 : 1);
	return dat;
}

u8 halI2cSimpleRead8(u8 busID, u8 addr, u8 reg) {
    CHECK_BUSID;

	halI2cStart(busID);
	halI2cAddr(busID, addr, 1);
    halI2cWrite(busID, reg);
	halI2cStop(busID);

	halI2cStart(busID);
	halI2cAddr(busID, addr, 0);
	u8 ret = halI2cRead(busID, 0);
	halI2cStop(busID);
	return ret;
}

u32 halI2cInit(u8 busID, u32 pinSDA, u32 pinSCL, u32 clkSpeed) {
    CHECK_BUSID;

	clkSpeed = 100000;
	i2cBus[busID].pinSCL = pinSCL;
	i2cBus[busID].pinSDA = pinSDA;
	SET_SCL(1);
	SET_SDA(1);
	// There should be a 10k-ohm pullup resistor on the pcb board.
	// When these resistors are absent, enabling internal pullup may help.
	halGpioConfig(pinSCL, GPIO_MODE_INPUT_OUTPUT_OD, 0);
	halGpioConfig(pinSDA, GPIO_MODE_INPUT_OUTPUT_OD, 0);
	return clkSpeed;
}
