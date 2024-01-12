# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(radio C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# turn on all compiler warnings 
add_compile_options(-Wall)

# Tell CMake where to find the executable source file #0
add_executable(${PROJECT_NAME} 
  main/radio/main.cpp
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Add libraries #1
add_library(pico_ch1115 INTERFACE )
add_library(pico_pushbutton INTERFACE)
add_library(pico_ahtxx INTERFACE)
add_library(pico_bitmapdata INTERFACE)
add_library(pico_tea5767 INTERFACE)

# Add target sources #2
target_sources(pico_ch1115 INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/ch1115/ER_OLEDM1_CH1115.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ch1115/ER_OLEDM1_CH1115_graphics.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ch1115/ER_OLEDM1_CH1115_Print.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/ch1115/ER_OLEDM1_CH1115_font.cpp)

target_sources(pico_ahtxx INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src/ahtxx/ahtxx.cpp)
target_sources(pico_pushbutton INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src/pushbutton/push_button.cpp)
target_sources(pico_bitmapdata INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src/bitmapdata/bitmap_data.cpp)
target_sources(pico_tea5767 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src/tea5767/tea5767.cpp)

# Add Target include directories  #3
target_include_directories(
pico_ch1115 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include
pico_ahtxx INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include
pico_pushbutton INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include
pico_bitmapdata INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include
pico_tea5767 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include
)

# Pull in pico libraries that we need #4
target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_i2c hardware_spi hardware_adc 
pico_ch1115 pico_ahtxx pico_pushbutton pico_bitmapdata pico_tea5767)


# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

