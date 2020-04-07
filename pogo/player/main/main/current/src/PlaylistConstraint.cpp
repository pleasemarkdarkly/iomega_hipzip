#include <main/main/PlaylistConstraint.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/SystemMessageScreen.h>
#include <main/main/AppSettings.h>     // PLAYLIST_STRING_SIZE
#include <main/main/FatHelper.h>
#include <main/main/Events.h>
#include <main/main/Recorder.h>

#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <main/content/metakitcontentmanager/MetakitContentManager.h>    // for CMetakitMediaContentRecord, and access to GetArtist et. al.
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <stdio.h>
#include <main/util/filenamestore/FileNameStore.h>

#include <util/registry/Registry.h>
static const RegKey PlaylistConstraintRegKey = REGKEY_CREATE( PLAYLIST_CONSTRAINT_REGISTRY_KEY_TYPE, PLAYLIST_CONSTRAINT_REGISTRY_KEY_NAME );

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_PLAYLIST_CONSTRAINT, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_PLAYLIST_CONSTRAINT);

static CPlaylistConstraint* m_pInstance;

#define SAVED_METADATA_MAX_CHARS (PLAYLIST_STRING_SIZE)
#define SAVED_URL_MAX_CHARS (PLAYLIST_STRING_SIZE*2)

struct PlaylistConstraintSavedSettings 
{
    char szGenre[SAVED_METADATA_MAX_CHARS];    
    char szArtist[SAVED_METADATA_MAX_CHARS];    
    char szAlbum[SAVED_METADATA_MAX_CHARS];    
    char szMediaURL[SAVED_URL_MAX_CHARS];
    char szPlaylistURL[SAVED_URL_MAX_CHARS];
    char szPlaylistEntryURL[SAVED_URL_MAX_CHARS];
    int iTrackTime;
};

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

void CPlaylistConstraint::ResyncCurrentPlaylistEntryById(int nCurrentEntryId, bool *pbCrntStillInList)
{
    DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, "pc:resync crnt\n"); 
    IPlaylist* pPL = CPlayManager::GetInstance()->GetPlaylist();
    bool bCurrentInList = false;
    if (nCurrentEntryId)
    {
        for (int i = 0; i < pPL->GetSize(); ++i)
        {
            IPlaylistEntry* entry = pPL->GetEntry(i);
            if (!entry)
                break;
            IContentRecord* content = entry->GetContentRecord();
            if (!content)
                break;
            if (content->GetID() == nCurrentEntryId)
            {
                pPL->SetCurrentEntry(entry);
                bCurrentInList = true;
            }
        }
    }
    if (!bCurrentInList)
    {
        if (pPL->GetSize() == 0)
            return;
        pPL->SetCurrentEntry(pPL->GetEntry(0));
    }        
    *pbCrntStillInList = bCurrentInList;
}

void DumpPlaylistContents(MediaRecordList &mrlTracks)
{
    int i = 0;
    char temp[256];
    for (MediaRecordIterator it = mrlTracks.GetHead(); it != mrlTracks.GetEnd(); ++it)
    {
        void* pdata;
        (*it)->GetAttribute(MDA_ARTIST, &pdata);
        TcharToChar(temp,(TCHAR*)pdata);
        DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, "track %d has artist '%s'", i++, temp);
        (*it)->GetAttribute(MDA_ALBUM, &pdata);
        TcharToChar(temp,(TCHAR*)pdata);
        DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, ", album '%s'", temp);
        (*it)->GetAttribute(MDA_GENRE, &pdata);
        TcharToChar(temp,(TCHAR*)pdata);
        DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, ", genre '%s'\n", temp);
    }
}

void CPlaylistConstraint::HandleInvalidPlaylist()
{
    DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, "pc:hndl invalid pl\n"); 
    CPlayerScreen* ps = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
    // hide menus: they might show through the system message screen
    ps->HideMenus();
    // notify the user
    CSystemMessageScreen::GetSystemMessageScreen()->ShowScreen(ps, CSystemMessageScreen::INVALID_PLAYLIST);
    // remove any constraint dots
    ps->UpdateConstraintDots(false, false, false, false);
    // clear out screen data.  this needs to happen after constraint dots are cleared, if we are to clear the set text.
    ps->ClearTrack();
    // set the control symbol to Stopped
    ps->SetControlSymbol(CPlayerScreen::STOP);
}

