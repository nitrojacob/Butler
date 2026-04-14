cmake_minimum_required(VERSION 3.16)

project(esp8266_unit_tests)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Use IDF_PATH from the environment instead of hardcoded SDK paths
set(IDF_PATH $ENV{IDF_PATH})
if(NOT IDF_PATH)
    message(FATAL_ERROR "Environment variable IDF_PATH is not set. Please export IDF_PATH to your ESP8266 RTOS SDK path.")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/unity/src
    ${CMAKE_CURRENT_SOURCE_DIR}/cmock/src
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../main
    ${CMAKE_CURRENT_SOURCE_DIR}/../chip_esp8266
    ${IDF_PATH}/components/nvs_flash/include
    ${IDF_PATH}/components/esp_system/include
    ${IDF_PATH}/components/esp_log/include
    ${IDF_PATH}/components/esp8266/include
    ${IDF_PATH}/components/esp_common/include
    ${IDF_PATH}/components/log/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../build/include
    ${IDF_PATH}/components/freertos/include
    ${IDF_PATH}/components/freertos/include/freertos
    ${IDF_PATH}/components/freertos/port/esp8266/include/freertos
    ${IDF_PATH}/components/freertos/port/esp8266/include
    ${IDF_PATH}/components/freertos/include/freertos/private
    ${IDF_PATH}/components/heap/include
    ${IDF_PATH}/components/heap/port/esp8266/include
    ${IDF_PATH}/components/freertos/port/esp8266/include
)

# Source files - Include mocks, test files, and test framework (no production code)
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/nvs_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/esp_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/time_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/ds1307_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/actuator_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/freertos_mock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/nvCron.c
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

# Build with debug symbols and no optimization to aid debugging the test runner.
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
target_compile_options(test_runner PRIVATE -g -O0)
# Pass -g to linker as well to ensure debug sections are preserved
target_link_options(test_runner PRIVATE -g)

# Link libraries
target_link_libraries(test_runner
    m
)

# Note: Do not link the prebuilt ../build/*.a libraries here — they are built
# for the target ISA and won't work for the host unit test runner. The
# test suite provides mocks for platform functionality (see mocks/). Keep
# only host libraries (libm) linked.