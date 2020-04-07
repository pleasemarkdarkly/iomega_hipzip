
# pogo-boot.mk: make options and configuration for Pogo boot loader
# temancl@iobjects.com 12/30/01
# (c) Interactive Objects
#

ECOS_BUILD_NAME := pogo-boot-rom

MLT_FILE := mlt_arm_edb7312_rom.mlt

COMPILER_FLAGS := -O2 -DDADIO_BOOT -DNOKERNEL -D__POGO

# -g 
MAIN_MODULE := bootloader/pogo

TARGET_FILE_NAME := pogoboot.exe

define post_link_step
        arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
        arm-elf-objcopy -O srec $(BUILD_TARGET) $(BUILD_TARGET:.exe=.srec)
	gzip -c9 $(BUILD_TARGET:.exe=.bin) > $(BUILD_TARGET:.exe=.gz)
        @echo $(BUILD_TARGET:.exe=.srec) >> $(BUILD_LIST)
        @echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.gz)  >> $(BUILD_LIST)
endef