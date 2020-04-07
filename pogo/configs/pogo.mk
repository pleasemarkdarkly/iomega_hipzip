#
# hdcdcf.mk: make options and configuration for hd, cd, cf demo build
# danc@iobjects.com 06/14/01
# (c) Interactive Objects
#

#ECOS_BUILD_NAME := net-ram
ECOS_BUILD_NAME := usb-ram-fast

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__DHARMA -O2 -DDEBUG_LEVEL=0

MAIN_MODULE := main/demos/pogo/main/current

TARGET_FILE_NAME := dadio.exe

define post_link_step
	arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
	arm-elf-objcopy -O srec $(BUILD_TARGET) $(BUILD_TARGET:.exe=.srec)
	@echo $(BUILD_TARGET:.exe=.srec) >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef

