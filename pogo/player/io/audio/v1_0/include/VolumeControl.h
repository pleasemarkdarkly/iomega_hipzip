//
// VolumeControl.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The volume control provides a simple way to adjust hardware volume, bass
//! and treble settings. It allows user specified volume maps, or it can
//! optionally attempt to generate a map (although this is does not always
//! provide smooth scaling). Additionally it provides an easy way to save
//! state through the registry, if available.

//@{

#ifndef VOLUMECONTROL_H_
#define VOLUMECONTROL_H_

class CVolumeControlImp;

//! The volume control implementation is a non-thread safe singleton.
class CVolumeControl
{
public:

    //! Get a pointer to the single instance of the volume control
    static CVolumeControl* GetInstance();

    //! Destroy the singleton
    static void Destroy();

    //! Set a volume range and let the control generate a volume map
    void SetVolumeRange( int iRange );
    //! Specify a volume map and range
    void SetVolumeRange( int iValueCount, int* iValues );
    //! Get the current volume range
    int GetVolumeRange() const;

    //! Raise the volume
    int VolumeUp();
    //! Lower the volume
    int VolumeDown();
    //! Get the current volume
    int GetVolume() const;
    //! Set the current volume
    int SetVolume( int level );

    //! Set the treble range and let the control generate a map
    void SetTrebleRange( int iRange );
    //! Set the treble map and range
    void SetTrebleRange( int iValueCount, int* iValues );
    //! Get the current treble range
    int GetTrebleRange() const;

    //! Raise the treble
    int TrebleUp();
    //! Lower the treble
    int TrebleDown();
    //! Get the current treble
    int GetTreble() const;
    //! Set the current treble
    int SetTreble( int level );
    
    //! Set the bass range and let the control generate a map
    void SetBassRange( int iRange );
    //! Set the bass map and range
    void SetBassRange( int iValueCount, int* iValues );
    //! Get the current bass range
    int GetBassRange() const;

    //! Raise the bass
    int BassUp();
    //! Lower the bass
    int BassDown();
    //! Get the current bass
    int GetBass() const;
    //! Set the current bass
    int SetBass( int level );

    //! Check the Mute Status
    bool IsMuted();
    //! Toggle mute on and off with a given fade on/off rate
    void ToggleMute(int rate);
    
    //! Save the volume, bass, and treble settings to the registry
    int SaveToRegistry();
    //! Restore the volume, bass, and treble settings from the registry
    int RestoreFromRegistry();

private:

    CVolumeControl();
    ~CVolumeControl();

    CVolumeControlImp*  m_pImp;
};

//@}

#endif  // VOLUMECONTROL_H_
