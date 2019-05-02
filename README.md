# ninux_esp32_ota component

## how to use it
example:
```

#include "ninux_esp32_ota.h"
...   
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
...
tcpip_adapter_init();
initialise_wifi();
...
ninux_esp32_ota();
...
```
## procedure
Generate self-signed certificate and key:

*NOTE: `Common Name` of server certificate should be host-name of your server.*

```
openssl req -x509 -newkey rsa:2048 -keyout ca_key.pem -out ca_cert.pem -days 365

```

* openssl configuration may require you to enter a passphrase for the key.
* When prompted for the `Common Name (CN)`, enter the name of the server that the ESP32 will connect to. For this local example, it is probably the IP address. The HTTPS client will make sure that the `CN` matches the address given in the HTTPS URL (see Step 3).


Copy the certificate to `server_certs` directory inside OTA example directory:

```
cp ca_cert.pem /path/to/ota/example/server_certs/
```


Start the HTTPS server:

```
openssl s_server -WWW -key ca_key.pem -cert ca_cert.pem -port 8070
```
