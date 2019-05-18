
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "nvs.h"
#include "nvs_flash.h"


// da ota_task
#include "string.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#define BUFFSIZE 1024
static char ota_write_data[BUFFSIZE + 1] = { 0 };
static void http_cleanup(esp_http_client_handle_t client);
//=========

// per http req
#include "esp_tls.h"
//=====

static const char *TAG = "ninux_esp32_ota";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

extern EventGroupHandle_t wifi_event_group;
extern const int CONNECTED_BIT;

char fw_url[512];
static void https_get_url(char *url, char *response);
//static void https_get_url(char *url);

esp_err_t _http_event_handler(esp_http_client_event_t *evt);
void simple_ota_example_task(void * pvParameter);

void ninux_esp32_ota();
