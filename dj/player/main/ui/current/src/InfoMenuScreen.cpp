//
// InfoMenuScreen.cpp: the menu screen that shows all info available for a specific object
// danb@fullplaymedia.com 04/15/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/InfoMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/EditScreen.h>
#include <main/ui/AlertScreen.h>
#include <main/ui/LibraryMenuScreen.h>
#include <main/ui/MainMenuScreen.h>
#include <main/ui/PlayerScreen.h>

#include <core/playmanager/PlayManager.h>
#include <main/main/DJPlayerState.h>
#include <main/main/ProgressWatcher.h>

#include <main/metadata/metadatafiletag/MetadataFileTag.h>

#ifdef DDOMOD_DJ_BUFFERING
#include <main/buffering/BufferInStream.h>
#endif

#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <content/common/QueryableContentManager.h>

#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
#include <main/content/datasourcecontentmanager/DataSourceContentManager.h>
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
#include <main/content/djcontentmanager/DJContentManager.h>
#endif

#include <stdio.h> // sprintf

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_INFO_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_INFO_SCREEN );

// the global reference for this class
CInfoMenuScreen* CInfoMenuScreen::s_pInfoMenuScreen = 0;

// This is a singleton class.
CInfoMenuScreen*
CInfoMenuScreen::GetInfoMenuScreen()
{
	if (!s_pInfoMenuScreen) {
		s_pInfoMenuScreen = new CInfoMenuScreen(NULL);
	}
	return s_pInfoMenuScreen;
}


CInfoMenuScreen::CInfoMenuScreen(CScreen* pParent)
	: CDynamicMenuScreen(NULL, SID_INFO)
{
    SetItemCount(0);
}


CInfoMenuScreen::~CInfoMenuScreen()
{
}


SIGNED
CInfoMenuScreen::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
    case PM_KEY:
        switch (Mesg.iData)
        {
		case IR_KEY_INFO:
            if( Parent() == CPlayerScreen::GetPlayerScreen() )
            {
			    GotoPreviousMenu();
                CPlayerScreen::GetPlayerScreen()->Invalidate(mReal);
                CPlayerScreen::GetPlayerScreen()->ResumeScrollingText();
            }
            else
			    GotoPreviousMenu();
			return 0;

        case IR_KEY_EDIT:
            EditItemInFocus();
            return 0;
            
        // drop these keys
		case IR_KEY_SAVE:
		case IR_KEY_SELECT:
        case IR_KEY_CLEAR:
        case IR_KEY_DELETE:
        case IR_KEY_ADD:
			return 0;

        default:
            break;
        }
        break;
       
    default:
        break;
    }

    return CDynamicMenuScreen::Message(Mesg);
}


bool 
CInfoMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
	return false;
}


const TCHAR* 
CInfoMenuScreen::MenuItemCaption(int iMenuIndex)
{
    if(iMenuIndex < m_vCaptions.Size() && iMenuIndex >= 0)
        return m_vCaptions[iMenuIndex].pszCaption;
    else
        return LS(SID_EMPTY_STRING);
}


void
CInfoMenuScreen::ClearCaptions()
{
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:ClearCaptions\n");
    CaptionItem_t Caption;
    while(!m_vCaptions.IsEmpty())
    {
        Caption = m_vCaptions.PopBack();
        free(Caption.pszCaption);
        free(Caption.pszCaptionTitle);
        free(Caption.pszCaptionValue);
        free(Caption.pszSavedValue);
    }
    SetItemCount(0);
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:ClearCaptions -\n");
}


