//
// Strings.cpp: contains the raw data arrays for strings used throughout the UI
// danb@fullplaymedia.com 09/04/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/main/AppSettings.h>

#include <gui/peg/peg.hpp>

/*---------------------------------------------------------------*/
ROMDATA TCHAR gsIDL_ENG_EMPTY_STRING[] = 
{   0};
ROMDATA TCHAR gsIDL_ENG_LATIN_CHARACTERS_TEST_STRING[] = 
{ '!','"','#','$','%','&','\'','(',')','*','+',',','-','.','/','0',
  '1','2','3','4','5','6','7','8','9',':',';','<','=','>','?','@',
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
  'Q','R','S','T','U','V','W','X','Y','Z','[','\\',']','^','_','`',
  'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p',
  'q','r','s','t','u','v','w','x','y','z','{','|','}',' ', 0};
ROMDATA TCHAR gsIDL_ENG_EXTENDED_LATIN_CHARACTERS_TEST_STRING[] = 
{ ' ','¡','¢','£','¤','¥','¦','§','¨','©','ª','«','¬','-','®','¯',
  '°','±','²','³','´','µ','¶','·','¸','¹','º','»','¼','½','¾','¿',
  'À','Á','Â','Ã','Ä','Å','Æ','Ç','È','É','Ê','Ë','Ì','Í','Î','Ï',
  'Ð','Ñ','Ò','Ó','Ô','Õ','Ö','×','Ø','Ù','Ú','Û','Ü','Ý','Þ','ß',
  'à','á','â','ã','ä','å','æ','ç','è','é','ê','ë','ì','í','î','ï',
  'ð','ñ','ò','ó','ô','õ','ö','÷','ø','ù','ú','û','ü','ý','þ','ÿ', 0};
ROMDATA TCHAR gsIDL_ENG_LATIN_CHARACTERS_BACKWARDS_TEST_STRING[] = 
{ '~','}','|','{','z','y','x','w','v','u','t','s','r','q','p','o',
  'n','m','l','k','j','i','h','g','f','e','d','c','b','a','`','_',
  '^',']','\\','[','Z','Y','X','W','V','U','T','S','R','Q','P','O',
  'N','M','L','K','J','I','H','G','F','E','D','C','B','A','@','?',
  '>','=','<',';',':','9','8','7','6','5','4','3','2','1','0','/',
  '.','-',',','+','*',')','(','\'','&','%','$','#','"','!',' ', 0};
ROMDATA TCHAR gsIDL_ENG_EXTENDED_LATIN_CHARACTERS_BACKWARDS_TEST_STRING[] = 
{ 'ÿ','þ','ý','ü','û','ú','ù','ø','÷','ö','õ','ô','ó','ò','ñ','ð',
  'ï','î','í','ì','ë','ê','é','è','ç','æ','å','ä','ã','â','á','à',
  'ß','Þ','Ý','Ü','Û','Ú','Ù','Ø','×','Ö','Õ','Ô','Ó','Ò','Ñ','Ð',
  'Ï','Î','Í','Ì','Ë','Ê','É','È','Ç','Æ','Å','Ä','Ã','Â','Á','À',
  '¿','¾','½','¼','»','º','¹','¸','·','¶','µ','´','³','²','±','°',
  '¯','®','-','¬','«','ª','©','¨','§','¦','¥','¤','£','¢','¡',' ', 0};
