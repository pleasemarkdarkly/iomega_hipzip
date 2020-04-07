#ifndef __DHARMA_KEYMAPS_H__
#define __DHARMA_KEYMAPS_H__

static unsigned int _press_map[] = 
{
    KEY_RECORD,             // 1
    KEY_PLAY,               // 2
    KEY_STOP,               // 3
    KEY_PAUSE,              // 4
    KEY_FWD,                // 5
    KEY_REW,                // 6
    KEY_REFRESH_CONTENT,    // 7
    KEY_BREAK_POINT,        // 8
    IR_KEY_NEXT,            // 9
    IR_KEY_PREV,            // 10
    KEY_MENU,               // 11
    KEY_SELECT,             // 12
    KEY_DOWN,               // 13
    KEY_UP,                 // 14
    KEY_EXIT,             // 15
    KEY_POWER               // 16
};
static unsigned int _hold_map[]  =
{
    KEY_RECORD,
    0,0,0,
    KEY_FWD,
    KEY_REW,
    0,0,
    IR_KEY_NEXT,
    IR_KEY_PREV,
    0,0,
    KEY_DOWN,
    KEY_UP,
    0,
    KEY_POWER
};
static unsigned int _release_map[]  =
{
    KEY_RECORD,
    0,0,0,
    KEY_FWD,
    KEY_REW,
    0,0,
    IR_KEY_NEXT,
    IR_KEY_PREV,
    0,0,0,0,0,
    KEY_POWER
};

static key_map_t key_map = 
{
    num_columns  :  8,
    num_rows     :  2,
    num_buttons  : 16,

    repeat_flags : (KEY_REPEAT_ENABLE),
    
    tick_length  :  2,
    repeat_rate  :  6,
    initial_delay: 12,
    
    press_map    : (const unsigned int*const)_press_map,
    hold_map     : (const unsigned int*const)_hold_map,
    release_map  : _release_map,
};

#endif // __DHARMA_KEYMAPS_H__