// Copies Caption Item Info into a vector
void
CInfoMenuScreen::AddCaptionItem(int iAttributeID, const TCHAR* pszCaptionTitle, const TCHAR* pszCaptionValue, bool bEditable, bool bNumeric)
{
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:AddCaptionItem %w %d\n", pszCaptionTitle, bNumeric);
    if(!pszCaptionTitle && ! pszCaptionValue)
        return;

    // create the full caption by putting the title and caption value together
    TCHAR* pszNewCaption = (TCHAR*)malloc((tstrlen(pszCaptionTitle) + tstrlen(pszCaptionValue) + 1) * sizeof(TCHAR));
    tstrcpy(pszNewCaption, pszCaptionTitle);
    tstrcat(pszNewCaption, pszCaptionValue);

    // make a new caption item object to put all of our groovy info into so we can play with it later.
    CaptionItem_t Caption;
    // put the edit info in as well
    Caption.iAttributeID = iAttributeID;
    Caption.bEditable = bEditable;
    Caption.bNumeric = bNumeric;
    Caption.bChanged = false;
    // copy the full caption to the vector
    Caption.pszCaption = (TCHAR*)malloc((tstrlen(pszNewCaption) + 1) * sizeof(TCHAR));
    tstrcpy(Caption.pszCaption, pszNewCaption);
    // copy the caption title to the vector
    Caption.pszCaptionTitle = (TCHAR*)malloc((tstrlen(pszCaptionTitle) + 1) * sizeof(TCHAR));
    tstrcpy(Caption.pszCaptionTitle, pszCaptionTitle);
    // copy the caption value to the vector
    Caption.pszCaptionValue = (TCHAR*)malloc((tstrlen(pszCaptionValue) + 1) * sizeof(TCHAR));
    tstrcpy(Caption.pszCaptionValue, pszCaptionValue);
    // copy the caption value to the saved value
    Caption.pszSavedValue = (TCHAR*)malloc((tstrlen(pszCaptionValue) + 1) * sizeof(TCHAR));
    tstrcpy(Caption.pszSavedValue, pszCaptionValue);

    // put the new item in the vector
    m_vCaptions.PushBack(Caption);
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_INFO, "info: %w\n", Caption.pszCaption);

    free(pszNewCaption);
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:AddCaptionItem -\n");
}


void
CInfoMenuScreen::AddCaptionItem(int iAttributeID, const char* pszCaptionTitle, const TCHAR* pszCaptionValue, bool bEditable, bool bNumeric)
{
    TCHAR* pszNewCaptionTitle = (TCHAR*)malloc((strlen(pszCaptionTitle) + 1) * sizeof(TCHAR));
    CharToTchar(pszNewCaptionTitle, pszCaptionTitle);
    AddCaptionItem(iAttributeID, pszNewCaptionTitle, pszCaptionValue, bEditable, bNumeric);
    free(pszNewCaptionTitle);
}


void
CInfoMenuScreen::AddCaptionItem(int iAttributeID, const TCHAR* pszCaptionTitle, const char* pszCaptionValue, bool bEditable, bool bNumeric)
{
    TCHAR* pszNewCaption = (TCHAR*)malloc((strlen(pszCaptionValue) + 1) * sizeof(TCHAR));
    CharToTchar(pszNewCaption, pszCaptionValue);
    AddCaptionItem(iAttributeID, pszCaptionTitle, pszNewCaption, bEditable, bNumeric);
    free(pszNewCaption);
}


void
CInfoMenuScreen::AddCaptionItem(int iAttributeID, const char* pszCaptionTitle, const char* pszCaptionValue, bool bEditable, bool bNumeric)
{
    TCHAR* pszNewCaptionTitle = (TCHAR*)malloc((strlen(pszCaptionTitle) + 1) * sizeof(TCHAR));
    TCHAR* pszNewCaption = (TCHAR*)malloc((strlen(pszCaptionValue) + 1) * sizeof(TCHAR));
    CharToTchar(pszNewCaptionTitle, pszCaptionTitle);
    CharToTchar(pszNewCaption, pszCaptionValue);
    AddCaptionItem(iAttributeID, pszNewCaptionTitle, pszNewCaption, bEditable, bNumeric);
    free(pszNewCaptionTitle);
    free(pszNewCaption);
}


void
CInfoMenuScreen::AddCaptionItem(int iAttributeID, const TCHAR* pszCaptionTitle, int iValue, bool bEditable, bool bNumeric)
{
    char szValue[32];
    sprintf(szValue, "%d", iValue);
    AddCaptionItem(iAttributeID, pszCaptionTitle, szValue, bEditable, bNumeric);
}


