
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//


#include <stdlib.h>
#include <stdio.h>
#include <cyg/infra/diag.h>
#include <stdarg.h>

#include <util/debug/debug.h>
#include <util/diag/diag.h>

#include <cyg/kernel/kapi.h>
#include <util/eventq/EventQueueAPI.h>

#include <main/ui/keys.h>
#include <main/main/AppSettings.h>  // PLAYLIST_STRING_SIZE
#include <core/events/SystemEvents.h>


#include <fs/fat/sdapi.h>

#include <main/ui/common/UserInterface.h>
#include <main/ui/PlayerInfoMenuScreen.h>

#include <main/ui/UI.h>
#include <main/ui/PlaylistConstraint.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/Strings.hpp>

#include <util/tchar/tchar.h>


#include <core/playmanager/PlayManager.h>
#include <playlist/common/Playlist.h>

#include <_modules.h>

#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
#include <main/content/datasourcecontentmanager/DataSourceContentManager.h>
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
#include <main/content/djcontentmanager/DJContentManager.h>
#endif

#ifndef NO_UPNP
#include <main/iml/manager/IMLManager.h>
#endif

#include <datastream/input/InputStream.h>
#include <datasource/fatdatasource/FatDataSource.h>

#include <main/main/FatHelper.h>
#include <main/main/Recording.h>
#include <main/main/LEDStateManager.h>

#include <main/content/simplercontentmanager/SimplerContentManager.h>

#include <devs/lcd/lcd.h>
#include <util/timer/Timer.h>
#include <main/serialcontrol/serialcontrol.h>
#include <main/externcontrol/ExternInterface.h>
#include <main/externcontrol/ExternControl.h>

#include <util/upnp/genlib/xstring.h>

#include <main/main/SpaceMgr.h>
#include <main/cddb/CDDBHelper.h>

#include <_version.h>

// defined in library menu screen
extern int QCompareKeyValueRecord( const void* a, const void* b);


extern FILE * fpSerialControl;

