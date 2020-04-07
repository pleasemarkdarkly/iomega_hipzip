//	key.cpp
//  temancl@fullplaymedia.com
//	dj keypad functionality test
//	does not ever really close, 
//  so weird event things might happen
//
//
#include <util/eventq/EventQueueAPI.h>
#include <devs/keyboard/Keyboard.h>

/*
mapping on REV-04 circuit board
1 POWER
2 MENU UP
3 SELECT
4 MENU
5 EXIT
6 MENU DOWN
7 CD EJECT
8 NEXT TRACK
9 STOP
10 PLAY
11 PREV TRACK
12 PAUSE
13 RECORD
*/

static char strbuttonmap[][16] = {"POWER","MENU UP","SELECT","MENU","EXIT","MENU DOWN","CD EJECT","NEXT TRACK","STOP","PLAY","PREV TRACK","PAUSE","RECORD"};

/* Keys on the device */
#define KEY_RECORD              13
#define KEY_PLAY                10
#define KEY_STOP                9
#define KEY_PAUSE               12
#define KEY_FWD                 8
#define KEY_REW                 11
#define KEY_MENU                4
#define KEY_SELECT              3
#define KEY_DOWN                6
#define KEY_UP                  2
#define KEY_CANCEL              5
#define KEY_POWER               1
#define KEY_CD_EJECT            7

static unsigned int _press_map[] = 
{
    KEY_POWER,              // 0
    KEY_CANCEL,             // 1
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
    KEY_FWD,                // 14
	KEY_REW,                // 13
    0                       // 15
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
    hold_map     : (const unsigned int*const)_press_map,
    release_map  : (const unsigned int*const)_press_map,
};



extern "C"
{

#include "cmds.h"
#include "io.h"

int test_key(char param_strs[][MAX_STRING_LEN],int* param_nums)
{  

	CEventQueue * EventQ = CEventQueue::GetInstance();
	
	DEBUG2("Starting Key Test\n");
    DEBUG3("EventQueue %screated\n", EventQ == 0 ? "not" : "");
   
	CKeyboard * Kbd = CKeyboard::GetInstance();
    Kbd->SetKeymap( &key_map );
    DEBUG3("Keyboard %screated\n", Kbd == 0 ? "not" : "");
   	Kbd->UnlockKeyboard();
    
	printf("Waiting for key %d - %s\n",param_nums[0],((param_nums[0] >= 1) && (param_nums[0] <= 13)) ? strbuttonmap[param_nums[0] - 1] : "invalid");
    for (;;) 
	{


		unsigned int Key;
		void * Data;
		while(!EventQ->TimedGetEvent( &Key, &Data, 100 ))
		{
			if(isbreak())
			{
				DEBUG3("Test canceled\n");
				Kbd->LockKeyboard();
				return TEST_ERR_FAIL;
			}
		}
	
		if((int)Data == param_nums[0])
		{
			if((int)Key == 3)
			{
				DEBUG3("Key Event: %d - %d\n",(int)Key,(int)Data);
				Kbd->LockKeyboard();
				return TEST_OK_PASS;
			}
		}
		else
		{
			DEBUG3("Key Event: %d - %d\n",(int)Key,(int)Data);
			DEBUG3("Wrong Key Generated\n");
			Kbd->LockKeyboard();
			return TEST_ERR_FAIL;
		}

	}

	Kbd->LockKeyboard();
	return TEST_ERR_FAIL;

	
}


}; // extern "C"
