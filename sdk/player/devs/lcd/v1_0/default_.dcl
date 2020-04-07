# default.dcl: default configuration for lcd
# danc@iobjects.com 07/18/01
# (c) Interactive Objects

name lcd
type dev

export lcd.h

link default.a

tests lcd_test.cpp

#
# custom headers
#

header _lcd_hw.h start

#define LCD_FRAME_BUFFER_ADDRESS 0xC0000000
#define LCD_WIDTH                320
#define LCD_HEIGHT               240
#define LCD_BITS_PER_PIXEL         4
#define LCD_REFRESH_RATE          70

header _lcd_hw.h end
