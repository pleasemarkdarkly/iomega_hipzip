//
// WebControlServer.cpp - the lean, mean web serving machine
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

#include <main/ui/UI.h>
#include <main/ui/PlaylistConstraint.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/QuickBrowseMenuScreen.h>
#include <main/ui/Strings.hpp>

#include <util/tchar/tchar.h>

#include <main/webcontrol/FunctionInterface.h>

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

extern "C"
{
#include <network.h>
}


// defined in library menu screen
extern int QCompareKeyValueRecord( const void* a, const void* b);

DEBUG_MODULE_S(WEBCONTROL, DBGLEV_DEFAULT | DBGLEV_INFO);

DEBUG_USE_MODULE(WEBCONTROL);  // debugging prefix : (21) wc

int ControlPlayer(int socket, const char * path);
int BrowseGenres(int socket, const char * path);
int BrowseArtists(int socket, const char * path);
int BrowseAlbums(int socket, const char * path);
int BrowseTracks(int socket, const char * path);
int BrowsePlaylists(int socket, const char * path);
int ScreenShot(int socket, const char * path);
int RecordButton(int socket, const char * path);
int ShowPlaylist(int socket, const char * path);
int SetPlaylist(int socket, const char * path);
int AddToPlaylist(int socket, const char * path);

t_FunctionTable g_FunctionHandlers[NUM_WEB_FUNCTIONS] = 
{
	{ "?control",ControlPlayer },
	{ "?screenshot.bmp",ScreenShot },
	{ "?record_btn.gif",RecordButton },
	{ "?browseGenres", BrowseGenres },
	{ "?browseArtists",BrowseArtists},
	{ "?browseAlbums",BrowseAlbums},
	{ "?browseTracks",BrowseTracks},
	{ "?browsePlaylists",BrowsePlaylists},
	{ "?showplaylist",ShowPlaylist},
	{ "?setPlaylist",SetPlaylist},
	{ "?addToPlaylist",AddToPlaylist}
};

// utility function
int GetParamValue(const char * path, const char * param, char * value, int valueLength)
{
	DEBUGP(WEBCONTROL, DBGLEV_TRACE,"GetParamValue\n");

	char * paramPosition = strstr(path,param);
	if (!paramPosition) return 0;
    if (*(paramPosition - 1) != '&') return 0;
    
	char * startValue = paramPosition + strlen(param) + 1;
    

	DEBUGP(WEBCONTROL, DBGLEV_TRACE,"startValue %s\n",startValue);

	// find the next parameter
	char * endValue = strstr(startValue,"&");

	DEBUGP(WEBCONTROL, DBGLEV_TRACE,"endValue %s\n",endValue);

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
    
	DEBUGP(WEBCONTROL, DBGLEV_TRACE,"value %s\n",endValue);

	return 1;
}

#define TOUPPER(c) ( (c >= 'a' && c <= 'z') ? c-'a'+'A' : c )
#define ISNUMERIC(c) (c>='0'&&c<='9')
#define HEXTODECIMAL(c) ( ISNUMERIC(c) ? c-'0' : TOUPPER(c)-'A'+10 )

void unescape(const char* in, char* out)
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

typedef struct s_EventTable
{
	char * szActionName;
	unsigned int  KeyEventValue;
} t_EventTable;

#define NUM_CONTROL_ACTIONS 13

t_EventTable g_ControlActions[NUM_CONTROL_ACTIONS] = 
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

char * szNullFrameResponse = "<html><META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\"><head><title>null</title><base target=\"_self\"></head><body bgcolor=\"#ffffff\"></body></html>";

char * defaultHeader = "HTTP/1.1 200 OK\r\nConnection: close\r\nSERVER: FullplayMediaOS\r\nContent-Type: text/html\r\n\r\n";

char * szFrameRight = 
"<html>\n"
"<head>\n"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">\n"
"<META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n"
"<title>Frame D</title>\n"
"<base target=\"_self\">\n"
"</head>\n"
"<body bgcolor=\"#b8c8db\">\n"
"</body>\n"
"</html>\n";

int ControlPlayer(int socket, const char * path)
{
	char action[20];
	int ret = GetParamValue(path,"action",action,20);
	if (ret)
	{
		int i=0;
		bool found = false;
        CEventQueue* pEventQueue = CEventQueue::GetInstance();

		while ((!found) && (i<NUM_CONTROL_ACTIONS))
		{
			if (strcmp(action,g_ControlActions[i].szActionName)==0)
			{
                // only post new key events if there's less than 10 events already in the event queue
                // this is to avoid the unpleasant situation of a full event queue
                if (pEventQueue->FullCount() < 10)
                {
				    pEventQueue->PutEvent((unsigned int)EVENT_KEY_PRESS,(void*)g_ControlActions[i].KeyEventValue);
				    pEventQueue->PutEvent((unsigned int)EVENT_KEY_RELEASE,(void*)g_ControlActions[i].KeyEventValue);
                }
				found = true;
			}
			i++;
		}

		char responseHeader[400];

		sprintf(responseHeader,
			"HTTP/1.1 200 OK\r\n"
			"Connection: close\r\n"
			"SERVER: FullplayMediaOS\r\n"
			//"Content-Length: %d\r\n"
			"Content-Type: text/html\r\n"
			"\r\n");

		send(socket,responseHeader,strlen(responseHeader),0);
		send(socket,szFrameRight,strlen(szFrameRight),0);
		shutdown(socket,SD_BOTH);
		close(socket);
		ret = 1;
	}

	return ret;

}

char * szStartBrowseListingHeader = 
"<html>\n"
"<head>\n"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">\n"
"<META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n"
"<link rel=\"stylesheet\" type=\"text/css\" href=\"djstyle.css\">\n"
"<title>Browse Listing</title>\n"
"</head>\n"
"<body>\n"
"<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"366\" height=\"100%\">\n"
"  <tr>\n"
"    <td width=\"033\" background=\"images/bg_frameB.gif\" align=\"center\"><img border=\"0\" src=\"images/clear.gif\" width=\"1\" height=\"1\"></td>\n"
"    <td valign=\"top\" bgcolor=\"#ffffff\" width=\"333\" align=\"center\">\n"
"      <div align=\"top\">\n"
"        <center>\n"
"        <table border=\"0\" cellpadding=\"4\" cellspacing=\"1\" width=\"333\">\n"
"          <tr>\n"
"            <td class=\"firstline\" colspan=\"4\" width=\"333\">\n";



