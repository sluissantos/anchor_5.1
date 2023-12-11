
idf_component_register( SRCS            "mqtt_main.c"
                        INCLUDE_DIRS    "."
                        REQUIRES        nvs_flash
                                        esp_event
                                        freertos
                                        spi_flash
                                        esp_netif
                                        esp_wifi
                                        driver
                                        mqtt
                                        status
                                        ledc
                                        ota
                                        json_messages
                                        esp_system
)                   