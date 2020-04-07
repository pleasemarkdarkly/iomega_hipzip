// emi.cpp 
// fooo!
// temancl@fullplaymedia.com 07/19/02
// (c) fullplay media 
// 
// description:
// emi test framework, main event queue
// contains serial, audio, and memory tests

#include <core/events/SystemEvents.h>   // event types
#include <cyg/error/codes.h>
#include <cyg/fileio/fileio.h>

#include <stdio.h>
#include <stdlib.h>
#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>
#include <devs/audio/dai.h>
#include <devs/audio/cs5332.h>
#include <util/eventq/EventQueueAPI.h>
#include <devs/keyboard/Keyboard.h>
#include <devs/ir/IR_UEI.h>
#include <pkgconf/io_serial.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "ata.h"
#include "lcd.h"
#include "net.h"
#include "MiniCDMgr.h"

#define WORDSIZE 4
#define HALFSIZE 2
#define BYTESIZE 1

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


/* Keys on the device */
#define KEY_RECORD              1
#define KEY_PLAY                2
#define KEY_STOP                3
#define KEY_PAUSE               4
#define KEY_FWD                 5
#define KEY_REW                 6
#define KEY_REFRESH_CONTENT     7  // dharma board only
#define KEY_BREAK_POINT         8  // dharma board only
#define KEY_MENU                11
#define KEY_SELECT              12
#define KEY_DOWN                13
#define KEY_UP                  14
#define KEY_CANCEL              15
#define KEY_POWER               16
#define KEY_CD_EJECT            17

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
    KEY_REW,                // 13
    KEY_FWD,                // 14
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

// thread data etc
#define NTHREADS       7
#define STACKSIZE   8192*4

static cyg_handle_t threadh[NTHREADS];
static cyg_thread   thread[ NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];
static CMiniCDMgr* pMiniCDMgr;
bool s_bSerial,s_bAudio,s_bMem;

void memtest_thread(cyg_uint32 ignored);

void ToggleMemtest();
void ToggleAudio();
void ToggleSerial();
void serial_thread(cyg_uint32 ignored);
void audio_thread(cyg_uint32 ignored);
void StartCDStress();
void StopCDStress();


void thread_entry(cyg_uint32 ignored)
{

	//
	// start various threads
	//

  	// ATA stuff
	InitATA();

	// LCD / LED rw thread
	cyg_thread_create( 8, lcd_thread, 0, "lcd led thread",
                       (void*)tstack[1], STACKSIZE, &threadh[1], &thread[1]);
    cyg_thread_resume( threadh[1] );

	
	// Network stress
	cyg_thread_create( 8, net_thread, 0, "net test thread",
                       (void*)tstack[2], STACKSIZE, &threadh[2], &thread[2]);
    cyg_thread_resume( threadh[2] );

	
	// Start ADC/DAC loopback fiq
	DAIInit();
	DACSetVolume(0);    
    DAIEnable();
	DAIWrite(0);
	DAISetLoopbackFIQ();
	
	// Audio clock stress
	cyg_thread_create( 8, audio_thread, 0, "audio test thread",
                       (void*)tstack[3], STACKSIZE, &threadh[3], &thread[3]);
    cyg_thread_resume( threadh[3] );

	
	// memory stress
	cyg_thread_create( 8, memtest_thread, 0, "memtest thread",
                       (void*)tstack[4], STACKSIZE, &threadh[4], &thread[4]);
    cyg_thread_resume( threadh[4] );



	// serial stress
	cyg_thread_create( 8, serial_thread, 0, "serial test thread",
                       (void*)tstack[5], STACKSIZE, &threadh[5], &thread[5]);
    cyg_thread_resume( threadh[5] );
	s_bSerial = true;
	s_bMem = true;
	s_bAudio = true;


	// register keyboard, IR, start main event loop
	CEventQueue * EventQ = CEventQueue::GetInstance();

	CKeyboard * Kbd = CKeyboard::GetInstance();
    Kbd->SetKeymap( &key_map );
    CIR * pIR = CIR::GetInstance();
    pIR->SetIRMap( &ir_map );

	// Setup mini-CD datasource
	// cd/hd stress (conditional, suspended by default)
	pMiniCDMgr = CMiniCDMgr::GetInstance();

	for (;;) 
	{
		unsigned int Key;
		void * Data;
		EventQ->GetEvent( &Key, &Data );


		switch(Key)
		{

			case EVENT_MEDIA_INSERTED:
				diag_printf("inserted\n");
				StartCDStress();
				break;
			case EVENT_MEDIA_REMOVED:
				diag_printf("removal detected\n");				
				break;
			case EVENT_KEY_PRESS:		
				switch((int)Data)
				{
					case KEY_STOP:
						ToggleSerial();
						break;
					case KEY_PLAY:
						ToggleAudio();
						break;
					case KEY_PAUSE:
						ToggleMemtest();
						break;
					case KEY_CANCEL:
						ToggleLED();
						break;
					case KEY_POWER:						
						ToggleLCD();
						break;

					case KEY_CD_EJECT:
						diag_printf("eject\n");
						StopCDStress();
						pMiniCDMgr->ToggleTray();
						break;
					
					default:
						break;
				}
				break;

			default:
				break;			
		}
	}
		
}

void StartCDStress()
{
	// is cd stress stopped?
	// stop it, if not

	
	if(pMiniCDMgr->IsAudioCD())
	{
		// start audio read test
		diag_printf("audio cd\n");
		StartAudioATA();

	}
	else
	{
		// start data cd read test
		diag_printf("data cd\n");
		StartDataATA();
	}


}

