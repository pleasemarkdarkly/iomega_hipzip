#
# dj.mk: make options and configuration for the darwin jukebox (dj) build
# danb@iobjects.com 11/26/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := dj-ram

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__DHARMA=1 -D__DJ -g -O2

MAIN_MODULE := bootloader/restore

TARGET_FILE_NAME := dj-restore.exe

define post_link_step
	arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
	arm-elf-objcopy -O srec $(BUILD_TARGET) $(BUILD_TARGET:.exe=.srec)
	$(BASE_DIR)/scripts/build-gzip -C fullplay_dj_restore_$(BUILD_VERSION) -c9 $(BUILD_TARGET:.exe=.bin) > $(BUILD_TARGET:.exe=.img)
	$(BASE_DIR)/scripts/build-gzip -C fullplay_dj_restore_9999 -c9 $(BUILD_TARGET:.exe=.bin) > dj_force_$(BUILD_VERSION).img
	@echo dj_force_$(BUILD_VERSION).img >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.img) >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.srec) >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef

