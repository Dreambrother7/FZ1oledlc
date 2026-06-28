cmake_minimum_required(VERSION 3.13)

# Initialize the SDK
include(pico_sdk_import.cmake)

project(FZ1_display C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

function(configure_common target variant_label)
    target_link_libraries(${target}
        pico_stdlib
        hardware_spi
    )
    target_compile_definitions(${target}
        PRIVATE OLED_VARIANT_LABEL="${variant_label}"
    )
    pico_enable_stdio_usb(${target} 0)
    pico_enable_stdio_uart(${target} 0)
    pico_add_extra_outputs(${target})
endfunction()

function(configure_main_firmware target variant_label)
    add_executable(${target}
        main.cpp
        oled_ssd1309.cpp
    )
    configure_common(${target} "${variant_label}")
    target_link_libraries(${target}
        hardware_pio
        hardware_dma
    )
    pico_generate_pio_header(${target} ${CMAKE_CURRENT_LIST_DIR}/fz1_capture.pio)
endfunction()

function(configure_test_firmware target variant_label)
    add_executable(${target}
        oled_test.cpp
        oled_ssd1309.cpp
    )
    configure_common(${target} "${variant_label}")
endfunction()

configure_main_firmware(FZ1_display_original "ORIGINAL")
configure_test_firmware(FZ1_oled_test_original "ORIGINAL")

configure_main_firmware(FZ1_display_deftaudio_pcb "DEFTAUDIO PCB")
configure_test_firmware(FZ1_oled_test_deftaudio_pcb "DEFTAUDIO PCB")

target_compile_definitions(FZ1_display_deftaudio_pcb
    PRIVATE
        OLED_PIN_DC=21
        OLED_PIN_RES=20
        OLED_ROTATE_180=1
)
target_compile_definitions(FZ1_oled_test_deftaudio_pcb
    PRIVATE
        OLED_PIN_DC=21
        OLED_PIN_RES=20
        OLED_ROTATE_180=1
)
