# ks0713.dcl: ks0713 configuration for lcd
# danb@iobjects.com 10/04/01
# (c) Interactive Objects

name lcd
type dev

export lcd.h

compile ks0713.c

header _lcd_hw.h start

#define LCD_WIDTH                128
#define LCD_HEIGHT                64
#define LCD_BITS_PER_PIXEL         1

header _lcd_hw.h end

tests ks0713_test.cpp
