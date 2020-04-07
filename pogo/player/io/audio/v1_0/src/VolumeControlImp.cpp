//
// VolumeControlImp.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "VolumeControlImp.h"

#include <devs/audio/dai.h>
#include <util/debug/debug.h>

#define VOLUMECONTROL_SAVE_STATE

#if defined(VOLUMECONTROL_SAVE_STATE)
#include <util/registry/Registry.h>

// TODO fix this
static const RegKey VolumeControlRegKey = REGKEY_CREATE( 0x79, 0x01 );
#endif

DEBUG_MODULE_S( VOLUMECONTROL, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( VOLUMECONTROL );

#define VOLUME_DEFAULT_RANGE  12
#define VOLUME_DEFAULT_LEVEL   8
#define TREBLE_DEFAULT_RANGE  12
#define TREBLE_DEFAULT_LEVEL   0
#define BASS_DEFAULT_RANGE    12
#define BASS_DEFAULT_LEVEL     0

CVolumeControlImp::CVolumeControlImp()
{
    DEBUGP( VOLUMECONTROL, DBGLEV_TRACE, "vcimp: ctor +\n");

    m_pVolumeMap = m_pTrebleMap = m_pBassMap = 0;
    m_iVolumeRange = m_iTrebleRange = m_iBassRange = m_iMuteVolumeLevel = 0;
    m_iTrebleLevel = m_iBassLevel = 0;
    m_bIsMuted = false;

#if defined(VOLUMECONTROL_SAVE_STATE)
    if( !RestoreFromRegistry() )
    {
#endif
        // load up some default settings
        SetVolumeRange( VOLUME_DEFAULT_RANGE );
        SetVolume( VOLUME_DEFAULT_LEVEL );
        
        SetTrebleRange( TREBLE_DEFAULT_RANGE );
        SetBassRange( BASS_DEFAULT_RANGE );

        SetTreble( TREBLE_DEFAULT_LEVEL );
        SetBass( BASS_DEFAULT_LEVEL );

#if defined(VOLUMECONTROL_SAVE_STATE)
        int* Settings = new int[ CheckStateSize() / sizeof(int) ];
        SaveState( (void*)Settings, CheckStateSize() );
        delete [] (int*)CRegistry::GetInstance()->RemoveItem(VolumeControlRegKey);;
        CRegistry::GetInstance()->AddItem( VolumeControlRegKey, (void*)Settings, REGFLAG_PERSISTENT, CheckStateSize() );
    }
#endif
    DEBUGP( VOLUMECONTROL, DBGLEV_TRACE, "vcimp: ctor -\n");
}

CVolumeControlImp::~CVolumeControlImp() 
{
#if defined(VOLUMECONTROL_SAVE_STATE)
    SaveToRegistry();
#endif

    // release resources
    if( m_pVolumeMap )
        delete [] m_pVolumeMap;
    if( m_pTrebleMap )
        delete [] m_pTrebleMap;
    if( m_pBassMap )
        delete [] m_pBassMap;
}
int CVolumeControlImp::SaveToRegistry() 
{
    void* buf = CRegistry::GetInstance()->FindByKey( VolumeControlRegKey );
    if( ! buf ) {
        return 0;
    } else {
        SaveState( buf, CheckStateSize() );
        return 1;
    }
}
int CVolumeControlImp::RestoreFromRegistry() 
{
    DEBUGP( VOLUMECONTROL, DBGLEV_TRACE, "vcimp: RestoreFromRegistry +\n");
    void* buf = CRegistry::GetInstance()->FindByKey( VolumeControlRegKey );
    if( !buf ) {
        return 0;
    } else {
        RestoreState( buf, CheckStateSize() );
        return 1;
    }
    DEBUGP( VOLUMECONTROL, DBGLEV_TRACE, "vcimp: RestoreFromRegistry -\n");
}

void CVolumeControlImp::SetVolumeRange( int iRange ) 
{
    if( m_pVolumeMap )
        delete [] m_pVolumeMap;

#if defined(VOLUMECONTROL_SAVE_STATE)
    bool bUpdateRegistry = m_iVolumeRange != iRange;
#endif
    m_iVolumeRange = iRange;
    m_pVolumeMap = new int[ m_iVolumeRange ];

    int vol = DAC_MIN_VOLUME;
    int step = (DAC_MAX_VOLUME-DAC_MIN_VOLUME) / m_iVolumeRange;

    for( int i = 0; i < m_iVolumeRange; i++ ) {
        m_pVolumeMap[i] = vol;
        vol += step;
    }

#if defined(VOLUMECONTROL_SAVE_STATE)
    if (bUpdateRegistry)
    {
        int* Settings = new int[ CheckStateSize() / sizeof(int) ];
        SaveState( (void*)Settings, CheckStateSize() );

        delete [] (int*)CRegistry::GetInstance()->RemoveItem(VolumeControlRegKey);
        CRegistry::GetInstance()->AddItem( VolumeControlRegKey, (void*)Settings, REGFLAG_PERSISTENT, CheckStateSize() );
    }
#endif
}

void CVolumeControlImp::SetVolumeRange( int iValueCount, int* iValues )
{
    if (m_pVolumeMap)
        delete [] m_pVolumeMap;

#if defined(VOLUMECONTROL_SAVE_STATE)
    bool bUpdateRegistry = m_iVolumeRange != iValueCount;
#endif
    m_iVolumeRange = iValueCount;
    m_pVolumeMap = new int[iValueCount];
    for (int i = 0; i < iValueCount; ++i)
        m_pVolumeMap[i] = iValues[i];

#if defined(VOLUMECONTROL_SAVE_STATE)
    if (bUpdateRegistry)
    {
        int* Settings = new int[ CheckStateSize() / sizeof(int) ];
        SaveState( (void*)Settings, CheckStateSize() );

        delete [] (int*)CRegistry::GetInstance()->RemoveItem(VolumeControlRegKey);
        CRegistry::GetInstance()->AddItem( VolumeControlRegKey, (void*)Settings, REGFLAG_PERSISTENT, CheckStateSize() );
    }
#endif
}

int CVolumeControlImp::GetVolumeRange() const 
{
    return m_iVolumeRange;
}

int CVolumeControlImp::VolumeUp()
{
    return SetVolume( m_iVolumeLevel+1 );
}

int CVolumeControlImp::VolumeDown() 
{
    return SetVolume( m_iVolumeLevel-1 );
}

int CVolumeControlImp::GetVolume() const 
{
    return m_iVolumeLevel;
}

int CVolumeControlImp::SetVolume( int level ) 
{
    if( level < 0 ) level = 0;
    if( level >= m_iVolumeRange ) level = m_iVolumeRange-1;

    m_iVolumeLevel = level;
    
    DACSetVolume( m_pVolumeMap[ m_iVolumeLevel ] );

    return m_iVolumeLevel;
}


void CVolumeControlImp::SetTrebleRange( int iRange ) 
{
    if( m_pTrebleMap )
        delete [] m_pTrebleMap;
    
#if defined(VOLUMECONTROL_SAVE_STATE)
    bool bUpdateRegistry = m_iTrebleRange != iRange;
#endif
    m_iTrebleRange = iRange;
    m_pTrebleMap = new int[ m_iTrebleRange ];

    int treb = DAC_MIN_TREBLE_BOOST;
    int step = (DAC_MAX_TREBLE_BOOST-DAC_MIN_TREBLE_BOOST) / m_iTrebleRange;

    for( int i = 0; i < m_iTrebleRange; i++ ) {
        m_pTrebleMap[i] = treb;
        treb += step;
    }
}

void CVolumeControlImp::SetTrebleRange( int iValueCount, int* iValues )
{
    if (m_pTrebleMap)
        delete [] m_pTrebleMap;

#if defined(VOLUMECONTROL_SAVE_STATE)
    bool bUpdateRegistry = m_iTrebleRange != iValueCount;
#endif
    m_iTrebleRange = iValueCount;
    m_pTrebleMap = new int[iValueCount];
    for (int i = 0; i < iValueCount; ++i)
        m_pTrebleMap[i] = iValues[i];

#if defined(VOLUMECONTROL_SAVE_STATE)
    if (bUpdateRegistry)
    {
        int* Settings = new int[ CheckStateSize() / sizeof(int) ];
        SaveState( (void*)Settings, CheckStateSize() );

        delete [] (int*)CRegistry::GetInstance()->RemoveItem(VolumeControlRegKey);
        CRegistry::GetInstance()->AddItem( VolumeControlRegKey, (void*)Settings, REGFLAG_PERSISTENT, CheckStateSize() );
    }
#endif
}

int CVolumeControlImp::GetTrebleRange() const 
{
    return m_iTrebleRange;
}

int CVolumeControlImp::TrebleUp()
{
    return SetTreble( m_iTrebleLevel + 1 );
}

int CVolumeControlImp::TrebleDown() 
{
    return SetTreble( m_iTrebleLevel - 1 );
}

int CVolumeControlImp::GetTreble() const 
{
    return m_iTrebleLevel;
}

int CVolumeControlImp::SetTreble( int level ) 
{
    if( level < 0 ) level = 0;
    if( level >= m_iTrebleRange ) level = m_iTrebleRange-1;

    m_iTrebleLevel = level;

    DACSetTone( m_pTrebleMap[m_iTrebleLevel],
                m_pBassMap[ m_iBassLevel ] );

    return m_iTrebleLevel;
}


void CVolumeControlImp::SetBassRange( int iRange ) 
{
    if( m_pBassMap )
        delete [] m_pBassMap;
    
#if defined(VOLUMECONTROL_SAVE_STATE)
    bool bUpdateRegistry = m_iBassRange != iRange;
#endif
    m_iBassRange = iRange;
    m_pBassMap = new int[ m_iBassRange ];

    int bass = DAC_MIN_BASS_BOOST;
    int step = (DAC_MAX_BASS_BOOST-DAC_MIN_BASS_BOOST) / m_iBassRange;

    for( int i = 0; i < m_iBassRange; i++ ) {
        m_pBassMap[i] = bass;
        bass += step;
    }

#if defined(VOLUMECONTROL_SAVE_STATE)
    if (bUpdateRegistry)
    {
        int* Settings = new int[ CheckStateSize() / sizeof(int) ];
        SaveState( (void*)Settings, CheckStateSize() );

        delete [] (int*)CRegistry::GetInstance()->RemoveItem(VolumeControlRegKey);
        CRegistry::GetInstance()->AddItem( VolumeControlRegKey, (void*)Settings, REGFLAG_PERSISTENT, CheckStateSize() );
    }
#endif

}

void CVolumeControlImp::SetBassRange( int iValueCount, int* iValues )
{
    if (m_pBassMap)
        delete [] m_pBassMap;

#if defined(VOLUMECONTROL_SAVE_STATE)
    bool bUpdateRegistry = m_iBassRange != iValueCount;
#endif
    m_iBassRange = iValueCount;
    m_pBassMap = new int[iValueCount];
    for (int i = 0; i < iValueCount; ++i)
        m_pBassMap[i] = iValues[i];

#if defined(VOLUMECONTROL_SAVE_STATE)
    if (bUpdateRegistry)
    {
        int* Settings = new int[ CheckStateSize() / sizeof(int) ];
        SaveState( (void*)Settings, CheckStateSize() );

        delete [] (int*)CRegistry::GetInstance()->RemoveItem(VolumeControlRegKey);
        CRegistry::GetInstance()->AddItem( VolumeControlRegKey, (void*)Settings, REGFLAG_PERSISTENT, CheckStateSize() );
    }
#endif
}

int CVolumeControlImp::GetBassRange() const 
{
    return m_iBassRange;
}

int CVolumeControlImp::BassUp()
{
    return SetBass( m_iBassLevel + 1 );
}

int CVolumeControlImp::BassDown() 
{
    return SetBass( m_iBassLevel - 1 );
}

int CVolumeControlImp::GetBass() const 
{
    return m_iBassLevel;
}

int CVolumeControlImp::SetBass( int level ) 
{
    if( level < 0 ) level = 0;
    if( level >= m_iBassRange ) level = m_iBassRange-1;

    m_iBassLevel = level;

    DACSetTone( m_pTrebleMap[m_iTrebleLevel],
                m_pBassMap[ m_iBassLevel ] );

    return m_iBassLevel;
}

bool CVolumeControlImp::IsMuted()
{
    return m_bIsMuted;
}

void CVolumeControlImp::ToggleMute(int rate)
{
    if (m_bIsMuted)
    {              
        for (int i=0; i <= m_iMuteVolumeLevel; i++)
        {                        
            for (int j=0; j<= rate; j++)
                SetVolume(i);                                    
        }
        m_bIsMuted = false; 
        m_iMuteVolumeLevel = 0;
    }
    else
    {                                        
        m_iMuteVolumeLevel = GetVolume();        
                
        for (int i=m_iMuteVolumeLevel; i >= 0  ; i--)
        {
            for (int j=0; j<= rate; j++)
                SetVolume(i);                                    
        }        
        m_bIsMuted = true;   
    }
}

#if defined(VOLUMECONTROL_SAVE_STATE)
int CVolumeControlImp::CheckStateSize() const 
{
    return (6 * sizeof( int ) +
        m_iVolumeRange * sizeof( int ) +
        m_iBassRange * sizeof( int ) +
        m_iTrebleRange * sizeof( int ) +
        m_bIsMuted * sizeof( int ) +           // even though this is bool, we use 4 bytes in the state
        m_iMuteVolumeLevel * sizeof(int)
        );
}

int CVolumeControlImp::SaveState( void* buf, int len ) 
{
    if( len < CheckStateSize() ) return 0;

    int* p = (int*) buf;

    *p++ = m_iVolumeRange;
    for (int i = 0; i < m_iVolumeRange; ++i)
        *p++ = m_pVolumeMap[i];       
    *p++ = m_bIsMuted ? m_iMuteVolumeLevel : m_iVolumeLevel;   
    *p++ = m_iTrebleRange;
    for (int i = 0; i < m_iTrebleRange; ++i)
        *p++ = m_pTrebleMap[i];
    *p++ = m_iTrebleLevel;
    *p++ = m_iBassRange;
    for (int i = 0; i < m_iBassRange; ++i)
        *p++ = m_pBassMap[i];
    *p++ = m_iBassLevel;

    // return how many bytes we wrote
    return (p - (int*)buf) * sizeof(int);
}

int CVolumeControlImp::RestoreState( void* buf, int len ) 
{
    DEBUGP( VOLUMECONTROL, DBGLEV_TRACE, "vcimp: RestoreState +\n");
    if( len < (6 * sizeof(int)) ) return 0; // Check against min state size.

    int* p = (int*) buf;

    m_iVolumeRange = *p++;
    DBASSERT( VOLUMECONTROL, len >= (6 + m_iVolumeRange) * sizeof(int), "Bad registry state");

    if( m_pVolumeMap )
        delete [] m_pVolumeMap;
    m_pVolumeMap = new int[m_iVolumeRange];
    for (int i = 0; i < m_iVolumeRange; ++i)
        m_pVolumeMap[i] = *p++;    
    m_iVolumeLevel = *p++;    
    SetVolume( m_iVolumeLevel );    
    m_iTrebleRange = *p++;
    DBASSERT( VOLUMECONTROL, len >= (6 + m_iVolumeRange + m_iTrebleRange) * sizeof(int), "Bad registry state");

    if( m_pTrebleMap )
        delete [] m_pTrebleMap;
    m_pTrebleMap = new int[m_iTrebleRange];
    for (int i = 0; i < m_iTrebleRange; ++i)
        m_pTrebleMap[i] = *p++;
    
    m_iTrebleLevel = *p++;
    SetTreble( m_iTrebleLevel );
    
    m_iBassRange   = *p++;
    DBASSERT( VOLUMECONTROL, len >= (6 + m_iVolumeRange + m_iTrebleRange + m_iBassRange) * sizeof(int), "Bad registry state");

    if( m_pBassMap )
        delete [] m_pBassMap;
    m_pBassMap = new int[m_iBassRange];
    for (int i = 0; i < m_iBassRange; ++i)
        m_pBassMap[i] = *p++;
    
    m_iBassLevel   = *p++;
    SetBass( m_iBassLevel );

    DEBUGP( VOLUMECONTROL, DBGLEV_TRACE, "vcimp: RestoreState -\n");
    // return how many bytes we read
    return (p - (int*)buf) * sizeof(int);
}
#endif // VOLUMECONTROL_SAVE_STATE



