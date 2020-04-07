// dj-keymaps.h: keyboard mappings for DJ hardware
// Note: this header assumes you have included the definition of key_map_t prior to including this

#ifndef __DJ_KEYMAPS__
#define __DJ_KEYMAPS__

// use the 'menu' key
#ifdef ENABLE_RESTORE_KEY
#define RESTORE_ROW 3
#define RESTORE_COMBO 0x01
#define EJECT_ROW 2
#define EJECT_COMBO 0x02
#endif

// use the 'power' key
#ifdef ENABLE_PARTIAL_BOOT
#define POWER_ROW 0
#define POWER_COMBO 0x01
#endif

static unsigned int _press_map[] = 
{
    KEY_POWER,              // 0
    KEY_EXIT,             // 1
    0,                      // 2
    0,                      // 3
    KEY_UP,                 // 4
    KEY_DOWN,               // 5
    KEY_STOP,               // 6
    KEY_PAUSE,              // 7
    KEY_SELECT,             // 8
    KEY_CD_EJECT,           // 9
    KEY_PLAY,               // 10
    KEY_RECORD,             // 11
    KEY_MENU,               // 12
    KEY_FWD,                // 13
    KEY_REW,                // 14
    0                       // 15
};

static unsigned int _hold_map[]  =
{
    KEY_POWER,
    0,0,0,
    KEY_UP,
    KEY_DOWN,
    0,0,0,0,0,0,0,
    KEY_FWD,
    KEY_REW,
    0
};

static unsigned int _release_map[]  =
{
    KEY_POWER,
    0,0,0,0,0,0,0,0,0,0,KEY_RECORD,0,
    KEY_FWD,
    KEY_REW,
    0
};

static key_map_t key_map = 
{
    num_columns  :  4,
    num_rows     :  4,
    num_buttons  : 16,

    repeat_flags : (KEY_REPEAT_ENABLE),
    
    tick_length  :  2,
    repeat_rate  :  6,
    initial_delay: 12,
    
    press_map    : (const unsigned int*const)_press_map,
    hold_map     : (const unsigned int*const)_hold_map,
    release_map  : _release_map,
};




#endif // __DJ_KEYMAPS__
