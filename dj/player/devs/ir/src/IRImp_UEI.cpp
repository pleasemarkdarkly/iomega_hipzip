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

#include "IRImp_UEI.h"


#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <util/eventq/EventQueueAPI.h>
#include <core/events/SystemEvents.h>
#include <util/debug/debug.h>
#include <devs/ir/IR_UEI.h>
#include <util/timer/Timer.h>
#include <cyg/kernel/kapi.h>

DEBUG_USE_MODULE(IR);

// #define DEBUG_MISS /* used for checking timing widths */
// #define DEBUG_PW  /* This is useful for debugging */

#define IR_THREAD_PRIORITY 3 
#define IR_THREAD_STACK_SIZE 8192

#define PW_SPEED(x) (x)

#if (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000)
#warning 90mhz timings enabled
#define PW_ADJUST(x) ((x*74)/90) // tweak numbers for 90mhz (90/74 = 1.22)
#else
#define PW_ADJUST(x) (x)
#endif

cyg_sem_t CIRImp::IRSem;
int CIRImp::m_iItemCount = 0;
CIRImp::ir_queue_t CIRImp::m_Queue;
CIRImp::ir_item_t CIRImp::m_CurrentItem;
CIRImp::ir_item_t CIRImp::m_ReleaseItem;
CIRImp * CIRImp::m_pSingleton = NULL;

CIRImp * CIRImp::GetInstance() 
{
    if (m_pSingleton == NULL) {
        m_Queue.init();
        m_pSingleton = new CIRImp;
    }
    return m_pSingleton;
}

void CIRImp::Destroy()
{
    if( m_pSingleton ) delete m_pSingleton;
    m_pSingleton = NULL;
}

CIRImp::CIRImp()
{
    // init this prior to starting the thread
    m_pIRMap = NULL;
    m_pKeyFilters = NULL;
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
    cyg_thread_suspend( ThreadHandle );
    while( !cyg_thread_delete( ThreadHandle ) ) {
        cyg_thread_delay( 1 );
    }
    delete [] Stack;

    if( m_pKeyFilters ) delete [] m_pKeyFilters;
    
    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_UMSINT);

    // destroy the interrupt object here
}


#ifdef DEBUG_PW
cyg_uint32 PWTab[5000];
cyg_uint32 PWTabI = 0;
cyg_uint32 PWi;
#endif

// new transition bits, based off clock ticks, not anything else
// adjusted for 90mhz with macro
#define IR_NEW(x)	((x) > PW_SPEED(18000))
#define IR_REPEAT(x) ((PW_SPEED(8000) <= (x)) && ((x) <= PW_SPEED(18000)))
#define IR_SYNCA(x) ((PW_SPEED(4200) <= (x)) && ((x) <= PW_SPEED(4500)))
#define IR_SYNCB(x) ((PW_SPEED(2000) <= (x)) && ((x) <= PW_SPEED(2300)))
#define IR_BITSYNC(x) ((PW_SPEED(120) <= (x)) && ((x) <= PW_SPEED(400)))
#define IR_ONEBIT(x) ((PW_SPEED(700) <= (x)) && ((x) <= PW_SPEED(850)))
#define IR_ZEROBIT(x) ((PW_SPEED(120) <= (x)) && ((x) <= PW_SPEED(400)))

volatile bool bRelease;

cyg_uint32
CIRImp::TC1ISR(cyg_vector_t Vector, cyg_addrword_t Data)
{
	cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_TC1OI);	
	
	m_Queue.push( &m_ReleaseItem );						
	m_iItemCount++;
	

	return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
}


void
CIRImp::TC1DSR(cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data)
{

    while( Count > 0 ) 
	{
        Count--;
        cyg_semaphore_post(&IRSem);
    }
}


bool
CIRImp::TimerInit(void)
{

	m_ReleaseItem.uiEventType = EVENT_KEY_RELEASE;
	m_ReleaseItem.uiKeyData = 0;

    /* Set timer to 512KHz, free-running mode */
    *((volatile cyg_uint32 *)SYSCON1) &=  ~(SYSCON1_TC1M|SYSCON1_TC1S);
    *((volatile cyg_uint32 *)SYSCON1) |=  SYSCON1_TC1S;

    // reset counter to max (128ms of time)
    *((volatile cyg_uint16 *)TC1D) = 0xFFFF;
    // clear interrupt
    *((volatile cyg_uint32 *)TC1EOI) = 0;


    cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_TC1OI,
			     1,
			     (cyg_addrword_t)0,
			     TC1ISR,
			     TC1DSR,
			     &TC1InterruptHandle,
			     &TC1Interrupt);

    cyg_drv_interrupt_attach(TC1InterruptHandle);
    
    return true;
}

