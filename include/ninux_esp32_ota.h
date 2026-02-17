#ifndef NINUX_ESP32_OTA
#define NINUX_ESP32_OTA

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_partition.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_tls.h"
#include "string.h"
#include <inttypes.h>

#define BUFFSIZE 1024

// TAG e fw_url definiti UNA SOLA VOLTA in ninux_esp32_ota.c
// qui solo dichiarazione extern
extern const char *TAG;
extern char fw_url[512];

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[]   asm("_binary_ca_cert_pem_end");

extern EventGroupHandle_t wifi_event_group;
extern const int CONNECTED_BIT;

esp_err_t _http_event_handler_fw(esp_http_client_event_t *evt);
esp_err_t _http_event_handler(esp_http_client_event_t *evt);
void simple_ota_version_task(void *pvParameter);
void ninux_esp32_ota();

#endif

