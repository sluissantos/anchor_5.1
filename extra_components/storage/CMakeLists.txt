idf_component_register( SRCS            "storage_main.c"
                        INCLUDE_DIRS    "."
                        REQUIRES        nvs_flash
                                        spi_flash
                                        esp_event
)