// RestoreEvents.cpp: how we get events for the restore system
// danb@iobjects.com 05/01/02
// (c) Fullplay Media

#ifndef __RESTORE_EVENTS_H__
#define __RESTORE_EVENTS_H__

#include <main/main/Events.h>

#define EVENT_RESTORE_CDDB  0x500
#define EVENT_RESTORE_KICKSTART 0x501

// fdecl
class CDJPlayerState;
class CMiniCDMgr;
class CRestoreEvents : public CEvents
{
public:
    CRestoreEvents();
    virtual ~CRestoreEvents();

    void SetUserInterface( IUserInterface* pUI );
    void RefreshInterface();
    int HandleEvent( int key, void* data );
   
private:

    void SynchPlayState();
    bool HandleCDInsertion();

    //
    // Class data
    //
    CDJPlayerState* m_pDJPlayerState;
	CMiniCDMgr* m_pMiniCDMgr;

    bool m_bPowerHeld;
};

#endif // __RESTORE_EVENTS_H__