DEBUG_MODULE_S(EXTERNINTERFACE, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(EXTERNINTERFACE);  

t_ControlReturn Power(const char * szRequest, char ** szResponse );
t_ControlReturn Backlight(const char * szRequest, char ** szResponse );
t_ControlReturn Control(const char * szRequest, char ** szResponse );
t_ControlReturn Screenshot(const char * szRequest, char ** szResponse );
t_ControlReturn SetStreaming(const char * szRequest, char ** szResponse );

t_ControlReturn GetStatus(const char * szRequest, char ** szResponse );
t_ControlReturn GetInfo(const char * szRequest, char ** szResponse );
t_ControlReturn SelectAlbum(const char * szRequest, char ** szResponse );
t_ControlReturn SetTrack(const char * szRequest, char ** szResponse );
t_ControlReturn SelectNextAlbum(const char * szRequest, char ** szResponse );

t_ControlReturn SelectPreviousAlbum(const char * szRequest, char ** szResponse );
t_ControlReturn SetPlayMode(const char * szRequest, char ** szResponse );
t_ControlReturn GetPlaylist(const char * szRequest, char ** szResponse );
t_ControlReturn BrowseGenres(const char * szRequest, char ** szResponse );
t_ControlReturn BrowseArtists(const char * szRequest, char ** szResponse );

t_ControlReturn BrowseAlbums(const char * szRequest, char ** szResponse );
t_ControlReturn BrowseTracks(const char * szRequest, char ** szResponse );
t_ControlReturn SetPlaylist(const char * szRequest, char ** szResponse );
t_ControlReturn AddToPlaylist(const char * szRequest, char ** szResponse );
t_ControlReturn InstantMessage(const char * szRequest, char ** szResponse );

t_ControlReturn PlayNow(const char * szRequest, char ** szResponse );
t_ControlReturn RipNow(const char * szRequest, char ** szResponse );
t_ControlReturn GetContent(const char * szRequest, char ** szResponse );
t_ControlReturn DeleteContent(const char * szRequest, char ** szResponse );
t_ControlReturn UpdateMetadata(const char * szRequest, char ** szResponse );

t_FunctionTable g_ExternFunctionHandlers[NUM_CONTROL_FUNCTIONS] = 
{
	{ "!setPower",			Power,					CONTROL_ALL},
	{ "!setBacklight",		Backlight,				CONTROL_ALL},
	{ "!control",			Control,				CONTROL_ALL},
	{ "!screenshot.bmp",	Screenshot,				CONTROL_SERIAL},
	{ "!setStreaming",		SetStreaming,			CONTROL_SERIAL},

	{ "!getStatus",			GetStatus,				CONTROL_ALL},
	{ "!getInfo",			GetInfo,				CONTROL_ALL},
	{ "!selectAlbum",		SelectAlbum,				CONTROL_ALL},
	{ "!setTrack",			SetTrack,				CONTROL_ALL},
	{ "!selectNextAlbum",	SelectNextAlbum,		CONTROL_ALL},

	{ "!selectPreviousAlbum",SelectPreviousAlbum,	CONTROL_ALL},
	{ "!setPlayMode",		SetPlayMode,			CONTROL_ALL},
	{ "!getPlaylist",		GetPlaylist,			CONTROL_ALL},
	{ "!browseGenres",		BrowseGenres,			CONTROL_ALL},
	{ "!browseArtists",		BrowseArtists,			CONTROL_ALL},

	{ "!browseAlbums",		BrowseAlbums,			CONTROL_ALL},
	{ "!browseTracks",		BrowseTracks,			CONTROL_ALL},
	{ "!setPlaylist",		SetPlaylist,			CONTROL_ALL},
	{ "!addToPlaylist",		AddToPlaylist,			CONTROL_ALL},
	{ "!instantMessage",	InstantMessage,			CONTROL_ALL},

	{ "!playNow",			PlayNow,				CONTROL_ALL},
	{ "!ripNow",			RipNow,					CONTROL_ALL},
	{ "!getContent",		GetContent,				CONTROL_ALL},
	{ "!deleteContent",		DeleteContent,			CONTROL_ALL},
	{ "!updateMetadata",	UpdateMetadata,			CONTROL_ALL}
};

TCHAR* GetMetadataString(IMetadata* pMetadata, int md_type);


// Retained state information for the extern control

static int g_currentAlbum = 0;

//m_pUserInterface->SetPlaylistMode(m_pPlayManager->GetPlaylistMode());


// utility function
int GetParameterValue(const char * path, const char * param, char * value, int valueLength)
{
	DEBUGP(EXTERNINTERFACE, DBGLEV_ERROR,"GetParamValue\n");

	char * paramPosition = strstr(path,param);
	if (!paramPosition) return 0;

	char * startValue = paramPosition + strlen(param) + 1;

	if (startValue > (path + strlen(path)))
	{
		DEBUGP(EXTERNINTERFACE, DBGLEV_ERROR,"no value provided for parameter %s\n",param);
		return 0;
	}

	DEBUGP(EXTERNINTERFACE, DBGLEV_ERROR,"startValue %s\n",startValue);

	// find the next parameter
	char * endValue = strstr(startValue,"&");

	DEBUGP(EXTERNINTERFACE, DBGLEV_ERROR,"endValue %s\n",endValue);

	if (!endValue)
	{
		// must be the last one
		endValue=(char*)(path+strlen(path));
	}

	if ((endValue-startValue) < valueLength)
	{
		strncpy(value,startValue,endValue-startValue);
		value[endValue-startValue]=0;
	}

	DEBUGP(EXTERNINTERFACE, DBGLEV_ERROR,"value %s\n",value);

	return 1;
}

#define TOUPPER(c) ( (c >= 'a' && c <= 'z') ? c-'a'+'A' : c )
#define ISNUMERIC(c) (c>='0'&&c<='9')
#define HEXTODECIMAL(c) ( ISNUMERIC(c) ? c-'0' : TOUPPER(c)-'A'+10 )

void TextUnescape(const char* in, char* out)
{
    int outlen = 0;
    int inlen = strlen(in);
    for (int i = 0; i < inlen+1; ++i)
    {
        switch (in[i])
        {
            case '%':
            {
                int n = 0;
                // first number
                ++i;
                n <<= 4;
                n += HEXTODECIMAL(in[i]);
                // second number
                ++i;
                n <<= 4;
                n += HEXTODECIMAL(in[i]);

                out[outlen++] = (char) n;
                break;
            }
            default:
                out[outlen++] = in[i];
                break;
        }
    }
}


t_ControlReturn Power(const char * szRequest, char ** szResponse ) 
{

	t_ControlReturn ret = CONTROL_ERR_SYNTAX;

	char state[5];
	memset(state,0,5);

	int parse = GetParameterValue(szRequest,"state",state,5);
	if (parse)
	{
		bool valid = true;
		valid &= ((state[0] == '0') || (state[0] == '1'));

		for (int i = 1; i < 5; i++)
			valid &= (state[i] == 0);

		if (valid)
		{
			if (state[0] == '0')
			{
				if (CDJPlayerState::GetInstance()->GetPowerState() != CDJPlayerState::HARD_POWER_OFF)
				{
					CDJPlayerState::GetInstance()->SetPowerState(CDJPlayerState::HARD_POWER_OFF);
				}
				ret = CONTROL_OK;
			}
			else if (state[0] == '1')
			{
					if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF)
				{
					CDJPlayerState::GetInstance()->SetPowerState(CDJPlayerState::POWER_ON);
				}
				ret = CONTROL_OK;
			}
		}
	}
	return ret;
}

t_ControlReturn Backlight(const char * szRequest, char ** szResponse )
{

	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;

	t_ControlReturn ret = CONTROL_ERR_SYNTAX;

	char state[5];
	memset(state,0,5);

	int parse = GetParameterValue(szRequest,"state",state,5);
	if (parse)
	{
		bool valid = true;
		valid &= ((state[0] == '0') || (state[0] == '1'));

		for (int i = 1; i < 5; i++)
			valid &= (state[i] == 0);

		if (valid)
		{
			if (state[0] == '0')
			{
				LCDSetBacklight(0);
				ret = CONTROL_OK;
			}
			else if (state[0] == '1')
			{
				LCDSetBacklight(1);
				ret = CONTROL_OK;
			}
		}
	}

	return ret;
}


typedef struct s_EventTable
{
	char * szActionName;
	unsigned int  KeyEventValue;
} t_EventTable;

#define NUM_CONTROL_ACTIONS 13

t_EventTable g_ExternControlActions[NUM_CONTROL_ACTIONS] = 
{
	{"record",KEY_RECORD},
	{"play",KEY_PLAY},
	{"stop",KEY_STOP},
	{"pause",KEY_PAUSE},
	{"fwd",KEY_FWD},
	{"rew",KEY_REW},
	{"menu",KEY_MENU},
	{"select",KEY_SELECT},
	{"down",KEY_DOWN},
	{"up",KEY_UP},
	{"cancel",KEY_EXIT},
	{"power",KEY_POWER},
	{"eject",KEY_CD_EJECT}
};

t_ControlReturn Control(const char * szRequest, char ** szResponse )
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;

	t_ControlReturn ret = CONTROL_ERR_SYNTAX;

	char action[20];
	int parse = GetParameterValue(szRequest,"action",action,20);
	if (parse)
	{
		int i=0;
		bool found = false;

		while ((!found) && (i<NUM_CONTROL_ACTIONS))
		{
			if (strcmp(action,g_ExternControlActions[i].szActionName)==0)
			{
				if (CEventQueue::GetInstance()->FullCount() < 10)
				{
					CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_PRESS,(void*)g_ExternControlActions[i].KeyEventValue);
					CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_RELEASE,(void*)g_ExternControlActions[i].KeyEventValue);
				}
				found = true;
			}
			i++;
		}

		if (found) ret = CONTROL_OK;
	}

	return ret;
}


t_ControlReturn Screenshot(const char * szRequest, char ** szResponse )
{
	return CONTROL_ERR_SYNTAX;
	/*
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;

	DWORD bmp_len = 0;
    UCHAR* bmp_buf=0;
	PegRect Rect;
	Rect.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
        // do a screen print
    PegThing pThing;

    bmp_buf = pThing.Screen()->DumpWindowsBitmap( &bmp_len, Rect);

	char* response = (char*)malloc(bmp_len*2);

	char temp[10];

	for (DWORD i = 0; i < bmp_len; i++)
	{
		sprintf(temp,"%x",(UCHAR)bmp_buf[i]);
		response[i*2]=temp[0];
		response[i*2+1]=temp[1];
	}

	*szResponse = response;

	free(bmp_buf);

	return CONTROL_OK;
	*/

}

t_ControlReturn SetStreaming(const char * szRequest, char ** szResponse )
{ 
	return CONTROL_ERR_SYNTAX;
	/*
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;

	t_ControlReturn ret = CONTROL_ERR_SYNTAX;

	char state[5];
	int parse = GetParameterValue(szRequest,"state",state,5);
	if (parse)
	{
		if (state[0] == '0')
		{
			suspend_timer(SerialStatusTimer);
			ret = CONTROL_OK;
		}
		else if (state[0] == '1')
		{
			resume_timer(SerialStatusTimer);
			ret = CONTROL_OK;
		}
	}
	return ret; */


};


