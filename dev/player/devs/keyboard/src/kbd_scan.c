//==========================================================================
//
//      kbd.c
//
//      RedBoot - KBD scan support
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

#include <devs/keyboard/kbd_scan.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/infra/diag.h>

cyg_uint8 __kbd_col_state[NUM_KBD_COLUMNS];

static void
kbd_delay(void)
{
    volatile int i;
    for (i = 0;  i < 250;  i++) ;
}
 
void
kbd_scan(void)
{
    int i;
    // Turn off drive (de-select) all columns
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
        SYSCON1_KBD_LOW;
    for (i = 0;  i < NUM_KBD_COLUMNS; i++) {
        // Select column 'i'
        *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
            SYSCON1_KBD_COL(i);
        // Small pause to let the wires charge up :-)
        kbd_delay();
        // Grab the data
        __kbd_col_state[i] = *(volatile cyg_uint8 *)PADR & NUM_KBD_ROWS_MASK;
        // De-Select column 'i'
        *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
            SYSCON1_KBD_TRISTATE;
        // Allow line to go slack
        kbd_delay();
    }
    // Turn on drive (select) all columns - necessary to see interrupts
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
        SYSCON1_KBD_HIGH;


//    for (i = 0; i < NUM_KBD_COLUMNS; i++) {
//		diag_printf("KBD Column[%d]: %x\n", i, __kbd_col_state[i]);
//    }

}