/*

"<html>\n"
"<head>\n"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">\n"
"<link rel=\"stylesheet\" type=\"text/css\" href=\"djstyle.css\">\n"
"<title>Browse Listing</title>\n"
"</head>\n"
"<body>\n"
"<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"444\" height=\"100%\">\n"
"  <tr>\n"
"    <td width=\"033\" background=\"images/bg_frameB.gif\" align=\"center\"><img border=\"0\" src=\"../images/clear.gif\" width=\"1\" height=\"1\"></td>\n"
"    <td valign=\"top\" bgcolor=\"#ffffff\" width=\"381\" align=\"center\">\n"
"      <div align=\"top\">\n"
"        <center>\n"
"        <table border=\"0\" cellpadding=\"4\" cellspacing=\"1\" width=\"381\">\n"
"          <tr>\n"
"            <td class=\"firstline\" colspan=\"4\" width=\"379\">\n";
*/


// * the heading goes here, which will be the metadata class or instance being browsed currently.

char * szEndBrowseListingHeader = 
"            </td>\n"
"          </tr>";
  
// start row
char * szStartBrowseListingItem =   
"          <tr>\n"
"            <td class=\"listing\" width=\"243\">\n";

// * the item text goes here

char * szMiddle1BrowseListingItem = 
"</td>\n"
"            <td class=\"actionlinks\" width=\"38\"><a href=\"?";

// next, browse function + query constraints, e.g.,
// browseAlbums&amp;genre=rock&amp;artist=madonna

char * szMiddle2BrowseListingItem = 
"\" target=\"content\">browse</a></td>\n"
"    <td class=\"actionlinks\" width=\"28\"><a href=\"?setPlaylist";

// next, query constraints, e.g.,
// genre=rock&amp;artist=madonna

char * szMiddle3BrowseListingItem = 
"\" target=\"playlist\">play</a></td>\n"
"    <td class=\"actionlinks\" width=\"25\"><a href=\"?addToPlaylist";

// next, query constraints, e.g.,
// genre=rock&amp;artist=madonna

char * szEndBrowseListingItem = 
"\" target=\"playlist\">add</a></td>\n"
"  </tr>";

char * szEndBrowseListing =   
"        </table>\n"
"        </center>\n"
"      </div>\n"
"    </td>\n"
"  </tr>\n"
"</table>\n"
"</body>\n"
"</html>\n";

char* szGenres = "genres";
char* szNoGenres = "no genres available";

void SendConstraintAux( int socket, char* name, char* value, int bytes)
{
    send(socket,"&amp;",strlen("&amp;"),0);
    send(socket,name,strlen(name),0);
    send(socket,"=",1,0);
    send(socket,value,bytes,0);
}

void SendConstraint( int socket, char* name, char* value)
{
    return SendConstraintAux(socket,name,value,strlen(value));
}

void SendConstraint( int socket, char* name, const TCHAR* value)
{
    return SendConstraintAux(socket,name,(char*)value,tstrlen(value)*2);
}

int BrowseGenres(int socket, const char * path)
{
    DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:browseGenres %s\n",path); 

#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
    CDataSourceContentManager* pCM = (CDataSourceContentManager*) CPlayManager::GetInstance()->GetContentManager();
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
    CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
#endif

    ContentKeyValueVector vGenres;
    int nDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
    pCM->GetGenres(vGenres,CMK_ALL,CMK_ALL,nDSID);

    if (vGenres.Size() > 1)
        vGenres.QSort(QCompareKeyValueRecord);

    send(socket,defaultHeader,strlen(defaultHeader),0);

    // start table header
	send(socket,szStartBrowseListingHeader,strlen(szStartBrowseListingHeader),0);
    // drop in a title
    if (!vGenres.IsEmpty())
        send(socket,szGenres,strlen(szGenres),0);
    else
        send(socket,szNoGenres,strlen(szNoGenres),0);

    // end header
    send(socket,szEndBrowseListingHeader,strlen(szEndBrowseListingHeader),0);

	for (int i = 0; i < vGenres.Size(); ++i)
	{
		// start the table entry
		send(socket,szStartBrowseListingItem,strlen(szStartBrowseListingItem),0);

		// genre text
    	send(socket,(char*)vGenres[i].szValue,tstrlen(vGenres[i].szValue)*2,0);

        // middle one html
        send(socket,szMiddle1BrowseListingItem,strlen(szMiddle1BrowseListingItem),0);

        // function for the next lower level of browsing
        send(socket,"browseArtists",strlen("browseArtists"),0);

        SendConstraint(socket,"genre",vGenres[i].szValue);
 
        // middle two html
        send(socket,szMiddle2BrowseListingItem,strlen(szMiddle2BrowseListingItem),0);

        SendConstraint(socket,"genre",vGenres[i].szValue);
       
        // middle three html
        send(socket,szMiddle3BrowseListingItem,strlen(szMiddle3BrowseListingItem),0);

        SendConstraint(socket,"genre",vGenres[i].szValue);

        // end table entry
		send(socket,szEndBrowseListingItem,strlen(szEndBrowseListingItem),0);
	}

    // end table
    send(socket,szEndBrowseListing,strlen(szEndBrowseListing),0);
	shutdown(socket,SD_BOTH);
	close(socket);

	return 1;
};

char* szArtists = "artists";
char* szNoArtists = "no artists available";
char* szInGenre = " in genre '";