void
CInfoMenuScreen::AddCaptionItem(int iAttributeID, const char* pszCaptionTitle, int iValue, bool bEditable, bool bNumeric)
{
    char szValue[32];
    sprintf(szValue, "%d", iValue);
    TCHAR* pszNewCaptionTitle = (TCHAR*)malloc((strlen(pszCaptionTitle) + 1) * sizeof(TCHAR));
    CharToTchar(pszNewCaptionTitle, pszCaptionTitle);
    AddCaptionItem(iAttributeID, pszNewCaptionTitle, szValue, bEditable, bNumeric);
    free(pszNewCaptionTitle);
}


void
CInfoMenuScreen::SetTrackInfo(IMediaContentRecord* pContentRecord)
{
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:SetTrackInfo\n");
    ClearCaptions();

    m_wScreenTitleSID = SID_TRACK_INFO;

    // make a one item list with this tracks info
    m_mrlTracks.Clear();
    m_mrlTracks.PushBack(pContentRecord);

    // make sure we have a good Content Record
    if (NULL == pContentRecord)
    {
        DEBUGP( DBG_INFO_SCREEN, DBGLEV_WARNING, "info:SetTrackInfo: Bad Content Record\n");
        SetItemCount(0);
        m_bChanged = false;
        return;
    }

    int iTitleIndex = 0;

    // try to display every possible attribute of the file
    TCHAR* pTchar;
    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_GENRE, (void**)&pTchar)))
    {
        AddCaptionItem(MDA_GENRE, "Genre: ", pTchar, true);
        ++iTitleIndex;
    }

    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_ARTIST, (void**)&pTchar)))
    {
        AddCaptionItem(MDA_ARTIST, "Artist: ", pTchar, true);
        ++iTitleIndex;
    }

    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_ALBUM, (void**)&pTchar)))
    {
        AddCaptionItem(MDA_ALBUM, "Album: ", pTchar, true);
        ++iTitleIndex;
    }

    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_TITLE, (void**)&pTchar)))
    {
        AddCaptionItem(MDA_TITLE, "Title: ", pTchar, true);
        SetHighlightedIndex(iTitleIndex);
    }

    int iInt = 0;
    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_ALBUM_TRACK_NUMBER, (void**)&iInt)))
        AddCaptionItem(MDA_ALBUM_TRACK_NUMBER, "Track Number: ", iInt, true);

    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_YEAR, (void**)&iInt)))
        AddCaptionItem(MDA_YEAR, "Year: ", iInt, true);

    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_DURATION, (void**)&iInt)))
        AddCaptionItem(MDA_DURATION, "Duration: ", iInt, false);

    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_BITRATE, (void**)&iInt)))
        AddCaptionItem(MDA_BITRATE, "Bitrate: ", iInt, false);

    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_FILE_SIZE, (void**)&iInt)))
        AddCaptionItem(MDA_FILE_SIZE, "File Size: ", iInt, false);

    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_HAS_DRM, (void**)&iInt)))
        if (iInt == 1)
            AddCaptionItem(MDA_HAS_DRM, "DRM: Yes", LS(SID_EMPTY_STRING), false);
        else
            AddCaptionItem(MDA_HAS_DRM, "DRM: No", LS(SID_EMPTY_STRING), false);

    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_SAMPLING_FREQUENCY, (void**)&iInt)))
        AddCaptionItem(MDA_SAMPLING_FREQUENCY, "Sampling Frequency: ", iInt, false);

    if (SUCCEEDED(pContentRecord->GetAttribute(MDA_CHANNELS, (void**)&iInt)))
        AddCaptionItem(MDA_CHANNELS, "Channels: ", iInt, false);

    SetItemCount(m_vCaptions.Size());
    m_bChanged = false;

    // get the source for this content record so we know how to deal with editing, etc...
    IDataSource* pDataSource = CDataSourceManager::GetInstance()->GetDataSourceByID(pContentRecord->GetDataSourceID());
    if (pDataSource)
    {
        switch (pDataSource->GetClassID())
        {
        case CD_DATA_SOURCE_CLASS_ID:
            m_eSource = CDJPlayerState::CD;
            break;
        case FAT_DATA_SOURCE_CLASS_ID:
            m_eSource = CDJPlayerState::HD;
            break;
        case NET_DATA_SOURCE_CLASS_ID:
            m_eSource = CDJPlayerState::FML;
        default:
            // yikes
            break;
        }
    }
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:SetTrackInfo -\n");
}