t_ControlReturn GetStatus(const char * szRequest, char ** szResponse )
{ 
	xstring retString;
	
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;
	t_ControlReturn ret = CONTROL_OK;
/*
	This is the format of szResponse:

	!#PT[playtime]|PS[playState]|ST[systemText]|PM[playmode]|
	CT[indexOfCurrentTrack]|TT[totalTracks]|CA[indexOfCurrentAlbum]|
	TA[totalAlbums]|TI[title]|AL[album]|AR[artist]|GR[genre]CRLF
*/
	// start building the string
	retString = "PT[";

	// get the playtime string

	char szplayTime[40];
	memset(szplayTime,0,sizeof(szplayTime));

	CMediaPlayer::PlayState playState = CMediaPlayer::GetInstance()->GetPlayState();

	int hours, minutes, seconds;

	unsigned long playTime;

	if ((playState == CMediaPlayer::PLAYING) ||
	    (playState == CMediaPlayer::PAUSED)) 
	{
		playTime = CMediaPlayer::GetInstance()->GetTrackTime();
	}
	else
	{
		playTime = 0;
	}

	hours   = (playTime > 60 * 60) ? playTime / (60*60) : 0;
	minutes = (playTime > 60)      ? playTime / (60)    : 0;
	seconds = (playTime > 0)       ? playTime % 60      : 0;

	if (hours > 0)
	{
		sprintf(szplayTime,"%d:%02d:%02d",hours,minutes,seconds);
	}
	else
	{
		sprintf(szplayTime,"%02d:%02d",minutes,seconds);
	}
	retString += szplayTime;

	retString += "]|PS[";

	
	switch(playState)
	{
	case CMediaPlayer::PLAYING:
		retString += "playing";
		break;
	case CMediaPlayer::PAUSED:
		retString += "paused";
		break;
	case CMediaPlayer::STOPPED:
		retString += "stopped";
		break;
	default:
		retString += "stopped";
		break;
	}

		retString += "]|PM[";
	// would add system text here - deferring for now since I can't see how to get the text
	//  next comes play mode

	IPlaylist::PlaylistMode mode = CPlayManager::GetInstance()->GetPlaylistMode();
	switch(mode)
	{
	case IPlaylist::NORMAL:
		retString += "normal";
		break;
	case IPlaylist::RANDOM:
		retString += "random";
		break;
	case IPlaylist::REPEAT_ALL:
		retString += "repeat";
		break;
	case IPlaylist::REPEAT_RANDOM:
		retString += "repeatrandom";
		break;
	case IPlaylist::REPEAT_TRACK:
		retString += "repeat";
		break;
	default:
		retString += "normal";
		break;
	}

	// now do the current track and total tracks part
	retString += "]|CT[";

	int playlistIndex = 0;

	int playlistSize = 0;
		
	IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
		playlistSize = pCurrentPlaylist->GetSize();

        IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
		{
			playlistIndex = pCurrentPlaylist->GetEntryIndex(pCurrentEntry) + 1;
		}
	}
	char scratch[100];
	memset(scratch,0,sizeof(scratch));

	sprintf(scratch,"%d]|TT[%d]|",playlistIndex,playlistSize);
	retString += scratch;

	// now do the album stats

	#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
    CDataSourceContentManager* pCM = (CDataSourceContentManager*) CPlayManager::GetInstance()->GetContentManager();
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
    CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
#endif

    int nDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
    int albumCount = 0;
	if (pCM) albumCount = pCM->GetAlbumCount(nDSID);

	//g_currentAlbum holds the current album index

	sprintf(scratch,"CA[%d]|TA[%d]",g_currentAlbum,albumCount);
	retString += scratch;

	// now do the metadata
	//TI[title]|AL[album]|AR[artist]|GR[genre]CRLF

	//IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
        {
            IMetadata* pMetadata = pCurrentEntry->GetContentRecord();
            if (pMetadata)
            {
				retString += "|TI[";

				TCHAR* pszTitleText = GetMetadataString(pMetadata, MDA_TITLE);
				retString += TcharToCharN(scratch,pszTitleText,99);

				TCHAR* pszAlbumText = GetMetadataString(pMetadata, MDA_ALBUM);
				retString += "]|AL[";

				retString += TcharToCharN(scratch,pszAlbumText,99);

				TCHAR* pszArtistText = GetMetadataString(pMetadata, MDA_ARTIST);
				retString += "]|AR[";

				retString += TcharToCharN(scratch,pszArtistText,99);

				TCHAR* pszGenreText = GetMetadataString(pMetadata, MDA_GENRE);
				retString += "]|GR[";

				retString += TcharToCharN(scratch,pszGenreText,99);

				retString += "]";

			}
			else
			{
				retString += "|TI[]|AL[]|AR[]|GR[]";
			}
		}
	}

	*szResponse = retString.detach();

	return ret;
}



t_ControlReturn GetInfo(const char * szRequest, char ** szResponse )
{ 
	xstring retString;
	char szTemp[100];

	
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;
	t_ControlReturn ret = CONTROL_OK;

/*
	!#FV[firmwareVersion]|SR[spaceRemaining]|TI[timeRemaining]|
TT[totalTracks|TA[totalAlbums]|TR[totalArtists]|
TG[totalGenres]|TP[totalPlaylists]CRLF
*/

	retString = "FV[";
	retString += DDO_VERSION_STR;
	retString += "]SR[";

    CDJContentManager* pDJCM = (CDJContentManager*)CPlayManager::GetInstance()->GetContentManager();
    int iFatDataSourceID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

    // Free space info
	long long BytesFree = CSpaceMgr::GetInstance()->BytesFromLow();

    // Free time info
    int iBitrate = CDJPlayerState::GetInstance()->GetEncodeBitrate();
    // quick hack for uncompressed bitrate, part 1
    if (iBitrate == 0)
        iBitrate = (44100 * 16 * 2) / DJ_BYTES_PER_KILOBYTE;
    
    long long lBytesPerSecond = (iBitrate * DJ_BYTES_PER_KILOBYTE) / 8;
    long long lSecondsFree    = BytesFree / lBytesPerSecond;
    long long lMinutesFree    = lSecondsFree / 60;
    long long lHoursFree      = lMinutesFree / 60;
    lMinutesFree              = lMinutesFree % 60;

    // quick hack for uncompressed bitrate, part 2
    if (CDJPlayerState::GetInstance()->GetEncodeBitrate() == 0)
        iBitrate = 1400;
    sprintf(szTemp, "%dh %dm (%d kbps)", (unsigned int)lHoursFree, (unsigned int)lMinutesFree, iBitrate);
 
	retString += szTemp;
	retString += "]|TT[";

    // Tracks
    sprintf(szTemp, "%d", pDJCM->GetMediaRecordCount(iFatDataSourceID));

	retString += szTemp;
	retString += "]|TA[";


    // Albums
    sprintf(szTemp, "%d", pDJCM->GetAlbumCount(iFatDataSourceID));

	retString += szTemp;
	retString += "]|TR[";

    // Artists
    sprintf(szTemp, "%d", pDJCM->GetArtistCount(iFatDataSourceID));

	retString += szTemp;
	retString += "]|TG[";

    // Genres
    sprintf(szTemp, "%d", pDJCM->GetGenreCount(iFatDataSourceID));
	retString += szTemp;
	retString += "]|TP[";


    // Playlists
    sprintf(szTemp, "%d", pDJCM->GetPlaylistRecordCount(iFatDataSourceID));

	retString += szTemp;
	retString += "]";

	*szResponse = retString.detach();
	return ret;
}


