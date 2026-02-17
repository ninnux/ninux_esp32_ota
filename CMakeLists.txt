idf_component_register(
    SRCS
        "ninux_esp32_ota.c"
    INCLUDE_DIRS
        "."
        "include"
    REQUIRES
        esp_http_client
        app_update
        nvs_flash
        esp_event
        freertos
        log
        esp_wifi
        esp_https_ota
        esp-tls
)

