// jogdial.cpp - pogo jogdial driver implementation
// temancl@iobjects.com 

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_diag.h>
#include <devs/jogdial/JogDial.h>
#include <cyg/hal/drv_api.h>
#include <string.h>   // memset
#include <cyg/infra/diag.h>
#include <util/eventq/EventQueueAPI.h>
#include <core/events/SystemEvents.h>

#define JOG_THREAD_PRIORITY   3
#define JOG_THREAD_STACK_SIZE 4096

// instance
static CJogDial* s_pSingleton = 0;

//
// CJogDial implementation
//

CJogDial::CJogDial() 
{
    m_bLocked = false;
	m_uiLeft = 0;
	m_uiRight = 0;
	m_state = 0;

    m_jog_stack = new char[JOG_THREAD_STACK_SIZE];
    
    cyg_thread_create( JOG_THREAD_PRIORITY, CJogDial::jogdial_entry, (cyg_addrword_t)this,
                       "JogDial", (void*) m_jog_stack, JOG_THREAD_STACK_SIZE, &m_jog_handle, &m_jog_data );
    cyg_thread_resume( m_jog_handle );
}

CJogDial::~CJogDial()
{
    // TODO: kill keyboard thread, release resources

}


CJogDial* CJogDial::GetInstance() 
{
    if( s_pSingleton == 0 ) {
        s_pSingleton = new CJogDial;
    }
    return s_pSingleton;
}

void CJogDial::SetKeymap( unsigned int left, unsigned int right) 
{
    // to avoid thread safety issues, dont allow the key map to be changed
    // "dynamically"
    if( m_uiLeft != 0 || m_uiRight != 0) {
        return ;
    }

	m_uiLeft = left;
	m_uiRight = right;

    m_pEventQueue = CEventQueue::GetInstance();

}

void CJogDial::LockWheel()
{
    m_bLocked = true;
}
void CJogDial::UnlockWheel()
{
    m_bLocked = false;
}


//
// ISR, DSR
//
int CJogDial::jogdial_isr( cyg_vector_t vector, cyg_addrword_t data ) 
{
    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_EINT3);
    return (CYG_ISR_HANDLED|CYG_ISR_CALL_DSR);  // Run the DSR
}

void CJogDial::jogdial_dsr( cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data )
{
    CJogDial* p = reinterpret_cast<CJogDial*>(data);
    cyg_semaphore_post( &(p->m_jog_sem) );
}



// Thread entry point
void CJogDial::jogdial_entry( cyg_addrword_t data ) 
{
    reinterpret_cast<CJogDial*>(data)->JogDialThread();
}

// class instance entry point
void CJogDial::JogDialThread() 
{

	cyg_uint8 jog;

	// double - check DDR
	*(volatile cyg_uint8 *)PBDDR &= ~0x03;

    cyg_semaphore_init( &m_jog_sem, 0 );

    cyg_drv_interrupt_create( CYGNUM_HAL_INTERRUPT_EINT3,
                              5,
                              (cyg_addrword_t) this,
                              (cyg_ISR_t*) CJogDial::jogdial_isr,
                              (cyg_DSR_t*) CJogDial::jogdial_dsr,
                              &m_jog_int_handle,
                              &m_jog_int );
    
    cyg_drv_interrupt_attach( m_jog_int_handle );


    
    while( 1 ) 
	{


        // do this in the loop, so if we continue because there is no keymap we
        // still get this. additionally we need to unmask the interrupt or the
        // semaphore will never be posted to
        cyg_drv_interrupt_acknowledge( CYGNUM_HAL_INTERRUPT_EINT3 );
        cyg_drv_interrupt_unmask( CYGNUM_HAL_INTERRUPT_EINT3 );
        
        cyg_semaphore_wait( &m_jog_sem );
	
		if(m_bLocked)
			continue;
	            
		// check PB0 and PB1
		jog = *(volatile cyg_uint8 *)PBDR & 0x03;
		//	diag_printf("JogDial state = 0x%x\n",state);


		// my funny little state machine
		// 1 or 2 set what event is to be generated
		// 0 sends the event
		// all other states are ignored.
		switch(jog)
		{
			case 0:

				switch(m_state)
				{
					case 0:
						break;
					case 1:
						m_pEventQueue->PutEvent( EVENT_KEY_PRESS,(void*)m_uiRight );
						break;
					case 2:
						m_pEventQueue->PutEvent( EVENT_KEY_PRESS,(void*)m_uiLeft );
						break;
					default:
						break;
				}

				m_state = 0;
				break;


			case 1:
			case 2:				
				m_state = jog;
				break;
			case 3:
				// ignore both high
				break;

			default:
				break;		

		} // switch

	} 

}