ROMDATA TCHAR gsIDL_ENG_ARTIST[] = 
{ 'A','r','t','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_ALBUM[] = 
{ 'A','l','b','u','m', 0};
ROMDATA TCHAR gsIDL_ENG_TITLE[] =
{ 'T','i','t','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_GENRE[] = 
{ 'G','e','n','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_FILE_NAME[] = 
{ 'F','i','l','e',' ','N','a','m','e', 0};
ROMDATA TCHAR gsIDL_ENG_NORMAL[] =
{ 'N','o','r','m','a','l', 0};
ROMDATA TCHAR gsIDL_ENG_RANDOM[] = 
{ 'R','a','n','d','o','m', 0};
ROMDATA TCHAR gsIDL_ENG_REPEAT[] = 
{ 'R','e','p','e','a','t', 0};
ROMDATA TCHAR gsIDL_ENG_REPEAT_RANDOM[] = 
{ 'R','e','p','e','a','t',' ','R','a','n','d','o','m', 0};
ROMDATA TCHAR gsIDL_ENG_TIME[] =
{ 'T','i','m','e', 0};
ROMDATA TCHAR gsIDL_ENG_NO_TRACKS_FOUND[] = 
{ 'N','o',' ','T','r','a','c','k','s',' ','f','o','u','n','d', 0};
ROMDATA TCHAR gsIDL_ENG_PLAYLIST[] = 
{ 'P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_PLAY_MODE[] = 
{ 'P','l','a','y',' ','M','o','d','e', 0};
ROMDATA TCHAR gsIDL_ENG_PLAYER_INFO[] =
{ 'P','l','a','y','e','r',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_SETTINGS[] =
{ 'S','e','t','t','i','n','g','s', 0 };
ROMDATA TCHAR gsIDL_ENG_INFO[] = 
{ 'I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_TRACK_TITLE[] = 
{ 'T','r','a','c','k',' ','t','i','t','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_TRACK[] =
{ 'T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_TRACKS[] =
{ 'T','r','a','c','k','s', 0};
ROMDATA TCHAR gsIDL_ENG_ARTISTS[] = 
{ 'A','r','t','i','s','t','s', 0};
ROMDATA TCHAR gsIDL_ENG_ALBUMS[] = 
{ 'A','l','b','u','m','s', 0};
ROMDATA TCHAR gsIDL_ENG_TITLES[] = 
{ 'T','i','t','l','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_GENRES[] = 
{ 'G','e','n','r','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_PLAYLISTS[] = 
{ 'P','l','a','y','l','i','s','t','s', 0};
ROMDATA TCHAR gsIDL_ENG_PLAY_ALL[] = 
{ 'P','l','a','y',' ','A','l','l', 0};
ROMDATA TCHAR gsIDL_ENG_CURRENT[] = 
{ 'C','u','r','r','e','n','t', 0};
ROMDATA TCHAR gsIDL_ENG_PLAY[] = 
{ 'P','l','a','y', 0}; 
ROMDATA TCHAR gsIDL_ENG_YES[] = 
{ 'Y','e','s', 0}; 
ROMDATA TCHAR gsIDL_ENG_NO[] = 
{ 'N','o', 0}; 
ROMDATA TCHAR gsIDL_ENG_OK[] = 
{ 'O','K', 0}; 
ROMDATA TCHAR gsIDL_ENG_CANCEL[] = 
{ 'C','a','n','c','e','l', 0}; 
ROMDATA TCHAR gsIDL_ENG_LENGTH[] = 
{ 'L','e','n','g','t','h', 0}; 
ROMDATA TCHAR gsIDL_ENG_SIZE[] = 
{ 'S','i','z','e', 0}; 
ROMDATA TCHAR gsIDL_ENG_TOTAL_TIME[] =
{ 'T','o','t','a','l',' ','T','i','m','e', 0};
ROMDATA TCHAR gsIDL_ENG_PLAY_EVERYTHING[] = 
{ 'P','l','a','y',' ','E','v','e','r','y','t','h','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_TRACK_ELAPSED[] = 
{ 'T','r','a','c','k',' ','E','l','a','p','s','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_TRACK_REMAINING[] = 
{ 'T','r','a','c','k',' ','R','e','m','a','i','n','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_PLAYER_V[] = 
{ 'P','l','a','y','e','r',' ','V','.', 0};
ROMDATA TCHAR gsIDL_ENG_PLAYER_BUILD[] = 
{ 'P','l','a','y','e','r',' ','B','u','i','l','d', 0};
ROMDATA TCHAR gsIDL_ENG_INSTALLER_V[] = 
{ 'I','n','s','t','a','l','l','e','r',' ','V','.', 0};
ROMDATA TCHAR gsIDL_ENG_SHUTTING_DOWN[] = 
{ 'S','h','u','t','t','i','n','g',' ','D','o','w','n', 0};
ROMDATA TCHAR gsIDL_ENG_INVALID_PLAYLIST[] = 
{ 'I','n','v','a','l','i','d',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_OPEN_TRACK[] =
{ 'C','o','u','l','d','n','\'','t',' ','O','p','e','n',' ','L','a','s','t',' ','T','r','a','c','k',0};
ROMDATA TCHAR gsIDL_ENG_FULLPLAY_MEDIA[] = 
{ 'F','u','l','l','p','l','a','y',' ','M','e','d','i','a', 0};
ROMDATA TCHAR gsIDL_ENG_FULLPLAY_MEDIA_C[] = 
{ 'F','u','l','l','p','l','a','y',' ','M','e','d','i','a',' ', 0x00a9, 0};
ROMDATA TCHAR gsIDL_ENG_SOURCE[] = 
{ 'S','o','u','r','c','e', 0};
ROMDATA TCHAR gsIDL_ENG_RADIO[] = 
{ 'R','a','d','i','o', 0};
ROMDATA TCHAR gsIDL_ENG_LOCAL[] = 
{ 'L','o','c','a','l', 0};
ROMDATA TCHAR gsIDL_ENG_LINE_INPUT[] = 
{ 'L','i','n','e',' ','I','n','p','u','t', 0};
ROMDATA TCHAR gsIDL_ENG_LINE_OUT[] = 
{ 'L','i','n','e',' ','O','u','t', 0};
ROMDATA TCHAR gsIDL_ENG_FML[] = 
{ 'F','M','L', 0};
ROMDATA TCHAR gsIDL_ENG_OPTION[] = 
{ 'O','p','t','i','o','n', 0};
ROMDATA TCHAR gsIDL_ENG_OPTIONS[] = 
{ 'O','p','t','i','o','n','s', 0};
ROMDATA TCHAR gsIDL_ENG_HD[] = 
{ 'H','D', 0};
ROMDATA TCHAR gsIDL_ENG_CD[] = 
{ 'C','D', 0};
ROMDATA TCHAR gsIDL_ENG_HARD_DISK[] = 
{ 'H','a','r','d',' ','D','i','s','k', 0};
ROMDATA TCHAR gsIDL_ENG_CD_DRIVE[] = 
{ 'C','D',' ','D','r','i','v','e', 0};
ROMDATA TCHAR gsIDL_ENG_SYSTEM_TOOLS[] = 
{ 'S','y','s','t','e','m',' ','T','o','o','l','s', 0};
ROMDATA TCHAR gsIDL_ENG_SYSTEM_INFO[] = 
{ 'S','y','s','t','e','m',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_SET_IP[] = 
{ 'S','e','t',' ','I','P', 0};
ROMDATA TCHAR gsIDL_ENG_DJ_NAME[] = 
{ 'D','J',' ','N','a','m','e', 0};
ROMDATA TCHAR gsIDL_ENG_DEFRAGMENT_HD[] = 
{ 'D','e','f','r','a','g','m','e','n','t',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_SCAN_HD_FOR_ERRORS[] = 
{ 'S','c','a','n',' ','H','D',' ','F','o','r',' ','E','r','r','o','r','s', 0};
ROMDATA TCHAR gsIDL_ENG_FIRMWARE_VERSION[] = 
{ 'F','i','r','m','w','a','r','e',' ','V','e','r','s','i','o','n', 0};
ROMDATA TCHAR gsIDL_ENG_NO_GENRES_AVAILABLE[] = 
{ 'N','o',' ','G','e','n','r','e','s',' ','A','v','a','l','i','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_NO_ARTISTS_AVAILABLE[] = 
{ 'N','o',' ','A','r','t','i','s','t','s',' ','A','v','a','i','l','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_NO_ALBUMS_AVAILABLE[] = 
{ 'N','o',' ','A','l','b','u','m','s',' ','A','v','a','l','i','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_NO_TRACKS_AVAILABLE[] = 
{ 'N','o',' ','T','r','a','c','k','s',' ','A','v','a','i','l','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_NO_PLAYLISTS_AVAILABLE[] = 
{ 'N','o',' ','P','l','a','y','l','i','s','t','s',' ','A','v','a','l','i','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_NO_RADIO_STATIONS_AVAILABLE[] = 
{ 'N','o',' ','R','a','d','i','o',' ','S','t','a','t','i','o','n','s',' ','A','v','a','i','l','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_MUSIC_SOURCE[] = 
{ 'M','u','s','i','c',' ','S','o','u','r','c','e', 0};
ROMDATA TCHAR gsIDL_ENG_MUSIC_SOURCES[] = 
{ 'M','u','s','i','c',' ','S','o','u','r','c','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_MUSIC_LIBRARY[] = 
{ 'M','u','s','i','c',' ','L','i','b','r','a','r','y', 0};
ROMDATA TCHAR gsIDL_ENG_MUSIC_LIBRARIES[] = 
{ 'M','u','s','i','c',' ','L','i','b','r','a','r','i','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_JUKEBOX_SETTINGS[] = 
{ 'J','u','k','e','b','o','x',' ','S','e','t','t','i','n','g','s', 0};
ROMDATA TCHAR gsIDL_ENG_SCAN_HD_FOR_MUSIC[] = 
{ 'S','c','a','n',' ','H','D',' ','f','o','r',' ','M','u','s','i','c', 0};
ROMDATA TCHAR gsIDL_ENG_SCAN_LOCAL_MUSIC[] =
{ 'S','c','a','n',' ','L','o','c','a','l',' ','M','u','s','i','c', 0};
ROMDATA TCHAR gsIDL_ENG_PUSH_SELECT_TO_RESTORE[] =
{ 'P','u','s','h',' ','S','e','l','e','c','t',' ','T','o',' ','R','e','s','t','o','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_RESTORE_PROCEDURE[] =
{ 'R','e','s','t','o','r','e',' ','P','r','o','c','e','d','u','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_PUSH_SELECT_TO_CHECK[] =
{ 'P','u','s','h',' ','S','e','l','e','c','t',' ','T','o',' ','B','e','g','i','n',' ','C','h','e','c','k',0};
ROMDATA TCHAR gsIDL_ENG_CHECK_PROCEDURE[] =
{ 'C','h','e','c','k',' ','H','D',' ','P','r','o','c','e','d','u','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_FORMAT_HARD_DISK[] =
{ 'F','o','r','m','a','t',' ','H','a','r','d',' ','D','i','s','k', 0};
ROMDATA TCHAR gsIDL_ENG_FORMATTING_HARD_DISK[] =
{ 'F','o','r','m','a','t','t','i','n','g',' ','H','a','r','d',' ','D','i','s','k', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATING_CDDB[] =
{ 'U','p','d','a','t','i','n','g',' ','G','r','a','c','e','n','o','t','e',' ','C','D','D','B', 0};
ROMDATA TCHAR gsIDL_ENG_COPY_CONTENT[] =
{ 'C','o','p','y',' ','C','o','n','t','e','n','t', 0};
ROMDATA TCHAR gsIDL_ENG_COPYING_SYSTEM_FILES[] =
{ 'C','o','p','y','i','n','g',' ','S','y','s','t','e','m',' ','F','i','l','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATE_FIRMWARE_LOCALLY[] = 
{ 'U','p','d','a','t','e',' ','F','i','r','m','w','a','r','e',' ','L','o','c','a','l','l','y', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATE_FIRMWARE[] =
{ 'U','p','d','a','t','e',' ','F','i','r','m','w','a','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATE_FIRMWARE_ONLINE[] = 
{ 'U','p','d','a','t','e',' ','F','i','r','m','w','a','r','e',' ','O','n','l','i','n','e', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATING_FIRMWARE[] =
{ 'U','p','d','a','t','i','n','g',' ','F','i','r','m','w','a','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_CREATE_NEW_PLAYLIST[] = 
{ 'C','r','e','a','t','e',' ','N','e','w',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_PLEASE_CHOOSE_CD_INFO[] = 
{ 'P','l','e','a','s','e',' ','C','h','o','o','s','e',' ','C','D',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_UPDOWN_TO_CHOOSE_SELECT_TO_SAVE[] = 
{ 'U','p','/','D','o','w','n',' ','t','o',' ','c','h','o','o','s','e',',',' ','S','e','l','e','c','t',' ','t','o',' ','s','a','v','e','.', 0};
ROMDATA TCHAR gsIDL_ENG_CD_INFO_SAVED[] = 
{ 'C','D',' ','I','n','f','o',' ','S','a','v','e','d','.', 0};
ROMDATA TCHAR gsIDL_ENG_DEFAULT_CD_INFO_SAVED[] = 
{ 'D','e','f','a','u','l','t',' ','C','D',' ','I','n','f','o',' ','S','a','v','e','d','.', 0};
ROMDATA TCHAR gsIDL_ENG_SAVE_PLAYLIST[] =
{ 'S','a','v','e',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_SAVE_CURRENT_PLAYLIST[] =
{ 'S','a','v','e',' ','C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_EMPTY_CURRENT_PLAYLIST[] =
{ 'E','m','p','t','y',' ','C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_CLEARED_CURRENT_PLAYLIST[] =
{ 'C','l','e','a','r','e','d',' ','C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_1400_KBPS_UNCOMPRESSED[] =
{ '1','4','0','0',' ','k','b','p','s',' ','u','n','c','o','m','p','r','e','s','s','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_320_KBPS_MP3[] =
{ '3','2','0',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_256_KBPS_MP3[] =
{ '2','5','6',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_224_KBPS_MP3[] =
{ '2','2','4',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_192_KBPS_MP3[] =
{ '1','9','2',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_160_KBPS_MP3[] =
{ '1','6','0',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_128_KBPS_MP3[] =
{ '1','2','8',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_112_KBPS_MP3[] =
{ '1','1','2',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_96_KBPS_MP3[] =
{ '9','6',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_80_KBPS_MP3[] =
{ '8','0',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_64_KBPS_MP3[] =
{ '6','4',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_56_KBPS_MP3[] =
{ '5','6',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_48_KBPS_MP3[] =
{ '4','8',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_40_KBPS_MP3[] =
{ '4','0',' ','k','b','p','s',' ','m','p','3', 0};
ROMDATA TCHAR gsIDL_ENG_32_KBPS_MP3[] =
{ '3','2',' ','k','b','p','s', 0};
ROMDATA TCHAR gsIDL_ENG_RECORDING_QUALITY[] =
{ 'R','e','c','o','r','d','i','n','g',' ','Q','u','a','l','i','t','y', 0};
ROMDATA TCHAR gsIDL_ENG_RECORDING_BITRATE[] =
{ 'R','e','c','o','r','d','i','n','g',' ','B','i','t','r','a','t','e', 0};
ROMDATA TCHAR gsIDL_ENG_CD_INSERTED[] =
{ 'C','D',' ','I','n','s','e','r','t','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_CD_REMOVED[] =
{ 'C','D',' ','R','e','m','o','v','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_CURRENT_CD_COLON_[] =
{ 'C','u','r','r','e','n','t',' ','C','D',' ',':',' ', 0};
ROMDATA TCHAR gsIDL_ENG_SCANNING_FOR_CD_INFO[] =
{ 'S','c','a','n','n','i','n','g',' ','f','o','r',' ','C','D',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_NO_INFO_FOUND_FOR_THIS_CD[] =
{ 'N','o',' ','i','n','f','o',' ','f','o','u','n','d',' ','f','o','r',' ','t','h','i','s',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_FOUND_FML_COLON_[] =
{ 'F','o','u','n','d',' ','F','M','L',' ',':',' ', 0};
ROMDATA TCHAR gsIDL_ENG_LOST_FML_COLON_[] =
{ 'L','o','s','t',' ','F','M','L',' ',':',' ', 0};
ROMDATA TCHAR gsIDL_ENG_PLEASE_INSERT_CD[] =
{ 'P','l','e','a','s','e',' ','I','n','s','e','r','t',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_MENU[] =
{ 'M','e','n','u', 0};
ROMDATA TCHAR gsIDL_ENG_VOLUME[] =
{ 'V','o','l','u','m','e', 0};
ROMDATA TCHAR gsIDL_ENG_LINE_INPUT_GAIN[] =
{ 'L','i','n','e',' ','I','n','p','u','t',' ','G','a','i','n', 0};
ROMDATA TCHAR gsIDL_ENG_LINEIN_MONITOR_ARTIST[] =
{ ' ', 0};
ROMDATA TCHAR gsIDL_ENG_LINEIN_MONITOR_TRACKNAME[] =
{ 'N','e','w',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_PRESS_RECORD_TWICE_TO_RE_RECORD[] =
{ 'P','r','e','s','s',' ','r','e','c','o','r','d',' ','t','w','i','c','e',' ','t','o',' ','r','e','-','r','e','c','o','r','d', 0 };
ROMDATA TCHAR gsIDL_ENG_STARTED_RECORDING_THE_CURRENT_TRACK[] =
{ 'S','t','a','r','t','e','d',' ','r','e','c','o','r','d','i','n','g',' ','t','h','e',' ','c','u','r','r','e','n','t',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_DHCP[] =
{ 'D','H','C','P', 0};
ROMDATA TCHAR gsIDL_ENG_DHCP_WITH_STATIC_BACKUP[] =
{ 'D','H','C','P',' ','w','i','t','h',' ','S','t','a','t','i','c',' ','b','a','c','k','u','p', 0};
ROMDATA TCHAR gsIDL_ENG_STATIC_ONLY[] =
{ 'S','t','a','t','i','c',' ','o','n','l','y', 0};
ROMDATA TCHAR gsIDL_ENG_EDIT_STATIC_SETTINGS[] =
{ 'E','d','i','t',' ','S','t','a','t','i','c',' ','S','e','t','t','i','n','g','s', 0};
ROMDATA TCHAR gsIDL_ENG_NETWORK_SETTINGS[] =
{ 'N','e','t','w','o','r','k',' ','S','e','t','t','i','n','g','s', 0};
ROMDATA TCHAR gsIDL_ENG_BLANK_IP_ADDRESS[] =
{ '0','.','0','.','0','.','0', 0};
ROMDATA TCHAR gsIDL_ENG_EDIT[] =
{ 'E','d','i','t', 0};
ROMDATA TCHAR gsIDL_ENG_EDIT_METADATA[] =
{ 'E','d','i','t',' ','M','e','t','a','d','a','t','a', 0};
ROMDATA TCHAR gsIDL_ENG_EDIT_ALBUM_INFO[] = 
{ 'E','d','i','t',' ','A','l','b','u','m',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_EDIT_TRACKS[] =
{ 'E','d','i','t',' ','T','r','a','c','k','s', 0};
ROMDATA TCHAR gsIDL_ENG_EDIT_IP[] =
{ 'E','d','i','t',' ','I','P',' ','a','d','d','r','e','s','s', 0};
ROMDATA TCHAR gsIDL_ENG_EDIT_GATEWAY[] =
{ 'E','d','i','t',' ','D','e','f','a','u','l','t',' ','g','a','t','e','w','a','y', 0};
ROMDATA TCHAR gsIDL_ENG_EDIT_SUBNET[] =
{ 'E','d','i','t',' ','S','u','b','n','e','t',' ','m','a','s','k', 0};
ROMDATA TCHAR gsIDL_ENG_EDIT_DNS[] =
{ 'E','d','i','t',' ','D','N','S',' ','s','e','r','v','e','r', 0};
ROMDATA TCHAR gsIDL_ENG_TRACK_INFO[] =
{ 'T','r','a','c','k',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_ALBUM_INFO[] = 
{ 'A','l','b','u','m',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_ARTIST_INFO[] = 
{ 'A','r','t','i','s','t',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_GENRE_INFO[] =
{ 'G','e','n','r','e',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_FACTORY_RESTORE[] =
{ 'F','a','c','t','o','r','y',' ','R','e','s','t','o','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_CHECK_HARD_DISK[] =
{ 'C','h','e','c','k',' ','H','a','r','d',' ','D','i','s','k', 0};
ROMDATA TCHAR gsIDL_ENG_SELECT_RESTORE_OPTIONS[] =
{ 'S','e','l','e','c','t',' ','R','e','s','t','o','r','e',' ','O','p','t','i','o','n','s', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATE_CDDB[] =
{ 'U','p','d','a','t','e',' ','G','r','a','c','e','n','o','t','e',' ','C','D','D','B', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATE_CDDB_ONLINE[] =
{ 'U','p','d','a','t','e',' ','G','r','a','c','e','n','o','t','e',' ','C','D','D','B',' ','O','n','l','i','n','e',0};
ROMDATA TCHAR gsIDL_ENG_UPDATE_CONTENT[] =
{ 'U','p','d','a','t','e',' ','C','o','n','t','e','n','t', 0};
ROMDATA TCHAR gsIDL_ENG_DELETE_LOCAL_CONTENT[] =
{ 'D','e','l','e','t','e',' ','L','o','c','a','l',' ','C','o','n','t','e','n','t', 0};
ROMDATA TCHAR gsIDL_ENG_CLEAR_CD_CACHE[] =
{ 'C','l','e','a','r',' ','C','D',' ','C','a','c','h','e', 0};
ROMDATA TCHAR gsIDL_ENG_CLEAR_LOCAL_CONTENT_DATABASE[] =
{ 'C','l','e','a','r',' ','L','o','c','a','l',' ','C','o','n','t','e','n','t',' ','D','a','t','a','b','a','s','e', 0};
ROMDATA TCHAR gsIDL_ENG_RESTORE_DEFAULT_USER_SETTINGS[] =
{ 'R','e','s','t','o','r','e',' ','D','e','f','a','u','l','t',' ','U','s','e','r',' ','S','e','t','t','i','n','g','s', 0};
ROMDATA TCHAR gsIDL_ENG_START_SYSTEM_RESTORE[] =
{ 'S','t','a','r','t',' ','S','y','s','t','e','m',' ','R','e','s','t','o','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_RESET_CDDB[] =
{ 'R','e','s','e','t',' ','G','r','a','c','e','n','o','t','e',' ','C','D','D','B', 0};
ROMDATA TCHAR gsIDL_ENG_LOADING[] = 
{ 'L','o','a','d','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_PUSH_CANCEL_TO_EXIT_WITHOUT_SAVING[] =
{ 'P','u','s','h',' ','C','a','n','c','e','l',' ','t','o',' ','E','x','i','t',' ','W','i','t','h','o','u','t',' ','S','a','v','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_PUSH_SAVE_TO_KEEP_CHANGES[] =
{ 'P','u','s','h',' ','S','a','v','e',' ','T','o',' ','K','e','e','p',' ','C','h','a','n','g','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_QUERYING_GENRES[] = 
{ 'Q','u','e','r','y','i','n','g',' ','G','e','n','r','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_QUERYING_ARTISTS[] =
{ 'Q','u','e','r','y','i','n','g',' ','A','r','t','i','s','t','s', 0};
ROMDATA TCHAR gsIDL_ENG_QUERYING_ALBUMS[] =
{ 'Q','u','e','r','y','i','n','g',' ','A','l','b','u','m','s', 0};
ROMDATA TCHAR gsIDL_ENG_QUERYING_TRACKS[] = 
{ 'Q','u','e','r','y','i','n','g',' ','T','r','a','c','k','s', 0};
ROMDATA TCHAR gsIDL_ENG_QUERYING_RADIO_STATIONS[] = 
{ 'Q','u','e','r','y','i','n','g',' ','R','a','d','i','o',' ','S','t','a','t','i','o','n','s', 0};
ROMDATA TCHAR gsIDL_ENG_QUERYING_PLAYLISTS[] = 
{ 'Q','u','e','r','y','i','n','g',' ','P','l','a','y','l','i','s','t','s', 0};
ROMDATA TCHAR gsIDL_ENG_HALTING_CURRENT_QUERY[] = 
{ 'H','a','l','t','i','n','g',' ','C','u','r','r','e','n','t',' ','Q','u','e','r','y', 0};
ROMDATA TCHAR gsIDL_ENG_STATION_NOT_FOUND[] = 
{ 'S','t','a','t','i','o','n',' ','N','o','t',' ','F','o','u','n','d', 0};
ROMDATA TCHAR gsIDL_ENG_CURRENT_PLAYLIST[] = 
{ 'C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_COLON_SPACE[] = 
{ ':',' ', 0};
ROMDATA TCHAR gsIDL_ENG_ADD[] = 
{ 'A','d','d', 0};
ROMDATA TCHAR gsIDL_ENG_DELETE[] = 
{ 'D','e','l','e','t','e', 0};
ROMDATA TCHAR gsIDL_ENG_SEARCHING[] = 
{ 'S','e','a','r','c','h','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_ADDING_GENRE[] = 
{ 'A','d','d','i','n','g',' ','G','e','n','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_ADDING_ARTIST[] = 
{ 'A','d','d','i','n','g',' ','A','r','t','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_ADDING_ALBUM[] = 
{ 'A','d','d','i','n','g',' ','A','l','b','u','m', 0};
ROMDATA TCHAR gsIDL_ENG_ADDING_TRACK[] = 
{ 'A','d','d','i','n','g',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_ADDING_PLAYLIST[] = 
{ 'A','d','d','i','n','g',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_ADDING_STATION[] = 
{ 'A','d','d','i','n','g',' ','S','t','a','t','i','o','n', 0};
ROMDATA TCHAR gsIDL_ENG_LOOKING_FOR_GENRES[] = 
{ 'L','o','o','k','i','n','g',' ','f','o','r',' ','G','e','n','r','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_LOOKING_FOR_ARTISTS[] = 
{ 'L','o','o','k','i','n','g',' ','f','o','r',' ','A','r','t','i','s','t','s', 0};
ROMDATA TCHAR gsIDL_ENG_LOOKING_FOR_ALBUMS[] = 
{ 'L','o','o','k','i','n','g',' ','f','o','r',' ','A','l','b','u','m','s', 0};
ROMDATA TCHAR gsIDL_ENG_LOOKING_FOR_TRACKS[] = 
{ 'L','o','o','k','i','n','g',' ','f','o','r',' ','T','r','a','c','k','s', 0};
ROMDATA TCHAR gsIDL_ENG_LOOKING_FOR_PLAYLISTS[] = 
{ 'L','o','o','k','i','n','g',' ','f','o','r',' ','P','l','a','y','l','i','s','t','s', 0};
ROMDATA TCHAR gsIDL_ENG_LOOKING_FOR_STATIONS[] = 
{ 'L','o','o','k','i','n','g',' ','f','o','r',' ','S','t','a','t','i','o','n','s', 0};
ROMDATA TCHAR gsIDL_ENG_COPYING_CDDB_FILE[] = 
{ 'C','o','p','y','i','n','g',' ','C','D','D','B',' ','F','i','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_DOWNLOADING_CDDB_FILE[] = 
{ 'D','o','w','n','l','o','a','d','i','n','g',' ','C','D','D','B',' ','F','i','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_PROCESSING_CDDB_FILE[] = 
{ 'P','r','o','c','e','s','s','i','n','g',' ','C','D','D','B',' ','F','i','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_STOPPING_PLAYBACK[] = 
{ 'S','t','o','p','p','i','n','g',' ','p','l','a','y','b','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_STOPPING_FAST_RECORDING[] = 
{ 'S','t','o','p','p','i','n','g',' ','f','a','s','t',' ','r','e','c','o','r','d', 0};
ROMDATA TCHAR gsIDL_ENG_LOOKING_FOR_HD_TRACKS[] = 
{ 'L','o','o','k','i','n','g',' ','f','o','r',' ','H','D',' ','T','r','a','c','k','s', 0};
ROMDATA TCHAR gsIDL_ENG_SCANNING_FOR_TRACK_INFO[] = 
{ 'S','c','a','n','n','i','n','g',' ','f','o','r',' ','T','r','a','c','k',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_FINISHED_SCANNING[] = 
{ 'F','i','n','i','s','h','e','d',' ','S','c','a','n','n','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_COMPLETED[] = 
{ 'C','o','m','p','l','e','t','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_OF[] = 
{ 'o','f', 0};
ROMDATA TCHAR gsIDL_ENG_NO_MUSIC_FOUND_ON_HD[] = 
{ 'N','o',' ','M','u','s','i','c',' ','F','o','u','n','d',' ','o','n',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_RETURNING_TO_MENU[] = 
{ 'R','e','t','u','r','n','i','n','g',' ','t','o',' ','M','e','n','u', 0};
ROMDATA TCHAR gsIDL_ENG_ERROR[] = 
{ 'E','r','r','o','r', 0};
ROMDATA TCHAR gsIDL_ENG_SAVING_CHANGES[] =
{ 'S','a','v','i','n','g',' ','C','h','a','n','g','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_SAVING_PLAYLIST[] =
{ 'S','a','v','i','n','g',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_SAVE[] =
{ 'S','a','v','e', 0};
ROMDATA TCHAR gsIDL_ENG_ITEM_IS_NOT_EDITABLE[] =
{ 'I','t','e','m',' ','i','s',' ','n','o','t',' ','e','d','i','t','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_PLEASE_WAIT[] =
{ 'P','l','e','a','s','e',' ','W','a','i','t', 0};
ROMDATA TCHAR gsIDL_ENG_PLEASE_TRY_AGAIN[] =
{ 'P','l','e','a','s','e',' ','t','r','y',' ','a','g','a','i','n', 0};
ROMDATA TCHAR gsIDL_ENG_EJECTING_CD[] =
{ 'E','j','e','c','t','i','n','g',' ','C','D',' ','T','r','a','y', 0};
ROMDATA TCHAR gsIDL_ENG_FML_NO_LONGER_AVAILABLE[] =
{ 'F','M','L',' ','n','o',' ','l','o','n','g','e','r',' ','a','v','a','i','l','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_CD_NO_LONGER_AVAILABLE[] =
{ 'C','D',' ','n','o',' ','l','o','n','g','e','r',' ','a','v','a','i','l','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_MUSIC_SOURCE_HAS_CHANGED[] =
{ 'M','u','s','i','c',' ','S','o','u','r','c','e',' ','h','a','s',' ','c','h','a','n','g','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_DELETE_THIS_ITEM[] =
{ 'C','a','n','\'','t',' ','d','e','l','e','t','e',' ','t','h','i','s',' ','i','t','e','m', 0};
ROMDATA TCHAR gsIDL_ENG_DELETING[] =
{ 'D','e','l','e','t','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_BAD_PLAYLIST_NAME[] =
{ 'B','a','d',' ','P','l','a','y','l','i','s','t',' ','n','a','m','e', 0};
ROMDATA TCHAR gsIDL_ENG_NOT_AVAILABLE[] =
{ 'N','o','t',' ','a','v','a','i','l','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_NOT_AVAILABLE_WHILE_USING_LINE_INPUT[] =
{ 'N','o','t',' ','a','v','a','i','l','a','b','l','e',' ','w','h','i','l','e',' ','u','s','i','n','g',' ','L','i','n','e',' ','I','n','p','u','t', 0};
ROMDATA TCHAR gsIDL_ENG_NOT_AVAILABLE_WHILE_USING_HARD_DISK[] =
{ 'N','o','t',' ','a','v','a','i','l','a','b','l','e',' ','w','h','i','l','e',' ','u','s','i','n','g',' ','H','a','r','d',' ','D','i','s','k', 0};
ROMDATA TCHAR gsIDL_ENG_NOT_AVAILABLE_WHILE_USING_CD_DRIVE[] =
{ 'N','o','t',' ','a','v','a','i','l','a','b','l','e',' ','w','h','i','l','e',' ','u','s','i','n','g',' ','C','D',' ','D','r','i','v','e', 0};
ROMDATA TCHAR gsIDL_ENG_INTERNET_RADIO_NOT_AVAILABLE[] =
{ 'I','n','t','e','r','n','e','t',' ','R','a','d','i','o',' ','n','o','t',' ','a','v','a','i','l','a','b','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_STARTING_NETWORK_SERVICES[] =
{ 'S','t','a','r','t','i','n','g',' ','N','e','t','w','o','r','k',' ','S','e','r','v','i','c','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_SAVING_NETWORK_SETTINGS[] =
{ 'S','a','v','i','n','g',' ','N','e','t','w','o','r','k',' ','S','e','t','t','i','n','g','s', 0};
ROMDATA TCHAR gsIDL_ENG_NETWORK_SETTINGS_FAILED[] =
{ 'N','e','t','w','o','r','k',' ','S','e','t','t','i','n','g','s',' ','f','a','i','l','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_CREATING_PLAYLIST[] =
{ 'C','r','e','a','t','i','n','g',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_UNABLE_TO_PLAY_TRACK[] =
{ 'U','n','a','b','l','e',' ','t','o',' ','p','l','a','y',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_CHECKING_FOR_UPDATES[] =
{ 'C','h','e','c','k','i','n','g',' ','f','o','r',' ','U','p','d','a','t','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_DOWNLOADING_UPDATES[] =
{ 'D','o','w','n','l','o','a','d','i','n','g',' ','U','p','d','a','t','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_FAILED_TO_DOWNLOAD_UPDATE_FILES[] =
{ 'F','a','i','l','e','d',' ','t','o',' ','D','o','w','n','l','o','a','d',' ','U','p','d','a','t','e',' ','F','i','l','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_CDDB_UPDATE_FAILED[] =
{ 'C','D','D','B',' ','U','p','d','a','t','e',' ','f','a','i','l','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_NO_CDDB_UPDATES_FOUND[] =
{ 'N','o',' ','C','D','D','B',' ','U','p','d','a','t','e','s',' ','f','o','u','n','d', 0};
ROMDATA TCHAR gsIDL_ENG_NO_UPDATES_FOUND[] =
{ 'N','o',' ','U','p','d','a','t','e','s',' ','F','o','u','n','d', 0};
ROMDATA TCHAR gsIDL_ENG_NO_NEW_UPDATES_TRY_AGAIN_LATER[] =
{ 'N','o',' ','N','e','w',' ','U','p','d','a','t','e','s','.',' ',' ','T','r','y',' ','A','g','a','i','n',' ','L','a','t','e','r','.', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATE_SERVER_NOT_FOUND[] =
{ 'U','p','d','a','t','e',' ','S','e','r','v','e','r',' ','N','o','t',' ','F','o','u','n','d', 0};
ROMDATA TCHAR gsIDL_ENG_CURRENT_UPDATE_LEVEL[] =
{ 'C','u','r','r','e','n','t',' ','U','p','d','a','t','e',' ','L','e','v','e','l',':',' ', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATING[] =
{ 'U','p','d','a','t','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_FIRMWARE_UPDATE_FAILED[] =
{ 'F','i','r','m','w','a','r','e',' ','U','p','d','a','t','e',' ','f','a','i','l','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_CORRUPT_FIRMWARE[] =
{ 'C','o','r','r','u','p','t',' ','F','i','r','m','w','a','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_RESETTING[] =
{ 'R','e','s','e','t','t','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_DELETING_GENRE[] =
{ 'D','e','l','e','t','i','n','g',' ','G','e','n','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_DELETING_ARTIST[] =
{ 'D','e','l','e','t','i','n','g',' ','A','r','t','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_DELETING_ALBUM[] =
{ 'D','e','l','e','t','i','n','g',' ','A','l','b','u','m', 0};
ROMDATA TCHAR gsIDL_ENG_DELETING_TRACK[] =
{ 'D','e','l','e','t','i','n','g',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_DELETING_PLAYLIST[] =
{ 'D','e','l','e','t','i','n','g',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_DELETING_STATION[] =
{ 'D','e','l','e','t','i','n','g',' ','S','t','a','t','i','o','n', 0};
ROMDATA TCHAR gsIDL_ENG_LOADING_GENRE[] =
{ 'L','o','a','d','i','n','g',' ','G','e','n','r','e', 0};
ROMDATA TCHAR gsIDL_ENG_LOADING_ARTIST[] =
{ 'L','o','a','d','i','n','g',' ','A','r','t','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_LOADING_ALBUM[] =
{ 'L','o','a','d','i','n','g',' ','A','l','b','u','m', 0};
ROMDATA TCHAR gsIDL_ENG_LOADING_TRACK[] =
{ 'L','o','a','d','i','n','g',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_LOADING_PLAYLIST[] =
{ 'L','o','a','d','i','n','g',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_LOADING_STATION[] =
{ 'L','o','a','d','i','n','g',' ','S','t','a','t','i','o','n', 0};
ROMDATA TCHAR gsIDL_ENG_NO_SPACE_LEFT_ON_HD_FOR_RECORDING[] =
{ 'N','o',' ','s','p','a','c','e',' ','l','e','f','t',' ','o','n',' ','H','D',' ','f','o','r',' ','r','e','c','o','r','d','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_MAXIMUM_NUMBER_OF_TRACKS_RECORDED[] =
{ 'M','a','x','i','m','u','m',' ','n','u','m','b','e','r',' ','o','f',' ','t','r','a','c','k','s',' ','r','e','c','o','r','d','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_DELETE_TRACKS_TO_ENABLE_RECORDING[] =
{ 'D','e','l','e','t','e',' ','t','r','a','c','k','s',' ','t','o',' ','e','n','a','b','l','e',' ','r','e','c','o','r','d','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_TRACK_ALREADY_RECORDED[] =
{ 'T','r','a','c','k',' ','i','s',' ','a','l','r','e','a','d','y',' ','o','n',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_UNABLE_TO_RECORD_TRACK[] =
{ 'U','n','a','b','l','e',' ','t','o',' ','r','e','c','o','r','d',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_RECORD_INTERNET_RADIO[] =
{ 'C','a','n','\'','t',' ','r','e','c','o','r','d',' ','I','n','t','e','r','n','e','t',' ','R','a','d','i','o', 0};
ROMDATA TCHAR gsIDL_ENG_RECORD[] =
{ 'R','e','c','o','r','d', 0};
ROMDATA TCHAR gsIDL_ENG_TRYING_NEXT_TRACK[] =
{ 'T','r','y','i','n','g',' ','n','e','x','t',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_PRESS_SAVE_TO_SAVE_CURRENT_PLAYLIST_TO_HD[] = 
{ 'P','r','e','s','s',' ','S','a','v','e',' ','t','o',' ','S','a','v','e',' ','C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t',' ','t','o',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_SCANNING[] =
{ 'S','c','a','n','n','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_SCANNED[] =
{ 'S','c','a','n','n','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_MEDIA_LIBRARY[] = 
{ 'M','e','d','i','a',' ','L','i','b','r','a','r','y', 0};
ROMDATA TCHAR gsIDL_ENG_MEDIA_LIBRARIES[] =
{ 'M','e','d','i','a',' ','L','i','b','r','a','r','i','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_NETWORK_DISCONNECTED[] = 
{ 'N','e','t','w','o','r','k',' ','D','i','s','c','o','n','n','e','c','t','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_NO_FULLPLAY_MEDIA_LIBRARIES[] = 
{ 'N','o',' ','F','u','l','l','p','l','a','y',' ','M','e','d','i','a',' ','L','i','b','r','a','r','i','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_PREPARING_TO_SAVE_CHANGES[] = 
{ 'P','r','e','p','a','r','i','n','g',' ','t','o',' ','S','a','v','e',' ','C','h','a','n','g','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_COUNTING_FILES[] = 
{ 'C','o','u','n','t','i','n','g',' ','F','i','l','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_SAVING_CHANGES_TO[] = 
{ 'S','a','v','i','n','g',' ','C','h','a','n','g','e','s',' ','t','o', 0};
ROMDATA TCHAR gsIDL_ENG_FILE[] = 
{ 'F','i','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_FILES[] = 
{ 'F','i','l','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_DATABASE[] = 
{ 'D','a','t','a','b','a','s','e', 0};
ROMDATA TCHAR gsIDL_ENG_WROTE[] = 
{ 'W','r','o','t','e', 0};
ROMDATA TCHAR gsIDL_ENG_PARTIAL_TRACK_RECORDING_CANCELLED[] = 
{ 'P','a','r','t','i','a','l',' ','T','r','a','c','k',' ','r','e','c','o','r','d','i','n','g',' ','c','a','n','c','e','l','l','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_RECORD_AFTER_SEEKING_ON_A_TRACK[] = 
{ 'C','a','n','\'','t',' ','r','e','c','o','r','d',' ','a','f','t','e','r',' ','s','e','e','k','i','n','g',' ','o','n',' ','a',' ','t','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_RESTART_TRACK_TO_ENABLE_RECORDING[] = 
{ 'R','e','s','t','a','r','t',' ','t','r','a','c','k',' ','t','o',' ','e','n','a','b','l','e',' ','r','e','c','o','r','d','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_SEEK_WHILE_RECORDING[] = 
{ 'C','a','n','\'','t',' ','s','e','e','k',' ','w','h','i','l','e',' ','r','e','c','o','r','d','i','n','g',' ','e','n','a','b','l','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_SEEK_ON_NETWORK_TRACK[] = 
{ 'C','a','n','\'','t',' ','s','e','e','k',' ','o','n',' ','n','e','t','w','o','r','k',' ','t','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_EJECT_TRAY_WHILE_RECORDING_LINE_INPUT[] = 
{ 'C','a','n','\'','t',' ','e','j','e','c','t',' ','t','r','a','y',' ','w','h','i','l','e',' ','r','e','c','o','r','d','i','n','g',' ','L','i','n','e',' ','I','n','p','u','t', 0};
ROMDATA TCHAR gsIDL_ENG_NEW_FIRMWARE_VERSION_FOUND_ON_CD[] = 
{ 'N','e','w',' ','f','i','r','m','w','a','r','e',' ','v','e','r','s','i','o','n',' ','f','o','u','n','d',' ','o','n',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_NEW_CDDB_VERSION_FOUND_ON_CD[] = 
{ 'N','e','w',' ','C','D','D','B',' ','v','e','r','s','i','o','n',' ','f','o','u','n','d',' ','o','n',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_ACCESSING_CDDB[] = 
{ 'A','c','c','e','s','s','i','n','g',' ','G','r','a','c','e','n','o','t','e',' ','C','D','D','B', 0};
ROMDATA TCHAR gsIDL_ENG_ACCESSING_CDDB_ONLINE[] = 
{ 'A','c','c','e','s','s','i','n','g',' ','G','r','a','c','e','n','o','t','e',' ','C','D','D','B',' ','o','n','l','i','n','e', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_ACCESS_CDDB_ONLINE[] = 
{ 'C','a','n','\'','t',' ','A','c','c','e','s','s',' ','C','D','D','B',' ','o','n','l','i','n','e', 0};
ROMDATA TCHAR gsIDL_ENG_STOPPED_RECORDING_TRACK_TO_HD[] = 
{ 'S','t','o','p','p','e','d',' ','r','e','c','o','r','d','i','n','g',' ','T','r','a','c','k',' ','t','o',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_STOPPED_RECORDING_CD_TO_HD[] = 
{ 'S','t','o','p','p','e','d',' ','r','e','c','o','r','d','i','n','g',' ','C','D',' ','t','o',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_STOPPED_RECORDING_CURRENT_PLAYLIST_TO_HD[] = 
{ 'S','t','o','p','p','e','d',' ','r','e','c','o','r','d','i','n','g',' ','C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t',' ','t','o',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_READ_CD[] = 
{ 'C','a','n','\'','t',' ','R','e','a','d',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_FINISHED_RECORDING_TRACK_TO_HD[] = 
{ 'F','i','n','i','s','h','e','d',' ','R','e','c','o','r','d','i','n','g',' ','T','r','a','c','k',' ','t','o',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_FINISHED_RECORDING_CD_TO_HD[] = 
{ 'F','i','n','i','s','h','e','d',' ','R','e','c','o','r','d','i','n','g',' ','C','D',' ','t','o',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_FINISHED_RECORDING_CURRENT_PLAYLIST_TO_HD[] = 
{ 'F','i','n','i','s','h','e','d',' ','R','e','c','o','r','d','i','n','g',' ','C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t',' ','t','o',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_FINISHED_RECORDING_QUERIED_TRACKS_TO_HD[] = 
{ 'F','i','n','i','s','h','e','d',' ','R','e','c','o','r','d','i','n','g',' ','Q','u','e','r','i','e','d',' ','T','r','a','c','k','s',' ','t','o',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_NO_TRACKS_TO_RECORD_IN_CURRENT_PLAYLIST[] =
{ 'N','o',' ','T','r','a','c','k','s',' ','t','o',' ','R','e','c','o','r','d',' ','i','n',' ','C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST[] = 
{ 'N','o',' ','V','a','l','i','d',' ','T','r','a','c','k','s',' ','i','n',' ','C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_COPY_PLAYLIST_TO_HD[] = 
{ 'C','o','p','y',' ','P','l','a','y','l','i','s','t',' ','t','o',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_COPYING_PLAYLIST_TO_HD[] = 
{ 'C','o','p','y','i','n','g',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_PREPARING_TO_COPY[] = 
{ 'P','r','e','p','a','r','i','n','g',' ','t','o',' ','C','o','p','y', 0};
ROMDATA TCHAR gsIDL_ENG_FAILED_TO_COPY_LAST_TRACK[] = 
{ 'F','a','i','l','e','d',' ','t','o',' ','C','o','p','y',' ','L','a','s','t',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_CANCELLING[] = 
{ 'C','a','n','c','e','l','l','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_COMMITTING_DB[] = 
{ 'S','a','v','i','n','g',' ','C','h','a','n','g','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_ABOUT_ELLIPSIS[] = 
{ 'A','b','o','u','t','.','.','.', 0};
ROMDATA TCHAR gsIDL_ENG_FREE_HD_SPACE[] = 
{ 'F','r','e','e',' ','H','D',' ','S','p','a','c','e', 0};
ROMDATA TCHAR gsIDL_ENG_HD_INFO[] = 
{ 'H','D',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_SAVE_CURRENT_PLAYLIST_AS_NEW[] = 
{ 'S','a','v','e',' ','C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t',' ','a','s',' ','N','e','w', 0};
ROMDATA TCHAR gsIDL_ENG_EJECT_CD_TRAY_AFTER_FAST_RECORD[] = 
{ 'E','j','e','c','t',' ','C','D',' ','T','r','a','y',' ','a','f','t','e','r',' ','F','a','s','t',' ','R','e','c','o','r','d', 0};
ROMDATA TCHAR gsIDL_ENG_ENABLE_EXTENDED_CHAR_EDITING[] = 
{ 'E','n','a','b','l','e',' ','E','x','t','e','n','d','e','d',' ','C','h','a','r',' ','E','d','i','t','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_ENABLE_WEB_CONTROL[] = 
{ 'E','n','a','b','l','e',' ','W','e','b',' ','C','o','n','t','r','o','l', 0};
ROMDATA TCHAR gsIDL_ENG_SHOW_TRACK_NUMBER_IN_TITLE[] = 
{ 'S','h','o','w',' ','T','r','a','c','k',' ','N','u','m','b','e','r',' ','i','n',' ','T','i','t','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_SHOW_ALBUM_WITH_ARTIST_NAME[] = 
{ 'S','h','o','w',' ','A','l','b','u','m',' ','w','i','t','h',' ','A','r','t','i','s','t',' ','N','a','m','e', 0};
ROMDATA TCHAR gsIDL_ENG_PLAY_CD_WHEN_INSERTED[] = 
{ 'P','l','a','y',' ','C','D',' ','w','h','e','n',' ','I','n','s','e','r','t','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_SET_TEXT_SCROLL_SPEED[] = 
{ 'S','e','t',' ','T','e','x','t',' ','S','c','r','o','l','l',' ','S','p','e','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_ENABLE_RECORD_LED[] = 
{ 'E','n','a','b','l','e',' ','R','e','c','o','r','d',' ','L','E','D', 0};
ROMDATA TCHAR gsIDL_ENG_SET_LCD_BRIGHTNESS[] = 
{ 'S','e','t',' ','L','C','D',' ','B','r','i','g','h','t','n','e','s','s', 0};
ROMDATA TCHAR gsIDL_ENG_CUSTOMIZATIONS[] = 
{ 'C','u','s','t','o','m','i','z','a','t','i','o','n','s', 0};
ROMDATA TCHAR gsIDL_ENG_BRIGHT[] = 
{ 'B','r','i','g','h','t', 0};
ROMDATA TCHAR gsIDL_ENG_DIM[] = 
{ 'D','i','m', 0};
ROMDATA TCHAR gsIDL_ENG_FAST[] = 
{ 'F','a','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_SLOW[] = 
{ 'S','l','o','w', 0};
ROMDATA TCHAR gsIDL_ENG_OFF[] = 
{ 'O','f','f', 0};
ROMDATA TCHAR gsIDL_ENG_RESTORE_SUCCEEDED[] = 
{ 'R','e','s','t','o','r','e',' ','S','u','c','c','e','e','d','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_RESTORE_FAILED[] = 
{ 'R','e','s','t','o','r','e',' ','F','a','i','l','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_RESTORE_MENU[] = 
{ 'R','e','s','t','o','r','e',' ','M','e','n','u', 0};
ROMDATA TCHAR gsIDL_ENG_ERASING_MUSIC_CONTINUE_QM[] = 
{ 'E','r','a','s','i','n','g',' ','M','u','s','i','c','.','.','.','C','o','n','t','i','n','u','e','?', 0};
ROMDATA TCHAR gsIDL_ENG_NO_RESTORE_CD_FOUND[] = 
{ 'N','o',' ','R','e','s','t','o','r','e',' ','C','D',' ','F','o','u','n','d', 0};
ROMDATA TCHAR gsIDL_ENG_CHECKING_FOR_RESTORE_CD[] = 
{ 'C','h','e','c','k','i','n','g',' ','f','o','r',' ','R','e','s','t','o','r','e',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_PLEASE_INSERT_RESTORE_CD[] = 
{ 'P','l','e','a','s','e',' ','I','n','s','e','r','t',' ','R','e','s','t','o','r','e',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATING_SYSTEM_FILES[] =
{ 'U','p','d','a','t','i','n','g',' ','S','y','s','t','e','m',' ','F','i','l','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_RETRYING[] = 
{ 'R','e','t','r','y','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_CALL_TECH_SUPPORT[] = 
{ 'C','a','l','l',' ','T','e','c','h',' ','S','u','p','p','o','r','t', 0};
ROMDATA TCHAR gsIDL_ENG_NO_UPDATES_PERFORMED[] = 
{ 'N','o',' ','U','p','d','a','t','e','s',' ','P','e','r','f','o','r','m','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_PLAYLIST_TRACK_LIMIT_REACHED[] =
{ 'T','r','a','c','k',' ','L','i','m','i','t',' ','R','e','a','c','h','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_ADDING[] = 
{ 'A','d','d','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_NO_CD[] = 
{ 'N','o',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_BROWSE_AUDIO_CD[] =
{ 'C','a','n','\'','t',' ','b','r','o','w','s','e',' ','A','u','d','i','o',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_BROWSE_WHILE_SCANNING_DATA_CD[] = 
{ 'C','a','n','\'','t',' ','b','r','o','w','s','e',' ','w','h','i','l','e',' ','s','c','a','n','n','i','n','g',' ','D','a','t','a',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_PLEASE_SELECT_TRACKS[] = 
{ 'P','l','e','a','s','e',' ','S','e','l','e','c','t',' ','T','r','a','c','k','s', 0};
ROMDATA TCHAR gsIDL_ENG_PRESS_EXIT_TO_CANCEL[] = 
{ 'P','r','e','s','s',' ','E','x','i','t',' ','t','o',' ','C','a','n','c','e','l', 0};
ROMDATA TCHAR gsIDL_ENG_SCANNING_INFO_FOR[] =
{ 'S','c','a','n','n','i','n','g',' ','I','n','f','o',' ','f','o','r', 0};
ROMDATA TCHAR gsIDL_ENG_CD_TRACKS[] =
{ 'C','D',' ','T','r','a','c','k','s', 0};
ROMDATA TCHAR gsIDL_ENG_HD_TRACKS[] =
{ 'H','D',' ','T','r','a','c','k','s', 0};
ROMDATA TCHAR gsIDL_ENG_ERROR_QUERYING_MEDIA_LIBRARY_COLON_[] =
{ 'E','r','r','o','r',' ','Q','u','e','r','y','i','n','g',' ','M','e','d','i','a',' ','L','i','b','r','a','r','y',':',' ', 0};
ROMDATA TCHAR gsIDL_ENG_RECORDING_WITH[] =
{ 'R','e','c','o','r','d','i','n','g',' ','w','i','t','h', 0};
ROMDATA TCHAR gsIDL_ENG_MINUTES_FREE[] =
{ 'm','i','n','u','t','e','s',' ','f','r','e','e', 0};
ROMDATA TCHAR gsIDL_ENG_RECORDING_TRACK[] =
{ 'R','e','c','o','r','d','i','n','g',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_TO_HD[] =
{ 't','o',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_LINE_INPUT_GAIN_NOW[] =
{ 'L','i','n','e',' ','I','n','p','u','t',' ','G','a','i','n',' ','N','o','w', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_CHANGE_PLAY_MODE_WHILE_FAST_RECORDING[] =
{ 'C','a','n','\'','t',' ','c','h','a','n','g','e',' ','P','l','a','y',' ','M','o','d','e',' ','w','h','i','l','e',' ','f','a','s','t',' ','r','e','c','o','r','d','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_CHANGE_PLAY_MODE_OF_LINE_INPUT_SOURCE[] =
{ 'C','a','n','\'','t',' ','c','h','a','n','g','e',' ','P','l','a','y',' ','M','o','d','e',' ','o','f',' ','L','i','n','e',' ','I','n','p','u','t',' ','S','o','u','r','c','e', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_SAVE_PLAYLIST_OF_UNRECORDED_CD_TRACKS[] =
{ 'C','a','n','\'','t',' ','S','a','v','e',' ','P','l','a','y','l','i','s','t',' ','o','f',' ','U','n','r','e','c','o','r','d','e','d',' ','C','D',' ','T','r','a','c','k','s', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_REMOVE_AUDIO_CD_TRACKS[] =
{ 'C','a','n','\'','t',' ','R','e','m','o','v','e',' ','A','u','d','i','o',' ','C','D',' ','T','r','a','c','k','s', 0};
ROMDATA TCHAR gsIDL_ENG_MUSIC_SOURCE_COLON_[] =
{ 'M','u','s','i','c',' ','S','o','u','r','c','e',':',' ', 0};
ROMDATA TCHAR gsIDL_ENG_FOUND_NEW_CD[] =
{ 'F','o','u','n','d',' ','n','e','w',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_ALREADY_HAVE_TRACK[] =
{ 'A','l','r','e','a','d','y',' ','h','a','v','e',' ','T','r','a','c','k', 0};
ROMDATA TCHAR gsIDL_ENG_ON_HD[] =
{ 'o','n',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_SEEKING_FORWARD[] =
{ 'S','e','e','k','i','n','g',' ','f','o','r','w','a','r','d', 0};
ROMDATA TCHAR gsIDL_ENG_SEEKING_BACKWARD[] =
{ 'S','e','e','k','i','n','g',' ','b','a','c','k','w','a','r','d', 0};
ROMDATA TCHAR gsIDL_ENG_SECONDS[] =
{ 'S','e','c','o','n','d','s', 0};
ROMDATA TCHAR gsIDL_ENG_CANT_LOAD_PLAYLIST[] =
{ 'C','a','n','\'','t',' ','l','o','a','d',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_CONSIDER_DELETING_IT[] =
{ 'c','o','n','s','i','d','e','r',' ','d','e','l','e','t','i','n','g',' ','i','t', 0};
ROMDATA TCHAR gsIDL_ENG_SAVED_CURRENT_PLAYLIST_AS[] =
{ 'S','a','v','e','d',' ','C','u','r','r','e','n','t',' ','P','l','a','y','l','i','s','t',' ','a','s', 0};
ROMDATA TCHAR gsIDL_ENG_CHECK_PHASE[] =
{ 'C','h','e','c','k',' ','P','h','a','s','e', 0};
ROMDATA TCHAR gsIDL_ENG_CHECKING_DRIVE_PASS[] =
{ 'C','h','e','c','k','i','n','g',' ','D','r','i','v','e',' ','-',' ','P','a','s','s', 0};
ROMDATA TCHAR gsIDL_ENG_READING_FATS[] =
{ 'R','e','a','d','i','n','g',' ','F','A','T','s', 0};
ROMDATA TCHAR gsIDL_ENG_CHECKING_CLUSTER_CHAINS[] =
{ 'C','h','e','c','k','i','n','g',' ','C','l','u','s','t','e','r',' ','C','h','a','i','n','s', 0};
ROMDATA TCHAR gsIDL_ENG_CHECKING_DIRECTORIES[] =
{ 'C','h','e','c','k','i','n','g',' ','D','i','r','e','c','t','o','r','i','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_CHECK_FOR_LOST_FILES[] =
{ 'C','h','e','c','k','i','n','g',' ','f','o','r',' ','L','o','s','t',' ','F','i','l','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_UPDATING_FATS[] =
{ 'U','p','d','a','t','i','n','g',' ','F','A','T','s', 0};
ROMDATA TCHAR gsIDL_ENG_RESTARTING_PLEASE_WAIT[] =
{ 'R','e','s','t','a','r','t','i','n','g',' ','-',' ','P','l','e','a','s','e',' ','W','a','i','t', 0};
ROMDATA TCHAR gsIDL_ENG_TRACKS_FOUND_COLON[] =
{ 'T','r','a','c','k','s',' ','F','o','u','n','d',':', 0};
ROMDATA TCHAR gsIDL_ENG_ERROR_COPYING_CDDB[] =
{ 'E','r','r','o','r',' ','c','o','p','y','i','n','g',' ','C','D','D','B', 0};
ROMDATA TCHAR gsIDL_ENG_DELETE_THIS_GENRE_QM[] =
{ 'D','e','l','e','t','e',' ','t','h','i','s',' ','G','e','n','r','e','?', 0};
ROMDATA TCHAR gsIDL_ENG_DELETE_THIS_ARTIST_QM[] =
{ 'D','e','l','e','t','e',' ','t','h','i','s',' ','A','r','t','i','s','t','?', 0};
ROMDATA TCHAR gsIDL_ENG_DELETE_THIS_ALBUM_QM[] =
{ 'D','e','l','e','t','e',' ','t','h','i','s',' ','A','l','b','u','m','?', 0};
ROMDATA TCHAR gsIDL_ENG_DELETE_THIS_TRACK_QM[] =
{ 'D','e','l','e','t','e',' ','t','h','i','s',' ','T','r','a','c','k','?', 0};
ROMDATA TCHAR gsIDL_ENG_DELETE_THIS_PLAYLIST_QM[] =
{ 'D','e','l','e','t','e',' ','t','h','i','s',' ','P','l','a','y','l','i','s','t','?', 0};
ROMDATA TCHAR gsIDL_ENG_DELETE_THIS_RADIO_STATION_QM[] =
{ 'D','e','l','e','t','e',' ','t','h','i','s',' ','R','a','d','i','o',' ','S','t','a','t','i','o','n','?', 0};
ROMDATA TCHAR gsIDL_ENG_FORMAT_HARD_DISK_QM[] =
{ 'F','o','r','m','a','t',' ','H','a','r','d',' ','D','i','s','k','?', 0};
ROMDATA TCHAR gsIDL_ENG_DRIVE_CHECK_COMPLETE[] =
{ 'D','r','i','v','e',' ','C','h','e','c','k',' ','C','o','m','p','l','e','t','e', 0};
ROMDATA TCHAR gsIDL_ENG_INSTALL_UPDATE_QM[] =
{ 'I','n','s','t','a','l','l',' ','U','p','d','a','t','e','?', 0};
ROMDATA TCHAR gsIDL_ENG_LOOKING_FOR_CD[] = 
{ 'L','o','o','k','i','n','g',' ','f','o','r',' ','C','D', 0};
ROMDATA TCHAR gsIDL_ENG_RESTORING_CDDB[] = 
{ 'R','e','s','t','o','r','i','n','g',' ','C','D','D','B', 0};
ROMDATA TCHAR gsIDL_ENG_FIXING_CORRUPT_CDDB_FILE[] = 
{ 'F','i','x','i','n','g',' ','c','o','r','r','u','p','t',' ','C','D','D','B',' ','f','i','l','e', 0};
ROMDATA TCHAR gsIDL_ENG_CHANGING[] = 
{ 'C','h','a','n','g','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_CHANGED[] = 
{ 'C','h','a','n','g','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_SAVING_INFO_TO[] = 
{ 'S','a','v','i','n','g',' ','I','n','f','o',' ','T','o', 0};
ROMDATA TCHAR gsIDL_ENG_SPACE_REMAINING[] = 
{ 'S','p','a','c','e',' ','R','e','m','a','i','n','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_TIME_REMAINING[] = 
{ 'T','i','m','e',' ','R','e','m','a','i','n','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_KBPS[] = 
{ 'k','b','p','s', 0};
ROMDATA TCHAR gsIDL_ENG_RECORDING[] =
{ 'R','e','c','o','r','d','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG__DASH_[] =
{ ' ','-',' ', 0};
ROMDATA TCHAR gsIDL_ENG_LINE_INPUT_CLIP_DETECTED[] = 
{ 'L','i','n','e',' ','I','n','p','u','t',' ','C','l','i','p',' ','D','e','t','e','c','t','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_MENU_ACCESS_DISABLED_WHILE_RECORDING[] = 
{ 'M','e','n','u',' ','A','c','c','e','s','s',' ','D','i','s','a','b','l','e','d',' ','W','h','i','l','e',' ','R','e','c','o','r','d','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_OVERWRITE_PLAYLIST[] = 
{ 'O','v','e','r','w','r','i','t','e',' ','P','l','a','y','l','i','s','t','?', 0};
ROMDATA TCHAR gsIDL_ENG_TRACKS_FROM_THE_HD[] = 
{ 'T','r','a','c','k','s',' ','f','r','o','m',' ','t','h','e',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_TRACK_FROM_THE_HD[] = 
{ 'T','r','a','c','k',' ','f','r','o','m',' ','t','h','e',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_DELETING_TRACK_FROM_THE_HD[] = 
{ 'D','e','l','e','t','i','n','g',' ','T','r','a','c','k',' ','f','r','o','m',' ','t','h','e',' ','H','D', 0};
ROMDATA TCHAR gsIDL_ENG_DELETING_PLAYLIST_FROM_THE_HD[] = 
{ 'D','e','l','e','t','i','n','g',' ','P','l','a','y','l','i','s','t',' ','f','r','o','m',' ','t','h','e',' ','H','D', 0};

/*---------------------------------------------------------------*/
ROMDATA TCHAR *IDL_ENGLISH_Table[] = {
    gsIDL_ENG_EMPTY_STRING,
    gsIDL_ENG_LATIN_CHARACTERS_TEST_STRING,
    gsIDL_ENG_EXTENDED_LATIN_CHARACTERS_TEST_STRING,
    gsIDL_ENG_LATIN_CHARACTERS_BACKWARDS_TEST_STRING,
    gsIDL_ENG_EXTENDED_LATIN_CHARACTERS_BACKWARDS_TEST_STRING,
    gsIDL_ENG_ARTIST,
    gsIDL_ENG_ALBUM,
    gsIDL_ENG_TITLE,
    gsIDL_ENG_GENRE,
    gsIDL_ENG_FILE_NAME,
    gsIDL_ENG_NORMAL,
    gsIDL_ENG_RANDOM,
    gsIDL_ENG_REPEAT,
    gsIDL_ENG_REPEAT_RANDOM,
    gsIDL_ENG_TIME,
    gsIDL_ENG_NO_TRACKS_FOUND,
    gsIDL_ENG_PLAYLIST,
    gsIDL_ENG_PLAY_MODE,
    gsIDL_ENG_PLAYER_INFO,
    gsIDL_ENG_SETTINGS,
    gsIDL_ENG_INFO,
    gsIDL_ENG_TRACK_TITLE,
    gsIDL_ENG_TRACK,
    gsIDL_ENG_TRACKS,
    gsIDL_ENG_ARTISTS,
    gsIDL_ENG_ALBUMS,
    gsIDL_ENG_TITLES,
    gsIDL_ENG_GENRES,
    gsIDL_ENG_PLAYLISTS,
    gsIDL_ENG_PLAY_ALL,
    gsIDL_ENG_CURRENT,
    gsIDL_ENG_PLAY,
    gsIDL_ENG_YES,
    gsIDL_ENG_NO,
    gsIDL_ENG_OK,
    gsIDL_ENG_CANCEL,
    gsIDL_ENG_LENGTH,
    gsIDL_ENG_SIZE,
    gsIDL_ENG_TOTAL_TIME,
    gsIDL_ENG_PLAY_EVERYTHING,
    gsIDL_ENG_TRACK_ELAPSED,
    gsIDL_ENG_TRACK_REMAINING,
    gsIDL_ENG_PLAYER_V,
    gsIDL_ENG_PLAYER_BUILD,
    gsIDL_ENG_INSTALLER_V,
    gsIDL_ENG_SHUTTING_DOWN,
    gsIDL_ENG_INVALID_PLAYLIST,
    gsIDL_ENG_CANT_OPEN_TRACK,
    gsIDL_ENG_FULLPLAY_MEDIA,
    gsIDL_ENG_FULLPLAY_MEDIA_C,
    gsIDL_ENG_SOURCE,
    gsIDL_ENG_RADIO,
    gsIDL_ENG_LOCAL,
    gsIDL_ENG_LINE_INPUT,
    gsIDL_ENG_LINE_OUT,
    gsIDL_ENG_FML,
    gsIDL_ENG_OPTION,
    gsIDL_ENG_OPTIONS,
    gsIDL_ENG_HD,
    gsIDL_ENG_CD,
    gsIDL_ENG_HARD_DISK,
    gsIDL_ENG_CD_DRIVE,
    gsIDL_ENG_SYSTEM_TOOLS,
    gsIDL_ENG_SYSTEM_INFO,
    gsIDL_ENG_SET_IP,
    gsIDL_ENG_DJ_NAME,
    gsIDL_ENG_DEFRAGMENT_HD,
    gsIDL_ENG_SCAN_HD_FOR_ERRORS,
    gsIDL_ENG_FIRMWARE_VERSION,
    gsIDL_ENG_NO_GENRES_AVAILABLE,
    gsIDL_ENG_NO_ARTISTS_AVAILABLE,
    gsIDL_ENG_NO_ALBUMS_AVAILABLE,
    gsIDL_ENG_NO_TRACKS_AVAILABLE,
    gsIDL_ENG_NO_PLAYLISTS_AVAILABLE,
    gsIDL_ENG_NO_RADIO_STATIONS_AVAILABLE,
    gsIDL_ENG_MUSIC_SOURCE,
    gsIDL_ENG_MUSIC_SOURCES,
    gsIDL_ENG_MUSIC_LIBRARY,
    gsIDL_ENG_MUSIC_LIBRARIES,
    gsIDL_ENG_JUKEBOX_SETTINGS,
    gsIDL_ENG_SCAN_HD_FOR_MUSIC,
    gsIDL_ENG_SCAN_LOCAL_MUSIC,
    gsIDL_ENG_PUSH_SELECT_TO_RESTORE,
    gsIDL_ENG_RESTORE_PROCEDURE,
    gsIDL_ENG_FORMAT_HARD_DISK,
    gsIDL_ENG_FORMATTING_HARD_DISK,
    gsIDL_ENG_UPDATING_CDDB,
    gsIDL_ENG_COPY_CONTENT,
    gsIDL_ENG_COPYING_SYSTEM_FILES,
    gsIDL_ENG_UPDATE_FIRMWARE_LOCALLY,
    gsIDL_ENG_UPDATE_FIRMWARE,
    gsIDL_ENG_UPDATE_FIRMWARE_ONLINE,
    gsIDL_ENG_UPDATING_FIRMWARE,
    gsIDL_ENG_CREATE_NEW_PLAYLIST,
    gsIDL_ENG_PLEASE_CHOOSE_CD_INFO,
    gsIDL_ENG_UPDOWN_TO_CHOOSE_SELECT_TO_SAVE,
    gsIDL_ENG_CD_INFO_SAVED,
    gsIDL_ENG_DEFAULT_CD_INFO_SAVED,
    gsIDL_ENG_SAVE_PLAYLIST,
    gsIDL_ENG_SAVE_CURRENT_PLAYLIST,
    gsIDL_ENG_EMPTY_CURRENT_PLAYLIST,
    gsIDL_ENG_CLEARED_CURRENT_PLAYLIST,
    gsIDL_ENG_1400_KBPS_UNCOMPRESSED,
    gsIDL_ENG_320_KBPS_MP3,
    gsIDL_ENG_256_KBPS_MP3,
    gsIDL_ENG_224_KBPS_MP3,
    gsIDL_ENG_192_KBPS_MP3,
    gsIDL_ENG_160_KBPS_MP3,
    gsIDL_ENG_128_KBPS_MP3,
    gsIDL_ENG_112_KBPS_MP3,
    gsIDL_ENG_96_KBPS_MP3,
    gsIDL_ENG_80_KBPS_MP3,
    gsIDL_ENG_64_KBPS_MP3,
    gsIDL_ENG_56_KBPS_MP3,
    gsIDL_ENG_48_KBPS_MP3,
    gsIDL_ENG_40_KBPS_MP3,
    gsIDL_ENG_32_KBPS_MP3,
    gsIDL_ENG_RECORDING_QUALITY,
    gsIDL_ENG_RECORDING_BITRATE,
    gsIDL_ENG_CD_INSERTED,
    gsIDL_ENG_CD_REMOVED,
    gsIDL_ENG_CURRENT_CD_COLON_,
    gsIDL_ENG_SCANNING_FOR_CD_INFO,
    gsIDL_ENG_NO_INFO_FOUND_FOR_THIS_CD,
    gsIDL_ENG_FOUND_FML_COLON_,
    gsIDL_ENG_LOST_FML_COLON_,
    gsIDL_ENG_PLEASE_INSERT_CD,
    gsIDL_ENG_MENU,
    gsIDL_ENG_VOLUME,
    gsIDL_ENG_LINE_INPUT_GAIN,
    gsIDL_ENG_LINEIN_MONITOR_ARTIST,
    gsIDL_ENG_LINEIN_MONITOR_TRACKNAME,   
    gsIDL_ENG_PRESS_RECORD_TWICE_TO_RE_RECORD,
    gsIDL_ENG_STARTED_RECORDING_THE_CURRENT_TRACK,
    gsIDL_ENG_DHCP,
    gsIDL_ENG_DHCP_WITH_STATIC_BACKUP,
    gsIDL_ENG_STATIC_ONLY,
    gsIDL_ENG_EDIT_STATIC_SETTINGS,
    gsIDL_ENG_NETWORK_SETTINGS,
    gsIDL_ENG_BLANK_IP_ADDRESS,
    gsIDL_ENG_EDIT,
    gsIDL_ENG_EDIT_METADATA,
    gsIDL_ENG_EDIT_ALBUM_INFO,
    gsIDL_ENG_EDIT_TRACKS,
    gsIDL_ENG_EDIT_IP,
    gsIDL_ENG_EDIT_GATEWAY,
    gsIDL_ENG_EDIT_SUBNET,
    gsIDL_ENG_EDIT_DNS,
    gsIDL_ENG_TRACK_INFO,
    gsIDL_ENG_ALBUM_INFO,
    gsIDL_ENG_ARTIST_INFO,
    gsIDL_ENG_GENRE_INFO,
    gsIDL_ENG_CHECK_PROCEDURE,
    gsIDL_ENG_PUSH_SELECT_TO_CHECK,
    gsIDL_ENG_FACTORY_RESTORE,
    gsIDL_ENG_CHECK_HARD_DISK,
    gsIDL_ENG_SELECT_RESTORE_OPTIONS,
    gsIDL_ENG_UPDATE_CDDB,
    gsIDL_ENG_UPDATE_CDDB_ONLINE,
    gsIDL_ENG_UPDATE_CONTENT,
    gsIDL_ENG_DELETE_LOCAL_CONTENT,
    gsIDL_ENG_CLEAR_CD_CACHE,
    gsIDL_ENG_CLEAR_LOCAL_CONTENT_DATABASE,
    gsIDL_ENG_RESTORE_DEFAULT_USER_SETTINGS,
    gsIDL_ENG_START_SYSTEM_RESTORE,
    gsIDL_ENG_RESET_CDDB,
    gsIDL_ENG_LOADING,
    gsIDL_ENG_PUSH_CANCEL_TO_EXIT_WITHOUT_SAVING,
    gsIDL_ENG_PUSH_SAVE_TO_KEEP_CHANGES,
    gsIDL_ENG_QUERYING_GENRES,
    gsIDL_ENG_QUERYING_ARTISTS,
    gsIDL_ENG_QUERYING_ALBUMS,
    gsIDL_ENG_QUERYING_TRACKS,
    gsIDL_ENG_QUERYING_RADIO_STATIONS,
    gsIDL_ENG_QUERYING_PLAYLISTS,
    gsIDL_ENG_HALTING_CURRENT_QUERY,
    gsIDL_ENG_STATION_NOT_FOUND,
    gsIDL_ENG_CURRENT_PLAYLIST,
    gsIDL_ENG_COLON_SPACE,
    gsIDL_ENG_ADD,
    gsIDL_ENG_DELETE,
    gsIDL_ENG_SEARCHING,
    gsIDL_ENG_ADDING_GENRE,
    gsIDL_ENG_ADDING_ARTIST,
    gsIDL_ENG_ADDING_ALBUM,
    gsIDL_ENG_ADDING_TRACK,
    gsIDL_ENG_ADDING_PLAYLIST,
    gsIDL_ENG_ADDING_STATION,
    gsIDL_ENG_LOOKING_FOR_GENRES,
    gsIDL_ENG_LOOKING_FOR_ARTISTS,
    gsIDL_ENG_LOOKING_FOR_ALBUMS,
    gsIDL_ENG_LOOKING_FOR_TRACKS,
    gsIDL_ENG_LOOKING_FOR_PLAYLISTS,
    gsIDL_ENG_LOOKING_FOR_STATIONS,
    gsIDL_ENG_COPYING_CDDB_FILE,
    gsIDL_ENG_DOWNLOADING_CDDB_FILE,
    gsIDL_ENG_PROCESSING_CDDB_FILE,
    gsIDL_ENG_STOPPING_PLAYBACK,
    gsIDL_ENG_STOPPING_FAST_RECORDING,
    gsIDL_ENG_LOOKING_FOR_HD_TRACKS,
    gsIDL_ENG_SCANNING_FOR_TRACK_INFO,
    gsIDL_ENG_FINISHED_SCANNING,
    gsIDL_ENG_COMPLETED,
    gsIDL_ENG_OF,
    gsIDL_ENG_NO_MUSIC_FOUND_ON_HD,
    gsIDL_ENG_RETURNING_TO_MENU,
    gsIDL_ENG_ERROR,
    gsIDL_ENG_SAVING_CHANGES,
    gsIDL_ENG_SAVING_PLAYLIST,
    gsIDL_ENG_SAVE,
    gsIDL_ENG_ITEM_IS_NOT_EDITABLE,
    gsIDL_ENG_PLEASE_WAIT,
    gsIDL_ENG_PLEASE_TRY_AGAIN,
    gsIDL_ENG_EJECTING_CD,
    gsIDL_ENG_FML_NO_LONGER_AVAILABLE,
    gsIDL_ENG_CD_NO_LONGER_AVAILABLE,
    gsIDL_ENG_MUSIC_SOURCE_HAS_CHANGED,
    gsIDL_ENG_CANT_DELETE_THIS_ITEM,
    gsIDL_ENG_DELETING,
    gsIDL_ENG_BAD_PLAYLIST_NAME,
    gsIDL_ENG_NOT_AVAILABLE,
    gsIDL_ENG_NOT_AVAILABLE_WHILE_USING_LINE_INPUT,
    gsIDL_ENG_NOT_AVAILABLE_WHILE_USING_HARD_DISK,
    gsIDL_ENG_NOT_AVAILABLE_WHILE_USING_CD_DRIVE,
    gsIDL_ENG_INTERNET_RADIO_NOT_AVAILABLE,
    gsIDL_ENG_STARTING_NETWORK_SERVICES,
    gsIDL_ENG_SAVING_NETWORK_SETTINGS,
    gsIDL_ENG_NETWORK_SETTINGS_FAILED,
    gsIDL_ENG_CREATING_PLAYLIST,
    gsIDL_ENG_UNABLE_TO_PLAY_TRACK,
    gsIDL_ENG_CHECKING_FOR_UPDATES,
    gsIDL_ENG_DOWNLOADING_UPDATES,
    gsIDL_ENG_FAILED_TO_DOWNLOAD_UPDATE_FILES,
    gsIDL_ENG_CDDB_UPDATE_FAILED,
    gsIDL_ENG_NO_CDDB_UPDATES_FOUND,
    gsIDL_ENG_NO_UPDATES_FOUND,
    gsIDL_ENG_NO_NEW_UPDATES_TRY_AGAIN_LATER,
    gsIDL_ENG_UPDATE_SERVER_NOT_FOUND,
    gsIDL_ENG_CURRENT_UPDATE_LEVEL,
    gsIDL_ENG_UPDATING,
    gsIDL_ENG_FIRMWARE_UPDATE_FAILED,
    gsIDL_ENG_CORRUPT_FIRMWARE,
    gsIDL_ENG_RESETTING,
    gsIDL_ENG_DELETING_GENRE,
    gsIDL_ENG_DELETING_ARTIST,
    gsIDL_ENG_DELETING_ALBUM,
    gsIDL_ENG_DELETING_TRACK,
    gsIDL_ENG_DELETING_PLAYLIST,
    gsIDL_ENG_DELETING_STATION,
    gsIDL_ENG_LOADING_GENRE,
    gsIDL_ENG_LOADING_ARTIST,
    gsIDL_ENG_LOADING_ALBUM,
    gsIDL_ENG_LOADING_TRACK,
    gsIDL_ENG_LOADING_PLAYLIST,
    gsIDL_ENG_LOADING_STATION,
    gsIDL_ENG_NO_SPACE_LEFT_ON_HD_FOR_RECORDING,
    gsIDL_ENG_MAXIMUM_NUMBER_OF_TRACKS_RECORDED,
    gsIDL_ENG_DELETE_TRACKS_TO_ENABLE_RECORDING,
    gsIDL_ENG_TRACK_ALREADY_RECORDED,
    gsIDL_ENG_UNABLE_TO_RECORD_TRACK,
    gsIDL_ENG_CANT_RECORD_INTERNET_RADIO,
    gsIDL_ENG_RECORD,
    gsIDL_ENG_TRYING_NEXT_TRACK,
    gsIDL_ENG_PRESS_SAVE_TO_SAVE_CURRENT_PLAYLIST_TO_HD,
    gsIDL_ENG_SCANNING,
    gsIDL_ENG_SCANNED,
    gsIDL_ENG_MEDIA_LIBRARY,
    gsIDL_ENG_MEDIA_LIBRARIES,
    gsIDL_ENG_NETWORK_DISCONNECTED,
    gsIDL_ENG_NO_FULLPLAY_MEDIA_LIBRARIES,
    gsIDL_ENG_PREPARING_TO_SAVE_CHANGES,
    gsIDL_ENG_COUNTING_FILES,
    gsIDL_ENG_SAVING_CHANGES_TO,
    gsIDL_ENG_FILE,
    gsIDL_ENG_FILES,
    gsIDL_ENG_DATABASE,
    gsIDL_ENG_WROTE,
    gsIDL_ENG_PARTIAL_TRACK_RECORDING_CANCELLED,
    gsIDL_ENG_CANT_RECORD_AFTER_SEEKING_ON_A_TRACK,
    gsIDL_ENG_RESTART_TRACK_TO_ENABLE_RECORDING,
    gsIDL_ENG_CANT_SEEK_WHILE_RECORDING,
    gsIDL_ENG_CANT_SEEK_ON_NETWORK_TRACK,
    gsIDL_ENG_CANT_EJECT_TRAY_WHILE_RECORDING_LINE_INPUT,
    gsIDL_ENG_NEW_FIRMWARE_VERSION_FOUND_ON_CD,
    gsIDL_ENG_NEW_CDDB_VERSION_FOUND_ON_CD,
    gsIDL_ENG_ACCESSING_CDDB,
    gsIDL_ENG_ACCESSING_CDDB_ONLINE,
    gsIDL_ENG_CANT_ACCESS_CDDB_ONLINE,
    gsIDL_ENG_STOPPED_RECORDING_TRACK_TO_HD,
    gsIDL_ENG_STOPPED_RECORDING_CD_TO_HD,
    gsIDL_ENG_STOPPED_RECORDING_CURRENT_PLAYLIST_TO_HD,
    gsIDL_ENG_CANT_READ_CD,
    gsIDL_ENG_FINISHED_RECORDING_TRACK_TO_HD,
    gsIDL_ENG_FINISHED_RECORDING_CD_TO_HD,
    gsIDL_ENG_FINISHED_RECORDING_CURRENT_PLAYLIST_TO_HD,
    gsIDL_ENG_FINISHED_RECORDING_QUERIED_TRACKS_TO_HD,
    gsIDL_ENG_NO_TRACKS_TO_RECORD_IN_CURRENT_PLAYLIST,
    gsIDL_ENG_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST,
    gsIDL_ENG_COPY_PLAYLIST_TO_HD,
    gsIDL_ENG_COPYING_PLAYLIST_TO_HD,
    gsIDL_ENG_PREPARING_TO_COPY,
    gsIDL_ENG_FAILED_TO_COPY_LAST_TRACK,
    gsIDL_ENG_CANCELLING,
    gsIDL_ENG_COMMITTING_DB,
    gsIDL_ENG_ABOUT_ELLIPSIS,
    gsIDL_ENG_FREE_HD_SPACE,
    gsIDL_ENG_HD_INFO,
    gsIDL_ENG_SAVE_CURRENT_PLAYLIST_AS_NEW,
    gsIDL_ENG_EJECT_CD_TRAY_AFTER_FAST_RECORD, 
    gsIDL_ENG_ENABLE_EXTENDED_CHAR_EDITING, 
    gsIDL_ENG_ENABLE_WEB_CONTROL, 
    gsIDL_ENG_SHOW_TRACK_NUMBER_IN_TITLE, 
    gsIDL_ENG_SHOW_ALBUM_WITH_ARTIST_NAME,
    gsIDL_ENG_PLAY_CD_WHEN_INSERTED,
    gsIDL_ENG_SET_TEXT_SCROLL_SPEED, 
    gsIDL_ENG_ENABLE_RECORD_LED, 
    gsIDL_ENG_SET_LCD_BRIGHTNESS, 
    gsIDL_ENG_CUSTOMIZATIONS, 
    gsIDL_ENG_BRIGHT, 
    gsIDL_ENG_DIM, 
    gsIDL_ENG_FAST, 
    gsIDL_ENG_SLOW,
    gsIDL_ENG_OFF,
    gsIDL_ENG_RESTORE_SUCCEEDED,
    gsIDL_ENG_RESTORE_FAILED,
    gsIDL_ENG_RESTORE_MENU,
    gsIDL_ENG_ERASING_MUSIC_CONTINUE_QM,
    gsIDL_ENG_NO_RESTORE_CD_FOUND,
    gsIDL_ENG_CHECKING_FOR_RESTORE_CD,
    gsIDL_ENG_PLEASE_INSERT_RESTORE_CD,
    gsIDL_ENG_UPDATING_SYSTEM_FILES,
    gsIDL_ENG_RETRYING,
    gsIDL_ENG_CALL_TECH_SUPPORT,
    gsIDL_ENG_NO_UPDATES_PERFORMED,
    gsIDL_ENG_PLAYLIST_TRACK_LIMIT_REACHED,
    gsIDL_ENG_ADDING,
    gsIDL_ENG_NO_CD,
    gsIDL_ENG_CANT_BROWSE_AUDIO_CD,
    gsIDL_ENG_CANT_BROWSE_WHILE_SCANNING_DATA_CD,
    gsIDL_ENG_PLEASE_SELECT_TRACKS,
    gsIDL_ENG_PRESS_EXIT_TO_CANCEL,
    gsIDL_ENG_SCANNING_INFO_FOR,
    gsIDL_ENG_CD_TRACKS,
    gsIDL_ENG_HD_TRACKS,
    gsIDL_ENG_ERROR_QUERYING_MEDIA_LIBRARY_COLON_,
    gsIDL_ENG_RECORDING_WITH,
    gsIDL_ENG_MINUTES_FREE,
    gsIDL_ENG_RECORDING_TRACK,
    gsIDL_ENG_TO_HD,
    gsIDL_ENG_LINE_INPUT_GAIN_NOW,
    gsIDL_ENG_CANT_CHANGE_PLAY_MODE_WHILE_FAST_RECORDING,
    gsIDL_ENG_CANT_CHANGE_PLAY_MODE_OF_LINE_INPUT_SOURCE,
    gsIDL_ENG_CANT_SAVE_PLAYLIST_OF_UNRECORDED_CD_TRACKS,
    gsIDL_ENG_CANT_REMOVE_AUDIO_CD_TRACKS,
    gsIDL_ENG_MUSIC_SOURCE_COLON_,
    gsIDL_ENG_FOUND_NEW_CD,
    gsIDL_ENG_ALREADY_HAVE_TRACK,
    gsIDL_ENG_ON_HD,
    gsIDL_ENG_SEEKING_FORWARD,
    gsIDL_ENG_SEEKING_BACKWARD,
    gsIDL_ENG_SECONDS,
    gsIDL_ENG_CANT_LOAD_PLAYLIST,
    gsIDL_ENG_CONSIDER_DELETING_IT,
    gsIDL_ENG_SAVED_CURRENT_PLAYLIST_AS,
    gsIDL_ENG_CHECK_PHASE,
    gsIDL_ENG_CHECKING_DRIVE_PASS,
    gsIDL_ENG_READING_FATS,
    gsIDL_ENG_CHECKING_CLUSTER_CHAINS,
    gsIDL_ENG_CHECKING_DIRECTORIES,
    gsIDL_ENG_CHECK_FOR_LOST_FILES,
    gsIDL_ENG_UPDATING_FATS,
    gsIDL_ENG_RESTARTING_PLEASE_WAIT,
    gsIDL_ENG_TRACKS_FOUND_COLON,
    gsIDL_ENG_ERROR_COPYING_CDDB,
    gsIDL_ENG_DELETE_THIS_GENRE_QM,
    gsIDL_ENG_DELETE_THIS_ARTIST_QM,
    gsIDL_ENG_DELETE_THIS_ALBUM_QM,
    gsIDL_ENG_DELETE_THIS_TRACK_QM,
    gsIDL_ENG_DELETE_THIS_PLAYLIST_QM,
    gsIDL_ENG_DELETE_THIS_RADIO_STATION_QM,
    gsIDL_ENG_FORMAT_HARD_DISK_QM,
    gsIDL_ENG_DRIVE_CHECK_COMPLETE,
    gsIDL_ENG_INSTALL_UPDATE_QM,
    gsIDL_ENG_LOOKING_FOR_CD,
    gsIDL_ENG_RESTORING_CDDB,
    gsIDL_ENG_FIXING_CORRUPT_CDDB_FILE,
    gsIDL_ENG_CHANGING,
    gsIDL_ENG_CHANGED,
    gsIDL_ENG_SAVING_INFO_TO,
    gsIDL_ENG_SPACE_REMAINING,
    gsIDL_ENG_TIME_REMAINING,
    gsIDL_ENG_KBPS,
    gsIDL_ENG_RECORDING,
    gsIDL_ENG__DASH_,
    gsIDL_ENG_LINE_INPUT_CLIP_DETECTED,
    gsIDL_ENG_MENU_ACCESS_DISABLED_WHILE_RECORDING,
    gsIDL_ENG_OVERWRITE_PLAYLIST,
    gsIDL_ENG_TRACKS_FROM_THE_HD,
    gsIDL_ENG_TRACK_FROM_THE_HD,
    gsIDL_ENG_DELETING_TRACK_FROM_THE_HD,
    gsIDL_ENG_DELETING_PLAYLIST_FROM_THE_HD

};


/*---------------------------------------------------------------*/
ROMDATA TCHAR **wbStringTable[1] = {
    IDL_ENGLISH_Table
};

/*---------------------------------------------------------------*/
UCHAR gbCurrentLanguage = CURRENT_LANGUAGE;

ROMDATA TCHAR *LookupString(WORD wSID)
{
    return wbStringTable[gbCurrentLanguage][wSID];
}

