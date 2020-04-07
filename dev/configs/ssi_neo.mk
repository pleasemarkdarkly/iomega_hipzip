#
# hdcdcf.mk: make options and configuration for hd, cd, cf demo build
# danc@iobjects.com 06/14/01
# (c) Interactive Objects
#

#ECOS_BUILD_NAME := net-ram
#ECOS_BUILD_NAME := ram
ECOS_BUILD_NAME := usb-ram

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__DHARMA -g

MAIN_MODULE := main/demos/ssi_neo/main

TARGET_FILE_NAME := dadio.exe
