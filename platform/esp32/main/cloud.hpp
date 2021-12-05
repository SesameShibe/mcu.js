#include "global.h"
#include "esp_websocket_client.h"
#include "mbedtls/sha256.h"

int idGenerateDeviceID(u8 deviceID[16]) {
  memset(deviceID, 0, 16);
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);

  u8 sha256Result[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  const char* seedStr = "NullTypeIDGenerate";
  mbedtls_sha256_update(&ctx, (const unsigned char*)seedStr, strlen(seedStr));
  mbedtls_sha256_update(&ctx, mac, 6);
  mbedtls_sha256_finish_ret(&ctx, sha256Result);
  mbedtls_sha256_free(&ctx);

  memcpy(deviceID, sha256Result, 16);
  return 0;
}

int idGenerateAuthResp(const u8 challenge[32], u8 response[32]) {
  memset(response, 0, 32);
  memcpy(response, challenge, 32);
  return 0;
}

void idConvertToHex(char* dst, u8* src, size_t srcLen) {
  const char* hex = "0123456789abcdef";
  for (size_t i = 0; i < srcLen; i++) {
    dst[i * 2] = hex[src[i] >> 4];
    dst[i * 2 + 1] = hex[src[i] & 0x0f];
  }
  dst[srcLen * 2] = '\0';
}

int idInit() {
  mjsPrintf("===========\nNanoID\n===========\n");
  mjsPrintf("NanoID type: Null\n");
  mjsPrintf(
      "Warning: This NanoID type is not secure and should not be used in "
      "production.\n");
  u8 deviceID[16];
  idGenerateDeviceID(deviceID);
  mjsPrintf("DeviceID: ");
  mjsPrintBufHex(deviceID, 16);

  mjsPrintf("\n===========\n");
  return 0;
}

typedef struct _MJS_CLOUD_CONTEXT {
  int valid;
  int handle;
  int state;
  esp_websocket_client_handle_t wsClient;

} MJS_CLOUD_CONTEXT;

MJS_CLOUD_CONTEXT mjsCloudContext;

void mjsCloudEventHandler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
  u8 packet[33] = {0};
  MJS_CLOUD_CONTEXT* ctx = (MJS_CLOUD_CONTEXT*)arg;
  esp_websocket_event_data_t* data = (esp_websocket_event_data_t*)event_data;

  MJS_QUEUE_MSG msg = {
      .objType = MJS_OBJ_TYPE_CLOUD,
      .handle = ctx->handle,
  };

  msg.args[0] = event_id;

  mjsPrintf("[cloud] event: %d, dataLen: %d\n", event_id, data->data_len);

  // Handle connection established event
  if (event_id == WEBSOCKET_EVENT_CONNECTED) {
    ctx->state = 1;
    mjsPrintf("[cloud] Connected to cloud, send device id\n");
    packet[0] = 0x01;
    idGenerateDeviceID(packet + 1);

    int ret = esp_websocket_client_send_bin(mjsCloudContext.wsClient,
                                            (char*)packet, sizeof(packet),
                                            60000 / portTICK_PERIOD_MS);
    if (ret <= 0) {
      mjsPrintf("[cloud] Failed to send device id: %d\n", ret);
    }
  } else if (event_id == WEBSOCKET_EVENT_DISCONNECTED) {
    ctx->state = 0;
    mjsPrintf("[cloud] Disconnected from cloud\n");
  } else if (event_id == WEBSOCKET_EVENT_DATA) {
    u8* dataBuf = (u8*)data->data_ptr;
    int dataLen = data->data_len;
    int opCode = data->op_code;
    msg.dataLen = dataLen;
    msg.args[1] = opCode;
    if (dataLen < sizeof(msg.data)) {
      memcpy(msg.data, dataBuf, dataLen);
    } else {
      msg.largeDataPtr = (u8*)mjsAlloc(dataLen);
      memcpy(msg.largeDataPtr, dataBuf, dataLen);
    }
    if ((dataLen >= 33) && (opCode == 2)) {
      if (dataBuf[0] == 0x02) {
        mjsPrintf("[cloud] Send auth response\n");
        packet[0] = 0x02;
        idGenerateAuthResp(dataBuf + 1, packet + 1);

        int ret = esp_websocket_client_send_bin(mjsCloudContext.wsClient,
                                                (char*)packet, sizeof(packet),
                                                60000 / portTICK_PERIOD_MS);
        if (ret <= 0) {
          mjsPrintf("[cloud] Failed to send auth response: %d\n", ret);
        }
      }
    }
  }
  if (!xQueueSend(mjsMsgQueue, &msg, 60000 / portTICK_PERIOD_MS)) {
    mjsPrintf("[cloud] queue send timeout\n");
    if (msg.largeDataPtr) {
      mjsFree(msg.largeDataPtr);
    }
  }
}

int halCloudSetup(const char* uri) {
  if (mjsCloudContext.valid) {
    mjsPrintf("[cloud] setup already called, ignore this call: %s\n", uri);
    return -1;
  }
  mjsCloudContext.valid = 1;
  mjsPrintf("[cloud] setup: %s\n", uri);
  mjsCloudContext.handle = mjsAllocHandle(MJS_OBJ_TYPE_CLOUD, &mjsCloudContext);

  char* uriCopy = strdup(uri);
  mjsPrintf("[cloud] uri: %s\n", uriCopy);

  esp_websocket_client_config_t cfg = {
      .uri = uriCopy,
  };

  mjsCloudContext.wsClient = esp_websocket_client_init(&cfg);
  esp_websocket_register_events(mjsCloudContext.wsClient, WEBSOCKET_EVENT_ANY,
                                mjsCloudEventHandler, &mjsCloudContext);
  esp_websocket_client_start(mjsCloudContext.wsClient);

  return mjsCloudContext.handle;
}

int halCloudSendBinary(int handle, JS_BUFFER buf) {
  return esp_websocket_client_send_bin(mjsCloudContext.wsClient, (char*)buf.buf,
                                       buf.size, 60000 / portTICK_PERIOD_MS);
}

int halCloudSendText(int handle, const char* str) {
  return esp_websocket_client_send_text(
      mjsCloudContext.wsClient, str, strlen(str), 60000 / portTICK_PERIOD_MS);
}