int BrowseArtists(int socket, const char * path)
{
    DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:browseArtists %s\n",path); 

#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
    CDataSourceContentManager* pCM = (CDataSourceContentManager*) CPlayManager::GetInstance()->GetContentManager();
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
    CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
#endif

    int nGenreConstraint = CMK_ALL;
    TCHAR tszGenreConstraint[PLAYLIST_STRING_SIZE];
    char szGenreConstraint[PLAYLIST_STRING_SIZE];
    char szUnescapedGenre[PLAYLIST_STRING_SIZE];
	int ret = GetParamValue(path,"genre",(char*)szGenreConstraint,PLAYLIST_STRING_SIZE);
	if (ret)
    {
        unescape(szGenreConstraint,szUnescapedGenre);
        CharToTchar(tszGenreConstraint,szUnescapedGenre);
        nGenreConstraint = pCM->GetGenreKey(tszGenreConstraint);
        if (nGenreConstraint == CMK_ALL)
        {
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:failed to find genre %s\n",szUnescapedGenre); 
        }
    }
    ContentKeyValueVector vArtists;
    int nDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
    pCM->GetArtists(vArtists,CMK_ALL,nGenreConstraint,nDSID);

    if (vArtists.Size() > 1)
        vArtists.QSort(QCompareKeyValueRecord);

    send(socket,defaultHeader,strlen(defaultHeader),0);

    // start table header
	send(socket,szStartBrowseListingHeader,strlen(szStartBrowseListingHeader),0);
    // drop in a title
    if (vArtists.Size() > 0)
        send(socket,szArtists,strlen(szArtists),0);
    else
        send(socket,szNoArtists,strlen(szNoArtists),0);

    // perhaps with a genre constraint indicated
    if (nGenreConstraint != CMK_ALL)
    {   
        send(socket, szInGenre, strlen(szInGenre),0);
        send(socket, tszGenreConstraint, tstrlen(tszGenreConstraint)*2,0);
        send(socket, "'", 1, 0);
    }

    // end header
    send(socket,szEndBrowseListingHeader,strlen(szEndBrowseListingHeader),0);

	for (int i = 0; i < vArtists.Size(); ++i)
	{
		// start the table entry
		send(socket,szStartBrowseListingItem,strlen(szStartBrowseListingItem),0);

		// genre text
    	send(socket,(char*)vArtists[i].szValue,tstrlen(vArtists[i].szValue)*2,0);

        // middle one html
        send(socket,szMiddle1BrowseListingItem,strlen(szMiddle1BrowseListingItem),0);

        // function for the next lower level of browsing
        send(socket,"browseAlbums",strlen("browseAlbums"),0);

        if (nGenreConstraint != CMK_ALL)
        {
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:passing along genre constraint\n"); 
            SendConstraint(socket,"genre",tszGenreConstraint);
        }
        SendConstraint(socket,"artist",vArtists[i].szValue);

        // middle two html
        send(socket,szMiddle2BrowseListingItem,strlen(szMiddle2BrowseListingItem),0);

        if (nGenreConstraint != CMK_ALL)
        {
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:passing along genre constraint\n"); 
            SendConstraint(socket,"genre",tszGenreConstraint);
        }
        SendConstraint(socket,"artist",vArtists[i].szValue);
       
        // middle three html
        send(socket,szMiddle3BrowseListingItem,strlen(szMiddle3BrowseListingItem),0);

        if (nGenreConstraint != CMK_ALL)
        {
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:passing along genre constraint\n"); 
            SendConstraint(socket,"genre",tszGenreConstraint);
        }
        SendConstraint(socket,"artist",vArtists[i].szValue);

        // end table entry
		send(socket,szEndBrowseListingItem,strlen(szEndBrowseListingItem),0);
	}

    // end table
    send(socket,szEndBrowseListing,strlen(szEndBrowseListing),0);
	shutdown(socket,SD_BOTH);
	close(socket);

	return 1;
};

char* szAlbums = "albums";
char* szNoAlbums = "no albums available";
char* szBrowseTracks = "browseTracks";
char* szByArtist = " by artist '";

int BrowseAlbums(int socket, const char * path)
{
    DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:browseAlbums %s\n",path); 

#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
    CDataSourceContentManager* pCM = (CDataSourceContentManager*) CPlayManager::GetInstance()->GetContentManager();
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
    CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
#endif
    
    // check for genre constraint
    int nGenreConstraint = CMK_ALL;
    TCHAR tszGenreConstraint[PLAYLIST_STRING_SIZE];
    char szGenreConstraint[PLAYLIST_STRING_SIZE];
    char szUnescapedGenre[PLAYLIST_STRING_SIZE];
	int ret = GetParamValue(path,"genre",(char*)szGenreConstraint,PLAYLIST_STRING_SIZE);
	if (ret)
    {
        unescape(szGenreConstraint,szUnescapedGenre);
        CharToTchar(tszGenreConstraint,szUnescapedGenre);
        nGenreConstraint = pCM->GetGenreKey(tszGenreConstraint);
        if (nGenreConstraint == CMK_ALL)
            DEBUGP( WEBCONTROL, DBGLEV_WARNING, "WC:genre %s not found\n",szUnescapedGenre); 
    }

    // check for artist constraint
    int nArtistConstraint = CMK_ALL;
    TCHAR tszArtistConstraint[PLAYLIST_STRING_SIZE];
    char szArtistConstraint[PLAYLIST_STRING_SIZE];
    char szUnescapedArtist[PLAYLIST_STRING_SIZE];
	ret = GetParamValue(path,"artist",(char*)szArtistConstraint,PLAYLIST_STRING_SIZE);
	if (ret)
    {
        unescape(szArtistConstraint,szUnescapedArtist);
        CharToTchar(tszArtistConstraint,szUnescapedArtist);
        nArtistConstraint = pCM->GetArtistKey(tszArtistConstraint);
        if (nArtistConstraint == CMK_ALL)
            DEBUGP( WEBCONTROL, DBGLEV_WARNING, "WC:artist %s not found\n",szUnescapedArtist); 
    }

    ContentKeyValueVector vAlbums;
    int nDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
    pCM->GetAlbums(vAlbums,nArtistConstraint,nGenreConstraint,nDSID);

    if (vAlbums.Size() > 1)
        vAlbums.QSort(QCompareKeyValueRecord);

    send(socket,defaultHeader,strlen(defaultHeader),0);

    // start table header
	send(socket,szStartBrowseListingHeader,strlen(szStartBrowseListingHeader),0);
    // drop in a title
    if (vAlbums.Size() > 0)
        send(socket,szAlbums,strlen(szAlbums),0);
    else
        send(socket,szNoAlbums,strlen(szNoAlbums),0);

    // perhaps with a genre constraint indicated
    if (nGenreConstraint != CMK_ALL)
    {   
        send(socket, szInGenre, strlen(szInGenre),0);
        send(socket, tszGenreConstraint, tstrlen(tszGenreConstraint)*2,0);
        send(socket, "'", 1, 0);
    }
    // perhaps with an artist constraint indicated
    if (nArtistConstraint != CMK_ALL)
    {   
        send(socket, szByArtist, strlen(szByArtist),0);
        send(socket, tszArtistConstraint, tstrlen(tszArtistConstraint)*2,0);
        send(socket, "'", 1, 0);
    }
    
    // end header
    send(socket,szEndBrowseListingHeader,strlen(szEndBrowseListingHeader),0);

	for (int i = 0; i < vAlbums.Size(); ++i)
	{
		// start the table entry
		send(socket,szStartBrowseListingItem,strlen(szStartBrowseListingItem),0);

		// genre text
    	send(socket,(char*)vAlbums[i].szValue,tstrlen(vAlbums[i].szValue)*2,0);

        // middle one html
        send(socket,szMiddle1BrowseListingItem,strlen(szMiddle1BrowseListingItem),0);

        // function for the next lower level of browsing
        send(socket,szBrowseTracks,strlen(szBrowseTracks),0);

        if (nGenreConstraint != CMK_ALL)
        {
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:passing along genre constraint\n"); 
            SendConstraint(socket,"genre",tszGenreConstraint);
        }
        if (nArtistConstraint != CMK_ALL)
        {
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:passing along genre constraint\n"); 
            SendConstraint(socket,"artist",tszArtistConstraint);
        }
        SendConstraint(socket,"album",vAlbums[i].szValue);

        // middle two html
        send(socket,szMiddle2BrowseListingItem,strlen(szMiddle2BrowseListingItem),0);

        if (nGenreConstraint != CMK_ALL)
        {
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:passing along genre constraint\n"); 
            SendConstraint(socket,"genre",tszGenreConstraint);
        }
        if (nArtistConstraint != CMK_ALL)
        {
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:passing along genre constraint\n"); 
            SendConstraint(socket,"artist",tszArtistConstraint);
        }
        SendConstraint(socket,"album",vAlbums[i].szValue);
       
        // middle three html
        send(socket,szMiddle3BrowseListingItem,strlen(szMiddle3BrowseListingItem),0);

        if (nGenreConstraint != CMK_ALL)
        {
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:passing along genre constraint\n"); 
            SendConstraint(socket,"genre",tszGenreConstraint);
        }
        if (nArtistConstraint != CMK_ALL)
        {
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:passing along genre constraint\n"); 
            SendConstraint(socket,"artist",tszArtistConstraint);
        }
        SendConstraint(socket,"album",vAlbums[i].szValue);

        // end table entry
		send(socket,szEndBrowseListingItem,strlen(szEndBrowseListingItem),0);
	}

    // end table
    send(socket,szEndBrowseListing,strlen(szEndBrowseListing),0);

	shutdown(socket,SD_BOTH);
	close(socket);

	return 1;
};

