add_library(lcd_actor STATIC
    lcd_actor.cpp
    pfnfont.cpp
    ssd1306.c
)

# .incbin "shnmk14u.pfn"
target_include_directories(lcd_actor PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# 依存するライブラリをリンク
target_link_libraries(lcd_actor PUBLIC
    tinykernel_lib
    hardware_i2c
    pico_stdlib
)
