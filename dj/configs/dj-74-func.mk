# dharma2_factory.mk: make options and configuration for dharma2 fast factory test
# temancl@iobjects.com 10/24/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := dj-rom-74

MLT_FILE := mlt_arm_edb7312_rom.mlt

COMPILER_FLAGS := -D__DHARMA=1 -D__DJ -DDEBUG_MISS

MAIN_MODULE := test/hwfunc

TARGET_FILE_NAME := func74.exe

define post_link_step
        arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
        @echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef
