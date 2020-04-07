// pogo key mappings
#ifdef __POGO

static unsigned int _press_map[] = 
{
    KEY_UP,                 // 1
    KEY_PLAY_PAUSE,         // 2 (jog button)
    KEY_PLAY_PAUSE,         // 3
    KEY_DOWN,               // 4
    KEY_PREVIOUS,           // 5
    0,			            // 6 (play dupe event)
    KEY_NEXT,               // 7
    KEY_MENU,               // 8
    0,			            // 9 (play dupe event)
};
static unsigned int _hold_map[]  =
{

    KEY_UP,                 // 1
    0,         // 2
    0,         // 3
    KEY_DOWN,               // 4
    KEY_PREVIOUS,           // 5
    0,			    // 6 (play dupe event)
    KEY_NEXT,               // 7
    KEY_MENU,               // 8
    0,			    // 9 (play dupe event)
};

static unsigned int _release_map[]  =
{
    0,                 // 1
    0,         // 2
    0,         // 3
    0,               // 4
    KEY_PREVIOUS,           // 5
    0,			    // 6 (play dupe event)
    KEY_NEXT,               // 7
    KEY_MENU,               // 8
    0,			    // 9 (play dupe event)
};


static key_map_t key_map = 
{
    num_columns  :  3,
    num_rows     :  3,
    num_buttons  : 9,

    repeat_flags : (KEY_REPEAT_ENABLE),
    
    tick_length  :  2,
    repeat_rate  :  6,
    initial_delay: 12,
    
    press_map    : (const unsigned int*const)_press_map,
    hold_map     : (const unsigned int*const)_hold_map,
    release_map  : _release_map,
};

#else // __POGO

#define __DHARMA_KBD_MAP 1
// buttons
// (16) (14) (12) (10) ( 8) ( 6) ( 4) ( 2)
// (15) (13) (11) ( 9) ( 7) ( 5) ( 3) ( 1)
#if __DHARMA_KBD_MAP
static unsigned int _press_map[] = 
{
    KEY_MENU,               // 1
    0,                      // 2
    KEY_RECORD,             // 3
    KEY_DIAL_IN,            // 4
    KEY_DIAL_DOWN,          // 5
    KEY_DIAL_UP,            // 6
    KEY_DOWN,               // 7
    KEY_UP,                 // 8
    KEY_PREVIOUS,           // 9
    KEY_NEXT,               // 10
    KEY_BREAK_POINT,        // 11
    KEY_PLAY_PAUSE,         // 12
    KEY_REFRESH_CONTENT,    // 13
    KEY_TEST_STIMULATE,     // 14
    0,
    0,
};
static unsigned int _hold_map[]  =
{
    KEY_MENU,               
    0,0,0,
    KEY_DIAL_DOWN,
    KEY_DIAL_UP,
    KEY_DOWN,
    KEY_UP,
    KEY_PREVIOUS,
    KEY_NEXT,
    0,0,0,0,0,0
};
static unsigned int _release_map[]  =
{
    KEY_MENU,               
    0,0,0,0,0,0,0,
    KEY_PREVIOUS,
    KEY_NEXT,
    0,0,0,0,0,0
};

#else // __DHARMA_KBD_MAP
// keymap
static unsigned int _press_map[] = 
{
    KEY_DIAL_IN,
    KEY_DIAL_DOWN,
    KEY_DIAL_UP,
    KEY_PREVIOUS,
    KEY_PLAY_PAUSE,
    KEY_NEXT,
    KEY_RECORD,
    KEY_DOWN,
    KEY_UP,
    KEY_MENU,
    0,0,0,0,0,0
};
static unsigned int _hold_map[]  =
{
    KEY_MENU,               
    KEY_DIAL_DOWN,
    KEY_DIAL_UP,
    KEY_PREVIOUS,
    0,
    KEY_NEXT,
    0,
    KEY_DOWN,
    KEY_UP,
    0,0,0,0,0,0,0
};
static unsigned int _release_map[]  =
{
    KEY_MENU,               
    0,0,
    KEY_PREVIOUS,
    0,
    KEY_NEXT,
    0,0,0,0,0,0,0,0,0,0
};
#endif // __DHARMA_KBD_MAP

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

#endif // POGO