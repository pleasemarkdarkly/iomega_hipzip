# default.dcl : hardware functionality testing code
# temancl@fullplaymedia.com 05/14/02
# (c) Fullplay Media

name hwfunc
type other

requires ecos_support_dj

# basic test harness
compile main.c parser.c io.c cmds.c 

# specific component tests (not removeable)
compile lcd.c audio.c key.cpp ir.cpp net.c ata.c audio_dac.c stress.cpp nc_test_slave.c nc_test_master.c serial.c flash.cpp

# export hwfunc.h
