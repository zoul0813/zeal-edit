add_library(conio STATIC
    highvideo.c
    lowvideo.c
    clrscr.c
    clreol.c
    clr_color.c
    kbhit.c
    gotox.c
    gotoy.c
    gotoxy.c
    wherex.c
    wherey.c
    cputc.c
    cputcxy.c
    cputs.c
    cputsxy.c
    cgetc.c
    cursor.c
    setcursortype.c
    setcursormode.c
    textcolor.c
    bgcolor.c
    chline.c
    chlinexy.c
    cvline.c
    cvlinexy.c
    cclear.c
    cclearxy.c
    screensize.c
)

target_include_directories(conio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(conio PRIVATE $ENV{ZVB_SDK_PATH}/include)
target_link_libraries(conio PRIVATE keyboard)