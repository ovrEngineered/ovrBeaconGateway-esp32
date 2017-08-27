#
# Component Makefile for ESP32 IoT Development Framework (IDF)
#
# Used only when compiling this library as a component for ESP32 projects
#
ifdef CONFIG_OTA_UPDATE


# MUST be relative to the current directory of this file
PROJECT_ROOT := ../..
OTAUPDATE_ROOT := $(PROJECT_ROOT)/externals/otaUpdate.net


COMPONENT_DEPENDS := freertos


COMPONENT_ADD_INCLUDEDIRS := \
		$(PROJECT_ROOT)/project/include \
		$(OTAUPDATE_ROOT)/include

COMPONENT_SRCDIRS := \
		$(OTAUPDATE_ROOT)/src \
		$(OTAUPDATE_ROOT)/src/port
		
		
endif