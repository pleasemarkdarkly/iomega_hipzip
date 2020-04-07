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

#ifndef _KBD_H_
#define _KBD_H_

#include <pkgconf/redboot.h>
#include <cyg/hal/basetype.h>

#define NUM_KBD_COLUMNS   8
#define NUM_KBD_ROWS      2
#define NUM_KBD_ROWS_MASK 0x03
#define NUM_KEYS          (NUM_KBD_COLUMNS * NUM_KBD_ROWS)

extern cyg_uint8 __kbd_col_state[NUM_KBD_COLUMNS];
void kbd_scan(void);


#endif // _KBD_H_
