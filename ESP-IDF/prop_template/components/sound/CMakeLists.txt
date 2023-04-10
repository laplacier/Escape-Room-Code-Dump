if(CONFIG_ENABLE_SOUND)
    set(srcs sound.c)
else()
    set(srcs sound-dummy.c)
endif()

# We need font if either console or plot is enabled
if(CONFIG_ENABLE_LCD_CONSOLE OR CONFIG_ENABLE_LCD_PLOT)
endif()

idf_component_register(SRCS "sound.c" "sound-dummy.c" "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES driver)