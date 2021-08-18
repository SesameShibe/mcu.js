/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "duktape.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global.h"
#include "nvs_flash.h"
#include "print-alert/duk_print_alert.h"

#define MCUJS_MALLOC_CAPS (MALLOC_CAP_SPIRAM)

DUK_INTERNAL void *mcujs_alloc_function(void *udata, duk_size_t size) {
  void *res;
  DUK_UNREF(udata);
  res = heap_caps_malloc(size, MCUJS_MALLOC_CAPS);
  return res;
}

DUK_INTERNAL void *mcujs_realloc_function(void *udata, void *ptr,
                                          duk_size_t newsize) {
  void *res;
  DUK_UNREF(udata);
  res = heap_caps_realloc(ptr, newsize, MCUJS_MALLOC_CAPS);
  return res;
}

DUK_INTERNAL void mcujs_free_function(void *udata, void *ptr) {
  DUK_UNREF(udata);
  heap_caps_free(ptr);
}

#include "hal/crypto.h"
#include "hal/fs.h"
#include "hal/os.h"
#include "hal/socket.h"
#include "hal/uart.h"
#include "hal/gpio.h"
#include "hal/i2c.h"
#include "hal/wifi.h"
#include "hal/lcd-driver.h"

#include "__generated/gen_jsmods.h"
#include "__generated/gen_js.h"

static void mcujs_fatal_handler(void *udata, const char *msg) {
  (void)udata; /* ignored in this case, silence warning */

  /* Note that 'msg' may be NULL. */
  printf("*** DUKTAPE FATAL ERROR: %s\n", (msg ? msg : "no message"));
  halOsSleepMs(5000);
  printf("rebooting...\n");
  halOsReboot();
  /* abort(); */
}

int mcujsHandleAssertFailed(const char *expr, const char *file, int line) {
  printf("*** MCUJS ASSERT FAILED: %s %s %d\n", expr, file, line);
  halOsSleepMs(5000);
  printf("rebooting...\n");
  halOsReboot();
  return 0;
}

void loadBuiltinJS(duk_context *ctx, const u8 *bin, const char *filename) {
  if (duk_peval_string(ctx, (char *)bin) != 0) {
    printf("load %s failed: %s\n", filename, duk_safe_to_stacktrace(ctx, -1));
    duk_pop(ctx);
  }
}

int mainLoop() {
  halLcdInit();

  duk_context *ctx =
      duk_create_heap(mcujs_alloc_function, mcujs_realloc_function,
                      mcujs_free_function, NULL, mcujs_fatal_handler);
  MCUJS_ASSERT(ctx);

  /* init print-alert */
  duk_print_alert_init(ctx, 0);

  /* init modules */
  genJSInit(ctx);
  module_fs_init(ctx);

  printf("Initializing Javascript modules...\n");
  /* boot */
  loadBuiltinJS(ctx, js_underscore, "underscore");
  loadBuiltinJS(ctx, js_boot, "boot");
  loadBuiltinJS(ctx, js_shell, "shell");
  /* init nvs */
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  /* callback */
  duk_eval_string(ctx, "boot();");

  duk_destroy_heap(ctx);
  return 0;
}

void mcuJsTask(void *arg) {
  mainLoop();
  MCUJS_ASSERT(0);
}

TaskHandle_t mcuJsTaskHandle;

extern "C"
void app_main() {
  xTaskCreatePinnedToCore(mcuJsTask, "mcuJsTask", 32768, NULL, 1,
                          &mcuJsTaskHandle, 1);
}