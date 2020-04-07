//
// IR.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

/** \addtogroup CommercialIR Commercial IR */
//@{

#ifndef __IR_H__
#define __IR_H__


//! The ir_map_t structure specifies what events the
//! IR driver should generate for various button
//! presses it receives. This allows an application
//! to mask changes in the physical button layout of a
//! remote by simply shuffling the order of the button
//! mapping. Additionally it provides configuration for
//! repeat events.
//! The IR driver does not support release events.

typedef struct ir_map_s 
{
    //! Number of buttons on the remote
    int num_buttons;
    //! Flags regarding button repeat behavior
    int repeat_flags;
    //! Number of repeat events to drop initially before
    //! actually generating the event
    int filter_start;
    //! Rate at which to filter repeat events. If the
    //! remote triggers repeat events too fast, raising
    //! this number will cause the driver to ignore more
    //! hardware events
    int filter_rate;
    
    //! Pointer to an array of events that are fired on
    //! button press.
    const unsigned int* const press_map;
    //! Pointer to an array of events that are fired on
    //! button hold.
    const unsigned int* const hold_map;
} ir_map_t;

//! This flag enables button repeat events from the
//! ir driver
#define IR_REPEAT_ENABLE 0x01


class CIRImp;
class CEventQueue;

//! CIR is the singleton IR driver object
class CIR
{
  public:
    //! Get a pointer to the driver instance
    static CIR * GetInstance();

    //! Set an IR map for the driver. This can only be called
    //! once currently.
    void SetIRMap( const ir_map_t* pIRmap );

    //! Prevent the IR driver from generating events
    void LockIR();

    //! If the IR driver has been locked, unlock it
    void UnlockIR();
    
    
  private:
    static CIR * m_pSingleton;
    CIR();
    ~CIR();

    CIRImp* m_pIRImp;
};

//@}

#endif /* __IR_H__ */
