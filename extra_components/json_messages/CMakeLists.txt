
idf_component_register( SRCS            "json_messages.c"
                        INCLUDE_DIRS    "."
                        REQUIRES        nvs_flash
                                        esp_event
                                        freertos
                                        spi_flash
                                        esp_netif
                                        esp_wifi
                                        driver
                                        #mqtt
                                        dwm1001
                                        json
                                        ota
)                   