void HelperSelectAlbum(int albumIndex,t_ControlReturn *ret)
{
	// first clear the playlist
#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
	CDataSourceContentManager* pCM = (CDataSourceContentManager*) CPlayManager::GetInstance()->GetContentManager();
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
	CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
#endif
	IPlaylist* pPL = CPlayManager::GetInstance()->GetPlaylist();
	CIMLManager* pIMLManager = CIMLManager::GetInstance();
	CPlaylistConstraint* pPC = CPlaylistConstraint::GetInstance();

	CPlayManager* pPM = CPlayManager::GetInstance();
	CPlayerScreen* pPS = CPlayerScreen::GetPlayerScreen();

	CMediaPlayer::PlayState oldPlayState = CMediaPlayer::GetInstance()->GetPlayState();


    // clear out the current playlist
	pPL->Clear();
	pPM->Deconfigure();
	pPS->ClearTrack();
	pPS->SetTrackText(LS(SID_LOADING));
	pPS->HideMenus();
	pIMLManager->ClearMediaRecords();
	ContentKeyValueVector vAlbums;
	int nDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
	pCM->GetAlbums(vAlbums,CMK_ALL,CMK_ALL,nDSID);

	if (vAlbums.Size() > 1)
		vAlbums.QSort(QCompareKeyValueRecord);

	if (albumIndex > vAlbums.Size()-1) albumIndex = 1;

	if (albumIndex <= 0) albumIndex = vAlbums.Size() - 1;

	int nAlbumConstraint = pCM->GetAlbumKey(vAlbums[albumIndex-1].szValue);

	*ret = CONTROL_OK;

	pPC->Constrain(CMK_ALL,nAlbumConstraint,CMK_ALL);
	pPC->UpdatePlaylist();
	CRecordingManager::GetInstance()->ProcessEncodingUpdates();

	if (oldPlayState == CMediaPlayer::PLAYING)
	{
		if (FAILED(pPM->Play()))
			if (FAILED(pPM->NextTrack()))
			{
				pPS->DisplayNoContentScreen();
				pPS->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
			}
	}
	else
	{
		if (FAILED(pPM->Stop()))
	        if (FAILED(pPM->NextTrack()))
			{
				pPS->DisplayNoContentScreen();
				pPS->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
			}
	}

	g_currentAlbum = albumIndex;
}


t_ControlReturn SelectAlbum(const char * szRequest, char ** szResponse )
{ 
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;

	t_ControlReturn ret = CONTROL_ERR_SYNTAX;

	char action[20];
	memset(action,0,sizeof(action));

	int parse = GetParameterValue(szRequest,"albumIndex",action,20);
	if (parse)
	{
		bool numeric = true;
		for (int i = 0; i < strlen(action); numeric &= ISNUMERIC(action[i++]));

		if (numeric)
		{
			int albumIndex = atoi(action);
			HelperSelectAlbum(albumIndex,&ret);
		}
	}
	return ret;
}

