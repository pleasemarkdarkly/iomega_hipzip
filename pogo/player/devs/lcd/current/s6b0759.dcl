# s6b0759.dcl: s6b0759 configuration for lcd
# toddm@iobjects.com 08/17/01
# (c) Interactive Objects

name lcd
type dev

export lcd.h

compile s6b0759.c

header _lcd_hw.h start

#define LCD_WIDTH                128
#define LCD_HEIGHT                80
#define LCD_BITS_PER_PIXEL         1

header _lcd_hw.h end

tests s6b0759_test.cpp