bool CPlaylistConstraint::UpdatePlaylist(bool *pbCrntStillInList)
{
    DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, "pc:update pl\n"); 
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->UpdateConstraintDots(m_iGenreKey != CMK_ALL || m_pPlaylistFileNameRef != 0, 
                                                                              m_iArtistKey != CMK_ALL, 
                                                                              m_iAlbumKey != CMK_ALL, 
                                                                              m_pContentRecord != 0);
    CPogoPlaylist* pPL = (CPogoPlaylist*)CPlayManager::GetInstance()->GetPlaylist();
    IContentRecord* pCurrentContentRec = 0;
    int iCurrentId = 0;
    if (!pPL->IsEmpty())
    {
        pCurrentContentRec = pPL->GetCurrentEntry()->GetContentRecord();
        iCurrentId = pCurrentContentRec->GetID();
    }
    if (m_pContentRecord)
    {
        pPL->Clear();
        pPL->AddEntry(m_pContentRecord);
        pPL->ResortSortEntries();
    }
    else if (m_pPlaylistFileNameRef)
    {
        // look up the playlist format from the file extension
        CPlaylistFormatManager* pfm = CPlaylistFormatManager::GetInstance();
        char* ext = ExtensionFromFilename(m_pPlaylistFileNameRef->Name());
        int id = pfm->FindPlaylistFormat(ext);
        // generate a Set text string to indicate playlist origination
        TCHAR temp[PLAYLIST_STRING_SIZE];
        if (strlen(m_pPlaylistFileNameRef->LongName()) < PLAYLIST_STRING_SIZE)
            CharToTchar(temp,m_pPlaylistFileNameRef->LongName());
        else
            CharToTchar(temp,m_pPlaylistFileNameRef->Name());
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetSetText(temp);
        pPL->Clear();
        // load the files into the playlist
        const char* szURL = m_pPlaylistFileNameRef->URL();
        if (!SUCCEEDED(pfm->LoadPlaylist(id, szURL, pPL, true)))
        {
            HandleInvalidPlaylist();
            return false;         
        }
        if (m_pPlaylistFileNameRef->DynamicAlloc())
            delete [] szURL;
    }
    else
    {
        MediaRecordList mrlTracks;
        IQueryableContentManager* qcm = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
        if (m_iGenreKey != CMK_ALL) {
            // (epg,10/11/2001): todo: this may make the playerscreen unhappy, trying to draw when it doesn't have focus.  if so, I'll have to make a non drawing version.
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetSetText(qcm->GetGenreByKey(m_iGenreKey));
        }
        qcm->GetMediaRecords(mrlTracks,m_iArtistKey,m_iAlbumKey,m_iGenreKey);
        //DumpPlaylistContents(mrlTracks);
        pPL->Clear();
        pPL->AddEntries(mrlTracks);
    }
    ResyncCurrentPlaylistEntryById(iCurrentId, pbCrntStillInList);
}

CPlaylistConstraint::CPlaylistConstraint() : m_iArtistKey(CMK_ALL), m_iAlbumKey(CMK_ALL), m_iGenreKey(CMK_ALL), m_bActiveRecordSession(false), m_pContentRecord(NULL), m_pPlaylistFileNameRef(NULL)
{
}

CPlaylistConstraint::~CPlaylistConstraint()
{
    delete m_pPlaylistFileNameRef;
}

void CPlaylistConstraint::Constrain(int artist, int album, int genre, bool bRecordSession)
{
    DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, "pc:constrain\n"); 
    // the file based constraints are removed if the user selects a metadata constraint.
    delete m_pPlaylistFileNameRef;
    m_pPlaylistFileNameRef = NULL;
    m_pContentRecord = NULL;

    m_iArtistKey = artist;
    m_iAlbumKey = album;
    m_iGenreKey = genre;

    if (m_bActiveRecordSession && !bRecordSession)
    {
        m_bActiveRecordSession = false;
        CRecorder::GetInstance()->CloseSession();
    }
    else
        m_bActiveRecordSession = bRecordSession;
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
    m_pContentRecord = pMCR;
}