t_ControlReturn SetTrack(const char * szRequest, char ** szResponse )
{ 
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;
	t_ControlReturn ret = CONTROL_ERR_SYNTAX;
	char szTrackIndex[20];
	memset(szTrackIndex,0,sizeof(szTrackIndex));
	int parse = GetParameterValue(szRequest,"trackIndex",szTrackIndex,20);
	if (parse)
	{
		bool numeric = true;
		for (int i = 0; i < strlen(szTrackIndex); numeric &= ISNUMERIC(szTrackIndex[i++]));
		if (numeric)
		{
			IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
			int playlistSize = pCurrentPlaylist->GetSize();

			int trackIndex = atoi(szTrackIndex);

			if ((trackIndex > 0) && (trackIndex <= playlistSize))
			{

				int iSelectIndex = trackIndex-1;
				CMediaPlayer::PlayState oldPlayState = CMediaPlayer::GetInstance()->GetPlayState();
				


				IPlaylistEntry* pTargetEntry = pCurrentPlaylist->GetEntry(iSelectIndex);
				if (pTargetEntry)
				{
					if (pCurrentPlaylist->SetCurrentEntry(pTargetEntry))
					{
						ret = CONTROL_OK;
						int res;
						if (SUCCEEDED(res = CPlayManager::GetInstance()->SetSong(pTargetEntry)))
						{
							if (oldPlayState == CMediaPlayer::PLAYING)
							{
								CPlayManager::GetInstance()->Play();
								CEventQueue::GetInstance()->PutEvent(EVENT_KEY_PRESS,(void*)KEY_PLAY);
							}
							else
							{
								CPlayManager::GetInstance()->Stop();
								CEventQueue::GetInstance()->PutEvent(EVENT_KEY_PRESS,(void*)KEY_STOP);
							}
						}
					}
				}
				else
				{
					ret = CONTROL_ERR_PARAM;
				}
			}
			else
			{
				ret = CONTROL_ERR_PARAM;
			}
		}
	}
	return ret;
}

t_ControlReturn SelectNextAlbum(const char * szRequest, char ** szResponse )
{ 
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;
	t_ControlReturn ret = CONTROL_ERR_SYNTAX;
	HelperSelectAlbum(g_currentAlbum+1,&ret);
	return ret;
}

t_ControlReturn SelectPreviousAlbum(const char * szRequest, char ** szResponse )
{ 
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;
	t_ControlReturn ret = CONTROL_ERR_SYNTAX;
	HelperSelectAlbum(g_currentAlbum-1,&ret);
	return ret;
}

t_ControlReturn SetPlayMode(const char * szRequest, char ** szResponse )
{ 
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;

	t_ControlReturn ret = CONTROL_ERR_SYNTAX;

	char szplayMode[15];
	memset(szplayMode,0,sizeof(szplayMode));

	int parse = GetParameterValue(szRequest,"mode",szplayMode,15);
	if (parse)
	{
		if (strcmp(szplayMode,"normal")==0)
		{
			CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::NORMAL);
			ret = CONTROL_OK;
		}
		else if (strcmp(szplayMode,"random")==0)
		{
			CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::RANDOM);
			ret = CONTROL_OK;
		}
		else if (strcmp(szplayMode,"repeat")==0)
		{
			CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::REPEAT_ALL);
			ret = CONTROL_OK;
		}
		else if (strcmp(szplayMode,"repeatrandom")==0)
		{
			CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::REPEAT_RANDOM);
			ret = CONTROL_OK;
		}
		else
		{
			ret = CONTROL_ERR_PARAM;
		}
	}

	CPlayerScreen* pPS = CPlayerScreen::GetPlayerScreen();
	pPS->SetPlayModeTextByPlaylistMode(CPlayManager::GetInstance()->GetPlaylistMode());

	return ret;
}


t_ControlReturn GetPlaylist(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };
t_ControlReturn BrowseGenres(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };
t_ControlReturn BrowseArtists(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };
t_ControlReturn BrowseAlbums(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };
t_ControlReturn BrowseTracks(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };
t_ControlReturn SetPlaylist(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };
t_ControlReturn AddToPlaylist(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };


t_ControlReturn InstantMessage(const char * szRequest, char ** szResponse )
{ 
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return CONTROL_ERR_STATE;

	t_ControlReturn ret = CONTROL_ERR_SYNTAX;

	char szMessage[100];
	char szUEMessage[100];
	memset(szMessage,0,sizeof(szMessage));
	memset(szUEMessage,0,sizeof(szUEMessage));

	int parse = GetParameterValue(szRequest,"message",szMessage,100);
	if (parse)
	{
		TextUnescape(szMessage,szUEMessage);

			CPlayerScreen* pPS = CPlayerScreen::GetPlayerScreen();
			TCHAR szTMessage[256];
	        CharToTcharN(szTMessage,szUEMessage,255 );
			if (pPS)
			{
				pPS->SetMessageText(szTMessage);
				ret = CONTROL_OK;
			}

	}
	return ret; 
};


t_ControlReturn PlayNow(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };
t_ControlReturn RipNow(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };
t_ControlReturn GetContent(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };
t_ControlReturn DeleteContent(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };
t_ControlReturn UpdateMetadata(const char * szRequest, char ** szResponse ){ return CONTROL_ERR_SYNTAX; };


TCHAR* GetMetadataString(IMetadata* pMetadata, int md_type)
{
    void *pData;
    if (SUCCEEDED(pMetadata->GetAttribute(md_type, &pData)))
        return (TCHAR*)pData;
    else
        return 0;
}

