#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAXIMUM_RETRY 5
static int retry_num = 0;
static bool _persistent = true;

static EventGroupHandle_t wifi_event_group;

char STA_HOSTNAME[17] = "mcujs";

/**
 * ESP32 WiFi event handler.
 *
 * SYSTEM_EVENT_AP_PROBEREQRECVED
 * SYSTEM_EVENT_AP_STACONNECTED
 * SYSTEM_EVENT_AP_STADISCONNECTED
 * SYSTEM_EVENT_AP_START
 * SYSTEM_EVENT_AP_STOP
 * SYSTEM_EVENT_STA_AUTHMODE_CHANGE
 * SYSTEM_EVENT_STA_CONNECTED
 * SYSTEM_EVENT_STA_DISCONNECTED
 * SYSTEM_EVENT_STA_GOT_IP
 * SYSTEM_EVENT_STA_START
 * SYSTEM_EVENT_STA_STOP
 * SYSTEM_EVENT_SCAN_DONE
 * SYSTEM_EVENT_WIFI_READY
 */
static void wifiEventHandler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("got ip:%s",ip4addr_ntoa(&event->ip_info.ip));
        retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        if (retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            retry_num++;
            printf("retry to connect to the AP\n");
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
        printf("connect to the AP fail!\n");
    }

    // case SYSTEM_EVENT_AP_STACONNECTED:
    //     wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
    //     printf("station "+MAC2STR(event->mac)+" join, AID=%d\n",event->aid);
    //     break;

    // case SYSTEM_EVENT_AP_STADISCONNECTED:
    //     wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
    //     printf("station "+MAC2STR(event->mac)+" leave, AID=%d\n",event->aid);
    //     break;


}

// static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
// {
//     switch(event->event_id) {
//         case SYSTEM_EVENT_STA_START:
//             break;
//         case SYSTEM_EVENT_STA_GOT_IP:
//             printf("sta ip: %s\n",ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
//             retry_num = 0;
//             xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
//             break;
//         case SYSTEM_EVENT_STA_DISCONNECTED:
//             {
//                 xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
//                 if (retry_num < MAXIMUM_RETRY) {
//                     esp_wifi_connect();
//                     retry_num++;
//                     printf("retry to connect to the AP\n");
//                 } else {
//                     xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
//                 }
//                 printf("connect to the AP fail!\n");
//                 break;
//             }
//         case SYSTEM_EVENT_AP_STACONNECTED:
//             break;
//         case SYSTEM_EVENT_AP_STADISCONNECTED:
//             break;
//         default:
//             break;
//     }
//     return ESP_OK;
// }

/* init */

bool startNetworkEventTask(){
    if(!wifi_event_group){
        wifi_event_group = xEventGroupCreate();
        if(!wifi_event_group){
            printf("WiFi Event Group Create Failed!\n");
            return false;
        }
    }
    if (esp_event_loop_create_default() != ESP_OK){
        return false;
    }
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL);
    return true;
}

static void tcpipInit(){
    static bool initialized = false;
    if(!initialized && startNetworkEventTask()){
        initialized = true;
        tcpip_adapter_init();
    }
}

static bool lowLevelInitDone = false;
static bool wifiLowLevelInit(bool persistent){
    if(!lowLevelInitDone){
        tcpipInit();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_err_t err = esp_wifi_init(&cfg);
        if(err){
            printf("esp_wifi_init %d\n", err);
            return false;
        }
        if(!persistent){
          esp_wifi_set_storage(WIFI_STORAGE_RAM);
        }
        lowLevelInitDone = true;
    }
    return true;
}

static bool wifiLowLevelDeinit(){
    esp_wifi_deinit();
    lowLevelInitDone = false;
    return true;
}

static bool wifi_started = false;

int halWifiOn(bool persistent){
    if(wifi_started){
        return true;
    }
    if(!wifiLowLevelInit(persistent)){
        return false;
    }
    esp_err_t err = esp_wifi_start();
    if (err != ESP_OK) {
        printf("esp_wifi_start %d\n", err);
        wifiLowLevelDeinit();
        return false;
    }
    wifi_started = true;

    return true;
}

int halWifiOff(){
    if(!wifi_started){
        return true;
    }
    wifi_started = false;
    esp_err_t err = esp_wifi_stop();
    if(err){
        printf("Could not stop WiFi! %d\n", err);
        wifi_started = true;
        return false;
    }
    return wifiLowLevelDeinit();
}

/* mode */

/* 
 * Get current operating mode of WiFi. 
 * return WIFI_MODE_NULL, if WiFi is not initialized.
 */
static wifi_mode_t getMode(){
    if(!wifi_started){
        return WIFI_MODE_NULL;
    }
    wifi_mode_t mode;
    if(esp_wifi_get_mode(&mode) == ESP_ERR_WIFI_NOT_INIT){
        printf("WiFi not started\n");
        return WIFI_MODE_NULL;
    }
    return mode;
}

static bool setMode(wifi_mode_t mode){
    wifi_mode_t currentMode = getMode();
    if(currentMode == mode) {
        return true;
    }
    if(!currentMode && mode){
        if(!halWifiOn(_persistent)){
            return false;
        }
    }
    if(currentMode && !mode){
        return halWifiOff();
    }

    esp_err_t err = esp_wifi_set_mode(mode);
    if(err){
        printf("Could not set mode! %d\n", err);
        return false;
    }
    return true;
}

bool enableSTA(bool enable){

    wifi_mode_t currentMode = getMode();
    bool isEnabled = ((currentMode & WIFI_MODE_STA) != 0);

    if(isEnabled != enable) {
        if(enable) {
            return setMode((wifi_mode_t)(currentMode | WIFI_MODE_STA));
        }
        return setMode((wifi_mode_t)(currentMode & (~WIFI_MODE_STA)));
    }
    return true;
}

bool enableAP(bool enable){

    wifi_mode_t currentMode = getMode();
    bool isEnabled = ((currentMode & WIFI_MODE_AP) != 0);

    if(isEnabled != enable) {
        if(enable) {
            return setMode((wifi_mode_t)(currentMode | WIFI_MODE_AP));
        }
        return setMode((wifi_mode_t)(currentMode & (~WIFI_MODE_AP)));
    }
    return true;
}

/* STA */

static bool isConfigEqual(const wifi_config_t& lhs, const wifi_config_t& rhs)
{
    if(memcmp(&lhs, &rhs, sizeof(wifi_config_t)) != 0) {
        return false;
    }
    return true;
}

// 工作到这里了~
void halWifiStaConfig(const char* ssid, const char* pass, bool persistent){
    wifi_config_t conf;
    memset(&conf, 0, sizeof(wifi_config_t));
    strncpy((char*) conf.sta.ssid, ssid, sizeof(conf.sta.ssid));
    if (strcmp(pass,"undefined")) {
        strncpy((char*) conf.sta.password, pass, sizeof(conf.sta.password));
    }

    wifi_config_t current_conf;
    esp_wifi_get_config(ESP_IF_WIFI_STA, &current_conf);
    if(!isConfigEqual(current_conf, conf)) {
        esp_wifi_set_config(ESP_IF_WIFI_STA, &conf);
    }

    _persistent = persistent;
}

int halWifiStaBegin(const char* ssid, const char* pass)
{
    if (!enableSTA(true)){
        printf("STA enable failed!\n");
        return false;
    }
    tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, STA_HOSTNAME);

    esp_wifi_connect();

    return true;
}


