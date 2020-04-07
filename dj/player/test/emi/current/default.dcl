# default.dcl : EMI stress test
# temancl@fullplaymedia.com 05/14/02
# (c) Fullplay Media

name emi
type other

requires ecos_support_dj

# basic test harness
compile emi.cpp lcd.cpp MiniCDMgr.cpp ata.cpp
compile nc_test_slave.c 

