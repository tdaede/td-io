cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(td-io)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(third_party/pico_i2c_slave/i2c_slave)

# rest of your project

add_executable(td-io
        td-io.c edid.c
        )

# Pull in our pico_stdlib which aggregates commonly used features
target_link_libraries(td-io pico_stdlib hardware_adc i2c_slave)

# enable usb output, disable uart output
pico_enable_stdio_usb(td-io 1)
pico_enable_stdio_uart(td-io 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(td-io)
