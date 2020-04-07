// LEDStateManager.h: Remembers the various states that cause the LED to be on
// edwardm@iobjects.com 12/06/01
// (c) Interactive Objects

#ifndef LEDSTATEMANAGER_H_
#define LEDSTATEMANAGER_H_

typedef enum LEDColor { OFF = 0, RED, GREEN, ORANGE };

typedef enum LEDState
{
    NO_STATE = 0,
    POWERING_ON,        //!< For the initialization period after a hard power-up
    POWERING_OFF,       //!< For the shutdown period after a soft power down
    HARD_POWERING_OFF,  //!< For the shutdown period after a hard power down
    POWER_OFF,          //!< Soft powered down
    IDLE_ENCODING,      //!< Currently idle coding a track
    RECORDING_ENABLED,  //!< CD in the drive, player in CD mode
    HEARTBEAT,          //!< Generic blink to indicate device is alive
    RECORDING,          //!< Recording a track off the CD
    RECORDING_PAUSED    //!< Recording a track off the CD, paused
};

void EnableLED();
void DisableLED();
void SetLEDState(LEDState eLEDState, bool bEnabled);
// return the current led color.  doesn't report on flashing.
LEDColor GetLEDColor();
void ShutdownLED();

#endif // LEDSTATEMANAGER_H_
