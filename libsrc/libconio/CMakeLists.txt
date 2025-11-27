add_library(conio STATIC
    conio.c
)

target_include_directories(conio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(conio PRIVATE $ENV{ZVB_SDK_PATH}/include)
# target_link_libraries(conio PRIVATE common)