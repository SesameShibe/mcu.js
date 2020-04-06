/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "global.h"
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "duktape.h"
#include "print-alert/duk_print_alert.h"
//#include "TFT_eSPI.h"

//TFT_eSPI tft = TFT_eSPI(135, 240);

extern "C"
{
#include "hal/os.h"
#include "hal/socket.h"
#include "hal/uart.h"
#include "hal/fs.h"
#include "hal/spi.h"
#include "hal/crypto.h"
}
#include "hal/fb.h"

#include "__generated/gen_js.h"
#include "__generated/gen_jsmods.h"


static void mcujs_fatal_handler(void *udata, const char *msg)
{
    (void)udata; /* ignored in this case, silence warning */

    /* Note that 'msg' may be NULL. */
    printf("*** DUKTAPE FATAL ERROR: %s\n", (msg ? msg : "no message"));
    halOsSleepMs(5000);
    printf("rebooting...\n");
    halOsReboot();
    /* abort(); */
}


void loadBuiltinJS(duk_context *ctx, const u8 *bin, const char *filename)
{
    if (duk_peval_string(ctx, (char *)bin) != 0)
    {
        printf("load %s failed: %s\n", filename, duk_safe_to_stacktrace(ctx, -1));
        duk_pop(ctx);
    }
}

int mainLoop()
{
    duk_context *ctx = duk_create_heap(NULL, NULL, NULL, NULL, mcujs_fatal_handler);
    if (!ctx)
    {
        fprintf(stderr, "Failed to create a Duktape heap.\n");
        exit(1);
    }

    /* init print-alert */
    duk_print_alert_init(ctx,0);

    /* init modules */
    genJSInit(ctx);
    //module_file_init(ctx);

    /* boot */
    loadBuiltinJS(ctx, js_boot, "boot");
    loadBuiltinJS(ctx, js_shell, "shell");
    loadBuiltinJS(ctx, js_net, "net");
    loadBuiltinJS(ctx, js_http, "http");
    loadBuiltinJS(ctx, js_ws, "ws");
    loadBuiltinJS(ctx, js_ui, "ui");
    //loadBuiltinJS(ctx, js_tft, "tft");

    /* callback */
    duk_eval_string(ctx, "boot();");

    duk_destroy_heap(ctx);
    return 0;
}

/**
void setup()
{
    printf("Hello mcu.js!\n");

    // Print chip information 
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);
    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}
**/

void mcuJsTask(void *arg)
{
    while (1)
    {
        mainLoop();
    }
}

TaskHandle_t mcuJsTaskHandle;

extern "C" void app_main()
{
    initArduino();
    // tft.init();
    // tft.setRotation(0);
    // tft.fillScreen(TFT_BLACK);
    // tft.setCursor(0, 0);
    // tft.setTextColor(TFT_GREEN);
    // tft.setTextSize(2);
    // tft.print("hello mcu.js!\n");
    xTaskCreateUniversal(mcuJsTask, "mcuJsTask", 16384, NULL, 1, &mcuJsTaskHandle, 1);
}