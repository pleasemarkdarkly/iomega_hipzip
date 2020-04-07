// LEDStateManager.h: Remembers the various states that cause the LED to be on
// edwardm@iobjects.com 12/06/01
// (c) Interactive Objects

#include <main/main/LEDStateManager.h>
#include <main/main/LED.h>
#include <util/timer/Timer.h>

#include <util/debug/debug.h>

#include <cyg/kernel/kapi.h>

DEBUG_MODULE_S( LEDMANAGER, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( LEDMANAGER );


typedef struct led_state_s
{
    LEDState    eLEDState;   //!< State this record represents.
    LEDColor    eLEDColor;   //!< The color to set the LED.
    int         iFlashCount; //!< -1 to flash forever, 0 for no flashing, or number of times to flash
    bool        bEnabled;    //!< True if the state is enabled.
} led_state_t;

static led_state_t s_ledStates[] =
{
    { RECORDING_PAUSED, RED, -1, false },
    { RECORDING, RED, 0, false },
    { IDLE_ENCODING, ORANGE, 0, false },
    { HARD_POWERING_OFF, ORANGE, 5, false },
    { POWERING_OFF, ORANGE, 0, false },
    { POWERING_ON, GREEN, 0, false },
    { POWER_OFF, OFF, 0, false },
    { HEARTBEAT, RED, -1, false },
    { NO_STATE, OFF, 0, true }
};

void LEDFlashCB(void* arg);

class CLEDStateManager
{
public:
    static CLEDStateManager* GetInstance()
    {
        if (!s_pSingleton)
            s_pSingleton = new CLEDStateManager;
        return s_pSingleton;
    }

    static void Destroy()
    {
        delete s_pSingleton;
        s_pSingleton = 0;
    }

    CLEDStateManager()
        : m_hTimer(0),
          m_color(OFF),
          m_bOn(false),
          m_bEnabled(true)
    {
        cyg_mutex_init(&m_mtxLED);
    }
    ~CLEDStateManager()
    {
        cyg_mutex_destroy(&m_mtxLED);
    }

    void SetLEDColor(LEDColor color)
    {
        if (!m_bEnabled)
        {
            LEDOff();
            return;
        }

        switch (color)
        {
            case OFF:
                LEDOff();
                break;
            case RED:
                LEDRed();
                break;
            case GREEN:
                LEDGreen();
                break;
            case ORANGE:
                LEDOrange();
                break;
        }
    }

    void EnableLED()
    {
        m_bEnabled = true;
        SetLEDColor(m_color);
    }

    void DisableLED()
    {
        m_bEnabled = false;
        LEDOff();
    }

    void SetLEDState(LEDState eLEDState, bool bEnabled)
    {
	    cyg_mutex_lock(&m_mtxLED);
        for (unsigned int i = 0; i < sizeof(s_ledStates) / sizeof(led_state_t) - 1; ++i)
            if (s_ledStates[i].eLEDState == eLEDState)
            {
                s_ledStates[i].bEnabled = bEnabled;
                // Go down the priority list, looking for the highest enabled state.
                unsigned int j;
                for (j = 0; j  < sizeof(s_ledStates) / sizeof(led_state_t); ++j) {
                    if (s_ledStates[j].bEnabled)
                    {
                        // Save our current color
                        m_color = s_ledStates[j].eLEDColor;

                        // Change the LED color.
                        SetLEDColor(m_color);

                        // Kill any existing timers.
                        if (m_hTimer)
                        {
                            unregister_timer(m_hTimer);
                            m_hTimer = 0;
                        }
                        // If this state is a flashing state, then set up a timer for it.
                        if (s_ledStates[j].iFlashCount)
                        {
                            // DC- unregister a timer if we already have one
                            if( m_hTimer ) {
                                unregister_timer(m_hTimer);
                            }
                            register_timer_persist(LEDFlashCB, (void*)this, TIMER_MILLISECONDS(350), s_ledStates[j].iFlashCount, &m_hTimer);
                            resume_timer(m_hTimer);
                            m_bOn = true;
                        }
                        break;
                    }
                }
                // DC- if we cycle through the list and dont find any enabled items, make sure we
                //     junk our timer
                if( j == sizeof(s_ledStates) / sizeof(led_state_t)) {
                    unregister_timer(m_hTimer);
                    m_hTimer = 0;
                }
                break;
            }
        cyg_mutex_unlock(&m_mtxLED);
    }
    
    // return the topmost enabled state.  this isn't that critical, so no mutexing.
    LEDColor GetLEDColor()
    {
        for (unsigned int i = 0; i < sizeof(s_ledStates) / sizeof(led_state_t) - 1; ++i)
            if (s_ledStates[i].bEnabled)
                return s_ledStates[i].eLEDColor;
    }

    void LEDFlash()
    {
	    cyg_mutex_lock(&m_mtxLED);
        if (m_bOn)
        {
            LEDOff();
            m_bOn = false;
        }
        else
        {
            SetLEDColor(m_color);
            m_bOn = true;
        }
        cyg_mutex_unlock(&m_mtxLED);
    }

private:

    static CLEDStateManager* s_pSingleton;

    cyg_mutex_t     m_mtxLED;   // Mutex to restrict LED access.
    timer_handle_t  m_hTimer;   // Handle to the timer used for making the LED flash.
    LEDColor        m_color;    // Color to flash.
    bool            m_bOn;      // Toggle flash state.
    bool            m_bEnabled; // Is the LED enabled.
};

CLEDStateManager* CLEDStateManager::s_pSingleton = 0;

void LEDFlashCB(void* arg)
{
    ((CLEDStateManager*)arg)->LEDFlash();
}

void EnableLED()
{
    CLEDStateManager::GetInstance()->EnableLED();
}

void DisableLED()
{
    CLEDStateManager::GetInstance()->DisableLED();
}

void SetLEDState(LEDState eLEDState, bool bEnabled)
{
    CLEDStateManager::GetInstance()->SetLEDState(eLEDState, bEnabled);
}

LEDColor GetLEDColor()
{
    return CLEDStateManager::GetInstance()->GetLEDColor();
}

void ShutdownLED()
{
    CLEDStateManager::Destroy();
}
