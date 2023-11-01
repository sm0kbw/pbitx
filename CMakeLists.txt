cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
project(pbitx_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-Wall -Wextra)

add_executable(pbitx)
 target_sources(pbitx PRIVATE
  src/e_storage.c
  src/encoder.c
  src/KBW_keyer.c
  src/setup.c
  src/dispatch.c
  src/num_conv.c
  src/ubitx_si5351.c
  src/ubitx_ui.c
  src/gui_driver.c
  src/pbitx.c
  src/fonts.c
  src/touch.c
)



pico_add_extra_outputs(pbitx)
target_link_libraries(pbitx PRIVATE pico_stdlib hardware_flash hardware_spi hardware_i2c hardware_adc hardware_pwm pico_multicore pico_unique_id) 


pico_enable_stdio_usb(pbitx 1)
pico_enable_stdio_uart(pbitx 0)

pico_add_extra_outputs(pbitx)
