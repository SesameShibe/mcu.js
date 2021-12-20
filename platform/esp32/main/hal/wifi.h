#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/sys.h"

#define STA 0
#define AP 1
// #define AP_STA 2

static char wifiStaIp[32];
esp_netif_t* wifi_sta_netif = nullptr;
esp_netif_t* wifi_ap_netif = nullptr;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		ESP_LOGI(TAG, "connect to the AP fail");
		wifiStaIp[0] = 0;
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		snprintf(wifiStaIp, sizeof(wifiStaIp) - 1, IPSTR, IP2STR(&event->ip_info.ip));
	} else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
		ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
	} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
		ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
	}
}

void halWifiStaBegin(const char* ssid, const char* pwd, bool save) {
	wifiStaIp[0] = 0;
	wifi_config_t wifi_config;
	memset(&wifi_config, 0, sizeof(wifi_config));
	strncpy((char*) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
	strncpy((char*) wifi_config.sta.password, pwd, sizeof(wifi_config.sta.password));
	wifi_config.sta.pmf_cfg.capable = true;
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_connect());
}

void halWifiApBegin(const char* ssid, const char* pwd, bool save) {
  if (strlen(pwd) < 8)
  {
    ESP_LOGE(TAG, "Password must be at least 8 characters long");
    return;
  }
  
	wifi_config_t wifi_config;
	memset(&wifi_config, 0, sizeof(wifi_config));
	strncpy((char*) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
	strncpy((char*) wifi_config.ap.password, pwd, sizeof(wifi_config.ap.password));
	wifi_config.ap.ssid_len = strlen(ssid);
	wifi_config.ap.max_connection = 4;
	wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
	wifi_config.ap.ssid_hidden = 0;
	wifi_config.ap.beacon_interval = 100;
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

const char* halWifiStaGetIP() {
	return wifiStaIp;
}

// sta:0, ap:1
void halWifiInit(u8 mode) {
	switch (mode) {
	case STA:
		if (wifi_sta_netif == nullptr) {
			wifi_sta_netif = esp_netif_create_default_wifi_sta();
		} else {
			ESP_LOGW(TAG, "wifi sta already created");
      return;
		}
		break;
	case AP:
		if (wifi_ap_netif == nullptr) {
			wifi_ap_netif = esp_netif_create_default_wifi_ap();
		} else {
			ESP_LOGW(TAG, "wifi ap already created");
			return;
		}
		break;
	// case AP_STA:
	// 	if (wifi_sta_netif == nullptr && wifi_ap_netif == nullptr) {
	// 		wifi_sta_netif = esp_netif_create_default_wifi_sta();
	// 		wifi_ap_netif = esp_netif_create_default_wifi_ap();
	// 	} else {
	// 		ESP_LOGW(TAG, "wifi ap or sta already created");
	// 		return;
	// 	}
	// 	break;
	default:
		break;
	}

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(
	    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
	ESP_ERROR_CHECK(
	    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));
}

void halWifiDeinit() {
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_netif_destroy_default_wifi(wifi_sta_netif);
  wifi_sta_netif = nullptr;
  esp_netif_destroy_default_wifi(wifi_ap_netif);
  wifi_ap_netif = nullptr;
}