# small_lcd.dcl: default configuration for small lcd
# danc@iobjects.com 07/18/01
# (c) Interactive Objects

name lcd
type dev

export lcd.h

compile lcd.c

arch lcd.o

dist include/lcd.h small_lcd.a tests/lcd_test.cpp

header _lcd_hw.h start

#define LCD_FRAME_BUFFER_ADDRESS 0xC0000000
#define LCD_WIDTH                240
#define LCD_HEIGHT               160
#define LCD_BITS_PER_PIXEL         4
#define LCD_REFRESH_RATE          70

header _lcd_hw.h end

tests lcd_test.cpp
