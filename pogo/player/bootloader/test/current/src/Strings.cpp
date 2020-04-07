
#include "Strings.h"

/*---------------------------------------------------------------*/
ROMDATA GCHAR gsIDL_ENG_DAN_BOLSTAD[] = 
{   'D','a','n',' ','B','o','l','s','t','a','d',' ','-',' ','d','a',
	'n','b','@','i','o','b','j','e','c','t','s','.','c','o','m', 0};
ROMDATA GCHAR gsIDL_ENG_FIRMWARE[] = 
{   'F','i','r','m','w','a','r','e', 0};
ROMDATA GCHAR gsIDL_ENG_FIRMWARE_NOT_FOUND[] = 
{   'F','i','r','m','w','a','r','e',' ','N','o','t',' ','F','o','u','n','d', 0};
ROMDATA GCHAR gsIDL_ENG_FIRMWARE_UPDATE_FAILED[] = 
{   'F','i','r','m','w','a','r','e',' ','U','p','d','a','t','e',' ','F','a','i','l','e','d', 0};
ROMDATA GCHAR gsIDL_ENG_SEARCHING_FOR_FIRMWARE[] = 
{   'S','e','a','r','c','h','i','n','g',' ','F','o','r',' ','F','i','r','m','w','a','r','e', 0};
ROMDATA GCHAR gsIDL_ENG_DOWNLOADING[] = 
{   'D','o','w','n','l','o','a','d','i','n','g', 0};
ROMDATA GCHAR gsIDL_ENG_PROGRAMMING[] = 
{   'P','r','o','g','r','a','m','m','i','n','g', 0};
ROMDATA GCHAR gsIDL_ENG_VERIFYING[] = 
{   'V','e','r','i','f','y','i','n','g', 0};
ROMDATA GCHAR gsIDL_ENG_SUCCESSFUL[] = 
{   'S','u','c','c','e','s','s','f','u','l', 0};
ROMDATA GCHAR gsIDL_ENG_PLAYER_V[] = 
{   'P','l','a','y','e','r',' ','V','.', 0};
ROMDATA GCHAR gsIDL_ENG_PLAYER_BUILD[] = 
{   'P','l','a','y','e','r',' ','B','u','i','l','d', 0};
ROMDATA GCHAR gsIDL_ENG_INSTALLER_V[] = 
{   'I','n','s','t','a','l','l','e','r',' ','V','.', 0};
ROMDATA GCHAR gsIDL_ENG_SHUTTING_DOWN[] = 
{   'S','h','u','t','t','i','n','g',' ','D','o','w','n', 0};
ROMDATA GCHAR gsIDL_ENG_LOW_POWER[] = 
{   'L','o','w',' ','P','o','w','e','r', 0};
ROMDATA GCHAR gsIDL_ENG_INTERACTIVE_OBJECTS_C[] = 
{   'I','n','t','e','r','a','c','t','i','v','e',' ','O','b','j','e','c','t','s',' ', 0x00a9, 0};
ROMDATA GCHAR gsIDL_ENG_FULLPLAY_MEDIA_C[] = 
{   'F','u','l','l','P','l','a','y',' ','M','e','d','i','a',' ', 0x00a9, 0};



/*---------------------------------------------------------------*/
ROMDATA GCHAR *IDL_ENGLISH_Table[] = {
	gsIDL_ENG_DAN_BOLSTAD,
	gsIDL_ENG_FIRMWARE,
	gsIDL_ENG_FIRMWARE_NOT_FOUND,
	gsIDL_ENG_FIRMWARE_UPDATE_FAILED,
	gsIDL_ENG_SEARCHING_FOR_FIRMWARE,
	gsIDL_ENG_DOWNLOADING,
	gsIDL_ENG_PROGRAMMING,
	gsIDL_ENG_VERIFYING,
	gsIDL_ENG_SUCCESSFUL,
	gsIDL_ENG_PLAYER_V,
	gsIDL_ENG_PLAYER_BUILD,
	gsIDL_ENG_INSTALLER_V,
	gsIDL_ENG_SHUTTING_DOWN,
	gsIDL_ENG_LOW_POWER,
	gsIDL_ENG_INTERACTIVE_OBJECTS_C,
	gsIDL_ENG_FULLPLAY_MEDIA_C,
};


/*---------------------------------------------------------------*/
ROMDATA GCHAR **wbStringTable[1] = {
    IDL_ENGLISH_Table
};

/*---------------------------------------------------------------*/
UCHAR gbCurrentLanguage = 0;

char* LookupString(unsigned short wSID)
{
    return (char*)wbStringTable[gbCurrentLanguage][wSID];
}

