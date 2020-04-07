

#include <gui/peg/peg.hpp>

/*---------------------------------------------------------------*/
ROMDATA TCHAR gsIDL_ENG_ARTIST[] = 
{   0x0041, 0x0072, 0x0074, 0x0069, 0x0073, 0x0074, 0};
ROMDATA TCHAR gsIDL_ENG_ALBUM[] = 
{   0x0041, 0x006c, 0x0062, 0x0075, 0x006d, 0};
ROMDATA TCHAR gsIDL_ENG_TITLE[] = 
{   0x0054, 0x0069, 0x0074, 0x006c, 0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_GENRE[] = 
{   0x0047, 0x0065, 0x006e, 0x0072, 0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_FILE_NAME[] = 
{   0x0046, 0x0069, 0x006c, 0x0065, 0x0020, 0x004e, 0x0061, 0x006d,
    0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_CONNECTED[] = 
{   0x0043, 0x006f, 0x006e, 0x006e, 0x0065, 0x0063, 0x0074, 0x0065,
    0x0064, 0};
ROMDATA TCHAR gsIDL_ENG_DISCONNECTED[] = 
{   0x0044, 0x0069, 0x0073, 0x0063, 0x006f, 0x006e, 0x006e, 0x0065,
    0x0063, 0x0074, 0x0065, 0x0064, 0};
ROMDATA TCHAR gsIDL_ENG_NORMAL[] = 
{   0x004e, 0x006f, 0x0072, 0x006d, 0x0061, 0x006c, 0};
ROMDATA TCHAR gsIDL_ENG_RANDOM[] = 
{   0x0052, 0x0061, 0x006e, 0x0064, 0x006f, 0x006d, 0};
ROMDATA TCHAR gsIDL_ENG_REPEAT_ALL[] = 
{   0x0052, 0x0065, 0x0070, 0x0065, 0x0061, 0x0074, 0x0020, 0x0041,
    0x006c, 0x006c, 0};
ROMDATA TCHAR gsIDL_ENG_REPEAT_RANDOM[] = 
{   0x0052, 0x0065, 0x0070, 0x0065, 0x0061, 0x0074, 0x0020, 0x0052,
    0x0061, 0x006e, 0x0064, 0x006f, 0x006d, 0};
ROMDATA TCHAR gsIDL_ENG_ALBUM_ORDER[] = 
{   'A', 'l', 'b', 'u', 'm', 0};
ROMDATA TCHAR gsIDL_ENG_REPEAT_ALBUM_ORDER[] = 
{   'R', 'e', 'p', 'e', 'a', 't', ' ', 'A', 'l', 'b', 'u', 'm', 0};
ROMDATA TCHAR gsIDL_ENG_RANDOM_ALBUM_ORDER[] = 
{   'R', 'a', 'n', 'd', 'o', 'm', ' ', 'A', 'l', 'b', 'u', 'm', 0};
ROMDATA TCHAR gsIDL_ENG_REPEAT_RANDOM_ALBUM_ORDER[] = 
{   'R', 'e', 'p', 'e', 'a', 't', ' ', 'R', 'a', 'n', 'd', 'o', 'm', ' ', 'A', 'l', 'b', 'u', 'm', 0};
ROMDATA TCHAR gsIDL_ENG_INTRO_SCAN[] = 
{   0x0049, 0x006e, 0x0074, 0x0072, 0x006f, 0x0020, 0x0053, 0x0063,
    0x0061, 0x006e, 0};
ROMDATA TCHAR gsIDL_ENG_REPEAT_TRACK[] = 
{   0x0052, 0x0065, 0x0070, 0x0065, 0x0061, 0x0074, 0x0020, 0x0054,
    0x0072, 0x0061, 0x0063, 0x006b, 0};
ROMDATA TCHAR gsIDL_ENG_BASS_TREBLE[] = 
{   0x0042, 0x0061, 0x0073, 0x0073, 0x0020, 0x0026, 0x0020, 0x0054,
    0x0072, 0x0065, 0x0062, 0x006c, 0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_STANDARD[] = 
{   0x0053, 0x0074, 0x0061, 0x006e, 0x0064, 0x0061, 0x0072, 0x0064, 0};
ROMDATA TCHAR gsIDL_ENG_ROCK[] = 
{   0x0052, 0x006f, 0x0063, 0x006b, 0};
ROMDATA TCHAR gsIDL_ENG_CLASSICAL[] = 
{   0x0043, 0x006c, 0x0061, 0x0073, 0x0073, 0x0069, 0x0063, 0x0061,
    0x006c, 0};
ROMDATA TCHAR gsIDL_ENG_JAZZ[] = 
{   0x004a, 0x0061, 0x007a, 0x007a, 0};
ROMDATA TCHAR gsIDL_ENG_TIME[] = 
{   0x0054, 0x0069, 0x006d, 0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_BASS[] = 
{   0x0042, 0x0061, 0x0073, 0x0073, 0};
ROMDATA TCHAR gsIDL_ENG_TREBLE[] = 
{   0x0054, 0x0072, 0x0065, 0x0062, 0x006c, 0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_NO_SONGS_FOUND[] = 
{   0x004e, 0x006f, 0x0020, 0x0053, 0x006f, 0x006e, 0x0067, 0x0073,
    0x0020, 0x0046, 0x006f, 0x0075, 0x006e, 0x0064, 0};
ROMDATA TCHAR gsIDL_ENG_PLAYLIST[] = 
{   0x0050, 0x006c, 0x0061, 0x0079, 0x006c, 0x0069, 0x0073, 0x0074, 0};
ROMDATA TCHAR gsIDL_ENG_PLAY_ORDER[] = 
{   'P','l','a','y',' ','O','r','d','e','r', 0};
ROMDATA TCHAR gsIDL_ENG_EQUALIZER[] = 

{   0x0045, 0x0071, 0x0075, 0x0061, 0x006c, 0x0069, 0x007a, 0x0065,
    0x0072, 0};
ROMDATA TCHAR gsIDL_ENG_BACKLIGHT[] = 
{   0x0042, 0x0061, 0x0063, 0x006b, 0x006c, 0x0069, 0x0067, 0x0068,
    0x0074, 0};
ROMDATA TCHAR gsIDL_ENG_PLAYER_INFO[] = 
{   0x0050, 0x006c, 0x0061, 0x0079, 0x0065, 0x0072, 0x0020, 0x0049,
    0x006e, 0x0066, 0x006f, 0};
ROMDATA TCHAR gsIDL_ENG_SETTINGS[] = 
{   0x0053, 0x0065, 0x0074, 0x0074, 0x0069, 0x006e, 0x0067, 0x0073, 0};
ROMDATA TCHAR gsIDL_ENG_BACKLIGHT_ON[] = 
{   0x0042, 0x0061, 0x0063, 0x006b, 0x006c, 0x0069, 0x0067, 0x0068,
    0x0074, 0x0020, 0x004f, 0x006e, 0};
ROMDATA TCHAR gsIDL_ENG_BACKLIGHT_OFF[] = 
{   0x0042, 0x0061, 0x0063, 0x006b, 0x006c, 0x0069, 0x0067, 0x0068,
    0x0074, 0x0020, 0x004f, 0x0066, 0x0066, 0};
ROMDATA TCHAR gsIDL_ENG_SONG_TITLE[] = 
{   0x0053, 0x006f, 0x006e, 0x0067, 0x0020, 0x0020, 0x0054, 0x0069,
    0x0074, 0x006c, 0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_TRACK[] = 
{   0x0054, 0x0072, 0x0061, 0x0063, 0x006b, 0};
ROMDATA TCHAR gsIDL_ENG_TRACKS[] = 
{   0x0054, 0x0072, 0x0061, 0x0063, 0x006b, 0x0073, 0};
ROMDATA TCHAR gsIDL_ENG_ARTISTS[] = 
{   0x0041, 0x0072, 0x0074, 0x0069, 0x0073, 0x0074, 0x0073, 0};
ROMDATA TCHAR gsIDL_ENG_ALBUMS[] = 
{   0x0041, 0x006c, 0x0062, 0x0075, 0x006d, 0x0073, 0};
ROMDATA TCHAR gsIDL_ENG_TITLES[] = 
{   0x0054, 0x0069, 0x0074, 0x006c, 0x0065, 0x0073, 0};
ROMDATA TCHAR gsIDL_ENG_GENRES[] = 
{   0x0047, 0x0065, 0x006e, 0x0072, 0x0065, 0x0073, 0};
ROMDATA TCHAR gsIDL_ENG_PLAYLISTS[] = 
{   0x0050, 0x006c, 0x0061, 0x0079, 0x006c, 0x0069, 0x0073, 0x0074,
    0x0073, 0};
ROMDATA TCHAR gsIDL_ENG_FOLDERS[] = 
{   0x0046, 0x006f, 0x006c, 0x0064, 0x0065, 0x0072, 0x0073, 0};
ROMDATA TCHAR gsIDL_ENG_PLAY_ALL[] = 
{   0x0050, 0x006c, 0x0061, 0x0079, 0x0020, 0x0041, 0x006c, 0x006c, 0};
ROMDATA TCHAR gsIDL_ENG_CURRENT[] = 
{   0x0043, 0x0075, 0x0072, 0x0072, 0x0065, 0x006e, 0x0074, 0};
ROMDATA TCHAR gsIDL_ENG_PLAY[] = 
{   0x0050, 0x006c, 0x0061, 0x0079, 0};
ROMDATA TCHAR gsIDL_ENG_YES[] = 
{   0x0059, 0x0065, 0x0073, 0};
ROMDATA TCHAR gsIDL_ENG_NO[] = 
{   0x004e, 0x006f, 0};
ROMDATA TCHAR gsIDL_ENG_OK[] = 
{   0x004f, 0x004b, 0};
ROMDATA TCHAR gsIDL_ENG_REBUILD_DATABASE[] = 
{   0x0052, 0x0065, 0x0062, 0x0075, 0x0069, 0x006c, 0x0064, 0x0020,
    0x0044, 0x0061, 0x0074, 0x0061, 0x0062, 0x0061, 0x0073, 0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_REBUILDING_DATABASE[] = 
{   0x0052, 0x0065, 0x0062, 0x0075, 0x0069, 0x006c, 0x0064, 0x0069,
    0x006e, 0x0067, 0x0020, 0x0044, 0x0061, 0x0074, 0x0061, 0x0062,
    0x0061, 0x0073, 0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_CANCEL[] = 
{   0x0043, 0x0061, 0x006e, 0x0063, 0x0065, 0x006c, 0};
ROMDATA TCHAR gsIDL_ENG_LENGTH[] = 
{   0x004c, 0x0065, 0x006e, 0x0067, 0x0074, 0x0068, 0};
ROMDATA TCHAR gsIDL_ENG_SIZE[] = 
{   0x0053, 0x0069, 0x007a, 0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_TOTAL_TIME[] = 
{   0x0054, 0x006f, 0x0074, 0x0061, 0x006c, 0x0020, 0x0054, 0x0069,
    0x006d, 0x0065, 0};
ROMDATA TCHAR gsIDL_ENG_SET[] = 
{   'S','e','t', 0};
ROMDATA TCHAR gsIDL_ENG_SETUP[] = 
{   'S','e','t','u','p', 0};
ROMDATA TCHAR gsIDL_ENG_TONE[] = 
{   'T','o','n','e', 0};
ROMDATA TCHAR gsIDL_ENG_DAN_BOLSTAD[] = 
{   'D','a','n',' ','B','o','l','s','t','a','d',' ','-',' ','d','a',
	'n','b','@','i','o','b','j','e','c','t','s','.','c','o','m', 0};
ROMDATA TCHAR gsIDL_ENG_DASHED_LINE[] = 
{   '-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-', 0};
ROMDATA TCHAR gsIDL_ENG_PLAY_EVERYTHING[] = 
{   'P','l','a','y',' ','E','v','e','r','y','t','h','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_TRACK_ELAPSED[] = 
{   'T','r','a','c','k',' ','E','l','a','p','s','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_TRACK_REMAINING[] = 
{   'T','r','a','c','k',' ','R','e','m','a','i','n','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_ALBUM_ELAPSED[] = 
{   'A','l','b','u','m',' ','E','l','a','p','s','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_ALBUM_REMAINING[] = 
{   'A','l','b','u','m',' ','R','e','m','a','i','n','i','n','g', 0};
ROMDATA TCHAR gsIDL_ENG_PLAYER_V[] = 
{   'P','l','a','y','e','r',' ','V','.', 0};
ROMDATA TCHAR gsIDL_ENG_PLAYER_BUILD[] = 
{   'P','l','a','y','e','r',' ','B','u','i','l','d', 0};
ROMDATA TCHAR gsIDL_ENG_INSTALLER_V[] = 
{   'I','n','s','t','a','l','l','e','r',' ','V','.', 0};
ROMDATA TCHAR gsIDL_ENG_SHUTTING_DOWN[] = 
{   'S','h','u','t','t','i','n','g',' ','D','o','w','n', 0};
ROMDATA TCHAR gsIDL_ENG_LOW_POWER[] = 
{   'L','o','w',' ','P','o','w','e','r', 0};
ROMDATA TCHAR gsIDL_ENG_USB_CONNECTED[] = 
{   'U','S','B',' ','C','o','n','n','e','c','t','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_PLAYER_LOCKED[] = 
{   'P','l','a','y','e','r',' ','L','o','c','k','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_INVALID_PLAYLIST[] = 
{   'I','n','v','a','l','i','d',' ','P','l','a','y','l','i','s','t', 0};
ROMDATA TCHAR gsIDL_ENG_INTERACTIVE_OBJECTS[] = 
{   'I','n','t','e','r','a','c','t','i','v','e',' ','O','b','j','e','c','t','s', 0};
ROMDATA TCHAR gsIDL_ENG_INTERACTIVE_OBJECTS_C[] = 
{   'I','n','t','e','r','a','c','t','i','v','e',' ','O','b','j','e','c','t','s',' ', 0x00a9, 0};
ROMDATA TCHAR gsIDL_ENG_FULLPLAY_MEDIA[] = 
{   'F','u','l','l','P','l','a','y',' ','M','e','d','i','a', 0};
ROMDATA TCHAR gsIDL_ENG_FULLPLAY_MEDIA_C[] = 
{   'F','u','l','l','P','l','a','y',' ','M','e','d','i','a',' ', 0x00a9, 0};
ROMDATA TCHAR gsIDL_ENG_DISKINFO_DISKFREE[] = 
{   'F','r','e','e', 0};
ROMDATA TCHAR gsIDL_ENG_DISKINFO_DISKUSED[] = 
{   'U','s','e','d', 0};
ROMDATA TCHAR gsIDL_ENG_DISKINFO_FILECOUNT[] = 
{   'F','i','l','e','s', 0};
ROMDATA TCHAR gsIDL_ENG_DISKINFO_FOLDERCOUNT[] = 
{   'F','o','l','d','e','r','s', 0};
ROMDATA TCHAR gsIDL_ENG_DISK_INFO[] = 
{   'D','i','s','k',' ','I','n','f','o', 0};
ROMDATA TCHAR gsIDL_ENG_BITRATE_64[] = 
{   '6','4',' ','K','b','p','s', 0};
ROMDATA TCHAR gsIDL_ENG_BITRATE_128[] = 
{   '1','2','8',' ','K','b','p','s', 0};
ROMDATA TCHAR gsIDL_ENG_BITRATE_192[] = 
{   '1','9','2',' ','K','b','p','s', 0};
ROMDATA TCHAR gsIDL_ENG_RECORDING_BITRATE[] = 
{   'R','e','c','o','r','d','i','n','g',' ','B','i','t','r','a','t','e', 0};
ROMDATA TCHAR gsIDL_ENG_INPUT_LINE_IN[] = 
{   'L','i','n','e','-','I','n', 0};
ROMDATA TCHAR gsIDL_ENG_INPUT_MIC[] = 
{   'M','i','c', 0};
ROMDATA TCHAR gsIDL_ENG_INPUT_SELECT[] = 
{   'R','e','c','o','r','d','i','n','g',' ','I','n','p','u','t', 0};
ROMDATA TCHAR gsIDL_ENG_FINDING_NEW_FILES[] = 
{   'F','i','n','d','i','n','g',' ','N','e','w',' ','F','i','l','e','s', 0};
/*---------------------------------------------------------------*/
ROMDATA TCHAR *IDL_ENGLISH_Table[] = {
    gsIDL_ENG_ARTIST,
    gsIDL_ENG_ALBUM,
    gsIDL_ENG_TITLE,
    gsIDL_ENG_GENRE,
    gsIDL_ENG_FILE_NAME,
    gsIDL_ENG_CONNECTED,
    gsIDL_ENG_DISCONNECTED,
    gsIDL_ENG_NORMAL,
    gsIDL_ENG_RANDOM,
    gsIDL_ENG_REPEAT_ALL,
    gsIDL_ENG_REPEAT_RANDOM,
    gsIDL_ENG_ALBUM_ORDER,
    gsIDL_ENG_REPEAT_ALBUM_ORDER,
    gsIDL_ENG_RANDOM_ALBUM_ORDER,
    gsIDL_ENG_REPEAT_RANDOM_ALBUM_ORDER,
    gsIDL_ENG_INTRO_SCAN,
    gsIDL_ENG_REPEAT_TRACK,
    gsIDL_ENG_BASS_TREBLE,
    gsIDL_ENG_STANDARD,
    gsIDL_ENG_ROCK,
    gsIDL_ENG_CLASSICAL,
    gsIDL_ENG_JAZZ,
    gsIDL_ENG_TIME,
    gsIDL_ENG_BASS,
    gsIDL_ENG_TREBLE,
    gsIDL_ENG_NO_SONGS_FOUND,
    gsIDL_ENG_PLAYLIST,
    gsIDL_ENG_PLAY_ORDER,
    gsIDL_ENG_EQUALIZER,
    gsIDL_ENG_BACKLIGHT,
    gsIDL_ENG_PLAYER_INFO,
    gsIDL_ENG_SETTINGS,
    gsIDL_ENG_BACKLIGHT_ON,
    gsIDL_ENG_BACKLIGHT_OFF,
    gsIDL_ENG_SONG_TITLE,
    gsIDL_ENG_TRACK,
    gsIDL_ENG_TRACKS,
    gsIDL_ENG_ARTISTS,
    gsIDL_ENG_ALBUMS,
    gsIDL_ENG_TITLES,
    gsIDL_ENG_GENRES,
    gsIDL_ENG_PLAYLISTS,
    gsIDL_ENG_FOLDERS,
    gsIDL_ENG_PLAY_ALL,
    gsIDL_ENG_CURRENT,
    gsIDL_ENG_PLAY,
    gsIDL_ENG_YES,
    gsIDL_ENG_NO,
    gsIDL_ENG_OK,
    gsIDL_ENG_REBUILD_DATABASE,
    gsIDL_ENG_REBUILDING_DATABASE,
    gsIDL_ENG_CANCEL,
    gsIDL_ENG_LENGTH,
    gsIDL_ENG_SIZE,
    gsIDL_ENG_TOTAL_TIME,
    gsIDL_ENG_SET,
    gsIDL_ENG_SETUP,
    gsIDL_ENG_TONE,
	gsIDL_ENG_DAN_BOLSTAD,
	gsIDL_ENG_DASHED_LINE,
	gsIDL_ENG_PLAY_EVERYTHING,
	gsIDL_ENG_TRACK_ELAPSED,
	gsIDL_ENG_TRACK_REMAINING,
	gsIDL_ENG_ALBUM_ELAPSED,
	gsIDL_ENG_ALBUM_REMAINING,
	gsIDL_ENG_PLAYER_V,
	gsIDL_ENG_PLAYER_BUILD,
	gsIDL_ENG_INSTALLER_V,
	gsIDL_ENG_SHUTTING_DOWN,
	gsIDL_ENG_LOW_POWER,
	gsIDL_ENG_USB_CONNECTED,
	gsIDL_ENG_PLAYER_LOCKED,
	gsIDL_ENG_INVALID_PLAYLIST,
	gsIDL_ENG_INTERACTIVE_OBJECTS,
	gsIDL_ENG_INTERACTIVE_OBJECTS_C,
	gsIDL_ENG_FULLPLAY_MEDIA,
	gsIDL_ENG_FULLPLAY_MEDIA_C,
    gsIDL_ENG_DISK_INFO,
    gsIDL_ENG_DISKINFO_DISKFREE,
    gsIDL_ENG_DISKINFO_DISKUSED,
    gsIDL_ENG_DISKINFO_FILECOUNT,
    gsIDL_ENG_DISKINFO_FOLDERCOUNT,
    gsIDL_ENG_BITRATE_64,
    gsIDL_ENG_BITRATE_128,
    gsIDL_ENG_BITRATE_192,
    gsIDL_ENG_RECORDING_BITRATE,
    gsIDL_ENG_INPUT_LINE_IN,
    gsIDL_ENG_INPUT_MIC,
    gsIDL_ENG_INPUT_SELECT,
    gsIDL_ENG_FINDING_NEW_FILES,
};


/*---------------------------------------------------------------*/
ROMDATA TCHAR **wbStringTable[1] = {
    IDL_ENGLISH_Table
};

/*---------------------------------------------------------------*/
UCHAR gbCurrentLanguage = 0;

ROMDATA TCHAR *LookupString(WORD wSID)
{
    return wbStringTable[gbCurrentLanguage][wSID];
}