// Give an Album Media Record Key and extract all info for this album from
//  the IQueryableContentManager
void
CInfoMenuScreen::SetAlbumInfo(int iAlbumKey, CDJPlayerState::ESourceMode eSource)
{
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:SetAlbumInfo\n");
    IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
    CDJPlayerState* pDJPlayerState = CDJPlayerState::GetInstance();

    int iDSID = 0; // nada
    m_eSource = eSource;
    switch (m_eSource)
    {
    case CDJPlayerState::CD:
        iDSID = pDJPlayerState->GetCDDataSource()->GetInstanceID();
        break;
    case CDJPlayerState::HD:
        iDSID = pDJPlayerState->GetFatDataSource()->GetInstanceID();
        break;
    case CDJPlayerState::FML:
        iDSID = pDJPlayerState->GetNetDataSource()->GetInstanceID();
    default:
        // yikes again.
        break;
    }

    // get a list of all the tracks
    m_mrlTracks.Clear();
    pQCM->GetMediaRecords(m_mrlTracks, CMK_ALL, iAlbumKey, CMK_ALL, iDSID);
    if (m_mrlTracks.IsEmpty())
    {
        DEBUGP( DBG_INFO_SCREEN, DBGLEV_WARNING, "info:SetAlbumInfo: No Tracks Available for this Album\n");
        SetItemCount(0);
        m_bChanged = false;
        return;
    }

    ClearCaptions();

    m_wScreenTitleSID = SID_ALBUM_INFO;

    int iAlbumIndex = 0;
    
    // try to display every possible attribute of the file
    m_vGenres.Clear();
    pQCM->GetGenres(m_vGenres, CMK_ALL, iAlbumKey, iDSID);
    for (int i = 0; i < m_vGenres.Size(); ++i)
    {
        AddCaptionItem(MDA_GENRE, "Genre: ", m_vGenres[i].szValue, true);
        ++iAlbumIndex;
    }

    m_vArtists.Clear();
    pQCM->GetArtists(m_vArtists, iAlbumKey, CMK_ALL, iDSID);
    for (int i = 0; i < m_vArtists.Size(); ++i)
    {
        AddCaptionItem(MDA_ARTIST, "Artist: ", m_vArtists[i].szValue, true);
        ++iAlbumIndex;
    }

    const TCHAR* pTchar = pQCM->GetAlbumByKey(iAlbumKey);
    if (pTchar)
    {
        AddCaptionItem(MDA_ALBUM, "Album: ", pTchar, true);
        SetHighlightedIndex(iAlbumIndex);
    }

    SetItemCount(m_vCaptions.Size());
    m_bChanged = false;
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:SetAlbumInfo -\n");
}


// Give an Artist Media Record Key and extract all info for this artist from
//  the IQueryableContentManager
void
CInfoMenuScreen::SetArtistInfo(int iArtistKey, CDJPlayerState::ESourceMode eSource)
{
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:SetArtistInfo\n");
    IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
    CDJPlayerState* pDJPlayerState = CDJPlayerState::GetInstance();

    int iDSID = 0; // nada
    m_eSource = eSource;
    switch (m_eSource)
    {
    case CDJPlayerState::CD:
        iDSID = pDJPlayerState->GetCDDataSource()->GetInstanceID();
        break;
    case CDJPlayerState::HD:
        iDSID = pDJPlayerState->GetFatDataSource()->GetInstanceID();
        break;
    case CDJPlayerState::FML:
        iDSID = pDJPlayerState->GetNetDataSource()->GetInstanceID();
    default:
        // yikes again.
        break;
    }

    m_mrlTracks.Clear();
    pQCM->GetMediaRecords(m_mrlTracks, iArtistKey, CMK_ALL, CMK_ALL, iDSID);
    if (m_mrlTracks.IsEmpty())
    {
        DEBUGP( DBG_INFO_SCREEN, DBGLEV_WARNING, "info:SetArtistInfo: No Tracks Available for this Album\n");
        SetItemCount(0);
        m_bChanged = false;
        return;
    }
    
    ClearCaptions();

    m_wScreenTitleSID = SID_ARTIST_INFO;

    int iArtistIndex = 0;
    
    m_vGenres.Clear();
    pQCM->GetGenres(m_vGenres, iArtistKey, CMK_ALL, iDSID);
    for (int i = 0; i < m_vGenres.Size(); ++i)
    {
        AddCaptionItem(MDA_GENRE, "Genre: ", m_vGenres[i].szValue, true);
        ++iArtistIndex;
    }
    
    const TCHAR* pTchar = pQCM->GetArtistByKey(iArtistKey);
    if (pTchar)
    {
        AddCaptionItem(MDA_ARTIST, "Artist: ", pTchar, true);
        SetHighlightedIndex(iArtistIndex);
    }
    
    SetItemCount(m_vCaptions.Size());
    m_bChanged = false;
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:SetArtistInfo -\n");
}


