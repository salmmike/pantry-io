idf_component_register(SRCS "bluetooth.c"
                    INCLUDE_DIRS "include"
                    INCLUDE_DIRS "../nvs_init/include"
                    REQUIRES esp_timer
                    REQUIRES bt
                    REQUIRES mbedtls)

component_compile_options(-Wno-error=format= -Wno-format)
