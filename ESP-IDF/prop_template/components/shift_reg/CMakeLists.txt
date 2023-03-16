if(CONFIG_ENABLE_SHIFT)
    set(srcs shift_reg.c)
else()
    set(srcs shift_reg-dummy.c)
endif()

idf_component_register(SRCS "shift_reg.c" "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES puzzle twai_can gpio_prop)