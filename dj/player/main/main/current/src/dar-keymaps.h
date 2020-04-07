// dar-keymaps.h: keymaps for the dar
// Note: this header assumes you have included the definition of key_map_t prior to including this

#ifndef __DAR_KEYMAPS_H__
#define __DAR_KEYMAPS_H__

#include <cyg/hal/hal_edb7xxx.h>

static unsigned int _press_map[] = 
{
    KEY_POWER,              // 0
    KEY_UP,                 // 1
    KEY_MENU,               // 2
    KEY_DOWN,               // 3
    KEY_BREAK_POINT,        // 4
    KEY_PREVIOUS,           // 5
    KEY_REFRESH_CONTENT,    // 6
    0,                      // 7
    0,                      // 8
    0,                      // 9
    0,                      // 10
    KEY_PLAY_PAUSE,         // 11
    KEY_STOP,               // 12
    KEY_NEXT,               // 13
    0,                      // 14
    KEY_RECORD              // 15
};

/* this are what the button presses are on the darwin
	POWER			0
	VOTEUP			1
	MENU			2
	VOTEDOWN		3
	PROFILE			4
	PREV			5
	PRESET_1		6
	PRESET_2		7
	PRESET_3		8
	PRESET_4		9
	PRESET_5		10
	PLAYPAUSE		11
	STOP			12
	NEXT			13
					14
	JOGPRESS		15
	JOGDOWN			needs dial code
	JOGUP			needs dial code
*/

static unsigned int _hold_map[]  =
{
    KEY_POWER,
    KEY_UP,
    0,
    KEY_DOWN,
    0,
    KEY_PREVIOUS,
    0,0,0,0,0,0,0,
    KEY_NEXT,
    0,
    KEY_RECORD
};

static unsigned int _release_map[]  =
{
    KEY_POWER,
    0,0,0,0,
    KEY_PREVIOUS,
    0,0,0,0,0,0,0,
    KEY_NEXT,
    0,
    KEY_RECORD
};

static key_map_t key_map = 
{
    num_columns  :  3,
    num_rows     :  6,
    num_buttons  : 16,

    repeat_flags : (KEY_REPEAT_ENABLE),
    
    tick_length  :  2,
    repeat_rate  :  6,
    initial_delay: 12,
    
    press_map    : (const unsigned int*const)_press_map,
    hold_map     : (const unsigned int*const)_hold_map,
    release_map  : _release_map,
};


#endif // __DAR_KEYMAPS_H__
