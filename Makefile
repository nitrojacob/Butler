#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := butler
EXTRA_COMPONENT_DIRS += ./chip_esp8266


include $(IDF_PATH)/make/project.mk
