idf_component_register(SRCS "main.c" INCLUDE_DIRS "."
  REQUIRES esp_wifi esp_event esp_netif nvs_flash esp_http_client esp_driver_sdmmc sdmmc fatfs mbedtls json
  EMBED_TXTFILES "certs/server-ca.pem" "certs/device.crt" "certs/device.key")
