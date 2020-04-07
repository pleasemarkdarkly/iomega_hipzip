
# dj-boot.mk: make options and configuration for dj boot loader
# temancl@iobjects.com 12/30/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := dj-boot-rom-90

MLT_FILE := mlt_arm_edb7312_rom.mlt

COMPILER_FLAGS := -D__DHARMA=1 -DNOKERNEL -D__DJ -g -O2

MAIN_MODULE := bootloader/grl/current

TARGET_FILE_NAME := dj-grl-90.exe

define post_link_step
        arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
        @echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef