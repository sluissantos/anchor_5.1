idf_component_register(SRCS             "blufi_example_main.c"
                                        "blufi_security.c"
                                        "blufi_init.c"
                    INCLUDE_DIRS        "."
                    REQUIRES            blufi
                                        bt
                                        driver
                                        nvs_flash
                                        freertos
                                        spi_flash
                                        status
                                        config
                                        esp_wifi
                                        esp_netif
                                        mbedtls
                                        esp_hw_support
                                        bootloader_support
                                        log
                                        lwip
                                        storage
                    
                        )
