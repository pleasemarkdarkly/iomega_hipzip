//
// IRImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "IRImp.h"


#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <util/eventq/EventQueueAPI.h>
#include <core/events/SystemEvents.h>
#include <util/debug/debug.h>
#include <devs/ir/IR.h>

DEBUG_USE_MODULE(IR);

//#define DEBUG_PW  /* This is useful for debugging */

#define IR_THREAD_PRIORITY 4 /* TODO Move this to the correct place */
#define IR_THREAD_STACK_SIZE 4096

// Timer data
cyg_uint32 CIRImp::TC1Tick = 0;

cyg_sem_t CIRImp::IRSem;
int CIRImp::m_iItemCount = 0;
CIRImp::ir_queue_t CIRImp::m_Queue;
CIRImp::ir_item_t CIRImp::m_CurrentItem;
CIRImp * CIRImp::m_pSingleton = NULL;

CIRImp * CIRImp::GetInstance() 
{
    if (m_pSingleton == NULL) {
        m_Queue.init();
	m_pSingleton = new CIRImp;
    }
    return m_pSingleton;
}

CIRImp::CIRImp()
{
    // init this prior to starting the thread
    m_pIRMap = NULL;
    UnlockIR();
    
    /* Create thread */
    Stack = new char[IR_THREAD_STACK_SIZE];
    
    cyg_thread_create(IR_THREAD_PRIORITY,
		      CIRImp::IRThreadEntry,
		      (cyg_addrword_t)this,
		      "IR Imp",
		      (void *)Stack,
		      IR_THREAD_STACK_SIZE,
		      &ThreadHandle,
		      &Thread);
    cyg_thread_resume(ThreadHandle);
}

CIRImp::~CIRImp()
{
    /* TODO Kill thread, release interrupts, etc */
}

cyg_uint32
CIRImp::TC1ISR(cyg_vector_t Vector, cyg_addrword_t Data)
{
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_TC1OI);
    ++TC1Tick;
    return (CYG_ISR_HANDLED);
}

bool
CIRImp::TC1Init(cyg_uint32 Period)
{
    volatile cyg_uint32 * syscon1 = (volatile cyg_uint32 *)SYSCON1;
    volatile cyg_uint32 * tc1d = (volatile cyg_uint32 *)TC1D;
    
    /* Set timer to 512KHz, prescale mode */
    *syscon1 = (*syscon1 & ~(SYSCON1_TC1M|SYSCON1_TC1S)) | SYSCON1_TC1S | SYSCON1_TC1M;
    
    /* Initialize counter */
    *tc1d = Period;

    cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_TC1OI,
			     1,
			     (cyg_addrword_t)0,
			     TC1ISR,
			     0,
			     &TC1InterruptHandle,
			     &TC1Interrupt);
    cyg_drv_interrupt_attach(TC1InterruptHandle);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_TC1OI);

    return true;
}

#ifdef DEBUG_PW
int PWTab[1000];
int PWTabI = 0;
#endif

#define MS9(x)    ((70 <= (x)) && ((x) <= 74))
#define MS4_5(x)  ((33 <= (x)) && ((x) <= 36))
#define MS2_25(x) ((16 <= (x)) && ((x) <= 20))
#define MS1_69(x) ((11 <= (x)) && ((x) <= 14))
#define MS0_565(x)(( 2 <= (x)) && ((x) <=  6))
#define MS0_56(x) (( 2 <= (x)) && ((x) <=  6))

cyg_uint32
CIRImp::MSISR(cyg_vector_t Vector, cyg_addrword_t Data)
{
    static cyg_uint32 StartTick = 0;
    static cyg_uint32 CurrentTick = 0;
    static int State = 0;
    static cyg_uint8 Byte = 0;
    static cyg_uint8 Flame[4]; /* The whole encoding of a new button press */
    static int Bit = 0;
    static int NumBytes = 0;
    
    cyg_uint32 Status = CYG_ISR_HANDLED;
    cyg_uint32 PulseWidth;

    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_UMSINT);

    /* Get width of pulse and save offset to next edge */
    CurrentTick = TC1Tick;
    PulseWidth = CurrentTick - StartTick;

    if( PulseWidth < 0 ) {
        // handle tick overflow, probably
        // the idea here is the PulseWidth is the distance from CurrentTick down to 0
        //  plus the distance from StartTick up to 0.
        PulseWidth = (CurrentTick) + (*(int*)&StartTick);
    }
    
#ifdef DEBUG_PW
    PWTab[PWTabI++] = PulseWidth;
    if (PWTabI == sizeof(PWTab)) {
	for(PWTabI = 0; PWTabI < sizeof(PWTab); ++PWTabI) {
	    DEBUG(IR, DBGLEV_WARNING, "%08x %d\n", PWTab[PWTabI], PWTab[PWTabI]);
	}
	for (;;)
	    ;
    }
