//........................................................................................
//........................................................................................
//.. File Name: DACManager.cpp                                                          ..
//.. Date: 06/13/2001                                                                   ..
//.. Author(s): Daniel Bolstad                                                          ..
//.. Description of content: contains the definition of the CDACManager class           ..
//.. Usage: The CDACManager class is an interface for controlling the DAC               ..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com                                ..
//.. Modification date: 06/14/2001                                                      ..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.                                   ..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com                                              ..
//........................................................................................
//........................................................................................

#include <string.h>
#include <main/demos/ssi_neo/ui/DACManager.h>
//#include "EventQueue.h"

//////////////////////////////////////////////////////////////////////
// Static values used by the mixer
//////////////////////////////////////////////////////////////////////

// used for equalizer presets...  
// BASS_TREBLE =0, STANDARD =1, ROCK =2, CLASSICAL =3, JAZZ =4
static short s_aryBass[5]	=   { 0, 0, 8, 3, 3 };
static short s_aryTreble[5]	=	{ 0, 0, 9, 1, 5 };

// If this is defined, then volume values will be taken from an array rather
// than calculated on-the-fly.
#define VOL_ARRAY

#ifdef VOL_ARRAY

#define VOL_MIN            0
#define VOL_MAX           20

#define DEFAULT_VOLUME 8

static short s_aryVolume[21] = {
//  DAI_MIN_VOLUME,
  -96,
  -42, -39, -36,  -33, -30,       //-42, -39, -36,  -33, -30,
  -27, -24, -21,  -18, -15,       //-27, -24, -21,  -18, -15,
  -13, -11,  -9,   -7,  -5,       //-12,  -9,  -6,   -3,   0,
   -3,  -1,   1,    3,	 5 };     //  3,   6,   9,   12,	   

#else	// VOL_ARRAY

#define DEFAULT_VOLUME -5

#define VOL_MIN     DAI_MIN_VOLUME
#define VOL_MAX     DAI_MAX_VOLUME

#endif	// VOL_ARRAY


CDACManager* CDACManager::s_pDACManager = 0;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// Get a pointer to the dac manager singleton.
CDACManager*
CDACManager::GetDACManager()
{
    if (!s_pDACManager) {
		s_pDACManager = new CDACManager;
    }

    return s_pDACManager;
}

// Destroy the instance of the dac manager.
void
CDACManager::Destroy()
{
    delete s_pDACManager;
    s_pDACManager = 0;
}

CDACManager::CDACManager()
{
	Initialize();
}

CDACManager::~CDACManager()
{
}

int
CDACManager::Initialize()
{
    // Set initial volume
    m_iVolume = DEFAULT_VOLUME;

    // Set initial tone
	m_eEqualizerMode = STANDARD;
    m_iBass = s_aryBass[STANDARD];
    m_iTreble = s_aryTreble[STANDARD];
	//CEventQueue::GetEventQueue()->PutEvent(new CEqualizerSettingsChangeEvent(m_eEqualizerMode, m_iBass, m_iTreble));

    //return CODEC_SUCCESS;
	return 0; 
}


int
CDACManager::SetVolume(int vol)
{
    if (vol < VOL_MIN || vol > VOL_MAX)
		return m_iVolume;
    
#ifdef VOL_ARRAY
    short Volume = (short)(s_aryVolume[vol]);
	m_iVolume = vol;
#else	// VOL_ARRAY
	short Volume = (short)vol;
	m_iVolume = Volume;
#endif	// VOL_ARRAY

	//CEventQueue::GetEventQueue()->PutEvent(new CVolumeChangeEvent(m_iVolume));

    return m_iVolume;
}

int
CDACManager::GetVolume() const
{
    return m_iVolume;
}

// Set the equalizer mode.
void
CDACManager::SetEqualizerMode(EqualizerMode eEqualizerMode)
{
	// Make sure the mode is within the range of valid values.
	if ((eEqualizerMode < BASS_TREBLE) || (eEqualizerMode > JAZZ))
		return;

	m_eEqualizerMode = eEqualizerMode;
	//CEventQueue::GetEventQueue()->PutEvent(new CEqualizerSettingsChangeEvent(m_eEqualizerMode, m_iBass, m_iTreble));
}

void
CDACManager::SetBassLevel(int iBassLevel)
{
	s_aryBass[BASS_TREBLE] = iBassLevel;
	m_iBass = iBassLevel;
	//CEventQueue::GetEventQueue()->PutEvent(new CEqualizerSettingsChangeEvent(m_eEqualizerMode, m_iBass, m_iTreble));
}

void
CDACManager::SetTrebleLevel(int iTrebleLevel)
{
	s_aryTreble[BASS_TREBLE] = iTrebleLevel;
	m_iTreble = iTrebleLevel;
	//CEventQueue::GetEventQueue()->PutEvent(new CEqualizerSettingsChangeEvent(m_eEqualizerMode, m_iBass, m_iTreble));
}

