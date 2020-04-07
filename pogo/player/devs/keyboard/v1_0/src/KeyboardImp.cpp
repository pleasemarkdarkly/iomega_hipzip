// KeyboardImp.cpp: keyboard (button pad) interface
// danc@iobjects.com 07/13/01
// (c) Interactive Objects

#include "KeyboardImp.h"
#include <devs/keyboard/Keyboard.h>

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_diag.h>
#include <devs/keyboard/Keyboard.h>
#include <cyg/hal/drv_api.h>
#include <string.h>   // memset

#include <util/eventq/EventQueueAPI.h>
#include <core/events/SystemEvents.h>

#if 0
#include <cyg/infra/diag.h>
#endif

#define KEYBOARD_THREAD_PRIORITY   3
#define KEYBOARD_THREAD_STACK_SIZE 4096

//
// CKeyboardImp implementation
//

CKeyboardImp::CKeyboardImp() 
{
    m_bLocked = false;
    m_pKeymap = NULL;
    m_kb_stack = new char[KEYBOARD_THREAD_STACK_SIZE];
    
    cyg_thread_create( KEYBOARD_THREAD_PRIORITY, CKeyboardImp::keyboard_entry, (cyg_addrword_t)this,
                       "Keyboard", (void*) m_kb_stack, KEYBOARD_THREAD_STACK_SIZE, &m_kb_handle, &m_kb_data );
    cyg_thread_resume( m_kb_handle );
}

CKeyboardImp::~CKeyboardImp()
{
    // TODO: kill keyboard thread, release resources
    cyg_thread_suspend( m_kb_handle );
    while( !cyg_thread_delete( m_kb_handle ) ) {
        cyg_thread_delay( 1 );
    }
    delete [] m_kb_stack;

    delete [] keys_pressed; delete [] key_state; delete [] new_key_state; delete [] key_tick_count;
    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_KBDINT);
}

void CKeyboardImp::SetKeymap( const key_map_t* pKeymap ) 
{
    // to avoid thread safety issues, dont allow the key map to be changed
    // "dynamically"
    if( m_pKeymap != NULL ) {
        return ;
    }

    int size = pKeymap->num_buttons;
    
    keys_pressed   = new cyg_uint8[ pKeymap->num_columns ];
    key_state      = new cyg_uint8[ size ];
    new_key_state  = new cyg_uint8[ size ];
    key_tick_count = new cyg_int32[ size ];

    memset( (void*) keys_pressed,   0, pKeymap->num_columns * sizeof( cyg_uint8 ) );
    memset( (void*) key_state,      0, size * sizeof( cyg_uint8 ) );
    memset( (void*) new_key_state,  0, size * sizeof( cyg_uint8 ) );
    memset( (void*) key_tick_count, pKeymap->initial_delay, size * sizeof( cyg_int32 ) );


    m_pEventQueue = CEventQueue::GetInstance();
    
    m_pKeymap = pKeymap;
}

void CKeyboardImp::LockKeyboard()
{
    m_bLocked = true;
}
void CKeyboardImp::UnlockKeyboard()
{
    m_bLocked = false;
}


//
// ISR, DSR, and our friendly keyboard scan routine
//
int CKeyboardImp::keyboard_isr( cyg_vector_t vector, cyg_addrword_t data ) 
{
    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_KBDINT);
#if 0
    // Last ditch effort at getting some info from the device after it's hung
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
        SYSCON1_KBD_LOW;
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
                                      SYSCON1_KBD_COL(0);
    cyg_uint8 keys_pressed          = *(volatile cyg_uint8 *)PADR;
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
        SYSCON1_KBD_HIGH;

    if (keys_pressed == 0x02) {
        diag_printf("BREAKPOINT\n");
        HAL_BREAKPOINT(_breakinst);
    }
#endif
    return (CYG_ISR_HANDLED|CYG_ISR_CALL_DSR); // Run the DSR
}

void CKeyboardImp::keyboard_dsr( cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data )
{
    CKeyboardImp* p = reinterpret_cast<CKeyboardImp*>(data);
    cyg_semaphore_post( &(p->m_kb_sem) );
}

#define NUM_KEYS    NUM_BUTTONS

