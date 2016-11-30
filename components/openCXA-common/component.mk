#
# Component Makefile for ESP32 IoT Development Framework (IDF)
#
# Used only when compiling this library as a component for ESP32 projects
#


# MUST be relative to the current directory of this file
PROJECT_ROOT := ../..
OPENCXA_ROOT := $(PROJECT_ROOT)/externals/openCXA-common


COMPONENT_DEPENDS := freertos


COMPONENT_ADD_INCLUDEDIRS := \
		$(PROJECT_ROOT)/project/include \
		$(OPENCXA_ROOT)/include/arch-common \
		$(OPENCXA_ROOT)/include/arch-esp32 \
		$(OPENCXA_ROOT)/include/collections \
		$(OPENCXA_ROOT)/include/console \
		$(OPENCXA_ROOT)/include/logger \
		$(OPENCXA_ROOT)/include/misc \
		$(OPENCXA_ROOT)/include/mqtt \
		$(OPENCXA_ROOT)/include/mqtt/messages \
		$(OPENCXA_ROOT)/include/net \
		$(OPENCXA_ROOT)/include/peripherals \
		$(OPENCXA_ROOT)/include/runLoop \
		$(OPENCXA_ROOT)/include/serial \
		$(OPENCXA_ROOT)/include/stateMachine \
		$(OPENCXA_ROOT)/include/timeUtils

COMPONENT_SRCDIRS := \
		$(OPENCXA_ROOT)/src/arch-common \
		$(OPENCXA_ROOT)/src/arch-esp32 \
		$(OPENCXA_ROOT)/src/collections \
		$(OPENCXA_ROOT)/src/console \
		$(OPENCXA_ROOT)/src/logger \
		$(OPENCXA_ROOT)/src/misc \
		$(OPENCXA_ROOT)/src/mqtt \
		$(OPENCXA_ROOT)/src/mqtt/messages \
		$(OPENCXA_ROOT)/src/net \
		$(OPENCXA_ROOT)/src/peripherals \
		$(OPENCXA_ROOT)/src/runLoop \
		$(OPENCXA_ROOT)/src/serial \
		$(OPENCXA_ROOT)/src/stateMachine \
		$(OPENCXA_ROOT)/src/timeUtils