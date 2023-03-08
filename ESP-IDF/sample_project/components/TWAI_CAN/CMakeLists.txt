if(CONFIG_ENABLE_CAN)
    if(CONFIG_CAN_OPERATION_FULL)
        set(srcs twai_can.c)
    else()
        set(srcs twai_can_half.c)
    endif()
else()
    set(srcs twai_can-dummy.c)
endif()

# We need font if either console or plot is enabled
if(CONFIG_ENABLE_LCD_CONSOLE OR CONFIG_ENABLE_LCD_PLOT)
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES driver puzzle)