// these versions are similar to the BrowseListing versions, but remove the 'browse' link and extend the item-text into that space.
// start row
char * szStartBrowseTracksItem =   

"          <tr>\n"
"            <td class=\"listing\" width=\"272\">\n";

// * the item text goes here

char * szMiddle1BrowseTracksItem = 
"            </td>\n"
"            <td class=\"actionlinks\" width=\"28\"><a href=\"?setPlaylist";

char * szTracks = "tracks";
char * szOnAlbum = " on album '";

int BrowseTracks(int socket, const char* path)
{
    DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:browseTracks %s\n",path); 

#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
    CDataSourceContentManager* pCM = (CDataSourceContentManager*) CPlayManager::GetInstance()->GetContentManager();
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
    CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
#endif
    
    // check for genre constraint
    int nGenreConstraint = CMK_ALL;
    TCHAR tszGenreConstraint[PLAYLIST_STRING_SIZE];
    char szGenreConstraint[PLAYLIST_STRING_SIZE];
    char szUnescapedGenre[PLAYLIST_STRING_SIZE];
	int ret = GetParamValue(path,"genre",(char*)szGenreConstraint,PLAYLIST_STRING_SIZE);
	if (ret)
    {
        unescape(szGenreConstraint,szUnescapedGenre);
        CharToTchar(tszGenreConstraint,szUnescapedGenre);
        nGenreConstraint = pCM->GetGenreKey(tszGenreConstraint);
        if (nGenreConstraint == CMK_ALL)
            DEBUGP( WEBCONTROL, DBGLEV_WARNING, "WC:genre %s not found\n",szUnescapedGenre); 
    }

    // check for artist constraint
    int nArtistConstraint = CMK_ALL;
    TCHAR tszArtistConstraint[PLAYLIST_STRING_SIZE];
    char szArtistConstraint[PLAYLIST_STRING_SIZE];
    char szUnescapedArtist[PLAYLIST_STRING_SIZE];
	ret = GetParamValue(path,"artist",(char*)szArtistConstraint,PLAYLIST_STRING_SIZE);
	if (ret)
    {
        unescape(szArtistConstraint,szUnescapedArtist);
        CharToTchar(tszArtistConstraint,szUnescapedArtist);
        nArtistConstraint = pCM->GetArtistKey(tszArtistConstraint);
        if (nArtistConstraint == CMK_ALL)
            DEBUGP( WEBCONTROL, DBGLEV_WARNING, "WC:artist %s not found\n",szUnescapedArtist); 
    }

    // check for album constraint
    int nAlbumConstraint = CMK_ALL;
    TCHAR tszAlbumConstraint[PLAYLIST_STRING_SIZE];
    char szAlbumConstraint[PLAYLIST_STRING_SIZE];
    char szUnescapedAlbum[PLAYLIST_STRING_SIZE];
	ret = GetParamValue(path,"album",(char*)szAlbumConstraint,PLAYLIST_STRING_SIZE);
	if (ret)
    {
        unescape(szAlbumConstraint,szUnescapedAlbum);
        CharToTchar(tszAlbumConstraint,szUnescapedAlbum);
        nAlbumConstraint = pCM->GetAlbumKey(tszAlbumConstraint);
        if (nAlbumConstraint == CMK_ALL)
            DEBUGP( WEBCONTROL, DBGLEV_WARNING, "WC:album %s not found\n",szUnescapedAlbum); 
    }

    int nDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
    MediaRecordList mrlTracks;


    if (nAlbumConstraint != CMK_ALL)
        pCM->GetMediaRecordsAlbumSorted(mrlTracks, nArtistConstraint, nAlbumConstraint, nGenreConstraint, nDSID);
    else
    {
        DEBUGP( WEBCONTROL, DBGLEV_WARNING, "WC:looking up tracks w/o a valid album.\n"); 
        pCM->GetMediaRecordsTitleSorted(mrlTracks, nArtistConstraint, nAlbumConstraint, nGenreConstraint, nDSID);
    }

    if (!mrlTracks.Size())
    {
        DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:no tracks, sending null response\n"); 
        // (epg,6/28/2002): todo: put up a reasonable title with something like "no tracks found".
        send(socket,szNullFrameResponse,strlen(szNullFrameResponse),0);
		shutdown(socket,SD_BOTH);
        close(socket);
        return 0;
    }

    send(socket,defaultHeader,strlen(defaultHeader),0);

    // start table header
	send(socket,szStartBrowseListingHeader,strlen(szStartBrowseListingHeader),0);
    // drop in a title
    send(socket,szTracks,strlen(szTracks),0);

    // perhaps with a genre constraint indicated
    if (nGenreConstraint != CMK_ALL)
    {   
        send(socket, szInGenre, strlen(szInGenre),0);
        send(socket, tszGenreConstraint, tstrlen(tszGenreConstraint)*2,0);
        send(socket, "'", 1, 0);
    }
    // perhaps with an artist constraint indicated
    if (nArtistConstraint != CMK_ALL)
    {   
        send(socket, szByArtist, strlen(szByArtist),0);
        send(socket, tszArtistConstraint, tstrlen(tszArtistConstraint)*2,0);
        send(socket, "'", 1, 0);
    }
    // perhaps with an album constraint indicated
    if (nAlbumConstraint != CMK_ALL)
    {   
        send(socket, szOnAlbum, strlen(szOnAlbum),0);
        send(socket, tszAlbumConstraint, tstrlen(tszAlbumConstraint)*2,0);
        send(socket, "'", 1, 0);
    }
    
    // end header
    send(socket,szEndBrowseListingHeader,strlen(szEndBrowseListingHeader),0);

    for (MediaRecordIterator itTrack = mrlTracks.GetHead(); itTrack != mrlTracks.GetEnd(); ++itTrack)
	{
		// start the table entry
		send(socket,szStartBrowseTracksItem,strlen(szStartBrowseTracksItem),0);

        // track text
        void* pData = 0;
        (*itTrack)->GetAttribute(MDA_TITLE,&pData);
    	send(socket,(char*)pData,tstrlen((TCHAR*)pData)*2,0);

        // middle one html
        send(socket,szMiddle1BrowseTracksItem,strlen(szMiddle1BrowseTracksItem),0);

        SendConstraint(socket,"track",(char*)(*itTrack)->GetURL());
       
        // middle three html
        send(socket,szMiddle3BrowseListingItem,strlen(szMiddle3BrowseListingItem),0);

        SendConstraint(socket,"track",(char*)(*itTrack)->GetURL());

        // end table entry
		send(socket,szEndBrowseListingItem,strlen(szEndBrowseListingItem),0);
	}

    // end table
    send(socket,szEndBrowseListing,strlen(szEndBrowseListing),0);

	shutdown(socket,SD_BOTH);
	close(socket);

	return 1;
}