cyg_uint32
CIRImp::MSISR(cyg_vector_t Vector, cyg_addrword_t Data)
{
    static int State = 0;
    static cyg_uint8 Byte = 0;
    static cyg_uint8 Flame[4]; /* The whole encoding of a new button press */
    static int Bit = 0;
    static int NumBytes = 0;

    cyg_uint32 Status = CYG_ISR_HANDLED;
    cyg_uint32 PulseWidth;
 
    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_UMSINT);
	
	if(!(*((volatile cyg_uint32 *)INTSR1) & INTSR1_TC1OI))
		PulseWidth = PW_ADJUST((cyg_uint32)(0xFFFF - *((volatile cyg_uint16 *)TC1D)));
	else
		PulseWidth = 0xFFFFFFFF; // overflow case

		
	// reset counter to hold timeout
	*((volatile cyg_uint16 *)TC1D) = 0xFFFF;
	// clear interrupt
	*((volatile cyg_uint32 *)TC1EOI) = 0;

#ifdef DEBUG_PW
// debug
	PWTab[PWTabI++] = PulseWidth;

	if (PWTabI > 100) {
	  for(PWi = 0; PWi < PWTabI ; ++PWi) {
		diag_printf("%u - %u : %u\n", PWi, PWTab[PWi], (cyg_uint32)(((double)(PWTab[PWi]) / 512000.0)*1000000));
	  }
	  PWTabI = 0;
	}
