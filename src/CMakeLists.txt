file(GLOB FILES *.cpp *.hpp)

# Add executable. Default name is the project name, version 0.1
add_executable(pico-temp-logger ${FILES})

pico_set_program_name(pico-temp-logger "pico-temp-logger")
pico_set_program_version(pico-temp-logger "0.1")

pico_enable_stdio_uart(pico-temp-logger 1)
pico_enable_stdio_usb(pico-temp-logger 0)

target_include_directories(pico-temp-logger PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

# Add the standard library to the build
target_link_libraries(pico-temp-logger pico_stdlib)

# Add any user requested libraries
target_link_libraries(pico-temp-logger
        hardware_spi
        hardware_i2c
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(pico-temp-logger)