void CPlaylistConstraint::SetPlaylistFileNameRef(IFileNameRef* file)
{
    delete m_pPlaylistFileNameRef;
    if (file != 0)
        m_pPlaylistFileNameRef = (**((CStoreFileNameRef*)file))->GetRef();
    else
        m_pPlaylistFileNameRef = 0;
}

// load registry settings if possible, or else create the entry in the registry for next time.
// I had this code in the ctor, but that creates issues with order of creation (playlist queried for mode before its creation).
void CPlaylistConstraint::InitRegistry()
{
    int* settings = (int*) new unsigned char[ GetStateSize() + 1 ];
    SaveState((void*)settings, GetStateSize());
    CRegistry::GetInstance()->AddItem( PlaylistConstraintRegKey, (void*)settings, REGFLAG_PERSISTENT, GetStateSize() );
}

int CPlaylistConstraint::SaveToRegistry() 
{
    DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_TRACE, "pc:SaveToReg\n");
    void* buf = CRegistry::GetInstance()->FindByKey( PlaylistConstraintRegKey );
    if( ! buf ) {
        DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_WARNING, "PC:Couldn't Find Registry Key\n");
        return 0;
    } else {
        SaveState( buf, GetStateSize() );
        return 1;
    }
}

int CPlaylistConstraint::RestoreFromRegistry() 
{
    DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_TRACE, "pc:RestFrReg\n");
    void* buf = CRegistry::GetInstance()->FindByKey( PlaylistConstraintRegKey );
    if( !buf ) {
        InitRegistry();
    } else {
        RestoreState( buf, GetStateSize() );
    }
    return 1;
}

