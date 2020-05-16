#pragma once
#include "global.h"
#include "driver/gpio.h"

#define HAL_GPIO_PUPD_PULLUP (1)
#define HAL_GPIO_PUPD_PULLDOWN (2)

static void halGpioConfig(u32 pin, u32 mode, u32 pupd) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1 << pin),
        .mode = (gpio_mode_t)mode,
        .pull_up_en = (pupd == HAL_GPIO_PUPD_PULLUP) ? GPIO_PULLUP_ENABLE : 0,
        .pull_down_en = (pupd == HAL_GPIO_PUPD_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : 0,
        .intr_type = 0
    };
    gpio_config(&io_conf);
}

static void halGpioWrite(u32 gpio_num, u32 level) {
    gpio_set_level(gpio_num, level);
}

static ALWAYS_INLINE u32 halGpioRead(u32 gpio_num) {
    return gpio_get_level(gpio_num);
}