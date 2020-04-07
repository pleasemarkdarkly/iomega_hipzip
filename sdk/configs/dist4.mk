#
# distX.mk: make options and configuration for sdk distribution
# danc@iobjects.com 06/14/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := ramv2

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__DHARMA=2 -O2 -DDEBUG_LEVEL=0

# this should be ignored
MAIN_MODULE := main/v1_0

BINARY_DIST := 1

TARGET_FILE_NAME := dadio.exe

define post_link_step
	arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
	arm-elf-objcopy -O srec $(BUILD_TARGET) $(BUILD_TARGET:.exe=.srec)
	@echo $(BUILD_TARGET:.exe=.srec) >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef

