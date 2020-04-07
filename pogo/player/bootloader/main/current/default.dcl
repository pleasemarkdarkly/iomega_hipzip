# bootloader modules DCL
# temancl@iobjects.com
# (c) Interactive Objects
# FIXME: dependencies on fat driver

name main
type other
export bootloader.h boot_ide.h boot_types.h boot_flash.h boot_mem.h
compile bootloader.cpp boot_ide.cpp boot_flash.cpp boot_mem.cpp decompress.c misc_funs.c
