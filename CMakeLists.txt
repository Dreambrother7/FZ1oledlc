cmake_minimum_required(VERSION 3.13)

# Initialize the SDK
include(pico_sdk_import.cmake)

project(FZ1_display C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

add_library(oled_ssd1309
    oled_ssd1309.cpp
)

target_link_libraries(oled_ssd1309
    pico_stdlib
    hardware_spi
)

# Main FZ-1 display firmware
add_executable(FZ1_display
    main.cpp
)

# Generate PIO header
pico_generate_pio_header(FZ1_display ${CMAKE_CURRENT_LIST_DIR}/fz1_capture.pio)

# Link hardware libraries
target_link_libraries(FZ1_display
    pico_stdlib
    hardware_pio
    hardware_dma
    oled_ssd1309
)

# Disable stdio for the release/test firmware
pico_enable_stdio_usb(FZ1_display 0)
pico_enable_stdio_uart(FZ1_display 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(FZ1_display)

# Standalone OLED wiring/display test firmware
add_executable(FZ1_oled_test
    oled_test.cpp
)

target_link_libraries(FZ1_oled_test
    pico_stdlib
    oled_ssd1309
)

pico_enable_stdio_usb(FZ1_oled_test 0)
pico_enable_stdio_uart(FZ1_oled_test 0)

pico_add_extra_outputs(FZ1_oled_test)
