#include <main/ui/PlaylistConstraint.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/Strings.hpp>
#include <main/main/AppSettings.h>     // PLAYLIST_STRING_SIZE
#include <main/main/DJPlayerState.h>
#include <main/main/FatHelper.h>

#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <main/content/djcontentmanager/DJContentManager.h>
#include <main/ui/PlaylistLoadEvents.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <stdio.h>

#include <util/registry/Registry.h>
static const RegKey PlaylistConstraintRegKey = REGKEY_CREATE( PLAYLIST_CONSTRAINT_REGISTRY_KEY_TYPE, PLAYLIST_CONSTRAINT_REGISTRY_KEY_NAME );

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_PLAYLIST_CONSTRAINT, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_PLAYLIST_CONSTRAINT );

extern void SetMainThreadPriority(int nPrio);
extern int GetMainThreadPriority();

static CPlaylistConstraint* m_pInstance;

CPlaylistConstraint* CPlaylistConstraint::GetInstance()
{
    if (!m_pInstance) m_pInstance = new CPlaylistConstraint;
    return m_pInstance;
}

void CPlaylistConstraint::Destroy()
{
    if (m_pInstance)
    {
        delete m_pInstance;
        m_pInstance = NULL;
    }
}

void DumpPlaylistContents(MediaRecordList &mrlTracks)
{
    int i = 0;
    char temp[256];
    for (MediaRecordIterator it = mrlTracks.GetHead(); it != mrlTracks.GetEnd(); ++it)
    {
        void* pdata;
        (*it)->GetAttribute(MDA_ARTIST, &pdata);
        TcharToCharN(temp,(TCHAR*)pdata,255);
        DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, "track %d has artist '%s'", i++, temp);
        (*it)->GetAttribute(MDA_ALBUM, &pdata);
        TcharToCharN(temp,(TCHAR*)pdata,255);
        DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, ", album '%s'", temp);
        (*it)->GetAttribute(MDA_GENRE, &pdata);
        TcharToCharN(temp,(TCHAR*)pdata,255);
        DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, ", genre '%s'\n", temp);
    }
}