char* szPlaylists = "playlists";
char* szPlayEverything = "play everything";

int BrowsePlaylists(int socket, const char * path)
{
    PlaylistRecordList prl;
    int nDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
	CPlayManager::GetInstance()->GetContentManager()->GetPlaylistRecordsByDataSourceID(prl, nDSID);

    send(socket,defaultHeader,strlen(defaultHeader),0);

    // start table header
	send(socket,szStartBrowseListingHeader,strlen(szStartBrowseListingHeader),0);
    
    // drop in a title
    send(socket,szPlaylists,strlen(szPlaylists),0);

    // end header
    send(socket,szEndBrowseListingHeader,strlen(szEndBrowseListingHeader),0);


    // add a static entry for "Play Everything".  by just leaving the constraints out of the links, we get the desired effect.
	// start the table entry
	send(socket,szStartBrowseTracksItem,strlen(szStartBrowseTracksItem),0);
    send(socket,szPlayEverything,strlen(szPlayEverything),0);
    // middle one html
    send(socket,szMiddle1BrowseTracksItem,strlen(szMiddle1BrowseTracksItem),0);
    // middle three html
    send(socket,szMiddle3BrowseListingItem,strlen(szMiddle3BrowseListingItem),0);
    // end table entry
	send(socket,szEndBrowseListingItem,strlen(szEndBrowseListingItem),0);


    for (PlaylistRecordIterator it = prl.GetHead(); it != prl.GetEnd(); ++it)
	{
		// start the table entry
		send(socket,szStartBrowseTracksItem,strlen(szStartBrowseTracksItem),0);

        const char* szFilename = FilenameFromURLInPlace((*it)->GetURL());
        // make a copy to molest
        char szCaption[PLAYLIST_STRING_SIZE];
        strcpy(szCaption,szFilename);
        int len = strlen(szCaption);
        // lop off the extension
        szCaption[len-4] = 0;

    	send(socket,szCaption,strlen(szCaption),0);

        // middle one html
        send(socket,szMiddle1BrowseTracksItem,strlen(szMiddle1BrowseTracksItem),0);
        
        SendConstraint(socket,"playlist",(char*)(*it)->GetURL());
       
        // middle three html
        send(socket,szMiddle3BrowseListingItem,strlen(szMiddle3BrowseListingItem),0);

        SendConstraint(socket,"playlist",(char*)(*it)->GetURL());

        // end table entry
		send(socket,szEndBrowseListingItem,strlen(szEndBrowseListingItem),0);
	}

    // end table
    send(socket,szEndBrowseListing,strlen(szEndBrowseListing),0);

	shutdown(socket,SD_BOTH);
	close(socket);

	return 1;

};

char* szBitmapHeaderSprintfBase = 
"HTTP/1.1 200 OK\r\n"
"Connection: close\r\n"
"SERVER: FullplayMediaOS\r\n"
"Content-Length: %d\r\n"
"Content-Type: image/bitmap\r\n"
"\r\n";


int ScreenShot(int socket, const char * path)
{
	
	DWORD bmp_len = 0;
       UCHAR* bmp_buf=0;
	PegRect Rect;
	Rect.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
        // do a screen print
    PegThing pThing;

    bmp_buf = pThing.Screen()->DumpWindowsBitmap( &bmp_len, Rect);

	if (bmp_buf)
	{

		char responseHeader[400];

		memset(responseHeader,0,400);

		sprintf(responseHeader,szBitmapHeaderSprintfBase,bmp_len);

		send(socket,responseHeader,strlen(responseHeader),0);
		send(socket,bmp_buf,bmp_len,0);

		free(bmp_buf);

		shutdown(socket,SD_BOTH);
		close(socket);

		return 1;
	}

	return 0;
}

static char sBlackButton[1024];
static int nBlackButtonLen;
static char sRedButton[1024];
static int nRedButtonLen;
static char sGreenButton[1024];
static int nGreenButtonLen;

static void InitRecordButtons()
{
    CFatDataSource* fds = CDJPlayerState::GetInstance()->GetFatDataSource();        
    // red
    IInputStream* in = fds->OpenInputStream("file://a:/webroot/images/fp_Ered.gif");
    DBASSERT( WEBCONTROL, ( in!=0 ) , "wc:can't find red button\n"); 
    nRedButtonLen = in->Read(sRedButton,1023);
    delete in;
    // black
    in = fds->OpenInputStream("file://a:/webroot/images/fp_Eblack.gif");
    DBASSERT( WEBCONTROL, ( in!=0 ) , "wc:can't find black button\n"); 
    nBlackButtonLen = in->Read(sBlackButton,1023);
    delete in;
    // green
    in = fds->OpenInputStream("file://a:/webroot/images/fp_Egreen.gif");
    DBASSERT( WEBCONTROL, ( in!=0 ) , "wc:can't find green button\n"); 
    nGreenButtonLen = in->Read(sGreenButton,1023);
    delete in;
}

// query the recording led state, and send the proper gif.
int RecordButton(int socket, const char * path)
{
    static bool bFirst = true;
    if (bFirst)
    {
        bFirst = false;
        InitRecordButtons();
    }

	char responseHeader[400];

	memset(responseHeader,0,400);

    LEDColor eColor = GetLEDColor();
    switch (eColor)
    {
        case RED:
        case ORANGE:
	        sprintf(responseHeader,szBitmapHeaderSprintfBase,nRedButtonLen);
	        send(socket,responseHeader,strlen(responseHeader),0);
	    	send(socket,sRedButton,nRedButtonLen,0);
            break;
        case GREEN:
	        sprintf(responseHeader,szBitmapHeaderSprintfBase,nGreenButtonLen);
	        send(socket,responseHeader,strlen(responseHeader),0);
    		send(socket,sGreenButton,nGreenButtonLen,0);
            break;
        case OFF:
        default:
	        sprintf(responseHeader,szBitmapHeaderSprintfBase,nBlackButtonLen);
	        send(socket,responseHeader,strlen(responseHeader),0);
	    	send(socket,sBlackButton,nBlackButtonLen,0);
            break;
    }
}

int SetPlaylist(int socket, const char * path)
{
    DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:setPlaylist %s\n",path); 

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
    CDJPlayerState::GetInstance()->GetUserInterface()->NotifyPlaylistCleared(false);

    pPM->Deconfigure();
    pPS->ClearTrack();
    pPS->SetTrackText(LS(SID_LOADING));
    pPS->HideMenus();
    pIMLManager->ClearMediaRecords();

    // add to the cleared out playlist
    return AddToPlaylist(socket,path);

}

int AddToPlaylist(int socket, const char * path)
{
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
    CQuickBrowseMenuScreen* pQB = CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen();

    // dc- unconditionally set the quickbrowse title to 'Current Playlist'. this is the default text used, and
    //     since web control is deprioritized it is safe to use this for 1.0.
    pQB->SetCurrentPlaylistTitle(LS(SID_CURRENT_PLAYLIST));
    
    // in Lib Menu Screen, this wouldn't happen on an add, but I think it makes sense to normalize a bit here, since the user isn't really
    // using the menus per se, but rather web control.  and since whatever menus they're looking at might not make sense, shortly.
    pPS->HideMenus();

    // quietly set the source to HD, so the query actually finds something.
    // (epg,7/5/2002): I'm a little concerned about what this will do if the user is on CD, and they try to add content from HDD.
    // Will SetSource have the (here) desired effect of removing cd tracks from the current list?  not sure how that separation is handled
    // internally.  perhaps bSendEvent IS needed?
    pPS->SetEventHandlerMode(ePSPlaybackEvents);
    CDJPlayerState::GetInstance()->SetSource(CDJPlayerState::HD,true);

    // check for genre constraint
    int nGenreConstraint = CMK_ALL;
    TCHAR tszGenreConstraint[PLAYLIST_STRING_SIZE];
    char szGenreConstraint[PLAYLIST_STRING_SIZE];
    char szUnescapedGenre[PLAYLIST_STRING_SIZE];
	int ret = GetParamValue(path,"genre",(char*)szGenreConstraint,PLAYLIST_STRING_SIZE);
	if (ret)
    {
        unescape(szGenreConstraint,szUnescapedGenre);
        CharToTchar(tszGenreConstraint,szUnescapedGenre);
        nGenreConstraint = pCM->GetGenreKey(tszGenreConstraint);
        if (nGenreConstraint == CMK_ALL) {
            DEBUGP( WEBCONTROL, DBGLEV_WARNING, "WC:genre %s not found\n",szUnescapedGenre); 
            return ShowPlaylist(socket,path);
        } else
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:genre %s has key %d\n",szUnescapedGenre,nGenreConstraint); 
    }
    else
        DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:no genre param\n"); 

    // check for artist constraint
    int nArtistConstraint = CMK_ALL;
    TCHAR tszArtistConstraint[PLAYLIST_STRING_SIZE];
    char szArtistConstraint[PLAYLIST_STRING_SIZE];
    char szUnescapedArtist[PLAYLIST_STRING_SIZE];
	ret = GetParamValue(path,"artist",(char*)szArtistConstraint,PLAYLIST_STRING_SIZE);
	if (ret)
    {
        unescape(szArtistConstraint,szUnescapedArtist);
        CharToTchar(tszArtistConstraint,szUnescapedArtist);
        nArtistConstraint = pCM->GetArtistKey(tszArtistConstraint);
        if (nArtistConstraint == CMK_ALL) {
            DEBUGP( WEBCONTROL, DBGLEV_WARNING, "WC:artist %s not found\n",szUnescapedArtist); 
            return ShowPlaylist(socket,path);
        } else
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:artist %s has key %d\n",szUnescapedArtist,nArtistConstraint); 
    }
    else
        DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:no artist param\n"); 

    // check for album constraint
    int nAlbumConstraint = CMK_ALL;
    TCHAR tszAlbumConstraint[PLAYLIST_STRING_SIZE];
    char szAlbumConstraint[PLAYLIST_STRING_SIZE];
    char szUnescapedAlbum[PLAYLIST_STRING_SIZE];
	ret = GetParamValue(path,"album",(char*)szAlbumConstraint,PLAYLIST_STRING_SIZE);
	if (ret)
    {
        unescape(szAlbumConstraint,szUnescapedAlbum);
        CharToTchar(tszAlbumConstraint,szUnescapedAlbum);
        nAlbumConstraint = pCM->GetAlbumKey(tszAlbumConstraint);
        if (nAlbumConstraint == CMK_ALL) {
            
            DEBUGP( WEBCONTROL, DBGLEV_WARNING, "WC:album %s not found\n",szUnescapedAlbum); 
            return ShowPlaylist(socket,path);
        } else
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:album %s has key %d\n",szUnescapedAlbum,nAlbumConstraint); 
    }
    else
        DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:no album param\n"); 

    // check for track url constraint
    // stricly speaking, this is an unbounded string, so make it fairly large, as there's no easy way to make it truly dynamic.
    IMediaContentRecord *pmcrTrack = NULL;
    char                 szTrackUrl[PLAYLIST_STRING_SIZE*4];
    char                 szUnescapedTrackUrl[PLAYLIST_STRING_SIZE*4];
	ret = GetParamValue(path,"track",(char*)szTrackUrl,PLAYLIST_STRING_SIZE*4);
	if (ret)
    {
        unescape(szTrackUrl,szUnescapedTrackUrl);
        pmcrTrack = pCM->GetMediaRecord(szUnescapedTrackUrl);
        if (!pmcrTrack) {
            
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:track %s not found\n",szUnescapedTrackUrl); 
            return ShowPlaylist(socket,path);
        }
        
    }

    // check for playlist url constraint
    // stricly speaking, this is an unbounded string, so make it fairly large, as there's no easy way to make it truly dynamic.
    IMediaContentRecord *pmcrPlaylist = NULL;
    char szPlaylistUrl[PLAYLIST_STRING_SIZE*4];
    char szUnescapedPlaylistUrl[PLAYLIST_STRING_SIZE*4];
	ret = GetParamValue(path,"playlist",(char*)szPlaylistUrl,PLAYLIST_STRING_SIZE*4);
	if (ret)
    {
        unescape(szPlaylistUrl,szUnescapedPlaylistUrl);
#if 0
        pmcrPlaylist = pCM->GetMediaRecord(szUnescapedPlaylistUrl);
        if (!pmcrPlaylist)
            DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:playlist %s not found\n",szUnescapedPlaylistUrl);
#endif
    }

    if (pmcrTrack)
    {
        DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:Loading Track\n"); 
        pPS->SetMessageText(LS(SID_LOADING_TRACK), CSystemMessageString::REALTIME_INFO);
        pPC->Constrain();
        pPC->SetTrack(pmcrTrack);
        pPC->UpdatePlaylist();
        CRecordingManager::GetInstance()->ProcessEncodingUpdates();
        if (FAILED(pPM->Play()))
            if (FAILED(pPM->NextTrack()))
            {
                pPS->DisplayNoContentScreen();
                pPS->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
            }
        return ShowPlaylist(socket,path);
    }

    if (ret)
    {
        DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:Loading Playlist\n"); 
        pPS->SetMessageText(LS(SID_LOADING_PLAYLIST), CSystemMessageString::REALTIME_INFO);
        pPC->Constrain();
        pPC->SetPlaylistURL(szUnescapedPlaylistUrl);
        pPC->UpdatePlaylist();
        CRecordingManager::GetInstance()->ProcessEncodingUpdates();
        if (FAILED(pPM->Play()))
            if (FAILED(pPM->NextTrack()))
            {
                pPS->DisplayNoContentScreen();
                pPS->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
            }
        return ShowPlaylist(socket,path);
    }


    DEBUGP( WEBCONTROL, DBGLEV_INFO, "wc:Loading Constraints Artist %d, Album %d, Genre %d\n",nArtistConstraint,nAlbumConstraint,nGenreConstraint); 
    pPC->Constrain(nArtistConstraint,nAlbumConstraint,nGenreConstraint);
    pPC->UpdatePlaylist();
    CRecordingManager::GetInstance()->ProcessEncodingUpdates();

    if (FAILED(pPM->Play()))
        if (FAILED(pPM->NextTrack()))
        {
            pPS->DisplayNoContentScreen();
            pPS->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
        }

    return ShowPlaylist(socket,path);
}

char * startPlaylist = 
"<html>\n"
"<head>\n"
//"  <script language=\"Javascript\">\n"
//"   <!-- hide\n"
//"    var x = 30;\n"
//"    var y = 1;\n"
//"    var now;\n"
//"    function startClock() {\n"
//"     x = x-y;\n"
//"     if (x <= 1) reload();\n"
//"     timerID = setTimeout(\"startClock()\", 1000);\n"
//"    }\n"
  
//"    function reload() {\n"
//"     now = new Date();\n"
//"     var newplaylist= \"?showplaylist&amp;updateID=\" + \"&\" + now.getTime();\n"
//"	parent.frames[2].location = newplaylist;\n"
//"    }\n"
//"   // end hide -->\n"
//"  </script>\n"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">\n"
"<META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n"
"<link rel=\"stylesheet\" type=\"text/css\" href=\"djstyle.css\">\n"
"<title>Current Playlist</title>\n"
"<base target=\"_self\">\n"
"</head>\n"
"<body bgcolor=\"#ffffff\">\n"
"  <table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"281\" height=\"100%\">\n"
"    <tr>\n"
"      <td width=\"30\"><img border=\"0\" src=\"images/clear.gif\" width=\"1\" height=\"1\"></td>\n"
"      <td valign=\"top\" width=\"281\" height=\"100%\">\n"
"        <div align=\"top\">\n"
"        <center>\n"
"		  <table border=\"0\" cellpadding=\"4\" cellspacing=\"1\" width=\"281\">\n"
"          		<tr>\n"
"            		<td class=\"firstline\" colspan=\"1\" width=\"230\" valign=\"top\">"
"current playlist</td>\n"
"                   <td class=\"actionlinks\" colspan=\"1\" width=\"30\" valign=\"top\">"
"<a href=\"?showplaylist\">refresh</a></td>\n"
"            	</tr>\n";

char * startPlaylistItem = "<tr><td class=\"listing\" width=\"215\">\n";
char * startCurrentPlaylistItem = "<tr><td class=\"crnt_listing\" width=\"215\">\n";

char * middlePlaylistItem = "</td> <td class=\"actionlinks\" width=25><a href=\"?showplaylist&amp;selection=";

char * endPlaylistItem = "\">play</a></td> </tr>";

char * endPlaylist =   
"			</table>\n"
"			</center>\n"
"			</div>\n"
"        </td>\n"
"    </tr>\n"
"  </table>\n"
"</body>\n";

char * refreshPlaylist =
"<html><head>\n"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">\n"
"<META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n"
"<META HTTP-EQUIV=Refresh CONTENT=\"5; URL=?showplaylist\">\n"
"<link rel=\"stylesheet\" type=\"text/css\" href=\"djstyle.css\">\n"
"<title>Refreshing Playlist...</title>\n"
"<base target=\"_self\">\n"
"</head>\n"
"<body bgcolor=\"#ffffff\">\n"
"  <table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"281\" height=\"100%\">\n"
"    <tr>\n"
"      <td width=\"30\"><img border=\"0\" src=\"images/clear.gif\" width=\"1\" height=\"1\"></td>\n"
"      <td valign=\"top\" width=\"281\" height=\"100%\">\n"
"        <div align=\"top\">\n"
"        <center>\n"
"		  <table border=\"0\" cellpadding=\"4\" cellspacing=\"1\" width=\"281\">\n"
"          		<tr>\n"
"            		<td class=\"firstline\" colspan=\"2\" width=\"100%\" valign=\"top\">"
"refreshing playlist, please wait</td>\n"
"            	</tr>\n"
"         </table>\n"
"        </center>\n"
"			</div>\n"
"        </td>\n"
"    </tr>\n"
"  </table>\n"
"</body></html>\n";

