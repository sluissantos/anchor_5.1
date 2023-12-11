
idf_component_register( SRCS            "button_main.c"
                        INCLUDE_DIRS    "."
                        REQUIRES        driver
                                        nvs_flash
                                        bt
                                        hal
                                        freertos
                                        spi_flash
                                        status
                                        dwm1001
                                        hal
)                