void StopCDStress()
{
	// is cd stress stopped?

	// stop it, wait for it to finish
	StopATA();

}

void ToggleMemtest()
{
	if(s_bMem)
	{
		diag_printf("Mem Off\n");
		cyg_thread_suspend( threadh[4] );
		s_bMem = false;
	}
	else
	{
		diag_printf("Mem On\n");
		cyg_thread_resume( threadh[4] );
		s_bMem = true;
	}

}

void ToggleSerial()
{
	if(s_bSerial)
	{
		diag_printf("Serial Off\n");
		cyg_thread_suspend( threadh[5] );
		s_bSerial = false;
	}
	else
	{
		diag_printf("Serial On\n");
		cyg_thread_resume( threadh[5] );
		s_bSerial = true;
	}

}

void ToggleAudio()
{
	if(s_bAudio)
	{
		diag_printf("Audio Off\n");
		DAIDisable();		
		s_bAudio = false;
	}
	else
	{
		diag_printf("Audio On\n");
		// Start ADC/DAC loopback fiq
		DAIInit();
		DACSetVolume(0);    
		DAIEnable();
		DAIWrite(0);
		DAISetLoopbackFIQ();
		s_bAudio = true;
	}

}

void memtest(unsigned long nBytes)
{

	unsigned long offset;
    unsigned long nWords;
    unsigned long nErrorCount = 0;

    cyg_uint32 pattern;
    cyg_uint32 antipattern;
	cyg_uint32 test;
	cyg_uint32* baseAddress;

	

	// align byte count
	(cyg_uint32)nBytes -= ((cyg_uint32)nBytes % WORDSIZE);		

	// try and reserve memory for test
	baseAddress = (cyg_uint32*)malloc(nBytes);

	if(baseAddress == NULL)
	{
		diag_printf("Couldn't reserve memory for test\n");
		return;
	}


	diag_printf("memtest: testing %d bytes at 0x%8.8x\n",nBytes,baseAddress);
	
    nWords = nBytes / 4;

    /*
     * Fill memory with a known pattern.
     */
 
	/*
	 * Fill memory with a known pattern.
	 */
	for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
	{
		baseAddress[offset] = pattern;
	}

	/*
	 * Check each location and invert it for the second pass.
	 */
	for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
	{
		test = baseAddress[offset];
		
		if (test != pattern)
		{
			diag_printf("memtest: error at addr 0x%8.8x read 0x%8.8x, expected 0x%8.8x\n",&baseAddress[offset],test,pattern);

			nErrorCount++;
			
			if(nErrorCount == 64)
			{
				diag_printf("memtest: error count exceeded, exiting\n");
				
				free(baseAddress);
				return;
			}

		}

		antipattern = ~pattern;
		baseAddress[offset] = antipattern;
	}

	/*
	 * Check each location for the inverted pattern and zero it.
	 */
	for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
	{
		antipattern = ~pattern;
		test = baseAddress[offset];

		if (test != antipattern)
		{
			diag_printf("memtest: error at addr 0x%8.8x read 0x%8.8x, expected 0x%8.8x\n",&baseAddress[offset],test,antipattern);
			
			nErrorCount++;

			if(nErrorCount == 64)
			{

				diag_printf("memtest: error count exceeded, exiting\n");
				free(baseAddress);
				return;
			}

		}
	}

	if(nErrorCount > 0)
	{
		diag_printf("memtest: failed - %d errors detected\n",nErrorCount);
	}
	else
	{
		// diag_printf("memtest: pass\n");
	}

	free(baseAddress);
    return;

	

}


void memtest_thread(cyg_uint32 ignored)
{
	while(1)
		memtest(1024*1024*4);
}

void serial_thread(cyg_uint32 ignored)
{

    int ser2,i,done,iread;

	unsigned int buff[64];
	char inbuff[64];

	for(i = 0; i < 64; i++) buff[i] = 0xAAAA5555;

	// 9600 8N1 (done by default)
	// COM2

	// write AAAA5555 forever

    diag_printf( "Thread2: calling open(/dev/ser2)\n");
    ser2 = open("/dev/ser2", O_RDWR );
    
    if( ser2 < 0) diag_printf("open(/dev/ser2) returned error");

	

	while(1)
	{

		done = write( ser2, (char*)buff, 64*4);

		// gate writing on reading some data back 
		// (require operator to send data from teraterm for r/w)
		iread = read(ser2,inbuff,1);


		// debug display data read over serial - yes, this does work
		// diag_printf("read %d %c\n",iread,inbuff[0]);
		

		if( done != 64*4 ) diag_printf("err: done = %d\n");    
	}
    
   // FD_SET( ser1, &wr );


}

void audio_thread(cyg_uint32 ignored)
{


	int rates[] = {48000,44100,32000,22050,11025};
	int i = 0;

	while(1)
	{

		cyg_thread_delay(500);
	
		diag_printf("setting sample rate to %d\n",rates[i]);
		
		DAISetSampleFrequency(rates[i]);

		i++;
		if(i>4)
			i=0;
	}
	
}

extern "C"
{

void cyg_user_start(void)
{
   cyg_thread_create( 8, thread_entry, 0, "main thread",
                       (void*)tstack[0], STACKSIZE, &threadh[0], &thread[0]);
    cyg_thread_resume( threadh[0] );
}

};

