#
# test-ramv2.mk: make options and configuration for ram builds
# toddm@iobjects.com 08/22/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := net-test

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__DHARMA_V2 -g -ffast-math

MAIN_MODULE := main

TARGET_FILE_NAME := dadio.exe




