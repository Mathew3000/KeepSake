idf_component_register(SRCS "backup_client.c" "usb_policy.c" "usb_detect.c" INCLUDE_DIRS "."
  REQUIRES esp_wifi esp_event esp_netif nvs_flash esp_http_client esp_driver_sdmmc esp_driver_gpio sdmmc fatfs esp_tinyusb mbedtls
  EMBED_TXTFILES "certs/server-ca.pem" "certs/device.crt" "certs/device.key")
