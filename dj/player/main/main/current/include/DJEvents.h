// DJEvents.h: how we get events
// danc@iobjects.com 08/08/01
// (c) Interactive Objects

#ifndef __DJ_EVENTS_H__
#define __DJ_EVENTS_H__

#define PLAYLIST_STRING_SIZE 128

#include <main/main/Events.h>
#include <main/main/AppSettings.h>     // PLAYLIST_STRING_SIZE
#include <_modules.h>


// fdecl
class CCDCache;
class IContentManager;
class CDJPlayerState;
#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
class CDataSourceContentManager;
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
class CDJContentManager;
#endif
class CSimplerContentManager;
class CPlayManager;
class IUserInterface;
#ifndef DISABLE_VOLUME_CONTROL
class CVolumeControl;
#endif // DISABLE_VOLUME_CONTROL
typedef struct cdda_toc_s cdda_toc_t;

#define VOLUME_RATE 10

class CDJEvents : public CEvents
{
public:
    CDJEvents();
    virtual ~CDJEvents();

    void SetUserInterface( IUserInterface* pUI );
    void RefreshInterface();
    int HandleEvent( int key, void* data );

    // Tells the player to use the last known playlist saved in the state manager
    // when constructing a CD playlist.
    void SetRestoreCDPlaylist()
        { m_bRestoreCDPlaylist = true; }

    // Tells the player to switch/not switch data sources on the first CD insertion message.
    // Used when the hard drive was the last data source used and a CD is in the drive.
    void SetSwitchSourceOnCDInsertion(bool bSwitchOnInsertion)
        { m_bSwitchSourceOnCDInsertion = bSwitchOnInsertion; }

    // Sets the current CD content scan ID.
    // Used when the DJ boots with a CD in the drive, bypassing the normal EVENT_MEDIA_INSERTED routine.
    void SetCDScanID(unsigned short usScanID)
        { m_bIgnoreCDScan = false; m_usCDScanID = usScanID; }

private:

    void SynchPlayState();
    bool HandleCDInsertion(const cdda_toc_t* pTOC);
    void OpenCDTray();

    // Called whenever a stream stops playing.
    // Use this chance to do time-consuming operations.
    void OnStop();

    //
    // Class data
    //
    CDJPlayerState* m_pDJPlayerState;
    IUserInterface* m_pUserInterface;
#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
    CDataSourceContentManager*  m_pContentManager;
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
    CDJContentManager*          m_pContentManager;
#endif
    CSimplerContentManager*     m_pSimplerContentManager;
    CPlayManager*   m_pPlayManager;
#ifndef DISABLE_VOLUME_CONTROL
    CVolumeControl* m_pVolumeControl;
#endif // DISABLE_VOLUME_CONTROL
    CCDCache*       m_pCDCache;

    int m_iTrackTime;

    // Used to generate session numbers for CDs to help in matching up
    // results of asynchronous CDDB queries.
    unsigned int m_uiCDInsertionCount;

    // True when the CD is ejected, false when EVENT_MEDIA_INSERTED arrives.
    // If true then any CD content scan events that arrive are ignored.
    bool m_bIgnoreCDScan;

    // The ID of the current CD scan cycle, as returned by the data source manager.
    // This is used for screening out old CD content scan events that can be fired when
    // the user inserts a CD, ejects it while scanning, then inserts the CD.
    unsigned short m_usCDScanID;

    // If true, then the last playlist remembered before shutdown will be
    // used by the CD after an insertion message.
    bool m_bRestoreCDPlaylist;

    // True when there are multiple local metadata hits for a CD.
    bool m_bLocalHits;

    short m_iCurrentPlaylistCount;
    char m_sCurrentPlaylistName[PLAYLIST_STRING_SIZE];
    bool m_bCurrentTrackSet;

    bool m_bPowerHeld;
    bool m_bDebouncePower;
    bool m_bPlayOnCDInsert;     // If true, playback will begin after a CD is inserted.
    bool m_bRecordOnCDInsert;   // If true, recording will begin after a CD is inserted.
    bool m_bSwitchSourceOnCDInsertion;
            // If true, then the data source will be switched to CD on an EVENT_MEDIA_INSERTED message.
};

#endif // __DJ_EVENTS_H__
