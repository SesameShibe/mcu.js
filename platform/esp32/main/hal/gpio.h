#pragma once
#include "driver/gpio.h"
#include "global.h"

#define HAL_GPIO_PUPD_PULLUP (1)
#define HAL_GPIO_PUPD_PULLDOWN (2)

static void halGpioConfig(u32 pin, u32 mode, u32 pupd) {
	gpio_config_t io_conf = { .pin_bit_mask = ((uint64_t)1 << pin),
		                      .mode = (gpio_mode_t) mode,
		                      .pull_up_en = (pupd == HAL_GPIO_PUPD_PULLUP) ? GPIO_PULLUP_ENABLE : 0,
		                      .pull_down_en = (pupd == HAL_GPIO_PUPD_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : 0,
		                      .intr_type = 0 };
	gpio_config(&io_conf);
}

static void IRAM_ATTR halGpioWrite(u32 gpio_num, u32 level) {
	if (level) {
		if (gpio_num < 32) {
			GPIO.out_w1ts = (1 << gpio_num);
		} else {
			GPIO.out1_w1ts.data = (1 << (gpio_num - 32));
		}
	} else {
		if (gpio_num < 32) {
			GPIO.out_w1tc = (1 << gpio_num);
		} else {
			GPIO.out1_w1tc.data = (1 << (gpio_num - 32));
		}
	}
}

static u32 IRAM_ATTR halGpioRead(u32 gpio_num) {
	if (gpio_num < 32) {
        return (GPIO.in >> gpio_num) & 0x1;
    } else {
        return (GPIO.in1.data >> (gpio_num - 32)) & 0x1;
    }
}