// write all persistent members into the buf stream, not to exceed len bytes.
void CPlaylistConstraint::SaveState(void* buf, int len)
{ 
    // normalize the buffer to zeros.
    memset (buf,0,sizeof(PlaylistConstraintSavedSettings));
    // cast a convenience struct pointer on top of the buffer
    PlaylistConstraintSavedSettings* settings = (PlaylistConstraintSavedSettings*) buf;
    CMetakitContentManager* mcm = (CMetakitContentManager*) CPlayManager::GetInstance()->GetContentManager();
    // copy artist into the buffer
    if (m_iArtistKey)
    {
        const TCHAR* tszArtist = mcm->GetArtistByKey(m_iArtistKey);
        if (tszArtist && tstrlen(tszArtist) < SAVED_METADATA_MAX_CHARS)
            TcharToChar(settings->szArtist, tszArtist);
    }
    // copy genre into the buffer
    if (m_iGenreKey)
    {
        const TCHAR* tszGenre = mcm->GetGenreByKey(m_iGenreKey);
        if (tszGenre && tstrlen(tszGenre) < SAVED_METADATA_MAX_CHARS)
            TcharToChar(settings->szGenre, tszGenre);
    }
    // copy album into the buffer
    if (m_iAlbumKey)
    {
        const TCHAR* tszAlbum = mcm->GetAlbumByKey(m_iAlbumKey);
        if (tszAlbum && tstrlen(tszAlbum) < SAVED_METADATA_MAX_CHARS)
            TcharToChar(settings->szAlbum, tszAlbum);
    }
    // copy media url into the buffer
    if (m_pContentRecord)
    {
        char* url = m_pContentRecord->GetFileNameRef()->URL();
        if (strlen(url) < SAVED_URL_MAX_CHARS)
            strcpy(settings->szMediaURL, url);
        if (m_pContentRecord->GetFileNameRef()->DynamicAlloc())
            delete url;
    }
    // copy playlist url into the buffer
    if (m_pPlaylistFileNameRef)
    {
        char* url = m_pPlaylistFileNameRef->URL();
        if (strlen(url) < SAVED_URL_MAX_CHARS)
            strcpy(settings->szPlaylistURL, url);
        if (m_pPlaylistFileNameRef->DynamicAlloc())
            delete url;
    }
    // copy playlist entry url into the buffer
    IPlaylist* pl = CPlayManager::GetInstance()->GetPlaylist();
    IPlaylistEntry* entry = pl->GetCurrentEntry();
    if (entry)
    {
        char* url = entry->GetContentRecord()->GetFileNameRef()->URL();
        if (strlen(url) < SAVED_URL_MAX_CHARS)
            strcpy(settings->szPlaylistEntryURL,url);
        if (entry->GetContentRecord()->GetFileNameRef()->DynamicAlloc())
            delete url;
    }
    // copy current track time into the buffer
    if (CMediaPlayer::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
        settings->iTrackTime = CMediaPlayer::GetInstance()->GetTrackTime();
}
// read all persistent members from the buf stream, not to exceed len bytes.
void CPlaylistConstraint::RestoreState(void* buf, int len)
{
    // cast a convenience struct pointer on top of the buffer
    PlaylistConstraintSavedSettings* settings = (PlaylistConstraintSavedSettings*) buf;
    CMetakitContentManager* mcm = (CMetakitContentManager*) CPlayManager::GetInstance()->GetContentManager();
    TCHAR tszTemp[SAVED_URL_MAX_CHARS];
    // copy artist from the buffer
    if (settings->szArtist[0])
    { 
        CharToTchar(tszTemp, settings->szArtist);
        m_iArtistKey = mcm->GetArtistKey(tszTemp);
    }
    // copy genre from the buffer
    if (settings->szGenre[0])
    {
        CharToTchar(tszTemp, settings->szGenre);
        m_iGenreKey = mcm->GetGenreKey(tszTemp);
    }
    // copy album from the buffer
    if (settings->szAlbum[0])
    {
        CharToTchar(tszTemp, settings->szAlbum);
        m_iAlbumKey = mcm->GetAlbumKey(tszTemp);
    }
    // copy media url from the buffer
    if (settings->szMediaURL[0])
        m_pContentRecord = mcm->GetMediaRecord(settings->szMediaURL);
    // copy playlist url from the buffer
    if (settings->szPlaylistURL[0])
        SetPlaylistFileNameRef(mcm->GetFileNameStore()->GetRefByURL(settings->szPlaylistURL));
    // stop playback so that loading the playlist won't result in immediate playback (to select the right track, if possible)
    CPlayManager::GetInstance()->Stop();
    // load up the relevant playlist
    bool bCrntStillInList = false;
    // update the playlist.  if this fails, further refinements are unnecessary
    if (!UpdatePlaylist(&bCrntStillInList))
        return;
    CPogoPlaylist* pl = (CPogoPlaylist*)CPlayManager::GetInstance()->GetPlaylist();
    // jump to the correct track
    bool bSynced = false;
    if (settings->szPlaylistEntryURL[0])
    {
        if (IContentRecord* content = mcm->GetMediaRecord(settings->szPlaylistEntryURL))
        {
            IPlaylistEntry* entry = pl->FindEntryByContentRecord(content);
            if (entry)
            {
                pl->SetCurrentEntry(entry);
                bSynced = true;
                SyncPlayerToPlaylist();
            }
        }
    }
    // if we're in a recording session, then we should jump to the last entry.  
    // (during recording, the current playlist isn't aware of the track being created, so this won't get saved into settings in the normal way)
    else if (CRecorder::GetInstance()->InSession())
        pl->SetCurrentEntry(pl->GetEntry(pl->GetSize()-1,IPlaylist::NORMAL));
    if (!bSynced)
        SyncPlayerToPlaylist();
}

// specify the byte count allowed in the state stream.
int CPlaylistConstraint::GetStateSize()
{
    return sizeof(PlaylistConstraintSavedSettings) + 1;
}

void CPlaylistConstraint::SyncPlayerToPlaylist()
{
    DEBUGP( DBG_PLAYLIST_CONSTRAINT, DBGLEV_INFO, "pc:sync plyr-pl\n"); 
    IPlaylist* pPL = CPlayManager::GetInstance()->GetPlaylist();
    bool bPlaying = (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING);
    CMediaPlayer::GetInstance()->Deconfigure();
    if (pPL->GetSize())
    {
        if (g_pEvents->SetCurrentSong() && bPlaying)
            CPlayManager::GetInstance()->Play();
    }
}