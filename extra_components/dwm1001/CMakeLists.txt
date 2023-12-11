
idf_component_register( SRCS            "dwm_api.c" "dwm1001_main.c" "dwm1001_fulltest.c" "hal_gpio.c" 
                                        "hal_spi.c" "hal.c" "lmh_spirx_drdy.c" "lmh_spirx.c" "lmh.c"
                        INCLUDE_DIRS    "."
                        REQUIRES        nvs_flash
                                        esp_event
                                        freertos
                                        spi_flash
                                        esp_netif
                                        esp_wifi
                                        driver
                                        mqtts
                                        json_messages
                                        button
                                        config
)                   