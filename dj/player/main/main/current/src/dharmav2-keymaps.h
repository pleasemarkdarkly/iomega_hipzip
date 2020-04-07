// dar-keymaps.h: keymaps for the dar
// Note: this header assumes you have included the definition of key_map_t prior to including this

#ifndef __DHARMAV2_KEYMAPS_H__
#define __DHARMAV2_KEYMAPS_H__

#ifdef ENABLE_RESTORE_KEY
#define RESTORE_ROW 0
#define RESTORE_COMBO 0x20
#define EJECT_ROW 2
#define EJECT_COMBO 0x02
#endif

static unsigned int _press_map[] = 
{

	// from right to left, bottom row
    KEY_RECORD,             // 1
	KEY_STOP,               // 3
    KEY_FWD,                // 5
	KEY_REFRESH_CONTENT,    // 7
    IR_KEY_NEXT,            // 23
	KEY_MENU,               // 11

	// top row, right to left
	KEY_PLAY,               // 2
    KEY_PAUSE,              // 4
    KEY_REW,                // 6
    KEY_CD_EJECT,           // 8
   	IR_KEY_PREV,            // 24
    KEY_SELECT,             // 12

	// rest of bottom row
	KEY_DOWN,               // 13
	KEY_EXIT,             // 15

	// rest of top row
    KEY_UP,                 // 14    
    KEY_POWER               // 16
};
static unsigned int _hold_map[]  =
{
    KEY_RECORD,
	0,
	KEY_FWD,
	0,
	IR_KEY_NEXT,
	0,0,0,
	KEY_REW,
	0,
	IR_KEY_PREV,
	0,	
	KEY_DOWN,
	0,
	KEY_UP,
	KEY_POWER
};
static unsigned int _release_map[]  =
{

    KEY_RECORD,
	0,
	KEY_FWD,
	0,
	IR_KEY_NEXT,
	0,0,0,
	KEY_REW,
	0,
	IR_KEY_PREV,
	0,	
	0,
	0,
	0,
	KEY_POWER

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

#endif // __DHARMAV2_KEYMAPS_H__