// Give an Genre Media Record Key and extract all info for this genre from
//  the IQueryableContentManager
void
CInfoMenuScreen::SetGenreInfo(int iGenreKey, CDJPlayerState::ESourceMode eSource)
{
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:SetGenreInfo\n");
    IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
    CDJPlayerState* pDJPlayerState = CDJPlayerState::GetInstance();

    int iDSID = 0; // nada
    m_eSource = eSource;
    switch (m_eSource)
    {
    case CDJPlayerState::CD:
        iDSID = pDJPlayerState->GetCDDataSource()->GetInstanceID();
        break;
    case CDJPlayerState::HD:
        iDSID = pDJPlayerState->GetFatDataSource()->GetInstanceID();
        break;
    case CDJPlayerState::FML:
        iDSID = pDJPlayerState->GetNetDataSource()->GetInstanceID();
    default:
        // yikes again.
        break;
    }

    m_mrlTracks.Clear();
    pQCM->GetMediaRecords(m_mrlTracks, CMK_ALL, CMK_ALL, iGenreKey, iDSID);
    if (m_mrlTracks.IsEmpty())
    {
        DEBUGP( DBG_INFO_SCREEN, DBGLEV_WARNING, "info:SetGenreInfo: No Tracks Available for this Album\n");
        SetItemCount(0);
        m_bChanged = false;
        return;
    }
    
    ClearCaptions();

    m_wScreenTitleSID = SID_GENRE_INFO;
    
    const TCHAR* pTchar = pQCM->GetGenreByKey(iGenreKey);
    if (pTchar)
        AddCaptionItem(MDA_GENRE, "Genre: ", pTchar, true);
    
    SetItemCount(m_vCaptions.Size());
    m_bChanged = false;
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:SetGenreInfo -\n");
}

