#include <redboot.h>
#include "demojumper/kbd_server.h"
#include "demojumper/debug.h"
#include "demojumper/GumiScreen.h"
#include "demojumper/gui_state.h"
#include <kbd.h>

#define MAX_SCRIPT_SIZE 1024

UCHAR g_ucCurrent = 0;
UCHAR g_ucHighlighted = 0;
UCHAR g_ucTop = 0;
UCHAR g_ucBottom = 0;
char* g_psMenu[MENU_ITEM_COUNT];
cyg_uint8 _NewKeyState[NUM_KEYS];
cyg_uint8 _KeyState[NUM_KEYS];
static char m_sScript[MAX_SCRIPT_SIZE];

static short m_cSeqStep = 0;	// how many keypresses have we received in the sequence?
#define KBD_SEQ_1			6
#define KBD_SEQ_2			15
#define KBD_SEQ_3			14
#define KBD_SEQ_4			7

#define FIS_DUMMY_IMAGE_NAME "dont_delete_me"
#define LOAD_STR "fis delete " FIS_DUMMY_IMAGE_NAME "\nload -v -m xmodem" // cmd string to pause, then initiate xmodem xfer
#define CREATE_FIS_COMMON_STR "fis create -b 0x10000 -e 0x10044 "
#define CREATE_DUMMY_STR CREATE_FIS_COMMON_STR "-l 0x20000 -f 0xe07c0000 " FIS_DUMMY_IMAGE_NAME
#define CREATE_FIS_PATTERN "%s%x %s%x %s"

void
KeyboardProc(void)
{
	int Key, Column, Row;
	static int iFirst = 1;			
	
	if (iFirst)
	{
		iFirst = 0;
	    for (Key = 0; Key < NUM_KEYS; ++Key) {
			_KeyState[Key] = 0;
		}
	}
	
	kbd_scan();

	// Reset state
	for (Key = 0; Key < NUM_KBD_KEYS; ++Key) {
		_NewKeyState[Key] = 0;
	}
	
	// Update state
	for (Column = 0; Column < NUM_KBD_COLUMNS; ++Column) {
		for (Row = 0; Row < NUM_KBD_ROWS; ++Row) {
			if (__kbd_col_state[Column] & (1 << Row)) {
				Key = (Column * NUM_KBD_ROWS) + Row;
				_NewKeyState[Key] = 1;
			}
		}
	}

	// Process key
	for (Key = 0; Key < NUM_KEYS; ++Key) {
		if (_KeyState[Key] != _NewKeyState[Key]) {
			_KeyState[Key] = _NewKeyState[Key];
			// If key pressed
			if (_KeyState[Key]) {
				switch(Key) {
					case (KBD_MENU_UP): // up
						//SHUNT_PRINT("KBD_Up\n");
						// if we're not already at the top
						if(g_ucCurrent > 0){
							if(g_ucHighlighted > 0){
								g_ucCurrent--;
								g_ucHighlighted--;
							}
							else{
								int i;
								g_ucCurrent--;
								g_ucTop--;
								g_ucBottom--;
								// push everything down
								for (i = MENU_ITEM_COUNT-1; i > 0; --i)
								{
									g_psMenu[i] = g_psMenu[i-1];
								}
								g_psMenu[0] = demo_names[g_ucCurrent];
							}
						}
						GumiScreen_DrawMenu();
						break;
					case (KBD_MENU_DOWN): // down
						//SHUNT_PRINT("KBD_Down\n");
						// if we're not already at the bottom
						if(g_ucCurrent < (DEMO_COUNT - 1)){
							if(g_ucHighlighted < MENU_ITEM_COUNT-1){
								g_ucCurrent++;
								g_ucHighlighted++;
							}
							else{
								int i;
								g_ucCurrent++;
								g_ucTop++;
								g_ucBottom++;
								// push everything up
								for (i = 0; i < MENU_ITEM_COUNT - 1; ++i)
								{
									g_psMenu[i] = g_psMenu[i+1];
								}
								g_psMenu[MENU_ITEM_COUNT-1] = demo_names[g_ucCurrent];
							}
						}
						GumiScreen_DrawMenu();
						break;
					case (KBD_MENU_SELECT): // menu/select
						// todo : orchestrate redboot loading of image here
						//load_demo(g_ucCurrent);
						//reset_device();
						//SHUNT_PRINT("KBD_Select\n");
						//SHUNT_PRINT("loading test script\n");
						sprintf(m_sScript,"fis load -b 0x10000 %s\ngo\n",demo_aliases[g_ucCurrent]);
						script = m_sScript;
						break;
					case (KBD_SEQ_1):
						if (m_cSeqStep==0)
							m_cSeqStep=1;
						else
							m_cSeqStep=0;
						break;
					case (KBD_SEQ_2):
						if (m_cSeqStep==1)
							m_cSeqStep=2;
						else
							m_cSeqStep=0;
						break;
					case (KBD_SEQ_3):
						if (m_cSeqStep==2)
							m_cSeqStep=3;
						else
							m_cSeqStep=0;
						break;
					case (KBD_SEQ_4):
						if (m_cSeqStep==3)
						{
							int script_len = 0;
							int i;
							// sequence complete
							// can't think of how to simply make this not depend on the # demos.  hard to iterate in the middle of constructing
							// a string, so I just iterate manually (unrolled loopage).  the only thing I could think to do it would be to track
							// the script length and iterate through strcpy'ing to the end of the string....

							printf("sequence code active, supply images in following order:\n");
							printf("1: codec_image\n");
							for (i = 0; i < DEMO_COUNT; ++i)
							{
								printf("%d: %s\n", i+2, demo_names[i]);
							}
							
							memset((void*) m_sScript, 0, MAX_SCRIPT_SIZE*sizeof(char));	// strlen will be accurate hence unless we overflow the script buffer
						
							// erase any previously installed images
							sprintf (m_sScript, "fis init\n");
							script_len = strlen(m_sScript);
			
							// create a dummy image in rom that just serves to allow us to prompt the user to delete it, to punctuate the script behavior
							sprintf (m_sScript+script_len, CREATE_DUMMY_STR "\n");
							script_len = strlen(m_sScript);
							// load the codecs into ram
							sprintf(m_sScript+script_len,LOAD_STR " -r -b 0x10000\n");	// std load, plus flags to load raw data (non-srec) to 0x10000
							script_len = strlen(m_sScript);
							// burn the codecs down to rom
							sprintf(m_sScript+script_len,"fis create -b 0x10000 -f 0x%x -l 0x%x codecs\n",CODEC_ROM_START,CODEC_ROM_LEN); 
							script_len = strlen(m_sScript);
							// load the demo srecs
							for (i = 0; i < DEMO_COUNT; ++i)
							{
								sprintf(m_sScript+script_len, LOAD_STR "\n" CREATE_FIS_PATTERN "\n",
									CREATE_FIS_COMMON_STR "-l 0x", demo_lens[i], " -f 0x", demo_flash_bases[i], demo_aliases[i]);
								script_len = strlen(m_sScript);
							}
							//printf("constructed script:\n");
							//printf("%s",m_sScript);
							//printf("running script...\n");
							script = m_sScript;																
							m_cSeqStep=0;
						}
						else
							m_cSeqStep=0;
						break;
				default:
					{
						//SHUNT_PRINT("unhandled key %d\n", Key);
						break;
					}
				}
				
			}
		}
	}
}
