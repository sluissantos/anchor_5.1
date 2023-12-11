
idf_component_register( SRCS            "config.c"
                        INCLUDE_DIRS    "."
                        REQUIRES        driver
                                        esp_common
                                        nvs_flash
                                        freertos
                                        spi_flash
                                        spiffs
                                        app_update
                                        esp_hw_support
                                        blufi
                                        bt
                                        #ota
                                        log
                                        esp_wifi
                                        status
                                        app_update
)                           