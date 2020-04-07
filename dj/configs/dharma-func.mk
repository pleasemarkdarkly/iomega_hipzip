# dharma2_factory.mk: make options and configuration for dharma2 fast factory test
# temancl@iobjects.com 10/24/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := dharmav2i-ram

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS :=  -D__DHARMA_V2 -D__DHARMA=2 -g

MAIN_MODULE := test/hwfunc

TARGET_FILE_NAME := func.exe

define post_link_step
        arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
        @echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef
