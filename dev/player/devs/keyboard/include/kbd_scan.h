//==========================================================================
//
//      kbd.h
//
//      Keyboard scan support for RedBoot
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    toddm
// Contributors: toddm
// Date:         2001-03-14
// Purpose:      
// Description:  
//              
//####DESCRIPTIONEND####
//
//==========================================================================

#include <cyg/hal/basetype.h>
// #include <cyg/hal/hal_if.h>
#include <cyg/hal/hal_tables.h>

#if defined(__cplusplus)
extern "C" {
#endif


#if defined(__DJ)
#define NUM_KBD_COLUMNS   4
#define NUM_KBD_ROWS      4
#define NUM_KBD_ROWS_MASK 0x0F
#define NUM_KEYS          (NUM_KBD_COLUMNS * NUM_KBD_ROWS)
#elif defined(__DHARMA_V2)
#define NUM_KBD_COLUMNS   3
#define NUM_KBD_ROWS      6
#define NUM_KBD_ROWS_MASK 0x3F
#define NUM_KEYS          (NUM_KBD_COLUMNS * NUM_KBD_ROWS)
#else
#define NUM_KBD_COLUMNS   8
#define NUM_KBD_ROWS      2
#define NUM_KBD_ROWS_MASK 0x03
#define NUM_KEYS          (NUM_KBD_COLUMNS * NUM_KBD_ROWS)
#endif

extern cyg_uint8 __kbd_col_state[NUM_KBD_COLUMNS];
void kbd_scan(void);


#if defined(__cplusplus)
}
#endif

