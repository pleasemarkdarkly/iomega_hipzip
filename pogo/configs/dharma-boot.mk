
# dharma-boot.mk: make options and configuration for pogo-dharma boot loader
# danc@iobjects.com 06/14/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := dharma-boot-rom

MLT_FILE := mlt_arm_edb7312_rom.mlt

COMPILER_FLAGS := -g -DDADIO_BOOT -DNOKERNEL -DBLOCK_SIZE_OVERRIDE=0x20000 -DFLASH_SIZE_OVERRIDE=0x200000

MAIN_MODULE := bootloader/pogo

TARGET_FILE_NAME := dadio.exe

define post_link_step
        arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
        arm-elf-objcopy -O srec $(BUILD_TARGET) $(BUILD_TARGET:.exe=.srec)
        @echo $(BUILD_TARGET:.exe=.srec) >> $(BUILD_LIST)
        @echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
endef