#endif
    
    StartTick = CurrentTick;
    
    switch (State) {
	case 0: 
	{
	    /* Transition */
	    if (MS9(PulseWidth)) {
		State = 1;
	    }
	    break;
	}

	case 1: 
	{
	    /* Transition */
	    if (MS4_5(PulseWidth)) {
		State = 2;
	    }
	    else if (MS2_25(PulseWidth)) {
		State = 3;
	    }
	    break;    
	}

	case 2: 
	{
	    /* Transition */
	    if (MS0_56(PulseWidth)) {
		State = 4;
            }
            
	    break;
	}

	case 3: 
	{
	    /* Transition */
	    if (MS0_56(PulseWidth)) {
		State = 0;
	    }

	    /* Action */
	    /* Key repeat */
            m_CurrentItem.uiEventType = EVENT_KEY_HOLD;
#if 1
            m_Queue.push( &m_CurrentItem );
            m_iItemCount++;
#endif
	    Status |= CYG_ISR_CALL_DSR;
	    break;
	}

	case 4: 
	{
	    /* Action */
	    if (MS0_565(PulseWidth)) {
		Byte |= (0 << Bit);
	    }
	    else if (MS1_69(PulseWidth)) {
		Byte |= (1 << Bit);
	    }
	    Bit++;

	    /* Transition */
	    if (Bit < 8) {
		State = 2;
	    }
	    else {
		Flame[NumBytes] = Byte;
		Byte = 0;
		Bit = 0;
		++NumBytes;
		if (NumBytes < 4) {
		    State = 2;
		}
		else {
		    if ((Flame[0] == 0x6e) && (Flame[1] == 0x91) && (Flame[2] == (cyg_uint8)~Flame[3])) {
                        m_CurrentItem.uiEventType = EVENT_KEY_PRESS;
                        m_CurrentItem.uiKeyData = (cyg_uint32) Flame[2];
#if 1
                        m_Queue.push( &m_CurrentItem );
                        m_iItemCount++;
#endif
                        Status |= CYG_ISR_CALL_DSR;
		    }
		    NumBytes = 0;
		    State = 0;
		}
	    }
	    break;
	}
    }
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_UMSINT);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_UMSINT);
    return Status;
}

void
CIRImp::MSDSR(cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data)
{
#if 0
    if (Count > 1) {
	DEBUG( IR, DBGLEV_ERROR, "FIXME Lost %d button presses\n", Count );
    }

    m_Queue.push( &m_CurrentItem );
    cyg_semaphore_post(&IRSem);
#else
    while( Count > 0 ) {
        Count--;
        cyg_semaphore_post(&IRSem);
    }
#endif
}

bool
CIRImp::MSInit(void)
{
    cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_UMSINT,
			     1,
			     (cyg_addrword_t)0,
			     MSISR,
			     MSDSR,
			     &MSInterruptHandle,
			     &MSInterrupt);
    cyg_drv_interrupt_attach(MSInterruptHandle);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_UMSINT);
    
    return true;
}

void
CIRImp::IRThreadEntry(cyg_addrword_t Data)
{
    reinterpret_cast<CIRImp *>(Data)->IRThread();
}

void
CIRImp::IRThread(void)
{
    m_pEventQueue = CEventQueue::GetInstance();
    
    cyg_semaphore_init(&IRSem, 0);
    TC1Init(64); /* .125ms period */
    MSInit();

    for (;;) {
        ir_item_t* p;
        unsigned int uiEventData = 0;
        
	/* Wait for complete byte from IR */
	cyg_semaphore_wait(&IRSem);

        p = m_Queue.pop();

        if( p && !m_bLocked && m_pIRMap ) {
            if( p->uiEventType == EVENT_KEY_PRESS ) {
                // always generate press
                uiEventData = m_pIRMap->press_map[ p->uiKeyData ];
            }
            else if( p->uiEventType == EVENT_KEY_HOLD &&
                     (m_pIRMap->repeat_flags & IR_REPEAT_ENABLE) ) {
                // for hold, look at the KeyFilter and determine if we
                // should trigger an event
                if( m_pKeyFilters[ p->uiKeyData ] == 0 ) {
                    uiEventData = m_pIRMap->hold_map[ p->uiKeyData ];
                    if( uiEventData ) {
                        // if there's an event, reset the filter
                        m_pKeyFilters[ p->uiKeyData ] = m_pIRMap->filter_rate;
                    }
                }
                else {
                    m_pKeyFilters[ p->uiKeyData ]--;
                }
            }

            if( m_uiLastEventData != uiEventData ||
                m_uiLastEventType != p->uiEventType ) {
                // unfortunately we get no key release with IR,
                // so step through the map and reset all the holds :/
                for( int i = 0; i < m_pIRMap->num_buttons; i++ ) {
                    if( m_pIRMap->hold_map [ i ] ) {
                        m_pKeyFilters[ i ] = m_pIRMap->filter_start;
                    }
                }
            }
            
            // ignored key events have a '0' in the map
            if( uiEventData ) {
                m_pEventQueue->PutEvent( p->uiEventType, (void *)uiEventData );
            }
            m_uiLastEventType = p->uiEventType;
            m_uiLastEventData = uiEventData;
        }
    }
}

void
CIRImp::SetIRMap( const ir_map_t* p ) 
{
    if( !m_pIRMap ) {
        m_pIRMap = p;
        m_pKeyFilters = new unsigned int[ m_pIRMap->num_buttons ];
        
        for( int i = 0; i < m_pIRMap->num_buttons; i++ ) {
            // if we trigger hold events for this key
            if( m_pIRMap->hold_map[ i ] ) {
                // set the initial filter up
                m_pKeyFilters[ i ] = m_pIRMap->filter_start;
            }
        }
    }
}

void
CIRImp::LockIR() 
{
    m_bLocked = true;
}

void
CIRImp::UnlockIR() 
{
    m_bLocked = false;
}

