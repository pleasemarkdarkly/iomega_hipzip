#include <util/debug/debug.h>          // debugging hooks
DEBUG_USE_MODULE( EV );

// note this is a pretty large function to inline, but it only gets called from one place, and speed is somewhat critical.
inline int CEvents::HandleKeyPress(int key, void* data)
{
    if ((int) data == KEY_BREAK_POINT)
    {
        BreakPointOnKey11();
        m_pPlayManager->HandleEvent(key, data);
        return -1;
    }
#ifdef DDOMOD_MAIN_TESTHARNESS
    else if ((int) data == KEY_TEST_STIMULATE)
    {
        static bool bOn = false;
        if (bOn)
        {
            bOn = false;
            StopTestStimulator();
        }
        else
        {
            bOn = true;
            StartTestStimulator();
        }
    }
#endif  
    // if kbd locked, reject
    if (m_bKbdLocked)
        return -1;
    // record that we have activity
    m_nTimeOfLastActivity = cyg_current_time();
    // turn on backlight if appropriate
    if (m_bBacklightEnabled && !m_bBacklightOn)
    {
#ifdef LCD_BACKLIGHT
        LCDSetBacklight(LCD_BACKLIGHT_ON);
#endif
        m_bBacklightOn = true;
    }
    // pass the message into peg for screen specific handling
    unsigned int keycode = (unsigned int)data;
    DEBUGP( EV, DBGLEV_INFO, "Kprs %d\n", keycode );
    PegThing* pt = 0;
    PegMessage Mesg;
    Mesg.wType = PM_KEY;
    Mesg.iData = keycode;
    pt->MessageQueue()->Push(Mesg);
    // let the play manager take a pass at the event
    m_pPlayManager->HandleEvent(key, data);
    return -1;
}

inline int CEvents::HandleKeyHold(int key, void* data)
{
    DEBUGP( EV, DBGLEV_INFO, "Khld %d\n", (unsigned int)data );
    PegThing* pt = 0;
    PegMessage Mesg;
    Mesg.wType = PM_KEY;
    Mesg.iData = (unsigned int)data;
    pt->MessageQueue()->Push(Mesg);
    m_pPlayManager->HandleEvent(key, data);
    return -1;
}

inline int CEvents::HandleKeyRelease(int key, void* data)
{
    DEBUGP( EV, DBGLEV_INFO, "Krel %d\n", (unsigned int)data );
    PegThing* pt = 0;
    PegMessage Mesg;
    Mesg.wType = PM_KEY_RELEASE;
    Mesg.iData = (unsigned int) data;
    pt->MessageQueue()->Push(Mesg);
    m_pPlayManager->HandleEvent(key, data);
    return -1;
}

inline int CEvents::HandleStreamProgress(int key, void* data)
{
    // record that we have activity
    m_nTimeOfLastActivity = cyg_current_time();
    // update the user interface with the new time
    m_pUserInterface->SetTrackTime( (int) data );
    return 1;
}
