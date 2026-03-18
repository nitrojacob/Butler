# ESP-IDF project CMakeLists.txt

# Set the project name
project(butler)

# Set the minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# Include the ESP-IDF CMakeLists.txt
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Add the source files
set(SOURCES
    main/main.c
    main/stateProbe.c
    main/trap.c
    main/nvLogRing.c
    main/actuator.c
    main/fota.c
    main/tmu.c
    main/nvCron.c
    main/comm.c
    main/clearButton.c
)

# Add the component
idf_component_register(SRCS ${SOURCES}
    INCLUDE_DIRS .
    REQUIRES esp_wifi esp_event nvs_flash mqtt_client freertos
)

# Set the target name
set(TARGET butler)

# Set the target type
set(TARGET_TYPE app)

# Set the target description
set(TARGET_DESCRIPTION "Butler ESP8266 Application")

# Set the target version
set(TARGET_VERSION 1.0.0)

# Set the target author
set(TARGET_AUTHOR "Your Name")

# Set the target license
set(TARGET_LICENSE "MIT")

# Set the target homepage
set(TARGET_HOMEPAGE "https://github.com/yourname/butler")

# Set the target repository
set(TARGET_REPOSITORY "https://github.com/yourname/butler.git")

# Set the target keywords
set(TARGET_KEYWORDS "ESP8266 IoT")

# Set the target dependencies
set(TARGET_DEPENDENCIES "")

# Set the target build options
set(TARGET_BUILD_OPTIONS "")

# Set the target linker script
set(TARGET_LINKER_SCRIPT "esp8266_out.ld")

# Set the target partition table
set(TARGET_PARTITION_TABLE "partitions_two_ota_largeNVS.csv")

# Set the target flash size
set(TARGET_FLASH_SIZE "4MB")

# Set the target flash mode
set(TARGET_FLASH_MODE "qio")

# Set the target flash frequency
set(TARGET_FLASH_FREQ "40m")

# Set the target boot mode
set(TARGET_BOOT_MODE "qio")

# Set the target debug level
set(TARGET_DEBUG_LEVEL "INFO")

# Set the target optimization level
set(TARGET_OPTIMIZATION_LEVEL "s")

# Set the target stack size
set(TARGET_STACK_SIZE "8192")

# Set the target heap size
set(TARGET_HEAP_SIZE "16384")

# Set the target task priority
set(TARGET_TASK_PRIORITY "1")

# Set the target task core
set(TARGET_TASK_CORE "0")

# Set the target task affinity
set(TARGET_TASK_AFFINITY "0")

# Set the target task stack size
set(TARGET_TASK_STACK_SIZE "2048")
