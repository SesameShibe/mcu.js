#include "global.h"
#include "driver/gpio.h"

void halGpioConfig(u32 pin, u32 mode, u32 pupd) {
    gpio_config_t io_conf {};
    io_conf.pin_bit_mask =  (1 << pin);
    io_conf.mode = (gpio_mode_t)mode;
    if (pupd == 1) {
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    }
    if (pupd == 2) {
        io_conf.pull_down_en =  GPIO_PULLDOWN_ENABLE;
    }
    gpio_config(&io_conf);
}

void halGpioWrite(u32 pin, u32 level) {
    gpio_set_level((gpio_num_t) pin, level);
}

u32 halGpioRead(u32 pin) {
    return gpio_get_level((gpio_num_t) pin);
}