cmake_minimum_required(VERSION 3.25.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(pico_sdk_import.cmake)

project(pomopico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(pomopico
    pomopico.c
    display.c
    clock.c
)

pico_enable_stdio_usb(pomopico 1)

pico_add_extra_outputs(pomopico)

target_link_libraries(pomopico 
    pico_stdlib
    hardware_gpio
    hardware_spi
    hardware_i2c
)