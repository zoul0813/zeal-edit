add_library(windows STATIC
    vram.c
    window.c
    window_active.c
    window_columns.c
    window_gotox.c
    window_gotoy.c
    window_gotoxy.c
    window_clrscr.c
    window_clreol.c
    window_save.c
    window_wherex.c
    window_wherey.c
    window_putc.c
    window_putc_color.c
    window_puts.c
    window_puts_color.c
    banners.c
)

target_include_directories(windows PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(windows PRIVATE $ENV{ZVB_SDK_PATH}/include)
target_link_libraries(windows PRIVATE common)