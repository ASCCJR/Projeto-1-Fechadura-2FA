# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Projeto1Fechadura2FA C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Projeto1Fechadura2FA 
        main.c
        display.c
        ssd1306_i2c.c
        mqtt_lwip.c
        matriz.c
        keypad.c
        tcs34725.c 
        rgb_led.c
        servo.c
        buzzer.c
        feedback.c
        )

# Linha que gera o header do PIO
pico_generate_pio_header(Projeto1Fechadura2FA ${CMAKE_CURRENT_SOURCE_DIR}/ws2812.pio)

pico_set_program_name(Projeto1Fechadura2FA "Projeto1Fechadura2FA")
pico_set_program_version(Projeto1Fechadura2FA "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Projeto1Fechadura2FA 0)
pico_enable_stdio_usb(Projeto1Fechadura2FA 1)

# Add the standard library to the build
target_link_libraries(Projeto1Fechadura2FA
        pico_stdlib
        pico_multicore
        pico_sync
        hardware_pwm
        pico_cyw43_arch_lwip_threadsafe_background
        hardware_i2c
        pico_lwip_mqtt
        hardware_adc
        )

# Add the standard include files to the build
target_include_directories(Projeto1Fechadura2FA PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries

#target_link_libraries(Projeto1Fechadura2FA)

# create map/bin/hex file etc.
pico_add_extra_outputs(Projeto1Fechadura2FA)