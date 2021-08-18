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

/**
 * ESP32 WiFi event handler.
 */
static void wifiEventHandler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    /*
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        if (retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            retry_num++;
            printf("retry to connect to the AP\n");
        } else if (retry_num == MAXIMUM_RETRY) {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            printf("connect to the AP fail!\n");
        } else {
            printf("disconnected!\n");
        }
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        //pass
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        //pass
    }*/
}

bool hasStatus(i32 statusBit){
    if (!wifi_event_group){
        return false;
    }
    return (xEventGroupClearBits(wifi_event_group, 0)&statusBit) != 0;
}

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
            //printf("esp_wifi_init %d\n", err);
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

static bool isConfigEqual(const wifi_config_t *lhs, const wifi_config_t *rhs)
{
    if(memcmp(lhs, rhs, sizeof(wifi_config_t)) != 0) {
        return false;
    }
    return true;
}

int halWifiStaIsConnected(){
    return hasStatus(WIFI_CONNECTED_BIT);
}

int halWifiStaBegin()
{
    if (!enableSTA(true)){
        //printf("STA enable failed!\n");
        return false;
    }
    retry_num = 0; //reset
    esp_wifi_connect();

    return true;
}

void halWifiStaConfig(const char* ssid, const char* pass, bool persistent){
    wifi_config_t conf;
    memset(&conf, 0, sizeof(wifi_config_t));
    strncpy((char*) conf.sta.ssid, ssid, sizeof(conf.sta.ssid)-1);
    if (strcmp(pass,"undefined")) {
        strncpy((char*) conf.sta.password, pass, sizeof(conf.sta.password)-1);
    }

    wifi_config_t current_conf;
    esp_wifi_get_config(WIFI_IF_STA, &current_conf);
    if(!isConfigEqual(&current_conf, &conf)) {
        esp_wifi_set_config(WIFI_IF_STA, &conf);
    }

    _persistent = persistent;

    //reconnect
    if(halWifiStaIsConnected()){
        esp_wifi_disconnect();
    }else{
        halWifiStaBegin();
    }
}

int halWifiStaEnd(){
    if(getMode() & WIFI_MODE_STA){
        retry_num = 6; //disable reconnect
        if(esp_wifi_disconnect()){
            retry_num = 0;
            printf("disconnect failed!\n");
            return false;
        }

        return enableSTA(false);
    }
    return false;
}

const char * halWifiStaIp()
{
    if(!halWifiStaIsConnected()){
        return "";
    }
    tcpip_adapter_ip_info_t ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
    return ip4addr_ntoa(&ip.ip);
}

bool halWifiStaSetHostname(const char * hostname)
{
    if(getMode() == WIFI_MODE_NULL){
        return false;
    }
    return tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname) == 0;
}

/* AP */

// Default: auth=open max=5
void halWifiApConfig(const char* ssid, const char* pass, uint32_t auth,
                                            uint32_t max, bool persistent){
                                    
    wifi_auth_mode_t auth_mode = (wifi_auth_mode_t)auth;
    wifi_config_t conf;
    memset(&conf, 0, sizeof(wifi_config_t));
    strncpy((char*) conf.ap.ssid, ssid, sizeof(conf.ap.ssid)-1);
    if (auth)
    {
        strncpy((char*) conf.ap.password, pass, sizeof(conf.ap.password)-1);
    }
    conf.ap.authmode = auth_mode;
    if (max)
    {
        conf.ap.max_connection = max;
    }else{
        conf.ap.max_connection = 5;
    }

    wifi_config_t current_conf;
    esp_wifi_get_config(WIFI_IF_AP, &current_conf);
    if(!isConfigEqual(&current_conf, &conf)) {
        esp_wifi_set_config(WIFI_IF_AP, &conf);
    }

    _persistent = persistent;
}

int halWifiApBegin(){
    if (!enableAP(true)){
        printf("AP enable failed!\n");
        return false;
    }
    return true;
}

int halWifiApEnd(){
    return enableAP(false);
}

