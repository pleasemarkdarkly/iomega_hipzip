// irmaps.h: ir mappings

#ifndef __IRMAPS_H__
#define __IRMAPS_H__

const ir_key_map_t ir_press_map[] = 
{

    // reprogram notes:
    // hold setup till 2 blinks
    // enter 9-9-4 (should blink twice on 4)
    // press setup key
    // enter code, then key

    {0x0e,      IR_KEY_POWER}, // power!
    {0x11,      IR_KEY_1_misc}, // 1
    {0x12,      IR_KEY_2_abc},
    {0x13,      IR_KEY_3_def},
    {0x14,      IR_KEY_4_ghi},
    {0x15,      IR_KEY_5_jkl},
    {0x16,      IR_KEY_6_mno},
    {0x17,      IR_KEY_7_pqrs},
    {0x18,      IR_KEY_8_tuv},
    {0x19,      IR_KEY_9_wxyz}, // 9
    {0x09,      IR_KEY_MUTE}, // mute
    {0x10,      IR_KEY_0_space}, // 0
    {0x07,      IR_KEY_MUTE}, // enter
    {0xd1,      IR_KEY_ABC_DOWN}, // setup code = 369
    {0x08,      IR_KEY_SHIFT},
    {0x01,      IR_KEY_CHANNEL_UP},
    {0xd5,      IR_KEY_ABC_UP}, // setup code = 368
    {0x5b,      IR_KEY_SOURCE},
    {0x02,      IR_KEY_CHANNEL_DOWN},
    {0x37,      IR_KEY_PRINT_SCREEN}, // guide
    {0x1e,      IR_KEY_UP},
    {0x36,      IR_KEY_EXIT},
    {0x04,      IR_KEY_PREV},
    {0x1d,      IR_KEY_SELECT},
    {0x03,      IR_KEY_NEXT},
    {0x06,      IR_KEY_INFO},
    {0x1f,      IR_KEY_DOWN},
    {0x00,      IR_KEY_MENU}, // menu
    {0x0b,      IR_KEY_REW},
    {0x34,      IR_KEY_PLAY},
    {0x1a,      IR_KEY_FWD},
    {0x1b,      IR_KEY_RECORD},
    {0x0d,      IR_KEY_STOP},
    {0x1c,      IR_KEY_PAUSE},
    {0x35,      IR_KEY_GENRE}, // vcr +
    {0x5a,      IR_KEY_ARTIST},
    {0x59,      IR_KEY_ALBUM},
    {0x58,      IR_KEY_PLAYLIST},
    {0x9f,      IR_KEY_CLEAR},
    {0x9e,      IR_KEY_PLAY_MODE},
    {0x9d,      IR_KEY_ZOOM},
    {0x9c,      IR_KEY_RADIO},
    {0x0a,      IR_KEY_DELETE},
    {0x0c,      IR_KEY_EDIT},
    {0x0f,      IR_KEY_SAVE},
    {0x05,      IR_KEY_ADD} // skip
};


const ir_key_map_t ir_hold_map[] = 
{
    {0x0e,      IR_KEY_POWER}, // power!
    {0x11,      0}, //IR_KEY_1_misc}, // 1
    {0x12,      0}, //IR_KEY_2_abc},
    {0x13,      0}, //IR_KEY_3_def},
    {0x14,      0}, //IR_KEY_4_ghi},
    {0x15,      0}, //IR_KEY_5_jkl},
    {0x16,      0}, //IR_KEY_6_mno},
    {0x17,      0}, //IR_KEY_7_pqrs},
    {0x18,      0}, //IR_KEY_8_tuv},
    {0x19,      0}, //IR_KEY_9_wxyz}, // 9
    {0x09,      0}, //IR_KEY_MUTE}, // mute
    {0x10,      0}, //IR_KEY_0_space}, // 0
    {0x07,      0}, //IR_KEY_SELECT}, // enter
    {0xd1,      IR_KEY_ABC_DOWN}, // setup code = 369
    {0x08,      0}, //IR_KEY_SHIFT},
    {0x01,      IR_KEY_CHANNEL_UP},
    {0xd5,      IR_KEY_ABC_UP}, // setup code = 368
    {0x5b,      0}, //IR_KEY_SOURCE},
    {0x02,      IR_KEY_CHANNEL_DOWN},
    {0x37,      0}, //IR_KEY_PRINT_SCREEN}, // guide
    {0x1e,      IR_KEY_UP},
    {0x36,      0}, //IR_KEY_EXIT},
    {0x04,      IR_KEY_PREV},
    {0x1d,      0}, //IR_KEY_SELECT},
    {0x03,      IR_KEY_NEXT},
    {0x06,      0}, //IR_KEY_INFO},
    {0x1f,      IR_KEY_DOWN},
    {0x00,      0}, //IR_KEY_MENU}, // menu
    {0x0b,      IR_KEY_REW},
    {0x34,      0}, //IR_KEY_PLAY},
    {0x1a,      IR_KEY_FWD},
    {0x1b,      0}, //IR_KEY_RECORD},
    {0x0d,      0}, //IR_KEY_STOP},
    {0x1c,      0}, //IR_KEY_PAUSE},
    {0x35,      0}, //IR_KEY_GENRE}, // vcr +
    {0x5a,      0}, //IR_KEY_ARTIST},
    {0x59,      0}, //IR_KEY_ALBUM},
    {0x58,      0}, //IR_KEY_PLAYLIST},
    {0x9f,      0}, //IR_KEY_CLEAR},
    {0x9e,      0}, //IR_KEY_PLAY_MODE},
    {0x9d,      0}, //IR_KEY_ZOOM},
    {0x9c,      0}, //IR_KEY_RADIO},
    {0x0a,      0}, //IR_KEY_DELETE},
    {0x0c,      0}, //IR_KEY_EDIT},
    {0x0f,      0}, //IR_KEY_SAVE},
    {0x05,      0}  // IR_KEY_ADD} // skip
};


const ir_map_t ir_map = 
{
    num_buttons: 46,
    repeat_flags: (IR_REPEAT_ENABLE),
    filter_start: 6,  // 0 == unfiltered
    filter_rate:  2,  // 0 == unfiltered
    press_map: ir_press_map,
    hold_map: ir_hold_map
};

#endif // __IRMAPS_H__
