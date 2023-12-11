idf_component_register( SRCS            "sntp_main.c"
                        INCLUDE_DIRS    "."
                        REQUIRES        driver
                                        nvs_flash
                                        spi_flash
                                        freertos
                                        esp_event
                                        esp_netif
                                        esp_wifi
                                        lwip
                                        config
)