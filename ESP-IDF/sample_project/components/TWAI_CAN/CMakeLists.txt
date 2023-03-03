if(CONFIG_ENABLE_LCD_OUTPUT)
else()
endif()

# We need font if either console or plot is enabled
if(CONFIG_ENABLE_LCD_CONSOLE OR CONFIG_ENABLE_LCD_PLOT)
endif()

idf_component_register(SRCS "prop_can.c"
                    ...)