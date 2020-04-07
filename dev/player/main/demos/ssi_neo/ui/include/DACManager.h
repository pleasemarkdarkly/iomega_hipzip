//........................................................................................
//........................................................................................
//.. File Name: DACManager.h                                                            ..
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

#ifndef CDACMANAGER_H_
#define CDACMANAGER_H_

class CDACManager
{
public:
	
    typedef enum EqualizerMode { BASS_TREBLE = 0, STANDARD, ROCK, CLASSICAL, JAZZ };
	
    // Get a pointer to the dac manager singleton.
    static CDACManager* GetDACManager();
    // Destroy the instance of the dac manager.
    static void Destroy();
	
    CDACManager();
    ~CDACManager();
	
    int Initialize();
	
    // Sets/gets the volume.
    int SetVolume(int vol);
    int GetVolume() const;
	
    // Get the ranges for the equalizer.
    static int GetMinLevel() { return 0; }
    static int GetMaxLevel() { return 12; }
    static int GetBaseLevel() { return 0; }
	
    // Set the equalizer mode.
    void SetEqualizerMode(EqualizerMode eEqualizerMode);
	
    // Get the equalizer mode.
    EqualizerMode GetEqualizerMode() const { return m_eEqualizerMode; }
	
    // Get/set the bass and treble levels.
    int GetBassLevel() const { return m_iBass; }
    void SetBassLevel(int iBassLevel);
    int GetTrebleLevel() const { return m_iTreble; }
    void SetTrebleLevel(int iTrebleLevel);
	
	
private:
	
	static CDACManager* s_pDACManager;
	
    EqualizerMode m_eEqualizerMode;	// The current equalizer mode.
    int m_iBass, m_iTreble;		// Levels for the bass and treble.
    int m_iVolume;
	
};

#endif // !CDACMANAGER_H_
