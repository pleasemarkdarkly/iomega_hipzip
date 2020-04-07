#
# dj-dharma.mk: make options and configuration for the darwin jukebox on dharma (dj-dharma) build
# danb@iobjects.com 10/02/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := dharmav2i-ram

MLT_FILE := mlt_arm_dharmav2_ram.mlt

COMPILER_FLAGS := -D__DHARMA_V2 -D__DHARMA=2 -g -O2 -DALWAYS_LOG_DEBUG

MAIN_MODULE := main/main/current

TARGET_FILE_NAME := dj-dharmav2i.exe

define post_link_step
	arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
	$(BASE_DIR)/scripts/build-gzip -C fullplay_dj_app_$(BUILD_VERSION) -c9 $(BUILD_TARGET:.exe=.bin) > $(BUILD_TARGET:.exe=.bin.gz)
	$(BASE_DIR)/scripts/build-gzip -C fullplay_dj_app_9999 -c9 $(BUILD_TARGET:.exe=.bin) > $(BUILD_TARGET:.exe=_force.bin.gz)
	$(BASE_DIR)/boot/mkimg.exe $(BASE_DIR)/boot/djv2iconfig.ilf $(BUILD_VERSION) "$(BUILD_VERSION)" $(BASE_DIR)/boot/
	@echo $(BUILD_TARGET:.exe=.bin)          >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.bin.gz)       >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.img)          >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=)_force.bin.gz >> $(BUILD_LIST)
endef