#endif


    switch (State) 
	{
	
		case 0: 
		{
			
			if(IR_NEW(PulseWidth)) 
			{	    
				m_CurrentItem.uiEventType = EVENT_KEY_PRESS;
				State = 1;
			}
			else if(IR_REPEAT(PulseWidth)) 
			{
				m_CurrentItem.uiEventType = EVENT_KEY_HOLD;
				State = 1;
			}
			else
			{
#ifdef DEBUG_MISS
				DEBUGP( IR, DBGLEV_ERROR, "IR: state %d, missed width = %d\n",State,PulseWidth);
#endif
			}
			
			break;
		}

    
		case 1:
		{
			// 9ms event

			NumBytes = 0;
			Bit = 0;
			Byte = 0;
			
			if (IR_SYNCA(PulseWidth)) 
			{
				State = 2;
			}
			else
			{
#ifdef DEBUG_MISS
				DEBUGP( IR, DBGLEV_ERROR, "IR: state %d, missed width = %d\n",State,PulseWidth);
#endif
				State = 0;
			}

			break;
		}

		case 2: 
		{
			// 4.5ms sync (sync for byte)
			if (IR_SYNCB(PulseWidth)) 
			{
				State = 3;
			}
			else
			{
#ifdef DEBUG_MISS
				DEBUGP( IR, DBGLEV_ERROR, "IR: state %d, missed width = %d\n",State,PulseWidth);
#endif
				State = 0;

			}


			break;    
		}

		case 3: 
		{
			// sync for first bit of a byte
			if (IR_BITSYNC(PulseWidth)) 
			{
				State = 4;		
			}
			else
			{
#ifdef DEBUG_MISS
			    DEBUGP( IR, DBGLEV_ERROR, "IR: state %d, missed width = %d\n",State,PulseWidth);
#endif
				State = 0;
			}
            
			break;
		}

		case 4: 
		{
			/* Action */
			if (IR_ZEROBIT(PulseWidth)) 
			{		
				// do nothing
			}
			else if(IR_ONEBIT(PulseWidth)) 
			{
				Byte |= (1 << Bit);
			}
			else
			{
#ifdef DEBUG_MISS
				DEBUGP( IR, DBGLEV_ERROR, "IR: state %d, missed width = %d\n",State,PulseWidth);
#endif
				State = 0;
				break;
			}
			
			Bit++;

			// resync each bit
			State = 5;
			break;
		}
		

		case 5:
		{

			if(IR_BITSYNC(PulseWidth))
			{
				if(Bit == 8)
				{
					Flame[NumBytes] = Byte;
					Byte = 0;
					Bit = 0;
					++NumBytes;

					// resync for next byte
					if(NumBytes == 1)
					{
						State = 2;
					}
					else if(NumBytes == 2)
					{

						//diag_printf("%2.2x %2.2x\n",Flame[0],Flame[1]);
						// got a full event
						if(0x02 == Flame[0])
						{
							// diag_printf("0x%2.2x\n",Flame[1]);
							m_CurrentItem.uiKeyData = (cyg_uint32)Flame[1];

							m_Queue.push( &m_CurrentItem );						
							m_iItemCount++;

							Status |= CYG_ISR_CALL_DSR;
						}

						NumBytes = 0;
						State = 0;

					}					
				}
				else
				{
					State = 4;
				}
			}
			else
			{
#ifdef DEBUG_MISS
				DEBUGP( IR, DBGLEV_ERROR, "IR: state %d, missed width = %d\n",State,PulseWidth);
#endif
				State = 0;
			}

			break;
		}

		default:
		{
			State = 0;
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

    while( Count > 0 ) 
	{
        Count--;
        cyg_semaphore_post(&IRSem);
    }

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
    int i;
	bool bPress = false;
	bool bDropKeyEvent = false;
    m_pEventQueue = CEventQueue::GetInstance();
    // static int count = 1;
    cyg_semaphore_init(&IRSem, 0);

    MSInit();
	TimerInit();

    for (;;) 
	{
		ir_item_t* p;
		unsigned int uiEventData = 0;
        
		/* Wait for complete byte from IR */
		cyg_semaphore_wait(&IRSem);

		p = m_Queue.pop();

		// only post new key events if there's less than 10 events already in the queue.
		// this is to avoid the unpleasant situation of a full event queue.
		// however, always post key release messages.
		if (m_pEventQueue->FullCount() >= 10)
			bDropKeyEvent = true;
		else
			bDropKeyEvent = false;

		if( p && !m_bLocked && m_pIRMap ) 
		{

			if( p->uiEventType == EVENT_KEY_RELEASE && bPress)
			{	
				
				bPress = false;

				// scan for match
				for(i = 0; i <  m_pIRMap->num_buttons; i++) 
				{

					if(m_pIRMap->press_map[i].remote == p->uiKeyData) 
					{
						uiEventData = m_pIRMap->press_map[i].player;
						bDropKeyEvent = false;
						break;
					}

				}
			}
			else if( p->uiEventType == EVENT_KEY_PRESS && !bPress && !bDropKeyEvent) 
			{				

				bPress = true;

				// setup and unmask release event timer
				m_ReleaseItem.uiKeyData = p->uiKeyData;
				cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_TC1OI);
				cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_TC1OI);
				
				// scan for match
				for(i = 0; i <  m_pIRMap->num_buttons; i++) 
				{

					if(m_pIRMap->press_map[i].remote == p->uiKeyData) 
					{

						uiEventData = m_pIRMap->press_map[i].player;						

						// reset countdown for repeat
						m_pKeyFilters[ i ] = m_pIRMap->filter_start;

						break;
					}

				}
			}
			else if( p->uiEventType == EVENT_KEY_HOLD &&
			(m_pIRMap->repeat_flags & IR_REPEAT_ENABLE) && bPress && !bDropKeyEvent) 
			{

				bool bFound = false;

				for(i = 0; i <  m_pIRMap->num_buttons; i++) 
				{

					if(m_pIRMap->press_map[i].remote == p->uiKeyData) 
					{		
						bFound = true;
						break;					
					}

				}

				if(bFound)
				{

					if( m_pKeyFilters[ i ] == 0 ) 
					{
						uiEventData = m_pIRMap->hold_map[i].player;
						
						if( uiEventData ) 
						{					
							m_pKeyFilters[ i ] = m_pIRMap->filter_rate;					
						}
					}
					else 
					{
						m_pKeyFilters[ i ]--;				
					}
				}
			}

			// ignored key events have a '0' in the map
			if( uiEventData && !bDropKeyEvent ) 
			{
				m_pEventQueue->PutEvent( p->uiEventType, (void *)uiEventData );
			}
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
            if( m_pIRMap->hold_map[ i ].player ) {
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
