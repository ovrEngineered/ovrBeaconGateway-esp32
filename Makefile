#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ovrBeaconGateway-esp32

include $(IDF_PATH)/make/project.mk

CFLAGS += -DCONFIG_PHY_LAN8720 -DCONFIG_PHY_ADDRESS=0 -DCONFIG_PHY_POWER_PIN=17 -DCONFIG_PHY_SMI_MDC_PIN=23 -DCONFIG_PHY_SMI_MDIO_PIN=18
CFLAGS += -DLWIP_AUTOIP -DLWIP_DHCP_AUTOIP_COOP
