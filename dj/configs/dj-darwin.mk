#
# dj-darwin.mk: make options and configuration for the darwin jukebox on darwin (dj-darwin) build
# danb@iobjects.com 11/11/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := darwin-net-ram

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__DHARMA -D__DAR -g

MAIN_MODULE := main

TARGET_FILE_NAME := dj-darwin.exe

define post_link_step
	arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
	arm-elf-objcopy -O srec $(BUILD_TARGET) $(BUILD_TARGET:.exe=.srec)
	@echo $(BUILD_TARGET:.exe=.srec) >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef

