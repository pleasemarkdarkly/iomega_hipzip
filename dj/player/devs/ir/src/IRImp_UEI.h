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

#ifndef __IRIMP_UEI_H__
#define __IRIMP_UEI_H__

#include <cyg/kernel/kapi.h>
#include <devs/ir/IR_UEI.h>

// dc- should fix at some point
#define NUM_QUEUE_ITEMS 10

class CEventQueue;

//! CIR is the singleton IR driver object
class CIRImp
{
  public:
    //! Get a pointer to the driver instance
    static CIRImp * GetInstance();
    static void Destroy();

    //! Set an IR map for the driver. This can only be called
    //! once currently.
    void SetIRMap( const ir_map_t* pIRmap );

    //! Prevent the IR driver from generating events
    void LockIR();

    //! If the IR driver has been locked, unlock it
    void UnlockIR();
    
    
  private:
    CIRImp();
    ~CIRImp();
    static CIRImp * m_pSingleton;    

    /* Modem status - where the IR data comes from */
    cyg_interrupt MSInterrupt;
    cyg_handle_t MSInterruptHandle;
    
    static cyg_uint32 MSISR(cyg_vector_t Vector, cyg_addrword_t Data);
    static void MSDSR(cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data);
    bool MSInit(void);
	
	/* Modem status - where the IR data comes from */
    cyg_interrupt TC1Interrupt;
    cyg_handle_t TC1InterruptHandle;

	static cyg_uint32 TC1ISR(cyg_vector_t Vector, cyg_addrword_t Data);
    static void TC1DSR(cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data);
    bool TimerInit(void);

    /* IR thread */
    char * Stack;
    cyg_handle_t ThreadHandle;
    cyg_thread Thread;
    static cyg_sem_t IRSem;
    
    static void IRThreadEntry(cyg_addrword_t Data);
    void IRThread(void);
    
    /* Local queue */
    typedef struct ir_item_s
    {
        cyg_uint32 uiEventType;
        cyg_uint32 uiKeyData;
    } ir_item_t;

    typedef struct ir_queue_s 
    {
        ir_item_t Queue[ NUM_QUEUE_ITEMS ];
        int iQueueStart, iQueueEnd;

        void init() 
            {
                iQueueStart = iQueueEnd = 0;
            }
        ir_item_t* pop() 
            {
                ir_item_t* p;
                if( iQueueStart == iQueueEnd )       return NULL;
                
                p = &Queue[iQueueStart++];
                
                if( iQueueStart == NUM_QUEUE_ITEMS ) iQueueStart = 0;
                return p;
            }
        void push( ir_item_t* p ) 
            {
                if( (iQueueEnd == (iQueueStart-1)) ||
                    (iQueueEnd == (NUM_QUEUE_ITEMS-1) && iQueueStart == 0))
                    return ;
                
                Queue[iQueueEnd].uiEventType = p->uiEventType;
                Queue[iQueueEnd++].uiKeyData = p->uiKeyData;
                
                if( iQueueEnd == NUM_QUEUE_ITEMS ) iQueueEnd = 0;
            }
    } ir_queue_t;

    /* Item data */
    static int m_iItemCount;
    static ir_queue_t m_Queue;
    static ir_item_t m_CurrentItem;
	static ir_item_t m_ReleaseItem;

    /* Key map */
    const ir_map_t* m_pIRMap;

    /* Key states */
    unsigned int m_uiLastEventType;
    unsigned int m_uiLastEventData;
    unsigned int* m_pKeyFilters;

    /* lock state */
    bool m_bLocked;
    

    
    /* System event queue */
    CEventQueue * m_pEventQueue;
};



#endif // __IRIMP_UEI_H__
