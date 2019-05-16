
#include "ninux_esp32_ota.h"

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}



static void https_get_url(char *url, char *response)
{
    char buf[512];
    int ret, len;

    esp_tls_cfg_t cfg = {
        .cacert_pem_buf  = server_cert_pem_start,
        .cacert_pem_bytes = server_cert_pem_end - server_cert_pem_start,
    };
    
    ESP_LOGI(TAG, "pippo1");
    struct esp_tls *tls = esp_tls_conn_http_new(url, &cfg);
    
    ESP_LOGI(TAG, "pippo2");
    if(tls != NULL) {
        ESP_LOGI(TAG, "Connection established...");
    } else {
        ESP_LOGE(TAG, "Connection failed...");
        goto exit;
    }
    
    ESP_LOGI(TAG, "pippo3");
    size_t written_bytes = 0;
    char request[1024];
    ESP_LOGI(TAG, "pippo4");
    sprintf(request,"GET %s HTTP/1.0\r\n"
         "Host: iotfw.ninux.org\r\n"
         "User-Agent: esp-idf/1.0 esp32\r\n"
         "\r\n",url);
    ESP_LOGI(TAG, "pippo5");
    do {
        ret = esp_tls_conn_write(tls, request + written_bytes, strlen(request) - written_bytes);
        if (ret >= 0) {
            ESP_LOGI(TAG, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != MBEDTLS_ERR_SSL_WANT_READ  && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAG, "esp_tls_conn_write  returned 0x%x", ret);
            goto exit;
        }
    } while(written_bytes < strlen(request));

    ESP_LOGI(TAG, "Reading HTTP response...");

    do
    {
        len = sizeof(buf) - 1;
        bzero(buf, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);
        
        if(ret == MBEDTLS_ERR_SSL_WANT_WRITE  || ret == MBEDTLS_ERR_SSL_WANT_READ)
            continue;
        
        if(ret < 0)
       {
            ESP_LOGE(TAG, "esp_tls_conn_read  returned -0x%x", -ret);
            break;
        }

        if(ret == 0)
        {
            ESP_LOGI(TAG, "connection closed");
            break;
        }

        len = ret;
        ESP_LOGD(TAG, "%d bytes read", len);
        /* Print response directly to stdout as it is read */
        for(int i = 0; i < len; i++) {
            putchar(buf[i]);
	    sprintf((response+i),"%c",buf[i]);
        }
    } while(1);

    exit:
        esp_tls_conn_delete(tls);    
        putchar('\n'); // JSON output doesn't have a newline at end
	
        static int request_count;
        ESP_LOGI(TAG, "Completed %d requests", ++request_count);
}


void simple_ota_example_task(void * pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA example");

    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to WiFi network! Attempting to connect to server...");


    /* ASK FOR FIRMWARE URL */
    
    esp_http_client_config_t config = {
        .url = CONFIG_FIRMWARE_UPGRADE_URL,
        .cert_pem = (char *)server_cert_pem_start,
        .event_handler = _http_event_handler,
    };
    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void simple_ota_version_task(void * pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA version check");

    const esp_partition_t *running = esp_ota_get_running_partition();

    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to WiFi network! Attempting to connect to server...");

    /* ASK FOR FIRMWARE URL */
    //strncpy(fw_url,https_get_url("https://iotfw.ninux.org/firwmare_check"),4096);
    char fw_url[512]; 
    https_get_url("https://iotfw.ninux.org/firwmare_check",fw_url);
    //https_get_url("https://10.162.0.77/firwmare_check",fw_url);
    ESP_LOGI(TAG, "To be downloaded:%s\n",fw_url);
    //https_get_url("https://iotfw.ninux.org/firwmare_check");
    //https_get_url("https://10.162.0.77/firwmare_check");

    /* version check */
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }
    
    esp_http_client_config_t config = {
        .url = CONFIG_FIRMWARE_UPGRADE_URL,
        .cert_pem = (char *)server_cert_pem_start,
        .event_handler = _http_event_handler,
    };
    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void ninux_esp32_ota()
{
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    //xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
    xTaskCreate(&simple_ota_version_task, "ota_version_task", 8192, NULL, 5, NULL);
}
