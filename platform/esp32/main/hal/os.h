
#pragma once

void halOsSleepMs(uint32_t t)
{
    vTaskDelay(t / portTICK_PERIOD_MS);
}

uint32_t halOsGetTickCountMs()
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

uint32_t halOsGetFreeMem() {
    return esp_get_free_heap_size();
}

uint32_t halOsGetMinFreeMem() {
    return esp_get_minimum_free_heap_size();
}

uint32_t halOsReboot() {
    esp_restart();
    return 0;
}