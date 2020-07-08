#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_heap_caps.h"
#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "duktape.h"
#include "global.h"

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

static duk_ret_t glueFsRead(duk_context *ctx)
{
    int32_t ret;
    int32_t fd;
    JS_BUFFER buf;
    int32_t count;


    fd = duk_to_int32(ctx, 0);

    buf.buf = (u8*) duk_get_buffer_data(ctx, 1, &buf.size);

    count = duk_to_int32(ctx, 2);
    
    ret = halFsRead(fd, buf, count);

    duk_push_int(ctx, ret);
    return 1;
}

// i32 halFsReadStr(duk_context *ctx) {
//     int32_t ret;
//     int32_t fd;
//     JS_BUFFER buf;
//     int32_t size;

//     fd = duk_to_int32(ctx, 0);
//     buf.size = lseek(fd, 0, SEEK_END);
//     lseek(fd, 0, SEEK_SET);

//     duk_push_int(ctx,fd);
//     duk_push_fixed_buffer(ctx,size);
//     duk_push_int(ctx,size);

//     halFsRead(ctx);

// }

i32 halFsWrite(i32 fd, JS_BUFFER buf, i32 count) {
  if (count < 0) {
    return -1;
  }
  if (count > buf.size) {
    return -1;
  }
  return write(fd, buf.buf, count);
}

static duk_ret_t halFsListDir(duk_context *ctx)
{
  duk_idx_t arr_idx;
  const char *path;
  DIR *dir = NULL;
  struct dirent *ent;
  
  path = duk_to_string(ctx, 0);
  dir = opendir(path);

  arr_idx = duk_push_array(ctx);
  if (!dir) {
      printf("Error opening directory\n");
      return 1; // []
  }

  
  for (int i = 0;(ent = readdir(dir)) != NULL;i++) {
    duk_push_string(ctx, ent->d_name);
    duk_put_prop_index(ctx, arr_idx, i);
  }

  return 1;
}

/* File generated automatically by the gluecodegen. */

static duk_ret_t glueFsLseek(duk_context *ctx)
{
    int32_t ret;
    int32_t arg0;
    int32_t arg1;
    int32_t arg2;


    arg0 = duk_to_int32(ctx, 0);

    arg1 = duk_to_int32(ctx, 1);

    arg2 = duk_to_int32(ctx, 2);

    ret = lseek(arg0, arg1, arg2);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueFsExists(duk_context *ctx)
{
    int32_t ret;
    const char *arg0;


    arg0 = duk_to_string(ctx, 0);

    ret = halFsExists(arg0);
    duk_push_int(ctx, ret);
    return 1;
}



static duk_ret_t glueFsWrite(duk_context *ctx)
{
    int32_t ret;
    int32_t arg0;
    JS_BUFFER arg1;
    int32_t arg2;


    arg0 = duk_to_int32(ctx, 0);

    arg1.buf = (u8*) duk_get_buffer_data(ctx, 1, &arg1.size);

    arg2 = duk_to_int32(ctx, 2);

    ret = halFsWrite(arg0, arg1, arg2);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueFsMountSpiffs(duk_context *ctx)
{


    halFsMountSpiffs();
    return 0;
}

static duk_ret_t glueFsClose(duk_context *ctx)
{
    int32_t ret;
    int32_t arg0;


    arg0 = duk_to_int32(ctx, 0);

    ret = close(arg0);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueFsOpen(duk_context *ctx)
{
    int32_t ret;
    const char *arg0;
    int32_t arg1;


    arg0 = duk_to_string(ctx, 0);

    arg1 = duk_to_int32(ctx, 1);

    ret = open(arg0, arg1);
    duk_push_int(ctx, ret);
    return 1;
}

const duk_function_list_entry module_fs_funcs[] = {
    { "lseek", glueFsLseek , 3 },
    { "exists", glueFsExists , 1 },
    { "read", glueFsRead , 3 },
    { "write", glueFsWrite , 3 },
    { "mountSpiffs", glueFsMountSpiffs , 0 },
    { "close", glueFsClose , 1 },
    { "open", glueFsOpen , 2 },
	  { "listDir", halFsListDir , 1 },

    { NULL, NULL, 0 }
};

const duk_number_list_entry module_fs_consts[] = {
    { "SEEK_END", SEEK_END },
    { "O_CREAT", O_CREAT },
    { "SEEK_SET", SEEK_SET },
    { "O_APPEND", O_APPEND },
    { "SEEK_CUR", SEEK_CUR },
    { "O_RDONLY", O_RDONLY },
    { "O_WRONLY", O_WRONLY },
    { "O_RDWR", O_RDWR },
    { "O_TRUNC", O_TRUNC },

    { NULL, 0.0 }
};

void module_fs_init(duk_context *ctx)
{
    duk_push_object(ctx);

    duk_put_function_list(ctx, -1, module_fs_funcs);
    duk_put_number_list(ctx, -1, module_fs_consts);

	  duk_put_global_string(ctx, "fs");
}