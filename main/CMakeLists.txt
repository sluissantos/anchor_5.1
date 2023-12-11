idf_component_register( SRCS            "main.c"
                        INCLUDE_DIRS    "."
                        REQUIRES        driver
                                        nvs_flash
                                        spi_flash
                                        freertos
                                        esp_event
                                        esp_netif
                                        esp_wifi
                                        esp_hw_support
                                        console
                                        bt
                                        blufi
                                        #mqtts
                                        #sntp
                                        status
                                        ledc
                                        config
                                        storage
                                        ota
                                        button
                                        dwm1001
                                        hal
)
