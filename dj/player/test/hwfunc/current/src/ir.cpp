//	ir.cpp
//  temancl@fullplaymedia.com
//	dj IR functionality test
//	does not ever really close, 
//  so weird event things might happen
//

#include <devs/ir/IR_UEI.h>
#include <util/eventq/EventQueueAPI.h>


/* Keys on the remote control */
#define IR_KEY_POWER            20
#define IR_KEY_UP               21
#define IR_KEY_DOWN             22
#define IR_KEY_NEXT             23
#define IR_KEY_PREV             24
#define IR_KEY_SELECT           25
#define IR_KEY_PLAY_MODE        26
#define IR_KEY_ADD              27
#define IR_KEY_SOURCE           28
#define IR_KEY_MENU             29
#define IR_KEY_REW              30
#define IR_KEY_PLAY             31
#define IR_KEY_FWD              32
#define IR_KEY_RECORD           33
#define IR_KEY_STOP             34
#define IR_KEY_PAUSE            35

#define IR_KEY_1_misc           41
#define IR_KEY_2_abc            42
#define IR_KEY_3_def            43
#define IR_KEY_4_ghi            44
#define IR_KEY_5_jkl            45
#define IR_KEY_6_mno            46
#define IR_KEY_7_pqrs           47
#define IR_KEY_8_tuv            48
#define IR_KEY_9_wxyz           49
#define IR_KEY_0_space          40

#define IR_KEY_MUTE             50
#define IR_KEY_SHIFT            51
#define IR_KEY_VOL_UP           52
#define IR_KEY_VOL_DOWN         53
#define IR_KEY_AV_POWER         54
#define IR_KEY_AV_INPUT         55
#define IR_KEY_CHANNEL_UP       56
#define IR_KEY_CHANNEL_DOWN     57

#define IR_KEY_GENRE            60
#define IR_KEY_ARTIST           61
#define IR_KEY_ALBUM            62
#define IR_KEY_PLAYLIST         63
#define IR_KEY_RADIO            64
#define IR_KEY_INFO             65
#define IR_KEY_ZOOM             66
#define IR_KEY_CLEAR            67
#define IR_KEY_EDIT             68
#define IR_KEY_SAVE             69
#define IR_KEY_DELETE           70
#define IR_KEY_CANCEL           71

#define IR_KEY_PRINT_SCREEN     80

const ir_key_map_t ir_press_map[] = 
{

	// reprogram notes:
	// hold setup till 2 blinks
	// enter 9-9-4 (should blink twice on 4)
	// press setup key
	// enter code, then key

    {0x0e,    	IR_KEY_POWER}, // power!
    {0x11,     	IR_KEY_1_misc}, // 1
    {0x12,     	IR_KEY_2_abc},
    {0x13,    	IR_KEY_3_def},
    {0x14,    	IR_KEY_4_ghi},
    {0x15,    	IR_KEY_5_jkl},
    {0x16,   	IR_KEY_6_mno},
    {0x17,    	IR_KEY_7_pqrs},
    {0x18,    	IR_KEY_8_tuv},
    {0x19,    	IR_KEY_9_wxyz}, // 9
    {0x09,    	IR_KEY_MUTE}, // mute
    {0x10,      IR_KEY_0_space}, // 0
    {0x07,    	IR_KEY_SELECT}, // enter
    {0xd1,		IR_KEY_VOL_UP}, // setup code = 369
    {0x08,    	IR_KEY_SHIFT},
    {0x01,     	IR_KEY_CHANNEL_UP},
    {0xd5,		IR_KEY_VOL_DOWN}, // setup code = 368
    {0x5b,      IR_KEY_SOURCE},
    {0x02,      IR_KEY_CHANNEL_DOWN},
    {0x37,    	IR_KEY_PRINT_SCREEN}, // guide
    {0x1e,    	IR_KEY_UP},
    {0x36,    	IR_KEY_CANCEL},
    {0x04,    	IR_KEY_PREV},
    {0x1d,    	IR_KEY_SELECT},
    {0x03,    	IR_KEY_NEXT},
    {0x06,    	IR_KEY_INFO},
    {0x1f,    	IR_KEY_DOWN},
    {0x00,      IR_KEY_MENU}, // menu
    {0x0b,    	IR_KEY_REW},
    {0x34,    	IR_KEY_PLAY},
    {0x1a,    	IR_KEY_FWD},
    {0x1b,    	IR_KEY_RECORD},
    {0x0d,    	IR_KEY_STOP},
    {0x1c,    	IR_KEY_PAUSE},
    {0x35,    	IR_KEY_GENRE}, // vcr +
    {0x5a,    	IR_KEY_ARTIST},
    {0x59,    	IR_KEY_ALBUM},
    {0x58,    	IR_KEY_PLAYLIST},
    {0x9f,    	IR_KEY_CLEAR},
    {0x9e,    	IR_KEY_PLAY_MODE},
    {0x9d,    	IR_KEY_ZOOM},
    {0x9c,    	IR_KEY_RADIO},
    {0x0a,    	IR_KEY_DELETE},
    {0x0c,    	IR_KEY_EDIT},
    {0x0f,    	IR_KEY_SAVE},
    {0x05,    	IR_KEY_ADD} // skip
};

const ir_map_t ir_map = 
{
    num_buttons: 46,
    repeat_flags: (IR_REPEAT_ENABLE),
    filter_start: 6,  // 0 == unfiltered
    filter_rate:  2,  // 0 == unfiltered
    press_map: ir_press_map,
    hold_map: ir_press_map
};


extern "C"
{
#include "cmds.h"
#include "io.h"

int test_ir(char param_strs[][MAX_STRING_LEN],int* param_nums)
{  

	CEventQueue * EventQ = CEventQueue::GetInstance();
	
	DEBUG2("Starting IR Test\n");
    DEBUG3("EventQueue %screated\n", EventQ == 0 ? "not" : "");

    CIR * pIR = CIR::GetInstance();
    DEBUG3("IR %screated\n", pIR == 0 ? "not" : "");

    pIR->SetIRMap( &ir_map );
    pIR->UnlockIR();

 	printf("Waiting for IR event %d\n",param_nums[0]);
	for (;;) 
	{

		unsigned int Key;
		void * Data;
		while(!EventQ->TimedGetEvent( &Key, &Data, 100 ))
		{
			if(isbreak())
			{
				DEBUG3("Test canceled\n");
				pIR->LockIR();
				return TEST_ERR_FAIL;
			}
		}
	
		if((int)Data == param_nums[0])
		{
			if((int)Key == 3)
			{
				DEBUG3("IR Event: %d - %d\n",(int)Key,(int)Data);
				pIR->LockIR();
				return TEST_OK_PASS;
			}
		}
		else
		{
			DEBUG3("IR Event: %d - %d\n",(int)Key,(int)Data);
			DEBUG3("Wrong IR Generated\n");
			pIR->LockIR();
			return TEST_ERR_FAIL;
		}

	}

	pIR->LockIR();
	return TEST_ERR_FAIL;
}

}; // extern "C"

