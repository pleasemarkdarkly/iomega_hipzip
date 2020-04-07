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

DEBUG_USE_MODULE(IR);

#define DEBUG_PW  /* This is useful for debugging */

#define IR_THREAD_PRIORITY 4 /* TODO Move this to the correct place */
#define IR_THREAD_STACK_SIZE 4096


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


#ifdef DEBUG_PW
cyg_uint16 PWTab[5000];
cyg_uint32 PWTabI = 0;
cyg_uint32 PWi;
#endif

#define MSBIG(x)  ((x) >= 900)
#define MSNOTASBIG(x) ((80 <= (x)) && ((x) < 800))  
#define MS9(x)    ((67 <= (x)) && ((x) <= 74))
#define MS4_5(x)  ((31 <= (x)) && ((x) <= 36))
#define MS2_25(x) ((16 <= (x)) && ((x) <= 20))
#define MS1_69(x) ((10 <= (x)) && ((x) <= 14))
#define MS0_565(x)(( 2 <= (x)) && ((x) <=  8))
#define MS0_56(x) (( 2 <= (x)) && ((x) <=  8))

cyg_uint32
CIRImp::MSISR(cyg_vector_t Vector, cyg_addrword_t Data)
{
    static int State = 0;
    static cyg_uint8 Byte = 0;
    static cyg_uint8 Flame[4]; /* The whole encoding of a new button press */
    static int Bit = 0;
    static int NumBytes = 0;

	cyg_uint32 time;
    cyg_uint16 reg;

    cyg_uint32 Status = CYG_ISR_HANDLED;
    cyg_uint32 PulseWidth;
 
    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_UMSINT);
	

	reg = *((volatile cyg_uint16 *)TC1D);

		
	// reset counter to max (128ms of time)
	*((volatile cyg_uint16 *)TC1D) = 0xFFFF;
	// clear interrupt
	*((volatile cyg_uint32 *)TC1EOI) = 0;


	PWTab[PWTabI++] = reg;

	if (PWTabI > 100) {
	  for(PWi = 0; PWi < PWTabI ; ++PWi) {
		diag_printf("%d - %d : %u\n", PWi, PWTab[PWi], (cyg_uint32)(((double)(0xFFFF - PWTab[PWi]) / 512000.0)*1000000));
	  }
	  PWTabI = 0;
	}

    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_UMSINT);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_UMSINT);
    return Status;

#if 0
	// diag_printf("IR interrupt!\n");
#if (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000)
#warning 90mhz IR Timings enabled
	PulseWidth = (0xFFFF - reg) / 78;     

#else
    PulseWidth = (0xFFFF - reg) / 64;
#endif

 

       //    PulseWidth = ((cyg_uint16)(StartTick - CurrentTick)); // magic number to make 2000khz 8000khz. :)

#ifdef DEBUG_PW

    PWTab[PWTabI++] = ;

    if (PWTabI > 100) {
      for(PWi = 0; PWi < PWTabI ; ++PWi) {
	// DEBUG(IR, DBGLEV_WARNING, "%08x %d\n", PWTab[PWTabI], PWTab[PWTabI]);
	diag_printf("%u\n", PWTab[PWi]);
      }
      PWTabI = 0;
    }

#endif

   

    switch (State) {
	case 0: 
	{
	  /* Transition */
	  if(MSBIG(PulseWidth)) {	    
	    m_CurrentItem.uiEventType = EVENT_KEY_PRESS;
	    State = 1;
	  }
	  else if (MSNOTASBIG(PulseWidth)) {
	    m_CurrentItem.uiEventType = EVENT_KEY_HOLD;
	    State = 1;
	  }


	  break;
	}

        case 1:
	{

	  NumBytes = 0;
	  Bit = 0;
	  Byte = 0;
	  if (MS9(PulseWidth)) {
	    
	    State = 2;
	  }

	    break;
	}

	case 2: 
	{
	    /* Transition */
	    if (MS4_5(PulseWidth)) {
		State = 3;
	    }


	    break;    
	}

	case 3: 
	{
	    /* Transition */
	  if (MS0_56(PulseWidth)) {
	    State = 4;		
	  }
            
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
	      State = 4;
	    }
	    else {
		Flame[NumBytes] = Byte;
		Byte = 0;
		Bit = 0;
		++NumBytes;
		if (NumBytes == 2) {
		  State = 2; // resync for next 2 bytes
		}
		else if(NumBytes < 4) {
		  State = 4;
		}		
		else {

		  //diag_printf("DJ DEBUG - %x %x %x %x :%x\n",Flame[0],Flame[1],Flame[2],Flame[3],m_CurrentItem.uiEventType);

		  if((Flame[0] == 0x04) && (Flame[1] == 0x00)) {

		    m_CurrentItem.uiKeyData = ((cyg_uint32) Flame[2] << 8) | ((cyg_uint32)Flame[3]) ;
		    //diag_printf("DJ DEBUG - %x %x %x %x :%x\n",m_CurrentItem.uiKeyData,m_CurrentItem.uiEventType);
		    // m_CurrentItem.uiEventType = EVENT_KEY_PRESS;
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
#endif

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

    int i;
    m_pEventQueue = CEventQueue::GetInstance();
    // static int count = 1;
    cyg_semaphore_init(&IRSem, 0);

    /* Set timer to 512KHz, free-running mode */
    *((volatile cyg_uint32 *)SYSCON1) &=  ~(SYSCON1_TC1M|SYSCON1_TC1S);
    *((volatile cyg_uint32 *)SYSCON1) |=  SYSCON1_TC1S;

    // reset counter to max (128ms of time)
    *((volatile cyg_uint16 *)TC1D) = 0xFFFF;
    // clear interrupt
    *((volatile cyg_uint32 *)TC1EOI) = 0;
   

    MSInit();

    for (;;) 
	{
		ir_item_t* p;
		unsigned int uiEventData = 0;
        
		/* Wait for complete byte from IR */
		cyg_semaphore_wait(&IRSem);

		p = m_Queue.pop();

		if( p && !m_bLocked && m_pIRMap ) 
		{
			if( p->uiEventType == EVENT_KEY_PRESS ) 
			{
				// always generate press
				// diag_printf("press: %x\n",p->uiKeyData);	

				// scan for match
				for(i = 0; i <  m_pIRMap->num_buttons; i++) 
				{

					if(m_pIRMap->press_map[i].remote == p->uiKeyData) 
					{
						uiEventData = m_pIRMap->press_map[i].player;
						break;
					}

				}
			}
			else if( p->uiEventType == EVENT_KEY_HOLD &&
			(m_pIRMap->repeat_flags & IR_REPEAT_ENABLE) ) 
			{
				// for hold, look at the KeyFilter and determine if we
				// should trigger an event	
				// diag_printf("hold: %x\n",p->uiKeyData);	


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

			if( m_uiLastEventData != uiEventData ||
			m_uiLastEventType != p->uiEventType ) 
			{
				// unfortunately we get no key release with IR,
				// so step through the map and reset all the holds :/
				for( int i = 0; i < m_pIRMap->num_buttons; i++ ) 
				{
					if( m_pIRMap->hold_map [ i ].player ) 
					{
						m_pKeyFilters[ i ] = m_pIRMap->filter_start;
					}
				}
			}

			// ignored key events have a '0' in the map
			if( uiEventData ) 
			{
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

