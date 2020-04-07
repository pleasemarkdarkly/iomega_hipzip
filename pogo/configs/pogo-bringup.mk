# dharma2_factory.mk: make options and configuration for dharma2 fast factory test
# temancl@iobjects.com 10/24/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := pogo-ram

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__POGO -D__DHARMA -g 

MAIN_MODULE := test/factory

TARGET_FILE_NAME := dadio.exe

define post_link_step
        arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
        arm-elf-objcopy -O srec $(BUILD_TARGET) $(BUILD_TARGET:.exe=.srec)
        @echo $(BUILD_TARGET:.exe=.srec) >> $(BUILD_LIST)
        @echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef
