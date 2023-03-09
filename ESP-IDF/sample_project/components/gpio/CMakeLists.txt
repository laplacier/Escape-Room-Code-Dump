if(CONFIG_ENABLE_GPIO)
    set(srcs gpio.c)
else()
    set(srcs gpio-dummy.c)
endif()

idf_component_register(SRCS "gpio.c" "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES puzzle shift_reg sound twai_can)