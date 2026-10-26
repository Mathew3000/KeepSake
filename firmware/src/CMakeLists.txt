set(credentials_source "${CMAKE_CURRENT_BINARY_DIR}/credentials.c")
file(WRITE "${credentials_source}" "#include <stdint.h>\n")
foreach(credential server-ca.pem device.crt device.key)
    set(credential_path "${CMAKE_CURRENT_LIST_DIR}/certs/${credential}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${credential_path}")
    file(READ "${credential_path}" credential_hex HEX)
    string(REGEX REPLACE "(..)" "0x\\1," credential_bytes "${credential_hex}")
    string(REGEX REPLACE "[^a-zA-Z0-9]" "_" credential_symbol "${credential}")
    file(APPEND "${credentials_source}" "const uint8_t _binary_certs_${credential_symbol}_start[] = {${credential_bytes}0x00};\n")
endforeach()
idf_component_register(
    SRCS "backup_client.c" "usb_policy.c" "usb_detect.c" "${credentials_source}"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event esp_netif nvs_flash esp_http_client esp_driver_sdmmc esp_driver_gpio sdmmc fatfs mbedtls esp_tinyusb
)
