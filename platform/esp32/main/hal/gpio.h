#pragma once
#include "global.h"
#include "driver/gpio.h"

static void halGpioConfig(u32 pin, u32 mode, u32 pupd) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1 << pin),
        .mode = (gpio_mode_t)mode,
        .pull_up_en = (pupd == 1) ? GPIO_PULLUP_ENABLE : 0,
        .pull_down_en = (pupd == 2) ? GPIO_PULLDOWN_ENABLE : 0,
        .intr_type = 0
    };
    gpio_config(&io_conf);
}

static inline void halGpioWrite(u32 pin, u32 level) {
    gpio_set_level((gpio_num_t) pin, level);
}

static inline u32 halGpioRead(u32 pin) {
    return gpio_get_level((gpio_num_t) pin);
}