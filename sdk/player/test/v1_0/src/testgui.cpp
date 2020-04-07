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
#include <devs/ir/IR.h>

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
volatile BOOL g_buttons[64];


/* FUNCTIONS */

// keymap
#if __DHARMA == 2
static unsigned int _press_map[] = 
{
    1,2,3,4,5,6,9,10,11,12,13,14,7,8,15,16
};
static unsigned int _hold_map[] = 
{
    1,2,3,4,5,6,9,10,11,12,13,14,7,8,15,16
};
static unsigned int _release_map[] = 
{
    1,2,3,4,5,6,9,10,11,12,13,14,7,8,15,16
};
#else /* __DHARMA == 1 */
static unsigned int _press_map[] =
{
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
};
static unsigned int _hold_map[]  =
{
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
};
static unsigned int _release_map[]  =
{
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
};
#endif

static key_map_t key_map =
{
#if __DHARMA == 2
    num_columns  :  3,
    num_rows     :  6,
    num_buttons  : 16,
#else /* __DHARMA == 1 */
    num_columns  :  8,
    num_rows     :  2,
    num_buttons  : 16,
#endif
    
    repeat_flags : (KEY_REPEAT_ENABLE),

    tick_length  :  2,
    repeat_rate  :  6,
    initial_delay: 12,

    press_map    : (const unsigned int*const)_press_map,
    hold_map     : (const unsigned int*const)_hold_map,
    release_map  : _release_map,
};

const unsigned int ir_press_map[] = 
{
    17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
};


const ir_map_t ir_map = 
{
    num_buttons: 2,
    repeat_flags: IR_REPEAT_ENABLE,
    filter_start: 5,  // 0 == unfiltered
    filter_rate:  2,  // 0 == unfiltered
    press_map: ir_press_map,
    hold_map: ir_press_map
};

extern "C" {

static void
_KeyboardTestThread(CYG_ADDRESS Data)
{       
  //   DEBUG("+%s\n", __FUNCTION__);

    CEventQueue * EventQ = CEventQueue::GetInstance();
    //    DEBUG("EventQueue %screated\n", EventQ == 0 ? "not" : "");
    
    CKeyboard * Kbd = CKeyboard::GetInstance();
    Kbd->SetKeymap( &key_map );

	CIR * pIR = CIR::GetInstance();
    DEBUG("IR %screated\n", pIR == 0 ? "not" : "");
    pIR->SetIRMap( &ir_map );
 
    //   DEBUG("Keyboard %screated\n", Kbd == 0 ? "not" : "");    
    for (;;) {
	unsigned int Key;
	void * Data;
	EventQ->GetEvent( &Key, &Data );
	DEBUG("Key: %x Data: %x\n", Key, Data);
	g_buttons[((int)Data - 1)] = TRUE;

    }

    //     DEBUG("-%s\n", __FUNCTION__);
}




void InitGUI()
{

  g_show = new CShow();
  g_screen = new CScreen();
  g_text = new CTextLabel();
  g_labeltext = new CTextLabel();
  // setup LCD
  g_show->AddScreen(g_screen);

  g_labeltext->SetFont(&Verdana12Font);
  g_labeltext->SetText("Dharma Factory Test");
  g_labeltext->m_clip.ul.x = 5;
  g_labeltext->m_clip.lr.x = 319;
  g_labeltext->m_clip.ul.y = 5;
  g_labeltext->m_clip.lr.y = 49;
  g_labeltext->m_start.x = 5;
  g_labeltext->m_start.y = 5;
  g_labeltext->SetVisible(GUI_TRUE);
 

  g_text->SetFont(&Verdana12Font);
  g_text->SetText("  Running tests...  ");
  g_text->m_clip.ul.x = 5;
  g_text->m_clip.lr.x = 319;
  g_text->m_clip.ul.y = 50;
  g_text->m_clip.lr.y = 100;
  g_text->m_start.x = 5;
  g_text->m_start.y = 50;
  g_text->SetVisible(GUI_TRUE);

  g_screen->AddScreenElem(g_labeltext);
  g_screen->AddScreenElem(g_text);

  g_screen->SetVisible(GUI_TRUE);
  g_show->Clear();
  g_show->Draw();

  cyg_thread_create(10, _KeyboardTestThread, (cyg_addrword_t) 0, "KeyboardTestThread",
		      (void *)_UIThreadStack[0], UISTACKSIZE, &_UIThreadH[0], &_UIThread[0]);
  cyg_thread_resume(_UIThreadH[0]);

}


void GUIPrint(const char * szText)
{

  // update text element
  diag_printf("%s\n",szText);
  g_text->SetText(szText);

  // redraw screen
  // g_show->Clear();
  g_show->Draw();

}

void ResetKeys() {
  int i;
  // reset the table of keys 
  for(i = 0; i < 64; i++) {
    g_buttons[i] = FALSE;
  }

}

BOOL KeyPressed(int key) {
  // if that key has been pressed, return true
  return g_buttons[key];
}

};
