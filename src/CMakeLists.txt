file(GLOB FILES main.c **/*.cpp)

# Add executable. Default name is the project name, version 0.1
add_executable(main ${FILES})

pico_set_program_name(main "pico-temp-logger")
pico_set_program_version(main "0.1")

pico_enable_stdio_uart(main 1)
pico_enable_stdio_usb(main 0)

target_include_directories(main PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

# Add the standard library to the build
target_link_libraries(main pico_stdlib)

# Add any user requested libraries
target_link_libraries(main
        hardware_spi
        hardware_i2c
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(main)
