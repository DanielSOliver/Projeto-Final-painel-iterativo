# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================




cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico CACHE STRING "Board type")

include(pico_sdk_import.cmake)
project(Painel_iterativo C CXX ASM)
pico_sdk_init()
add_executable(Painel_iterativo Painel_iterativo.c lib/ssd1306.c)
pico_set_program_name(Painel_iterativo "Painel_iterativo")
pico_set_program_version(Painel_iterativo "0.1")
pico_enable_stdio_uart(Painel_iterativo 1)
pico_enable_stdio_usb(Painel_iterativo 1)


file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(Painel_iterativo ${CMAKE_CURRENT_LIST_DIR}/animacoes_led.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)
target_sources(Painel_iterativo PRIVATE Painel_iterativo.c)


target_link_libraries(Painel_iterativo 
                      pico_stdlib
                      hardware_i2c 
                      hardware_adc 
                      hardware_pwm
                      hardware_pio
                      pico_bootrom
                      )
target_include_directories(Painel_iterativo PRIVATE   ${CMAKE_CURRENT_LIST_DIR})
pico_add_extra_outputs(Painel_iterativo)

