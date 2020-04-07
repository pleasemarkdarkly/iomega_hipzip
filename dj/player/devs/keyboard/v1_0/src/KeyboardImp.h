//
// KeyboardImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef KEYBOARDIMP_H_
#define KEYBOARDIMP_H_

#include <cyg/kernel/kapi.h>  // for cyg_vector_t

//
// fdecl
//
class CEventQueue;
typedef struct key_map_s key_map_t;

//! The keyboard driver processes keyboard interrupts, scans the keyboard
//! matrix, and generates events based on the current keymap. It allows the
//! user application to mask and unmask event generation for software key locks
class CKeyboardImp
{
public:

    CKeyboardImp();
    ~CKeyboardImp();

    //! Set a keymap for this keyboard. This can only be called once currently
    //! \param pKeymap A key_map_t defining the keyboard layout and events to generate
    void SetKeymap( const key_map_t* pKeymap );

    //! Prevent the keyboard driver from generating events
    void LockKeyboard();
    //! If the keyboard driver has been locked, unlock it
    void UnlockKeyboard();
    
private:

    // interrupt handling
    static int keyboard_isr( cyg_vector_t vector, cyg_addrword_t data );
    static void keyboard_dsr( cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data );
    
    // thread entry point
    static void keyboard_entry( unsigned int );
    void KeyboardThread();

    // scan routine to check the keyboard lines
    void KeyboardScan();

    // system event queue, we'll keep a ptr to it
    CEventQueue* m_pEventQueue;


    // thread data
    cyg_thread m_kb_data;
    cyg_handle_t m_kb_handle;
    char* m_kb_stack;

    // interrupt data
    cyg_interrupt m_kb_int;
    cyg_handle_t m_kb_int_handle;

    // thread/int semaphore
    cyg_sem_t m_kb_sem;

    // key pad state data
    cyg_uint8* key_state;
    cyg_uint8* new_key_state;
    cyg_uint8* keys_pressed;
    cyg_int32* key_tick_count;
    
    // our keymap
    const key_map_t* m_pKeymap;

    // is the button pad locked?
    bool m_bLocked;
};


#endif  // KEYBOARDIMP_H_
