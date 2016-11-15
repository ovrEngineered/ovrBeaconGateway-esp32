#
# Component Makefile for ESP32 IoT Development Framework (IDF)
#
# This "component" contains all of the project-specific source files
#


# MUST be relative to the current directory of this file
PROJECT_ROOT := ../..


COMPONENT_DEPENDS := openCXA-common


COMPONENT_ADD_INCLUDEDIRS := \
		$(PROJECT_ROOT)/project/include

COMPONENT_SRCDIRS := \
		$(PROJECT_ROOT)/project/src

include $(IDF_PATH)/make/component_common.mk