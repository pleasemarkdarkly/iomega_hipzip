#
# dj.mk: make options and configuration for the darwin jukebox (dj) build
# danb@iobjects.com 11/26/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := dj-ram-90

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__DHARMA=1 -D__DJ -g -O2 -DDEBUG_LEVEL=0xC0 -DDJ_RELEASE

MAIN_MODULE := main/main/current

TARGET_FILE_NAME := dj.exe

define post_link_step
	arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
	$(BASE_DIR)/scripts/build-gzip -C fullplay_dj_app_$(BUILD_VERSION) -c9 $(BUILD_TARGET:.exe=.bin) > $(BUILD_TARGET:.exe=.bin.gz)
	$(BASE_DIR)/scripts/build-gzip -C fullplay_dj_app_9999 -c9 $(BUILD_TARGET:.exe=.bin) > $(BUILD_TARGET:.exe=_force.bin.gz)
	$(BASE_DIR)/boot/mkimg.exe $(BASE_DIR)/boot/djconfig.ilf $(BUILD_VERSION) "$(BUILD_VERSION)" 1 $(BASE_DIR)/boot/
	$(BASE_DIR)/boot/fisutil.exe $(BASE_DIR)/boot/djconfig.fis $(BASE_DIR)/boot/
	@echo $(BUILD_TARGET:.exe=.bin)          >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.bin.gz)       >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.img)          >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=)_force.bin.gz >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=)_full_image.bin >> $(BUILD_LIST)
endef
