
#pragma once

void halOsSleepMs(u32 ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}


#define halOsDelayUs ets_delay_us


uint32_t halOsGetTickCountMs()
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

uint32_t halOsGetFreeMem() {
    return esp_get_free_heap_size();
}

uint32_t halOsGetFreeSRAM() {
    return heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
}

uint32_t halOsGetMinFreeMem() {
    return esp_get_minimum_free_heap_size();
}

uint32_t halOsReboot() {
    esp_restart();
    return 0;
}