//
// VolumeControlImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef VOLUMECONTROLIMP_H_
#define VOLUMECONTROLIMP_H_

class CVolumeControlImp
{
public:

    CVolumeControlImp();
    ~CVolumeControlImp();

    void SetVolumeRange( int iRange );
    void SetVolumeRange( int iValueCount, int* iValues );
    int GetVolumeRange() const;

    int VolumeUp();
    int VolumeDown();
    int GetVolume() const;
    int SetVolume( int level );
    
    void SetTrebleRange( int iRange );
    void SetTrebleRange( int iValueCount, int* iValues );
    int GetTrebleRange() const;

    int TrebleUp();
    int TrebleDown();
    int GetTreble() const;
    int SetTreble( int level );
    
    void SetBassRange( int iRange );
    void SetBassRange( int iValueCount, int* iValues );
    int GetBassRange() const;

    int BassUp();
    int BassDown();
    int GetBass() const;
    int SetBass( int level );

    bool IsMuted();
    void ToggleMute(int rate);
    
    int SaveToRegistry();
    int RestoreFromRegistry();

private:

    int CheckStateSize() const;
    int SaveState( void* buf, int len );
    int RestoreState( void* buf, int len );
    
    int* m_pVolumeMap;
    int m_iVolumeRange;
    int m_iVolumeLevel;
    int m_iMuteVolumeLevel;

    int* m_pTrebleMap;
    int m_iTrebleRange;
    int m_iTrebleLevel;

    int* m_pBassMap;
    int m_iBassRange;
    int m_iBassLevel;

    bool m_bIsMuted;    
};

#endif  // VOLUMECONTROLIMP_H_
