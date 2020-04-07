#
# dj-dharma.mk: make options and configuration for the darwin jukebox (dj-noupnp-dharma) build
# danb@iobjects.com 10/02/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := dj-redboot

MLT_FILE := mlt_arm_edb7312_rom.mlt

COMPILER_FLAGS := -D__DHARMA -D__DJ -O2
#LINKER_FLAGS := ../../ecos/builds/$(ECOS_BUILD_NAME)/$(ECOS_BUILD_NAME)_install/lib/version.o

MAIN_MODULE := main

TARGET_FILE_NAME := dj-redboot.exe

define post_link_step
	arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
	arm-elf-objcopy -O srec $(BUILD_TARGET) $(BUILD_TARGET:.exe=.srec)
	@echo $(BUILD_TARGET:.exe=.srec) >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef

