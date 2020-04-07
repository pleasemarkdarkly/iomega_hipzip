//
// JogDial.h
// temancl@iobjects.com
// Pogo Jogdial driver
// uses EINT3 and PB0-1 to drive a wheel

#ifndef __POGO
#error Only Pogo has a jogdial!
#endif

#ifndef __JOGDIAL_H__
#define __JOGDIAL_H__

  
//
// fdecl
//
class CEventQueue;


class CJogDial
{
public:
    CJogDial();
    ~CJogDial();

    static CJogDial* GetInstance();

    void SetKeymap( unsigned int left, unsigned int right);

    void LockWheel();
    void UnlockWheel();
    
private:


	    // interrupt handling
    static int jogdial_isr( cyg_vector_t vector, cyg_addrword_t data );
    static void jogdial_dsr( cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data );
    
    // thread entry point
    static void jogdial_entry( unsigned int );
    void JogDialThread();

    // system event queue, we'll keep a ptr to it
    CEventQueue* m_pEventQueue;


    // thread data
    cyg_thread m_jog_data;
    cyg_handle_t m_jog_handle;
    char* m_jog_stack;

    // interrupt data
    cyg_interrupt m_jog_int;
    cyg_handle_t m_jog_int_handle;

    // thread/int semaphore
    cyg_sem_t m_jog_sem;

    // is the button pad locked?

    bool m_bLocked;

	// mapping for direction to key events
	unsigned int m_uiLeft;
	unsigned int m_uiRight;
	cyg_uint8 m_state;

};


#endif // __JOGDIAL_H__