int ShowPlaylist(int socket, const char * path)
{

	char selection[20];
	int res;

    // dc- if the 'playlist' tag is in here, it means we are loading a playlist from disk. in this case,
    //     it is not immediately available, so issue a redirect to showplaylist in this pane with a few seconds on it
    //     scoped so the tmp var doesn't eat stack.
    {
        char tmp[ PLAYLIST_STRING_SIZE*4 ];
        if( GetParamValue(path,"playlist",(char*)tmp,PLAYLIST_STRING_SIZE*4) ) {
            send(socket, refreshPlaylist, strlen(refreshPlaylist),0);
            shutdown(socket,SD_BOTH);
            close(socket);
            return 1;
        }
    }

	int DoSelection = GetParamValue(path,"selection",selection,20);

	IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();

	if (DoSelection)
	{
		int iSelectIndex = atoi(selection);

		
		IPlaylistEntry* pTargetEntry = pCurrentPlaylist->GetEntry(iSelectIndex);

		if (pTargetEntry)
		{

			if (pCurrentPlaylist->SetCurrentEntry(pTargetEntry))
			{
				if (SUCCEEDED(res = CPlayManager::GetInstance()->SetSong(pTargetEntry)))
				{
                    // get the play manager playing the track.
                    CPlayManager::GetInstance()->Play();
                    // then post a key event to sync up the play state icon.  this has to be an event to prevent UI corruption,
                    // and OTOH it won't suffice to just do this w/o the playmanager command, as the keypress won't necessarily
                    // GET to the playerscreen, if e.g. the menu is up.
                    CEventQueue::GetInstance()->PutEvent(EVENT_KEY_PRESS,(void*)KEY_PLAY);
				}
			}

		}
		
	}

	int iSizePlaylist = pCurrentPlaylist->GetSize();
	// looks like we are good to go

	send(socket,defaultHeader,strlen(defaultHeader),0);

	send(socket,startPlaylist,strlen(startPlaylist),0);

	IPlaylistEntry* pCurrentEntry;
	int iCurrentEntryIndex = 0;

	if (iSizePlaylist > 0)
	{
		pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();

		iCurrentEntryIndex = pCurrentEntry->GetIndex();
	

		for (int i = 0; i < iSizePlaylist; i++)
		{
			pCurrentEntry = pCurrentPlaylist->GetEntry(i);

			if (pCurrentEntry)
			{
				int iThisEntryIndex = pCurrentEntry->GetIndex();
				char index[6];
				sprintf(index,"%d",iThisEntryIndex);

				// start of the table entry
                if (i==iCurrentEntryIndex)
                    // for the curent entry, make the text bold and white
                    send(socket,startCurrentPlaylistItem,strlen(startCurrentPlaylistItem),0);
                else
                    // otherwise, standard listing font.
				    send(socket,startPlaylistItem,strlen(startPlaylistItem),0);

                // the text of the item
				void* pdata;
				if (SUCCEEDED(pCurrentEntry->GetContentRecord()->GetAttribute(MDA_TITLE,&pdata)))
				{
					if (i==iCurrentEntryIndex) send(socket,"<b>",3,0);
					send(socket,(char*)pdata,tstrlen((TCHAR*)pdata)*2,0);
					if (i==iCurrentEntryIndex) send(socket,"</b>",4,0);
				}

				send(socket,middlePlaylistItem,strlen(middlePlaylistItem),0);

				send(socket,index,strlen(index),0);

				send(socket,endPlaylistItem,strlen(endPlaylistItem),0);

			}
		}
	}

	send(socket,endPlaylist,strlen(endPlaylist),0);

	shutdown(socket,SD_BOTH);
	close(socket);
	return 1;


}

/*

   DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:RefreshPlaylist\n");
    // check to see if there is a current playlist
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    char szNumberOf[32];

    // get the size of the playlist
    if (pCurrentPlaylist)
        m_iPlaylistCount = pCurrentPlaylist->GetSize();
    else
        m_iPlaylistCount = 0;

    if (m_iPlaylistCount > 0)
    {
        SetItemCount(m_iPlaylistCount);

        // get the playlist info
        IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
        {
            int iCurrentEntryIndex = pCurrentEntry->GetIndex();
            if (bForceMenuSynch)
            {
                m_iTopIndex = iCurrentEntryIndex - 1;  // ?

			    // find the offset.  what do we want selected?
			    if (iCurrentTrackOffset != 0)
			    {
				    int iIndex = iCurrentEntryIndex + iCurrentTrackOffset;
				    if (iIndex < 0)
                        m_iTopIndex = -1;
                    else if (iIndex >= m_iPlaylistCount)
                        m_iTopIndex = m_iPlaylistCount - 2;
                    else
					    m_iTopIndex = iIndex - 1;
			    }
            }

            // get the current tracks info
            void* pdata;
            if (SUCCEEDED(pCurrentEntry->GetContentRecord()->GetAttribute(MDA_TITLE,&pdata)))
                m_pTrackTextString->DataSet((TCHAR*)pdata);
            sprintf(szNumberOf, "%d / %d", iCurrentEntryIndex + 1 , m_iPlaylistCount);
        }
        else
        {
            m_pTrackTextString->DataSet(NULL);
            sprintf(szNumberOf, "/ %d", m_iPlaylistCount);
        }

    }
    else
    {
        m_pTrackTextString->DataSet(NULL);
        SetItemCount(0);
        m_iTopIndex = -1;
        strcpy(szNumberOf, "0 / 0");
    }

    // now that we know how the info on the playlist, display it
    SetNumberOfText(szNumberOf);

  */

char* szServerBusy =

"<HTML>"
"<META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n"
"<HEAD>"
"<TITLE>Server busy</TITLE>"
"<BODY BGCOLOR=\"#b8c8db\">"
"<H2>Server busy</H2>"
"<HR>"
"<P>The server is too busy to serve your request at the moment. Please try again later by refreshing the whole page.</P>"
"</BODY>"
"</HTML>";


int SendRefusalReply(int nSocket)
{
    send (nSocket, defaultHeader, strlen(defaultHeader),0);
    send (nSocket, szServerBusy,strlen(szServerBusy),0);

	shutdown(nSocket,SD_BOTH);
	close(nSocket);
	return 1;
}

char* szServerAsleep =

"<HTML>"
"<META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n"
"<HEAD>"
"<TITLE>Fullplay DJ-C40</TITLE>"
"<BODY BGCOLOR=\"#b8c8db\">"
"<H2>Server not configured for web control</H2>"
"<HR>"
"<P>The jukebox you are contacting is not configured for web control.  To turn this feature on, go to Jukebox Settings, Customizations, Enable Web Control, and select Yes.</P>"
"</BODY>"
"</HTML>";

int SendAsleepReply(int nSocket)
{
    send (nSocket, defaultHeader, strlen(defaultHeader),0);
    send (nSocket, szServerAsleep,strlen(szServerAsleep),0);

	shutdown(nSocket,SD_BOTH);
	close(nSocket);
	return 1;
}

char* szServerPoweredDown =

"<HTML>"
"<META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\n"
"<HEAD>"
"<TITLE>Fullplay DJ-C40</TITLE>"
"<BODY BGCOLOR=\"#b8c8db\">"
"<H2>Device is powered down</H2>"
"<HR>"
"<P>The jukebox you are contacting is powered down. Please turn on the device and then refresh this page.</P>"
"</BODY>"
"</HTML>";

int SendPoweredDownReply(int nSocket)
{
    send (nSocket, defaultHeader, strlen(defaultHeader),0);
    send (nSocket, szServerPoweredDown,strlen(szServerPoweredDown),0);

	shutdown(nSocket,SD_BOTH);
	close(nSocket);
	return 1;
}

