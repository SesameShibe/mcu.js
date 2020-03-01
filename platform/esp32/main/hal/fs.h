#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_heap_caps.h"
#include "esp_err.h"
#include "esp_spiffs.h"

void halFsMountSpiffs()
{
  esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spi",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true
  };

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      printf("Failed to mount or format filesystem\n");
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
      printf("Failed to find SPIFFS partition\n");
    }
    else
    {
      printf("Failed to initialize SPIFFS (%s)\n", esp_err_to_name(ret));
    }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
    printf("Failed to get SPIFFS partition information (%s)\n", esp_err_to_name(ret));
  } else {
    printf("Partition size: total: %d, used: %d\n", total, used);
  }
}


int halFsExists(const char* fn) {
  struct stat st;
  return (stat(fn, &st) == 0);
}

i32 halFsRead(i32 fd, JS_BUFFER buf, i32 count) {
  if (count < 0) {
    return -1;
  }
  if (count > buf.size) {
    return -1;
  }
  return read(fd, buf.buf, count);
}

i32 halFsWrite(i32 fd, JS_BUFFER buf, i32 count) {
  if (count < 0) {
    return -1;
  }
  if (count > buf.size) {
    return -1;
  }
  return write(fd, buf.buf, count);
}

