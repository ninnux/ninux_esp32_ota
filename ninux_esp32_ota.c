
#include "ninux_esp32_ota.h"
esp_err_t _http_event_handler_fw(esp_http_client_event_t *evt)
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
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, pirulupiruli len=%d", evt->data_len);
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
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                //printf("%.*s", evt->data_len, (char*)evt->data);
		int init_size = evt->data_len;
		char delim[] = ",";
		char update[] = "0";
		int updateint = 0;
		char *url = malloc(evt->data_len-2);
		bzero(url,evt->data_len-2);

		char *ptr = strtok((char*)evt->data, delim);

		strncpy(update,ptr,strlen(ptr));
		ptr = strtok(NULL, delim);
		sprintf(url,"%s",ptr);
		ptr = strtok(NULL, delim);
		updateint=atoi(update);	
		bzero(fw_url,strlen(fw_url));
		if(updateint==1){
			printf("%s\n",url);
			sprintf(fw_url,"%s",url);
		}
            }
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


static void https_with_url(char* fw_ver_url)
{
    esp_http_client_config_t config = {
        .url = fw_ver_url,
        //.url = "https://iotfw.ninux.org/pippo/pluto",
        .event_handler = _http_event_handler,
        .cert_pem = (char *)server_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}


void simple_ota_version_task(void * pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA version check");
    char fw_ver_url[128];
    bzero(fw_ver_url,sizeof(fw_ver_url));
    const esp_partition_t *running = esp_ota_get_running_partition();

    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to WiFi network! Attempting to connect to server...");

    /* ASK FOR FIRMWARE URL */
    //strncpy(fw_url,https_get_url("https://iotfw.ninux.org/firwmare_check"),4096);
    //char fw_url[512]; 
    //https_get_url("https://iotfw.ninux.org/firwmare_check",fw_url);
    //https_get_url("https://10.162.0.77/firwmare_check",fw_url);
    //ESP_LOGI(TAG, "To be downloaded:%s\n",fw_url);
    //https_get_url("https://iotfw.ninux.org/firwmare_check");
    //https_get_url("https://10.162.0.77/firwmare_check");
    /* version check */
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }
    sprintf(fw_ver_url,"%s/%s/%s",CONFIG_FIRMWARE_UPGRADE_HOST,running_app_info.project_name,running_app_info.version);
    https_with_url(fw_ver_url);
    if(strlen(fw_url)!=0){ 
      esp_http_client_config_t config = {
          //.url = CONFIG_FIRMWARE_UPGRADE_URL,
          .url = fw_url,
          .cert_pem = (char *)server_cert_pem_start,
          .event_handler = _http_event_handler_fw,
      };
      esp_err_t ret = esp_https_ota(&config);
      if (ret == ESP_OK) {
          esp_restart();
      } else {
          ESP_LOGE(TAG, "Firmware upgrade failed");
      }
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
