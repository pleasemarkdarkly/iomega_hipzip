// 
// super-simple SimpleGUI calls, for displaying text to the screen
// and detecting key presses


#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <stdio.h>
#include <devs/keyboard/Keyboard.h>
#include <util/eventq/EventQueueAPI.h>/* DEFINES */
#include "testgui.h"
#include <gui/simplegui/common/UITypes.h>
#include <gui/simplegui/font/BmpFont.h>
#include <gui/simplegui/show/Show.h>
#include <gui/simplegui/screen/Screen.h>
#include <gui/simplegui/screenelem/textlabel/TextLabel.h>

extern CBmpFont Verdana12Font;

#define DEBUG(s...) diag_printf(##s)

#define UINTHREADS 1
#define UISTACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */
static cyg_handle_t _UIThreadH[UINTHREADS];
static cyg_thread _UIThread[UINTHREADS];
static char _UIThreadStack[UINTHREADS][UISTACKSIZE];

/* DEFINES */

#define DEBUG(s...) diag_printf(##s)

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* MORE STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

// simplegui elements
static CShow *g_show;
static CScreen *g_screen;
static CTextLabel *g_labeltext;
static CTextLabel *g_text;

// 16 buttons
volatile BOOL g_buttons[16];

extern "C" {

  void ToggleHD();
  void ToggleBl();
  void ToggleUSB();
  void ATATest();
  BOOL usbtest();

}; 




/* FUNCTIONS */

// keymap

static unsigned int _press_map[] = 
{
  1,2,3,4,5,6,7,8,9,
};
static unsigned int _hold_map[] = 
{
  1,2,3,4,5,6,7,8,9,
};
static unsigned int _release_map[]  =
{
  1,2,3,4,5,6,7,8,9,
};


static key_map_t key_map =
{
    num_columns  :  3,
    num_rows     :  3,
    num_buttons  :  9,
    repeat_flags : (KEY_REPEAT_ENABLE),
    tick_length  :  2,
    repeat_rate  :  6,
    initial_delay: 12,

    press_map    : (const unsigned int*const)_press_map,
    hold_map     : (const unsigned int*const)_hold_map,
    release_map  : _release_map,
};

extern "C" {


void MemTest()
{

  cyg_uint32 *test;
  cyg_uint32 temp;
  int i,count;
  
  diag_printf("Starting ram test\n");

   for(count = 0; count < 100; count++) {

  test = new cyg_uint32[400000];
  for(i = 0; i < 400000; i++) {
    test[i] = i;
    temp = test[i];
    if(temp != i) {
      diag_printf("Aaargh!\n");
    }
   }

  delete[] test;

   }

  diag_printf("Ending ram test\n");

}

static void
_KeyboardTestThread(CYG_ADDRESS Data)
{       
  //   DEBUG("+%s\n", __FUNCTION__);

    CEventQueue * EventQ = CEventQueue::GetInstance();
    //    DEBUG("EventQueue %screated\n", EventQ == 0 ? "not" : "");
    
    CKeyboard * Kbd = CKeyboard::GetInstance();
    Kbd->SetKeymap( &key_map );
    //   DEBUG("Keyboard %screated\n", Kbd == 0 ? "not" : "");    
    for (;;) {
	unsigned int Key;
	void * Data;
	EventQ->GetEvent( &Key, &Data );

#if 0
	if(Key == 1) {
	  switch((int)Data) {
	  case 1:
	    ToggleHD();
	    break;
	  case 5:
	    ToggleUSB();
	    break;
	  case 7:
	    ToggleBl();
	    break;
	  case 4:
	    MemTest();
	    break;
	  case 3:
	    usbtest();
	    break;

	  case 8:
	    ATATest();
	    break;
	  default:
	    break;
	  }
	}
#endif
	
	DEBUG("Key: %x Data: %x\n", Key, Data);
	g_buttons[((int)Data - 1)] = TRUE;

    }

    //     DEBUG("-%s\n", __FUNCTION__);
}




void InitGUI()
{

  diag_printf("gui init start\n");
  g_show = new CShow();
  g_screen = new CScreen();
  g_text = new CTextLabel();
  g_labeltext = new CTextLabel();
  // setup LCD
  g_show->AddScreen(g_screen);

  g_labeltext->SetFont(&Verdana12Font);
  g_labeltext->SetText("Dharma Factory Test");
  g_labeltext->m_clip.ul.x = 0;
  g_labeltext->m_clip.lr.x = 128;
  g_labeltext->m_clip.ul.y = 0;
  g_labeltext->m_clip.lr.y = 32;
  g_labeltext->m_start.x = 5;
  g_labeltext->m_start.y = 5;
  g_labeltext->SetVisible(GUI_TRUE);
 

  g_text->SetFont(&Verdana12Font);
  g_text->SetText("  Running tests...  ");
  g_text->m_clip.ul.x = 0;
  g_text->m_clip.lr.x = 128;
  g_text->m_clip.ul.y = 32;
  g_text->m_clip.lr.y = 64;
  g_text->m_start.x = 5;
  g_text->m_start.y = 37;
  g_text->SetVisible(GUI_TRUE);

  g_screen->AddScreenElem(g_labeltext);
  g_screen->AddScreenElem(g_text);

  g_screen->SetVisible(GUI_TRUE);
  g_show->Clear();
  g_show->Draw();

  diag_printf("gui init done\n");
  cyg_thread_create(10, _KeyboardTestThread, (cyg_addrword_t) 0, "KeyboardTestThread",
		      (void *)_UIThreadStack[0], UISTACKSIZE, &_UIThreadH[0], &_UIThread[0]);
  cyg_thread_resume(_UIThreadH[0]);

}


void GUIPrint(const char * szText)
{

  // update text element
  g_text->SetText(szText);
  diag_printf("%s\n",szText);

  // redraw screen
  // g_show->Clear();
  g_show->Draw();

}

void ResetKeys() {
  int i;
  // reset the table of keys 
  for(i = 0; i < 16; i++) {
    g_buttons[i] = FALSE;
  }

}

BOOL KeyPressed(int key) {
  // if that key has been pressed, return true
  return g_buttons[key];
}

};