// add whatever content meets the current constraints to the current playlist
void CPlaylistConstraint::UpdatePlaylist()
{
    IPlaylist* pPL = CPlayManager::GetInstance()->GetPlaylist();
    bool bWasEmpty = pPL->IsEmpty();
    
    if (m_pContentRecord)
        pPL->AddEntry(m_pContentRecord);
    else if (m_szPlaylistURL)
    {
        CPlaylistFormatManager* pfm = CPlaylistFormatManager::GetInstance();
        char* ext = ExtensionFromFilename(m_szPlaylistURL);
        int id = pfm->FindPlaylistFormat(ext);
        
        // dvb (6/6/2002) for consistency, print the "loading playlist" message from the LibraryMenuScreen
        
        //const char* filename = FilenameFromURLInPlace(m_szPlaylistURL);
        // TODO: input source   // (epg,2/21/2002): elaborate?
        //char temp[PLAYLIST_STRING_SIZE];
        //sprintf(temp,"Loading %s",filename);
        //TCHAR tcTemp[PLAYLIST_STRING_SIZE];
        // strip extension
        //int len = strlen(temp);
        //len = (len < 127) ? len - 4 : 127;                    
        
        //CharToTcharN(tcTemp, temp, len);
        //CPlayerScreen::GetPlayerScreen()->SetMessageText(tcTemp, CSystemMessageString::STATUS);
        //pPL->Clear();

        playlist_load_info_t* pLoadInfo = new playlist_load_info_t;

        int nPrio = GetMainThreadPriority();
        SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);

        if (!SUCCEEDED(pfm->LoadPlaylist(id, m_szPlaylistURL, pLoadInfo->records, 0)))
        {
            DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_TRACE, "pc:UpdatePlaylist failed to load playlist file\n");		
            // notify user of the failure
            const char* filename = FilenameFromURLInPlace(m_szPlaylistURL);
            char szName[PLAYLIST_STRING_SIZE];
            TCHAR tszName[PLAYLIST_STRING_SIZE];
            sprintf(szName, " %s, ", filename);
            // construct our system text message
            TCHAR tszMessage[32 + PLAYLIST_STRING_SIZE];
            tstrcpy(tszMessage, LS(SID_CANT_LOAD_PLAYLIST));
            CharToTcharN(tszName, szName, PLAYLIST_STRING_SIZE - 1);
            tstrcat(tszMessage, tszName);
            tstrcat(tszMessage, LS(SID_CONSIDER_DELETING_IT));
            CPlayerScreen::GetPlayerScreen()->SetMessageText(tszMessage, CSystemMessageString::REALTIME_INFO);
            delete pLoadInfo;
            // clear the invalid constraint to prevent trying again
            delete [] m_szPlaylistURL;
            m_szPlaylistURL = NULL;
            put_event(EVENT_PLAYLIST_LOAD_END, (void*)0);
        }
        else
        {
            pLoadInfo->index = 0;
            put_event(EVENT_PLAYLIST_LOAD_BEGIN, (void*)pLoadInfo->records.Size());
            put_event(EVENT_PLAYLIST_LOAD, (void*)pLoadInfo);
        }

        SetMainThreadPriority(nPrio);
    }
    else
    {
        MediaRecordList mrlTracks;
        CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
        if (m_iAlbumKey == CMK_ALL)
        {
            if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD)
            {
                pCM->GetMediaRecords(mrlTracks,m_iArtistKey,m_iAlbumKey,m_iGenreKey,
                    CDJPlayerState::GetInstance()->GetCDDataSource()->GetInstanceID());
            }
            else
            {
                pCM->GetMediaRecordsSorted(mrlTracks,m_iArtistKey,m_iAlbumKey,m_iGenreKey,
                    CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID(),
                    1, MDA_TITLE);
            }
        }
        else
        {
            pCM->GetMediaRecordsAlbumSorted(mrlTracks,m_iArtistKey,m_iAlbumKey,m_iGenreKey,
                CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD ?
                CDJPlayerState::GetInstance()->GetCDDataSource()->GetInstanceID() :
                CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID());
        }

        //DumpPlaylistContents(mrlTracks);
        pPL->AddEntries(mrlTracks);

        // If the playlist was empty, reset the current entry here; this randomizes the first track
        if( bWasEmpty ) {
            pPL->SetCurrentEntry(pPL->GetEntry(0, CPlayManager::GetInstance()->GetPlaylistMode()));
        }
    }
}

CPlaylistConstraint::CPlaylistConstraint() : m_iArtistKey(CMK_ALL), m_iAlbumKey(CMK_ALL), m_iGenreKey(CMK_ALL), m_pContentRecord(NULL), m_szPlaylistURL(NULL)
{
}

CPlaylistConstraint::~CPlaylistConstraint()
{
    delete [] m_szPlaylistURL;
}

void CPlaylistConstraint::Constrain(int artist, int album, int genre)
{
    // the file based constraints are removed if the user selects a metadata constraint.
    delete [] m_szPlaylistURL;
    m_szPlaylistURL = NULL;
    m_pContentRecord = NULL;

    m_iArtistKey = artist;
    m_iAlbumKey = album;
    m_iGenreKey = genre;
}

void CPlaylistConstraint::SetArtist(int artist)
{
    m_iArtistKey = artist;
}

void CPlaylistConstraint::SetAlbum(int album)
{
    m_iAlbumKey = album;
}

void CPlaylistConstraint::SetGenre(int genre)
{
    m_iGenreKey = genre;
}

void CPlaylistConstraint::SetTrack(IMediaContentRecord* pMCR)
{
    // the playlist based constraint is removed if the user selects a track constraint.
    delete [] m_szPlaylistURL;
    m_szPlaylistURL = NULL;

    m_pContentRecord = pMCR;
}

void CPlaylistConstraint::SetPlaylistURL(const char* url)
{
    // the track based constraint is removed if the user selects a playlist constraint.
    m_pContentRecord = NULL;

    delete [] m_szPlaylistURL;
    m_szPlaylistURL = new char[strlen(url)+1];
    strcpy(m_szPlaylistURL,url);
}
