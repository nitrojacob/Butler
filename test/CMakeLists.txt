cmake_minimum_required(VERSION 3.16)

project(esp8266_unit_tests)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/unity/src
    ${CMAKE_CURRENT_SOURCE_DIR}/cmock/src
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../main
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/nvs_flash/include
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/esp_system/include
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/esp_log/include
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/esp8266/include
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/esp_common/include
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/log/include
    /mnt/wksp/embedded/esp8266/butler/build/include
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/freertos/include
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/freertos/include/freertos
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/freertos/port/esp8266/include/freertos
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/freertos/port/esp8266/include
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/freertos/include/freertos/private
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/heap/include
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/heap/port/esp8266/include
    /mnt/prog/programs/esp/ESP8266_RTOS_SDK/components/freertos/port/esp8266/include
)

# Source files - Include mocks, test files, and test framework (no production code)
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/nvs_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/time_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/ds1307_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/actuator_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/freertos_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/nvCron_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/tmu_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unity/src/unity.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_runner.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_nvCron.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_tmu.c
)

# Add executable
message(STATUS "Compiling test sources: ${TEST_SOURCES}")
add_executable(test_runner
    ${TEST_SOURCES}
)

# Define __ESP_FILE__ macro to bypass ESP_ERROR_CHECK reliance on __FILE__
target_compile_definitions(test_runner PRIVATE __ESP_FILE__=__BASE_FILE__)

# Link libraries
target_link_libraries(test_runner
    m
    /mnt/wksp/embedded/esp8266/butler/build/freertos/libfreertos.a
    /mnt/wksp/embedded/esp8266/butler/build/esp8266/libesp8266.a
    /mnt/wksp/embedded/esp8266/butler/build/esp_common/libesp_common.a
    /mnt/wksp/embedded/esp8266/butler/build/nvs_flash/libnvs_flash.a
    /mnt/wksp/embedded/esp8266/butler/build/log/liblog.a
)