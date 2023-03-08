if(CONFIG_ENABLE_SOUND)
    set(srcs shift_reg.c)
else()
    set(srcs shift_reg-dummy.c)
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES puzzle)