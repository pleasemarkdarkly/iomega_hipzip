#ifndef _GUI_TYPES_H_
#define _GUI_TYPES_H_

#include <devs/lcd/_lcd_hw.h>

#define LCD_BPP 4
#define SCREEN_X LCD_WIDTH
#define SCREEN_Y LCD_HEIGHT

#define GUI_OK 0
#define GUI_ERROR 1
#define GUI_MEMORY 2

#define GUI_TRUE 1
#define GUI_FALSE 0

typedef int guiRes;
typedef unsigned char UChar;
typedef short Bool;
typedef unsigned char guiColor;
typedef unsigned char* guiScreenBuffer;

typedef struct tagPoint {
	int x;
	int y;
} guiPoint;

typedef struct tagRect {
	guiPoint ul;	// upper left
	guiPoint lr;	// lower right
} guiRect;

#endif // _GUI_TYPES_H_