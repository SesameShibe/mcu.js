
#pragma once
#include "global.h"
#include "gpio.h"

typedef struct _HAL_SWI2C_BUS {
	u32 pinSDA;
	u32 pinSCL;
	//u32 maskSDA;
	//u32 maskSCL;
} HAL_SWI2C_BUS;

#define HAL_SWI2C_MAX_BUS (8)
static HAL_SWI2C_BUS i2cBus[HAL_SWI2C_MAX_BUS];


#define SET_SDA(v) halGpioWrite(i2cBus[busID].pinSDA, (v))
#define SET_SCL(v) halGpioWrite(i2cBus[busID].pinSCL, (v))
#define GET_SDA() halGpioRead(i2cBus[busID].pinSDA)

#define SWI2C_DELAY halOsDelayUs(10)

/*
static ALWAYS_INLINE void i2cGpioWrite(u32 mask, u8 level) {
	if (level) {
		GPIO.out_w1ts = mask;
	} else {
		GPIO.out_w1tc = mask;
	}
}

static ALWAYS_INLINE u8 i2cGpioRead(u32 mask) {
	return (GPIO.in & mask) ? 1 : 0;
}

#define SET_SDA(v) i2cGpioWrite(i2cBus[busID].maskSDA, (v))
#define SET_SCL(v) i2cGpioWrite(i2cBus[busID].maskSCL, (v))
#define GET_SDA() i2cGpioRead(i2cBus[busID].maskSDA)*/

void halI2cStart(u8 busID) {
	SET_SDA(1);
	//SWI2C_DELAY;
	SET_SCL(1);
	SWI2C_DELAY;
	SET_SDA(0);
	SWI2C_DELAY;
	SET_SCL(0);
	SWI2C_DELAY;
}

void halI2cStop(u8 busID) {
	SET_SDA(0);
	//SWI2C_DELAY;
	SET_SCL(1);
	SWI2C_DELAY;
	SET_SDA(1);
	SWI2C_DELAY;
}

static void halI2cWriteBit(u8 busID, u8 bit) {
	SET_SDA(bit);
	//SWI2C_DELAY;
	SET_SCL(1);
	SWI2C_DELAY;
	SET_SCL(0);
	SWI2C_DELAY;
}

static u8 halI2cReadBit(u8 busID) {
	SET_SDA(1);
	//SWI2C_DELAY;
	SET_SCL(1);
	SWI2C_DELAY;
	u8 ret = GET_SDA();
	//SWI2C_DELAY;
	SET_SCL(0);
	SWI2C_DELAY;
    return ret;
}

u8 halI2cWrite(u8 busID, u8 dat) {
    CHECK_BUSID(HAL_SWI2C_MAX_BUS);
    
	int i;
	for (i = 0; i < 8; i++) {
		halI2cWriteBit(busID, dat & 0x80);
		dat <<= 1;
	}
    // return 1 if ACK received
	return halI2cReadBit(busID) == 0;
}

u8 halI2cAddr(u8 busID, u8 addr, u8 isWrite) {
    CHECK_BUSID(HAL_SWI2C_MAX_BUS);

	return halI2cWrite(busID, (addr << 1) + (isWrite ? 0 : 1));
}

// ack should be 0 for last byte, otherwise ack should be 1
u8 halI2cRead(u8 busID, u8 ack) {
    CHECK_BUSID(HAL_SWI2C_MAX_BUS);

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

u8 halI2cSimpleReadRaw(u8 busID, u8 addr, u8 reg, u8* buf, size_t len) {
    CHECK_BUSID(HAL_SWI2C_MAX_BUS);

	int i;
	halI2cStart(busID);
	u8 ack = halI2cAddr(busID, addr, 1);
    halI2cWrite(busID, reg);
	halI2cStop(busID);

	halI2cStart(busID);
	halI2cAddr(busID, addr, 0);
	for (i = 0; i < len; i++) {
		buf[i] = halI2cRead(busID, (i < len - 1) ? 1 : 0);
	}
	halI2cStop(busID);
	return ack;
}

u8 halI2cSimpleWriteRaw(u8 busID, u8 addr, u8 reg, u8* buf, size_t len) {
    CHECK_BUSID(HAL_SWI2C_MAX_BUS);

	int i;
	halI2cStart(busID);
	u8 ack = halI2cAddr(busID, addr, 1);
    halI2cWrite(busID, reg);
	for (i = 0; i < len; i++) {
		halI2cWrite(busID, buf[i]);
	}
	halI2cStop(busID);
	return ack;
}

u8 halI2cSimpleRead8(u8 busID, u8 addr, u8 reg) {
	u8 tmp[4];
	tmp[0] = 0xff;
	halI2cSimpleReadRaw(busID, addr, reg, tmp, 1);
	return tmp[0];
}

u8 halI2cSimpleWrite8(u8 busID, u8 addr, u8 reg, u8 val) {
	u8 tmp[4];
	tmp[0] = val;
	return halI2cSimpleWriteRaw(busID, addr, reg, tmp, 1);
}

u8 halI2cSimpleReadBuf(u8 busID, u8 addr, u8 reg, JS_BUFFER buf) {
	CHECK_JSBUF_SIZE(buf, 1);
	return halI2cSimpleReadRaw(busID, addr, reg, buf.buf, buf.size);
}

u8 halI2cSimpleWriteBuf(u8 busID, u8 addr, u8 reg, JS_BUFFER buf) {
	CHECK_JSBUF_SIZE(buf, 1);
	return halI2cSimpleWriteRaw(busID, addr, reg, buf.buf, buf.size);
}


u32 halI2cInit(u8 busID, u32 pinSDA, u32 pinSCL, u32 clkSpeed) {
    CHECK_BUSID(HAL_SWI2C_MAX_BUS);

	clkSpeed = 100000;
	i2cBus[busID].pinSCL = pinSCL;
	i2cBus[busID].pinSDA = pinSDA;
	//i2cBus[busID].maskSCL = (u32)1 << pinSCL;
	//i2cBus[busID].maskSDA = (u32)1 << pinSDA;
	SET_SCL(1);
	SET_SDA(1);
	// There should be a 10k-ohm pullup resistor on the pcb board.
	// When these resistors are absent, enabling internal pullup may help.
	halGpioConfig(pinSCL, GPIO_MODE_INPUT_OUTPUT_OD, HAL_GPIO_PUPD_PULLUP);
	halGpioConfig(pinSDA, GPIO_MODE_INPUT_OUTPUT_OD, HAL_GPIO_PUPD_PULLUP);
	return clkSpeed;
}
