#
# dj-dharma.mk: make options and configuration for the darwin jukebox on dharma (dj-dharma) build
# danb@iobjects.com 10/02/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := net-ram

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__DHARMA -g -O2 -DALWAYS_LOG_DEBUG

MAIN_MODULE := main

TARGET_FILE_NAME := dj-dharma.exe

define post_link_step
	arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
	arm-elf-objcopy -O srec $(BUILD_TARGET) $(BUILD_TARGET:.exe=.srec)
	$(BASE_DIR)/scripts/build-gzip -C fullplay_dj_app_$(BUILD_VERSION) -c9 $(BUILD_TARGET:.exe=.bin) > $(BUILD_TARGET:.exe=.img)
	$(BASE_DIR)/scripts/build-gzip -C fullplay_dj_app_9999 -c9 $(BUILD_TARGET:.exe=.bin) > $(BUILD_TARGET:.exe=)_force_$(BUILD_VERSION).img
	@echo $(BUILD_TARGET:.exe=)_force_$(BUILD_VERSION).img >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.img) >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.srec) >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef

