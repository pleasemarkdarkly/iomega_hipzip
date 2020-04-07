#ifndef KBD_SERVER_HEADER_FILE_DEFINED
#define KBD_SERVER_HEADER_FILE_DEFINED

#include "kbd.h"

#if 0
#define NUM_KBD_COLUMNS 8
#define NUM_KBD_ROWS    2
#define NUM_KEYS        NUM_KBD_KEYS
#define NUM_KBD_KEYS    (NUM_KBD_COLUMNS * NUM_KBD_ROWS)
#endif

#define NUM_KBD_KEYS NUM_KEYS

// keyboard constants, mapping keypress values to application meaning
#define KBD_MENU_UP			1
#define KBD_MENU_DOWN		0
#define KBD_MENU_SELECT		2

/* keyboard fns */
void KeyboardProc(void);

/* keyboard data members */
extern cyg_uint8 _KeysPressed[NUM_KBD_COLUMNS];
extern cyg_uint8 _NewKeyState[NUM_KEYS];
extern cyg_uint8 _KeyState[NUM_KEYS];

#endif // defined KBD_SERVER_HEADER_FILE_DEFINED
