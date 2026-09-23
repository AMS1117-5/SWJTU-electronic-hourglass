cmake_minimum_required(VERSION 3.22)
project(hourglass_host_tests LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(ENABLE_SANITIZERS "Enable address and undefined-behavior checks" ON)
enable_testing()

set(FIRMWARE "${CMAKE_CURRENT_LIST_DIR}/..")

function(firmware_test name)
    add_executable(${name} ${name}.c ${ARGN})
    target_include_directories(${name} PRIVATE
        mocks "${FIRMWARE}/App" "${FIRMWARE}/Physics" "${FIRMWARE}/Drivers"
        "${FIRMWARE}/Config" "${FIRMWARE}/Core/Inc")
    target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    if(ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${name} PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES TIMEOUT 120)
endfunction()

firmware_test(mpu6050_test "${FIRMWARE}/Drivers/mpu6050.c")
firmware_test(motion_test "${FIRMWARE}/Physics/motion.c")
firmware_test(particles_test "${FIRMWARE}/Physics/particles.c")
firmware_test(ocean_test "${FIRMWARE}/App/ocean.c" "${FIRMWARE}/Physics/particles.c")
firmware_test(hourglass_test "${FIRMWARE}/App/hourglass.c" "${FIRMWARE}/Physics/particles.c")
firmware_test(buzzer_test "${FIRMWARE}/Drivers/buzzer.c")
firmware_test(buttons_test "${FIRMWARE}/Drivers/buttons.c")
firmware_test(max7219_test "${FIRMWARE}/Drivers/max7219.c")
firmware_test(display_test "${FIRMWARE}/Drivers/display.c")
firmware_test(settings_ui_test "${FIRMWARE}/App/settings.c" "${FIRMWARE}/App/menu.c"
    "${FIRMWARE}/App/ui.c" "${FIRMWARE}/Drivers/display.c")
firmware_test(app_test
    "${FIRMWARE}/App/app.c" "${FIRMWARE}/App/settings.c" "${FIRMWARE}/App/menu.c"
    "${FIRMWARE}/App/ui.c" "${FIRMWARE}/App/hourglass.c" "${FIRMWARE}/App/ocean.c"
    "${FIRMWARE}/Physics/particles.c" "${FIRMWARE}/Drivers/buttons.c"
    "${FIRMWARE}/Drivers/display.c" "${FIRMWARE}/Drivers/buzzer.c")
