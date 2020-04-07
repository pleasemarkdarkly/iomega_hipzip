#
# ram.mk: make options and configuration for ram builds
# danc@iobjects.com 06/14/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := net-ram

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__DHARMA -g -ffast-math

MAIN_MODULE := main

TARGET_FILE_NAME := dadio.exe
