/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "global.h"
#include "nvs_flash.h"

#include "duktape.h"
#include "print-alert/duk_print_alert.h"


#include "hal/crypto.h"
#include "hal/fs.h"
#include "hal/os.h"
#include "hal/socket.h"
#include "hal/uart.h"
#include "hal/gpio.h"
#include "hal/wifi.h"

#include "__generated/gen_js.h"
#include "__generated/gen_jsmods.h"

static void mcujs_fatal_handler(void* udata, const char* msg) {
	(void) udata; /* ignored in this case, silence warning */

	/* Note that 'msg' may be NULL. */
	printf("*** DUKTAPE FATAL ERROR: %s\n", (msg ? msg : "no message"));
	halOsSleepMs(5000);
	printf("rebooting...\n");
	halOsReboot();
	/* abort(); */
}

void loadBuiltinJS(duk_context* ctx, const u8* bin, const char* filename) {
	if (duk_peval_string(ctx, (char*) bin) != 0) {
		printf("load %s failed: %s\n", filename, duk_safe_to_stacktrace(ctx, -1));
		duk_pop(ctx);
	}
}

int mainLoop() {
	duk_context* ctx = duk_create_heap(NULL, NULL, NULL, NULL, mcujs_fatal_handler);
	if (!ctx) {
		fprintf(stderr, "Failed to create a Duktape heap.\n");
		exit(1);
	}

	/* init print-alert */
	duk_print_alert_init(ctx, 0);

	/* init modules */
	genJSInit(ctx);
	// module_file_init(ctx);

	/* boot */
	loadBuiltinJS(ctx, js_underscore, "underscore");
	loadBuiltinJS(ctx, js_boot, "boot");
	loadBuiltinJS(ctx, js_shell, "shell");
	loadBuiltinJS(ctx, js_net, "net");
	loadBuiltinJS(ctx, js_http, "http");
	loadBuiltinJS(ctx, js_ws, "ws");
	loadBuiltinJS(ctx, js_ui, "ui");
	loadBuiltinJS(ctx, js_devserver, "devserver");

	/* init nvs */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      nvs_flash_erase();
      nvs_flash_init();
    }

	/* callback */
	duk_eval_string(ctx, "boot();");

	duk_destroy_heap(ctx);
	return 0;
}



void mcuJsTask(void* arg) {
	while (1) {
		mainLoop();
	}
}

TaskHandle_t mcuJsTaskHandle;

extern "C" void app_main() {
	xTaskCreatePinnedToCore(mcuJsTask, "mcuJsTask", 32768, NULL, 1, &mcuJsTaskHandle, 1);
}