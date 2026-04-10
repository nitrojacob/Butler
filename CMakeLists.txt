# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(COMPONENTS main chip_esp32)

set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/chip_esp32")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(butler)
