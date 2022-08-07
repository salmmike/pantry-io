idf_component_register(SRCS "button-states.c"
                    INCLUDE_DIRS "include"
                    INCLUDE_DIRS "../http/include"
                    REQUIRES bluetooth
                    REQUIRES driver
                    REQUIRES esp_timer)