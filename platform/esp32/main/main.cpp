/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// Disable warnings about missing field initializers.
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#define TAG "mcujs"
#include "duktape.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "global.h"
#include "nvs_flash.h"
#include "print-alert/duk_print_alert.h"
#include "usb-debug.hpp"


#define MJS_HANDLE_TABLE_SIZE (1024)
typedef struct _MJS_HANDLE_ENTRY {
  int objType;
  void* opaque;
} MJS_HANDLE_ENTRY;
MJS_HANDLE_ENTRY mjsHandleTable[MJS_HANDLE_TABLE_SIZE];

typedef struct _MJS_QUEUE_MSG {
  int objType;
  int handle;
  int args[4];
  int dataLen;
  u8 data[1024];
  u8* largeDataPtr;
} MJS_QUEUE_MSG;

QueueHandle_t mjsMsgQueue;


#define MCUJS_MALLOC_CAPS (MALLOC_CAP_SPIRAM)


void* mjsAlloc(size_t size) {
  return heap_caps_malloc(size, MCUJS_MALLOC_CAPS);
}

void mjsFree(void* ptr) {
  heap_caps_free(ptr);
}


int mjsAllocHandle(int objType, void *opaque) {
  int i;
  for (i = 1; i < MJS_HANDLE_TABLE_SIZE; i++) {
    if (mjsHandleTable[i].objType == 0) {
      mjsHandleTable[i].objType = objType;
      mjsHandleTable[i].opaque = opaque;
      return i;
    }
  }
  return -1;
}

void* mjsRefHandle(int handle, int desiredType) {
  if (handle < 0 || handle >= MJS_HANDLE_TABLE_SIZE) {
    return NULL;
  }
  if (mjsHandleTable[handle].objType != desiredType) {
    return NULL;
  }
  return mjsHandleTable[handle].opaque;
}



#include "cloud.hpp"

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

static void mcujs_fatal_handler(void *udata, const char *msg) {
  (void)udata; /* ignored in this case, silence warning */

  /* Note that 'msg' may be NULL. */
  mjsPrintf("*** DUKTAPE FATAL ERROR: %s\n", (msg ? msg : "no message"));
  halOsSleepMs(5000);
  mjsPrintf("rebooting...\n");
  halOsReboot();
  /* abort(); */
}

int mjsHandleAssertFailed(const char *expr, const char *file, int line) {
  mjsPrintf("*** MCUJS ASSERT FAILED: %s %s %d\n", expr, file, line);
  halOsSleepMs(5000);
  mjsPrintf("rebooting...\n");
  halOsReboot();
  return 0;
}

void loadBuiltinJS(duk_context *ctx, const u8 *bin, const char *filename) {
  if (duk_peval_string(ctx, (char *)bin) != 0) {
    mjsPrintf("load %s failed: %s\n", filename, duk_safe_to_stacktrace(ctx, -1));
  }
  duk_pop(ctx);
}



void ioLoop(duk_context *ctx) {
  static MJS_QUEUE_MSG msg;
  while(1) {
    if (!xQueueReceive(mjsMsgQueue, &msg, 0)) {
      break;
    }
    // Now we have a message, call the handler
    duk_get_global_string(ctx, "ioMsgHandler");
    duk_push_int(ctx, msg.objType);
    duk_push_int(ctx, msg.handle);
    duk_push_int(ctx, msg.args[0]);
    duk_push_int(ctx, msg.args[1]);
    duk_push_int(ctx, msg.args[2]);
    duk_push_int(ctx, msg.args[3]);
    if (msg.dataLen > 0) {
      u8* ptr = msg.data;
      if (msg.largeDataPtr) {
        ptr = msg.largeDataPtr;
      }
      duk_push_external_buffer(ctx);
      duk_config_buffer(ctx, -1, ptr, msg.dataLen);

    } else {
      // Push NULL
      duk_push_null(ctx);
    }
    if (duk_pcall(ctx, 7) != 0) {
      mjsPrintf("ioLoop: %s\n", duk_safe_to_string(ctx, -1));
    }
    if (msg.largeDataPtr) {
      mjsFree(msg.largeDataPtr);
    }
    duk_pop(ctx);
  }
}

int mainLoop() {
  halLcdInit();
  halI2cInit(0, 38, 39, 100000);

  mjsMsgQueue = xQueueCreate(128, sizeof(MJS_QUEUE_MSG));
  MCUJS_ASSERT(mjsMsgQueue);

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  // Initialize Network
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  halWifiInit();

  // Initialize Duktape
  duk_context *ctx =
      duk_create_heap(mcujs_alloc_function, mcujs_realloc_function,
                      mcujs_free_function, NULL, mcujs_fatal_handler);
  MCUJS_ASSERT(ctx);

  // Initialize Duktape modules
  duk_print_alert_init(ctx, 0);

  // Load builtin modules
  genJSInit(ctx);
  module_fs_init(ctx);

  mjsPrintf("Initializing Javascript modules...\n");
  loadBuiltinJS(ctx, js_underscore, "underscore");
  loadBuiltinJS(ctx, js_boot, "boot");

  idInit();
  
  // Call boot()
  duk_eval_string(ctx, "boot();");
  duk_pop(ctx);


  while(1) {
    // Call the mainLoop function
    duk_get_global_string(ctx, "mainLoop");
    duk_call(ctx, 0);
    duk_pop(ctx);

    const char* line = udReadLine();
    if (line) {
      // Evaluate the line
      if (duk_peval_string(ctx, line) != 0) {
        // Print the error
        mjsPrintf("eval failed: ");
        mjsPrintString(duk_safe_to_string(ctx, -1));
        mjsPrintf("\n");
      } else {
        // Print the result
        mjsPrintString(duk_safe_to_string(ctx, -1));
        mjsPrintf("\n");
      }
      duk_pop(ctx);
    }

    ioLoop(ctx);

    // Sleep 100ms
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  duk_destroy_heap(ctx);
  return 0;
}

void mcuJsTask(void *arg) {
  mainLoop();
  MCUJS_ASSERT(0);
}

TaskHandle_t mcuJsTaskHandle;

extern "C" void app_main() {
  halUartInitPort(0, -1, -1, -1, -1);
  udInit();
  mjsPrintf("Starting mcu.js task...\n");
  xTaskCreatePinnedToCore(mcuJsTask, "mcuJsTask", 32768, NULL, 1,
                          &mcuJsTaskHandle, 1);
}

