//
// Keyboard.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The keyboard driver provides generic key support for the Cirrus
//! logic 7312 processor. Alternate hardware button matrices can be supported
//! by adjusting the configuration in the key_map_t structure, avoiding any
//! need to rework code for physical layout changes.

/** \addtogroup Keyboard Keyboard driver */
//@{

#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

//! The key_map_t structure specifies what events the keyboard
//! driver should trigger based on the key input. It also allows
//! the application to specify the keyboard layout and customize
//! keyboard driver details such as repeat delay and rate.
//! press_map, hold_map, and release_map should point to event
//! arrays of size num_buttons
typedef struct key_map_s 
{
    //! Number of columns on the keyboard
    int num_columns;
    //! Number of rows on the keyboard
    int num_rows;
    //! Number of actual buttons
    int num_buttons;
    //! Flags regarding key repeat behavior
    int repeat_flags;
    //! Length of a keyboard tick in 10ms units
    int tick_length;
    //! Number of ticks between events
    int repeat_rate;
    //! Number of ticks before the first event is fired
    int initial_delay;

    //! Pointer to an array of events that are fired on
    //! button press.
    const unsigned int* const press_map;
    //! Pointer to an array of events to be fired if the
    //! button is being held down.
    const unsigned int* const hold_map;
    //! Pointer to an array of events that are fired on
    //! button release.
    const unsigned int* const release_map;
} key_map_t;

//! This flag enables key repeat events from the keyboard driver
#define KEY_REPEAT_ENABLE 0x01
    
//
// fdecl
//
class CEventQueue;
class CKeyboardImp;

//! The keyboard driver processes keyboard interrupts, scans the keyboard
//! matrix, and generates events based on the current keymap. It allows the
//! user application to mask and unmask event generation for software key locks
class CKeyboard
{
public:

    //! Get a pointer to the one global instance of the keyboard driver
    static CKeyboard* GetInstance();
    static void Destroy();

    //! Set a keymap for this keyboard. This can only be called once currently
    //! \param pKeymap A key_map_t defining the keyboard layout and events to generate
    void SetKeymap( const key_map_t* pKeymap );

    //! Prevent the keyboard driver from generating events
    void LockKeyboard();
    //! If the keyboard driver has been locked, unlock it
    void UnlockKeyboard();
    
private:

    CKeyboard();
    ~CKeyboard();

    CKeyboardImp*   m_pImp;
};

//@}

#endif // __KEYBOARD_H__
