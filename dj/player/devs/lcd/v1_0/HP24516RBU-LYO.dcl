# HP24516RBU-LYO.dcl: HP24516RBU-LYO configuration for lcd
# danb@iobjects.com 10/25/01
# (c) Interactive Objects

name lcd
type dev

export lcd.h

compile HP24516RBU-LYO.c

header _lcd_hw.h start

#define LCD_WIDTH                240
#define LCD_HEIGHT                64
#define LCD_BITS_PER_PIXEL         1

header _lcd_hw.h end

tests HP24516RBU-LYO_test.cpp
