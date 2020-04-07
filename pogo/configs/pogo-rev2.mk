#
# pogo-rev2.mk Pogo Revision 2 hardware build
# temancl@fullplaymedia.com
# (c) fullplay
#

ECOS_BUILD_NAME := pogo-ram

MLT_FILE := mlt_arm_edb7312_ram.mlt

COMPILER_FLAGS := -D__DHARMA -D__POGO -O2 -DDEBUG_LEVEL=0


MAIN_MODULE := main/demos/pogo/main/current

TARGET_FILE_NAME := pogo.exe

define post_link_step
	arm-elf-objcopy -O binary $(BUILD_TARGET) $(BUILD_TARGET:.exe=.bin)
	arm-elf-objcopy -O srec $(BUILD_TARGET) $(BUILD_TARGET:.exe=.srec)
    gzip -c9 $(BUILD_TARGET:.exe=.bin) > $(BUILD_TARGET:.exe=.bin.gz)
	@echo $(BUILD_TARGET:.exe=.srec) >> $(BUILD_LIST)
	@echo $(BUILD_TARGET:.exe=.bin)  >> $(BUILD_LIST)
    @echo $(BUILD_TARGET:.exe=.bin.gz)  >> $(BUILD_LIST)
endef

