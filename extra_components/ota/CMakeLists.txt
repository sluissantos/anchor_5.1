
idf_component_register( SRCS            "https_ota.c"
                        INCLUDE_DIRS    "."
                        REQUIRES        nvs_flash
                                        esp_event
                                        freertos
                                        spi_flash
                                        esp_netif
                                        esp_wifi
                                        driver
                                        mqtt
                                        app_update
                                        esp_http_client
                                        esp_https_ota
                                        status
                                        esp_system
                                        esp_hw_support
                                        config
)                   