void CKeyboardImp::KeyboardScan(void)
{
    // Turn off drive (de-select) all columns
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
        SYSCON1_KBD_LOW;

    for (int Column = 0; Column < m_pKeymap->num_columns; ++Column) {

	// Select column 'i'
	*(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
	    SYSCON1_KBD_COL(Column);
	
	// Small pause to let the wires charge up :-)
	//	_KeyboardDelay();
	hal_delay_us( 250 );
	
	// Grab the data
	keys_pressed[Column] = *(volatile cyg_uint8 *)PADR;

	// De-Select column 'i'
	*(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
	    SYSCON1_KBD_TRISTATE;
    
	// Allow line to go slack
	//	_KeyboardDelay();
	hal_delay_us( 250 );
    }

    // Turn on drive (select) all columns - necessary to see interrupts
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) |
        SYSCON1_KBD_HIGH;
}

// Thread entry point
void CKeyboardImp::keyboard_entry( cyg_addrword_t data ) 
{
    reinterpret_cast<CKeyboardImp*>(data)->KeyboardThread();
}

// class instance entry point
void CKeyboardImp::KeyboardThread() 
{
    cyg_semaphore_init( &m_kb_sem, 0 );

    cyg_drv_interrupt_create( CYGNUM_HAL_INTERRUPT_KBDINT,
                              5,
                              (cyg_addrword_t) this,
                              (cyg_ISR_t*) CKeyboardImp::keyboard_isr,
                              (cyg_DSR_t*) CKeyboardImp::keyboard_dsr,
                              &m_kb_int_handle,
                              &m_kb_int );
    
    cyg_drv_interrupt_attach( m_kb_int_handle );
    
    while( 1 ) {

        // do this in the loop, so if we continue because there is no keymap we
        // still get this. additionally we need to unmask the interrupt or the
        // semaphore will never be posted to
        cyg_drv_interrupt_acknowledge( CYGNUM_HAL_INTERRUPT_KBDINT );
        cyg_drv_interrupt_unmask( CYGNUM_HAL_INTERRUPT_KBDINT );
        
        cyg_semaphore_wait( &m_kb_sem );

        // if we wake up, make sure we have a keymap before we actually
        // try to process anything
        if( m_pKeymap == NULL ) {
            // Wait for 20ms - time to debounce keyboard
            cyg_thread_delay(2);
            continue;
        }

        // As long as keys are pressed, scan and update
        int Timeout = 0;
        for (;;) {
      
            // Wait for 20ms - time to debounce keyboard
            cyg_thread_delay(2);
      
            // Wait to scan the keyboard until the event queue has less than 10 events
            // already queued up.  We do this in an effort to keep the event queue
            // from filling up b/c bad things happen (dead locks, race conditions, etc)
            // when it's full.
            if (m_pEventQueue->FullCount() < 10)
            {
                KeyboardScan();
      
                // Reset state
                for (int Key = 0; Key < m_pKeymap->num_buttons; ++Key) {
                    new_key_state[Key] = 0;
                }
      
                // Update state
                for (int Column = 0; Column < m_pKeymap->num_columns; ++Column) {
                    for (int Row = 0; Row < m_pKeymap->num_rows; ++Row) {
                        if (keys_pressed[Column] & (1 << Row)) {
                            int Key = (Column * m_pKeymap->num_rows) + Row;
                            new_key_state[Key] = 1;
                        }
                    }
                }
      
                // dont bother generating keyboard events while locked
                if( m_bLocked ) {
                    for( int Key = 0; Key < m_pKeymap->num_buttons; ++Key ) {
                        key_state[Key] = new_key_state[Key];
                    }
                } else {
                    // Process key
                    for (int Key = 0; Key < m_pKeymap->num_buttons; ++Key) {
                        if (key_state[Key] != new_key_state[Key]) {
                            key_state[Key] = new_key_state[Key];
	      
                            if( key_state[Key] && m_pKeymap->press_map[ Key ] ) {
                                // If key pressed
                                if( m_pKeymap->hold_map[ Key ] ) {
                                    key_tick_count[ Key ] = m_pKeymap->initial_delay;
                                }
                                m_pEventQueue->PutEvent( EVENT_KEY_PRESS,
                                                         (void*)m_pKeymap->press_map[ Key ] );
                            } else if( !key_state[Key] && m_pKeymap->release_map[ Key ] ) {
                                // If key released
                                key_tick_count[Key] = m_pKeymap->initial_delay;
                                m_pEventQueue->PutEvent( EVENT_KEY_RELEASE,
                                                         (void*)m_pKeymap->release_map[ Key ] );
                            }
                        } else if( key_state[ Key ] && m_pKeymap->hold_map[ Key ] ) {
                            // If key hold
                            if( --key_tick_count[ Key ] <= 0  &&
                                ((-key_tick_count[Key]) % m_pKeymap->repeat_rate) == 0) {
                                m_pEventQueue->PutEvent( EVENT_KEY_HOLD,
                                                         (void*)m_pKeymap->hold_map[ Key ] );
                            }
                        }
                    } // Process key
                }
            }
      
            // Clear interrupt (true when keys are pressed)
            cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_KBDINT);
      
            if (*(volatile cyg_uint32 *)INTSR2 & INTSR2_KBDINT) {
                Timeout = 0;
            } else if (++Timeout == 5) {
	
                // No keys for 100ms
                break;
            }
            cyg_thread_delay( m_pKeymap->tick_length );   // loop control
        }
    }
}