/*

t_SFHReturn GetAlbumName(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	t_SFHReturn ret = SH_RET_ERR_STATE;

	IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
        {
            IMetadata* pMetadata = pCurrentEntry->GetContentRecord();
            if (pMetadata)
            {
				TCHAR* pszAlbumText = GetMetadataString(pMetadata, MDA_ALBUM);

				char szAlbumText[100];
				fprintf(fp,"OK [%s]\n\r",TcharToChar(szAlbumText,pszAlbumText));

				ret = SH_RET_OK_RESPONDED;
			}
		}
	}
	return ret;
};

t_SFHReturn GetArtistName(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	t_SFHReturn ret = SH_RET_ERR_STATE;

	IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
        {
            IMetadata* pMetadata = pCurrentEntry->GetContentRecord();
            if (pMetadata)
            {
				TCHAR* pszArtistText = GetMetadataString(pMetadata, MDA_ARTIST);
				char szArtistText[100];
				fprintf(fp,"OK [%s]\n\r",TcharToChar(szArtistText,pszArtistText));

				ret = SH_RET_OK_RESPONDED;
			}
		}
	}
	return ret;

};

t_SFHReturn GetCurrentAlbum(FILE * fp, int param)
{
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;
	if (g_currentAlbum == 0) return SH_RET_ERR_STATE;

	fprintf(fp,"OK [%d]\n\r",g_currentAlbum);

	return SH_RET_OK_RESPONDED;
};

t_SFHReturn GetCurrentTrack(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();

	int currentIndex = pCurrentPlaylist->GetEntryIndex(pCurrentPlaylist->GetCurrentEntry());

	if (currentIndex != -1)
	{
		fprintf(fp,"OK [%d]\n\r",currentIndex+1);
		return SH_RET_OK_RESPONDED;
	}
	else
	{
		return SH_RET_ERR_STATE;
	}
};


t_SFHReturn GetGenre(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	t_SFHReturn ret = SH_RET_ERR_STATE;

	IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
        {
            IMetadata* pMetadata = pCurrentEntry->GetContentRecord();
            if (pMetadata)
            {
				TCHAR* pszGenreText = GetMetadataString(pMetadata, MDA_GENRE);
				char szGenreText[100];
				fprintf(fp,"OK [%s]\n\r",TcharToChar(szGenreText,pszGenreText));

				ret = SH_RET_OK_RESPONDED;
			}
		}
	}
	return ret;
};


t_SFHReturn GetPlayingTime(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	unsigned long playTime = CMediaPlayer::GetInstance()->GetTrackTime();

	fprintf(fp,"OK [%d]\n\r",playTime);

	return SH_RET_OK_RESPONDED;
};


t_SFHReturn GetNumberAlbums(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;
	
#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
    CDataSourceContentManager* pCM = (CDataSourceContentManager*) CPlayManager::GetInstance()->GetContentManager();
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
    CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
#endif

    int nDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
    int albumCount = pCM->GetAlbumCount(nDSID);

	fprintf(fp,"OK [%d]\n\r",albumCount);

	return SH_RET_OK_RESPONDED;
};

t_SFHReturn GetNumberTracks(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
	int iSizePlaylist = pCurrentPlaylist->GetSize();

	fprintf(fp,"OK [%d]\n\r",iSizePlaylist);
	return SH_RET_OK_RESPONDED;
};


t_SFHReturn GetPlayState(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	int playValue = -1;
	CMediaPlayer::PlayState playState = CMediaPlayer::GetInstance()->GetPlayState();

	switch(playState)
	{
	case CMediaPlayer::PLAYING:
		playValue = 1;
		break;
	case CMediaPlayer::PAUSED:
		playValue = 2;
		break;
	case CMediaPlayer::STOPPED:
		playValue = 0;
		break;
	default:
		playValue = -1;
		break;
	}
	if (playValue >= 0)
	{
		fprintf(fp,"OK [%d]\n\r",playValue);
		return SH_RET_OK_RESPONDED;
	}
	else
	{
		return SH_RET_ERR_STATE;
	}
};


t_SFHReturn GetTrackTitle(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	t_SFHReturn ret = SH_RET_ERR_STATE;

	IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
        {
            IMetadata* pMetadata = pCurrentEntry->GetContentRecord();
            if (pMetadata)
            {
				TCHAR* pszTitleText = GetMetadataString(pMetadata, MDA_TITLE);
				char szTitleText[100];
				fprintf(fp,"OK [%s]\n\r",TcharToChar(szTitleText,pszTitleText));
				ret = SH_RET_OK_RESPONDED;
			}
		}
	}
	return ret;

};

t_SFHReturn SelectNextAlbum(FILE * fp, int param)
{
	return SetCurrentAlbum(fp,g_currentAlbum+1);
};

t_SFHReturn SkipTrackForward(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_PRESS,(void*)KEY_FWD);
	CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_RELEASE,(void*)KEY_FWD);
	return SH_RET_OK;
};

t_SFHReturn Play(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;
	
	CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_PRESS,(void*)KEY_PLAY);
	CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_RELEASE,(void*)KEY_PLAY);

	return SH_RET_OK;
}

t_SFHReturn SelectPreviousAlbum(FILE * fp, int param)
{
	return SetCurrentAlbum(fp,g_currentAlbum-1);
};


t_SFHReturn SkipTrackPrevious(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_PRESS,(void*)KEY_REW);
	CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_RELEASE,(void*)KEY_REW);
	return SH_RET_OK;
}

t_SFHReturn Pause(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;
	CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_PRESS,(void*)KEY_PAUSE);
	CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_RELEASE,(void*)KEY_PAUSE);
	return SH_RET_OK;
}

t_SFHReturn SetBackLight(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	t_SFHReturn ret = SH_RET_ERR_SYNTAX;
	if ((param == 0) || (param==1))
	{
		LCDSetBacklight(param);
		ret = SH_RET_OK;
	}
	return ret;
}


// defined in library menu screen
extern int QCompareKeyValueRecord( const void* a, const void* b);

t_SFHReturn SetCurrentAlbum(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param == NO_PARAM) return SH_RET_ERR_SYNTAX;

	// first clear the playlist

#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
    CDataSourceContentManager* pCM = (CDataSourceContentManager*) CPlayManager::GetInstance()->GetContentManager();
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
    CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
#endif
    IPlaylist* pPL = CPlayManager::GetInstance()->GetPlaylist();
    CIMLManager* pIMLManager = CIMLManager::GetInstance();
    CPlaylistConstraint* pPC = CPlaylistConstraint::GetInstance();

    CPlayManager* pPM = CPlayManager::GetInstance();
    CPlayerScreen* pPS = CPlayerScreen::GetPlayerScreen();

    // clear out the current playlist
	pPL->Clear();
    pPM->Deconfigure();
    pPS->ClearTrack();
    pPS->SetTrackText(LS(SID_LOADING));
    pPS->HideMenus();
    pIMLManager->ClearMediaRecords();

	ContentKeyValueVector vAlbums;
    int nDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
    pCM->GetAlbums(vAlbums,CMK_ALL,CMK_ALL,nDSID);

    if (vAlbums.Size() > 1)
        vAlbums.QSort(QCompareKeyValueRecord);

	if (param > vAlbums.Size()-1) param = 1;

	if (param <= 0) param = vAlbums.Size() - 1;

	int nAlbumConstraint = pCM->GetAlbumKey(vAlbums[param-1].szValue);

	pPC->Constrain(CMK_ALL,nAlbumConstraint,CMK_ALL);
    pPC->UpdatePlaylist();
    CRecordingManager::GetInstance()->ProcessEncodingUpdates();

	if (FAILED(pPM->Stop()))
        if (FAILED(pPM->NextTrack()))
        {
            pPS->DisplayNoContentScreen();
            pPS->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
        }

	g_currentAlbum = param;

	return SH_RET_OK;
}


t_SFHReturn SetCurrentTrack(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;

	int res;

	IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();

	int iSelectIndex = param-1;
	
	IPlaylistEntry* pTargetEntry = pCurrentPlaylist->GetEntry(iSelectIndex);
	if (pTargetEntry)
	{
		if (pCurrentPlaylist->SetCurrentEntry(pTargetEntry))
		{
			if (SUCCEEDED(res = CPlayManager::GetInstance()->SetSong(pTargetEntry)))
			{
                 // get the play manager playing the track.
                CPlayManager::GetInstance()->Stop();
                 // then post a key event to sync up the play state icon.  this has to be an event to prevent UI corruption,
                 // and OTOH it won't suffice to just do this w/o the playmanager command, as the keypress won't necessarily
                 // GET to the playerscreen, if e.g. the menu is up.
                CEventQueue::GetInstance()->PutEvent(EVENT_KEY_PRESS,(void*)KEY_STOP);
				return SH_RET_OK;
			}
		}
	}

	return SH_RET_ERR_PARAM;
}


t_SFHReturn SetPowerState(FILE * fp, int param)
{
	t_SFHReturn ret = SH_RET_ERR_SYNTAX;
	if (param == 0)
	{
		if (CDJPlayerState::GetInstance()->GetPowerState() != CDJPlayerState::HARD_POWER_OFF)
		{
			CDJPlayerState::GetInstance()->SetPowerState(CDJPlayerState::HARD_POWER_OFF);
		}
		ret = SH_RET_OK;
	}
	else if (param == 1)
	{
		if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF)
		{
			CDJPlayerState::GetInstance()->SetPowerState(CDJPlayerState::POWER_ON);
		}
		ret = SH_RET_OK;
	}
	return ret;
}

t_SFHReturn Stop(FILE * fp, int param)
{
	if (CDJPlayerState::GetInstance()->GetPowerState() == CDJPlayerState::HARD_POWER_OFF) return SH_RET_ERR_STATE;
	if (param != NO_PARAM) return SH_RET_ERR_SYNTAX;

	CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_PRESS,(void*)KEY_STOP);
	CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_KEY_RELEASE,(void*)KEY_STOP);
	return SH_RET_OK;
}

*/