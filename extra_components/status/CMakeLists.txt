idf_component_register( SRCS            "status.c"
                        INCLUDE_DIRS    "."
                        REQUIRES        nvs_flash
                                        esp_event
                                        freertos
                                        spi_flash
                                        app_update
                                        esp_http_client
                                        esp_https_ota
                                        efuse
                                        driver
                                        bt
                                        ledc
)