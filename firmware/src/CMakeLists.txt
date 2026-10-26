idf_component_register(
    SRCS "backup_client.c" "usb_policy.c" "usb_detect.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event esp_netif nvs_flash esp_http_client esp_driver_sdmmc esp_driver_gpio sdmmc fatfs mbedtls esp_tinyusb
    EMBED_TXTFILES "${CMAKE_CURRENT_SOURCE_DIR}/certs/server-ca.pem" "${CMAKE_CURRENT_SOURCE_DIR}/certs/device.crt" "${CMAKE_CURRENT_SOURCE_DIR}/certs/device.key"
)