void
CInfoMenuScreen::CommitChanges()
{
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:CommitChanges\n");
    if (!m_bChanged)
    {
        DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info: no changes to commit\n");
        return;
    }

	// Remember that we've started an update -- this will trigger a content rescan if the device reboots.
	CProgressWatcher::GetInstance()->SetTask(TASK_CONTENT_UPDATE);

    CAlertScreen::GetInstance()->Config(this, 10);
    CAlertScreen::GetInstance()->SetTitleText(LS(SID_SAVE));
    CAlertScreen::GetInstance()->SetActionText(LS(SID_PREPARING_TO_SAVE_CHANGES));
    CAlertScreen::GetInstance()->SetMessageText(LS(SID_COUNTING_FILES));
    Add(CAlertScreen::GetInstance());
    
    int iNumberOfTracks   = m_mrlTracks.Size();
    int iNumberOfCaptions = m_vCaptions.Size();

    char cTotalCount[32];
    sprintf(cTotalCount, " %d ", iNumberOfTracks);
    TCHAR tcTotalCount[32];
    CharToTcharN(tcTotalCount, cTotalCount, 32);
    TCHAR tcCaption[128];
    tcCaption[0] = 0;
    tstrcat(tcCaption, LS(SID_CHANGING));
    tstrcat(tcCaption, tcTotalCount);
    if (iNumberOfTracks > 1)
        tstrcat(tcCaption, LS(SID_TRACKS));
    else
        tstrcat(tcCaption, LS(SID_TRACK));
    CAlertScreen::GetInstance()->SetActionText(tcCaption);
    CAlertScreen::GetInstance()->ResetProgressBar(0, iNumberOfTracks);
    
    IMediaContentRecord* pContentRecord;
    bool                 bUpdateContentManager = false;

    m_itTrack = m_mrlTracks.GetHead();
            
    // cycle through our list of tracks
    for (int c = 0; c < iNumberOfTracks; c++)
    {
        pContentRecord = *(m_itTrack);

        // need to skip tracks that shouldn't be changed, but may be in list.
        // these arise in cases, e.g. multiple genres for an artist.
        bool bSuccess = false;
                
        // look at each caption and see if it has been changed
        for (int i = 0; i < iNumberOfCaptions; i++)
        {
            if (m_vCaptions[i].bChanged)
            {
                // If we can get the previous value, and it's what we think the previous value was, and we can set the new value,
                // then we had success
                // dc- handle cases of integer vs. string vars seperately
                if( m_vCaptions[i].bNumeric )
                {
                    int iCurrentValue;
                    if( SUCCEEDED(pContentRecord->GetAttribute(m_vCaptions[i].iAttributeID, (void**)&iCurrentValue)) &&
                        tatoi(m_vCaptions[i].pszSavedValue) == iCurrentValue &&
                        SUCCEEDED(pContentRecord->SetAttribute(m_vCaptions[i].iAttributeID, (void*)tatoi(m_vCaptions[i].pszCaptionValue))) )
                    {
                        bSuccess = true;
                    }
                }
                else
                {
                    TCHAR* szCurrentValue;
                    if( SUCCEEDED(pContentRecord->GetAttribute(m_vCaptions[i].iAttributeID, (void**)&szCurrentValue)) &&
                        tstrcmp(m_vCaptions[i].pszSavedValue, szCurrentValue) == 0 &&
                        SUCCEEDED(pContentRecord->SetAttribute(m_vCaptions[i].iAttributeID, (void*)(m_vCaptions[i].pszCaptionValue))) )
                    {
                        bSuccess = true;
                    }
                }
            }
        }
        if( bSuccess )
        {
            bUpdateContentManager = true;
            // todo:  check to see if updating the actual file fails.  this could easily happen
            //  if we're trying to edit the currently playing song
#ifdef DDOMOD_DJ_BUFFERING
            CBuffering::GetInstance()->PauseHDAccess();
#endif
            CMetadataFileTag::GetInstance()->UpdateTag(pContentRecord->GetURL(), pContentRecord);
#ifdef DDOMOD_DJ_BUFFERING
            CBuffering::GetInstance()->ResumeHDAccess();
#endif
        }
        else
        {
            DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:CommitChanges ERROR: couldn't commit a change\n");
        }

        char cCount[32];
        sprintf(cCount, " %d ", c);
        TCHAR tcCount[32];
        CharToTcharN(tcCount, cCount, 32);
        TCHAR tcCaption[128];
        tcCaption[0] = 0;
        tstrcat(tcCaption, LS(SID_WROTE));
        tstrcat(tcCaption, tcCount);
        tstrcat(tcCaption, LS(SID_OF));
        tstrcat(tcCaption, tcTotalCount);
        CAlertScreen::GetInstance()->SetMessageText(tcCaption);
        CAlertScreen::GetInstance()->UpdateProgressBar(c);
        
        ++m_itTrack;
    }
    CAlertScreen::GetInstance()->UpdateProgressBar(iNumberOfTracks);

    tcCaption[0] = 0;
    tstrcat(tcCaption, LS(SID_SAVING_INFO_TO));
    TCHAR tcSpace[] = { ' ',0 };
    tstrcat(tcCaption, tcSpace);
    tstrcat(tcCaption, LS(SID_HD));
    CAlertScreen::GetInstance()->SetActionText(tcCaption);
    CAlertScreen::GetInstance()->SetMessageText(LS(SID_PLEASE_WAIT));
    
    // Save the content manager DB.
    if (bUpdateContentManager)
    {
#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
        ((CDataSourceContentManager*)CPlayManager::GetInstance()->GetContentManager())->Commit();
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
        ((CDJContentManager*)CPlayManager::GetInstance()->GetContentManager())->Commit();
#endif
    }

    if (m_pParent == CLibraryMenuScreen::GetLibraryMenuScreen())
        ((CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen())->ResynchWithChanges();
    
    // synch the player screen in case we've edited the currently playing track
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->RefreshCurrentTrackMetadata();

    m_bChanged = false;
    for (int i = 0; i < iNumberOfCaptions; i++)
        m_vCaptions[i].bChanged = false;
    
    CAlertScreen::GetInstance()->HideScreen();
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:CommitChanges -\n");
}


void
CInfoMenuScreen::EditItemInFocus()
{
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:EditItemInFocus\n");
    int iMenuIndex = GetHighlightedIndex();

    if (m_eSource == CDJPlayerState::HD && m_cItems && iMenuIndex < m_vCaptions.Size() && m_vCaptions[iMenuIndex].bEditable)
    {
        CEditScreen::GetInstance()->Config(this, EditItemInFocusCallback, m_vCaptions[iMenuIndex].pszCaptionValue, false, m_vCaptions[iMenuIndex].bNumeric, (m_vCaptions[iMenuIndex].bNumeric ? 5 : 80));
        Add(CEditScreen::GetInstance());
    }
}


void
CInfoMenuScreen::EditItemInFocusCallback(bool bSave)
{
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:EditItemInFocusCallback\n");
    if (!s_pInfoMenuScreen)
        return;

    s_pInfoMenuScreen->EditItemInFocusCB(bSave);
}


void
CInfoMenuScreen::EditItemInFocusCB(bool bSave)
{
    DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info:EditItemInFocusCB\n");
    if (bSave)
    {
        int iMenuIndex = GetHighlightedIndex();
        if (m_cItems && iMenuIndex < m_vCaptions.Size() && m_vCaptions[iMenuIndex].bEditable)
        {
            // get the string from the edit string
            const TCHAR* pszNewCaptionValue = CEditScreen::GetInstance()->GetDataString();
            // check to see if the values have changed
            if (tstrcmp(pszNewCaptionValue, m_vCaptions[iMenuIndex].pszCaptionValue))
            {
                DEBUGP( DBG_INFO_SCREEN, DBGLEV_INFO, "info: [%w] Changed To [%w]\n", m_vCaptions[iMenuIndex].pszCaptionValue, pszNewCaptionValue);
                // free what was there previously
                free(m_vCaptions[iMenuIndex].pszCaption);
                free(m_vCaptions[iMenuIndex].pszCaptionValue);

                // save the new caption value
                m_vCaptions[iMenuIndex].pszCaptionValue = (TCHAR*)malloc((tstrlen(pszNewCaptionValue) + 1) * sizeof(TCHAR));
                tstrcpy(m_vCaptions[iMenuIndex].pszCaptionValue, pszNewCaptionValue);

                // create the new caption
                m_vCaptions[iMenuIndex].pszCaption = (TCHAR*)malloc((tstrlen(m_vCaptions[iMenuIndex].pszCaptionTitle) + tstrlen(m_vCaptions[iMenuIndex].pszCaptionValue) + 1) * sizeof(TCHAR));
                tstrcpy(m_vCaptions[iMenuIndex].pszCaption, m_vCaptions[iMenuIndex].pszCaptionTitle);
                tstrcat(m_vCaptions[iMenuIndex].pszCaption, m_vCaptions[iMenuIndex].pszCaptionValue);

                // note that a value was changed so we can save it later
                m_vCaptions[iMenuIndex].bChanged = true;
                m_bChanged = true;

                Draw();

                CommitChanges();
            }
            else
            {
                DEBUGP( DBG_INFO_SCREEN, DBGLEV_TRACE, "info: [%w] is unchanged, don't save\n", pszNewCaptionValue);
            }
        }
    }
}

