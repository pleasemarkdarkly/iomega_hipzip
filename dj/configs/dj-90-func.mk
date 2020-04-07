# dj-90-func.mk: make options and configuration for dj-90 functional test shell
# temancl@iobjects.com 10/24/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := dj-rom-90

MLT_FILE := mlt_arm_edb7312_rom.mlt

COMPILER_FLAGS := -D__DHARMA=1 -D__DJ -DDEBUG_MISS -DDISABLE_VOLUME_CONTROL -DDAC_MUTE_DISABLE -DFACTORY_TEST -DSAHARA -DPIOMODE

MAIN_MODULE := test/hwfunc

TARGET_FILE_NAME := func90.exe

define post_link_step
        arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
        @echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef
