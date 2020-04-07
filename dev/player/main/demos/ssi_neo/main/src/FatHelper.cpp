#include <main/demos/ssi_neo/main/FatHelper.h>
#include <core/playmanager/PlayManager.h>
#include <fs/fat/sdapi.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <main/demos/ssi_neo/main/AppSettings.h>     // PLAYLIST_STRING_SIZE

#include <ctype.h>      /* isspace */
#include <stdlib.h>     /* malloc */
#include <stdio.h>      /* sprintf */

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_FAT_HELPER, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DBG_FAT_HELPER);

// return just the filename part of the url, as a new string
char* FilenameFromURL(const char* szURL)
{
    int len = strlen(szURL);
    int index = len-1;
    while (szURL[index-1] != '\\')
        --index;
    char* szRet = new char[len-index+1];
    strcpy(szRet,szURL+index);
    return szRet;
}

int PathlenFromURL(const char* szURL)
{
    int len = strlen(szURL);
    while (szURL[len-1] != '\\')
        --len;
    return len;
}

// add a subfolder to a full path sequence (while moving down into subfolders)
bool
PushPathSegment(char* szPath, char* szSubFolder)
{
    // if the folder is so deep that its path is unusable, then fail
    if (strlen(szPath) + strlen(szSubFolder) + 5 > PLAYLIST_STRING_SIZE)
        return false;
    strcat(szPath,szSubFolder);
    strcat(szPath,"\\");
    return true;
}
// push a segment onto the current URL member.
void PushURLSegment(char** pszURL, int &nAllocSize, char* szSubFolder)
{
    if (!*pszURL)
    {
        nAllocSize += 1024;
        *pszURL = (char*)malloc(nAllocSize);
        *pszURL[0] = 0;
    }
    if (strlen(*pszURL) + strlen(szSubFolder) + 2 > nAllocSize)
    {
        while (strlen(*pszURL) + strlen(szSubFolder) + 2 > nAllocSize)
            nAllocSize += 1024;
        char* temp = (char*)malloc(nAllocSize);
        strcpy (temp,*pszURL);
        delete [] *pszURL;
        *pszURL = temp;
    }
    strcat (*pszURL,szSubFolder);
    strcat (*pszURL,"\\");
}

// remove a subfolder from a full path (while moving up towards the folder root)
void
PopPathSegment(char* szPath)
{
    int index = strlen(szPath) - 1;
    while (szPath[index-1] != '\\')
        --index;
    szPath[index] = 0;
}

void TrimInPlace(char* str)
{
    int len = strlen(str);
    while (str[len-1] == ' ')
    {
        str[len-1] = 0;
        --len;
    }
}

void AddStrippedString(char* szBase, const char* szAddition, int iAdditionLength)
{
    const char* p = szAddition + iAdditionLength - 1;
    while ((p >= szAddition) && isspace(*p))
        --p;
    if (p >= szAddition)
        strncat(szBase, szAddition, p - szAddition + 1);
}

// allocate and populate a long filename (not including path information)
char* MakeLongFilename(DSTAT& stat)
{
    char* filename = 0;
    if(stat.longFileName[0] != '\0')
    {
        char* p = stat.longFileName;
        make_string( filename,  strlen( p ) + 1);
		strcpy( filename, p );
    }
	else
    {
        if (!isspace(stat.fext[0]))
        {
            make_string( filename, strlen(stat.fname) + 1 + strlen(stat.fext) + 1);
            filename[0] = 0;
            // Remove trailing spaces.
            AddStrippedString(filename, stat.fname, 8);
            strcat(filename, ".");
            AddStrippedString(filename, stat.fext, 3);
        }
        else
        {
            make_string( filename, strlen(stat.fname) + 1);
            filename[0] = 0;
            // Remove trailing spaces.
            AddStrippedString(filename, stat.fname, 8);
        }
    }
    return filename;
}

// allocate and populate a full path URL
char* GenerateURL(const char* szRoot, DSTAT& statobj)
{
    char* filename = 0;
    if(statobj.longFileName[0] != '\0')
    {
        char* p = statobj.longFileName;
        make_string( filename, strlen(szRoot) + strlen( p ) + (statobj.fattribute & ADIRENT ? 1 /* \\ */ : 0) + 1);
		sprintf( filename, "%s%s%s", szRoot, p, statobj.fattribute & ADIRENT ? "\\" : "");
    }
	else
    {
        if (!isspace(statobj.fext[0]) && !isspace(statobj.fext[1]) && !isspace(statobj.fext[2]))
        {
            make_string( filename, strlen(szRoot) + strlen(statobj.fname) + strlen(statobj.fext) + (statobj.fattribute & ADIRENT ? 1 /* \\ */ : 0) + 1);
            strcpy(filename, szRoot);
            // Remove trailing spaces.
            AddStrippedString(filename, statobj.fname, 8);
            strcat(filename, ".");
            AddStrippedString(filename, statobj.fext, 3);
            if (statobj.fattribute & ADIRENT)
                strcat(filename, "\\");
        }
        else
        {
            make_string( filename, strlen(szRoot) + strlen(statobj.fname) + (statobj.fattribute & ADIRENT ? 1 /* \\ */ : 0) + 1);
            strcpy(filename, szRoot);
            // Remove trailing spaces.
            AddStrippedString(filename, statobj.fname, 8);
            if (statobj.fattribute & ADIRENT)
                strcat(filename, "\\");
        }
    }
    return filename;
}
// just check the extension passed in and rate it as a playlist file or not.
bool IsPlaylistExtension(char* ext)
{
    return (CPlaylistFormatManager::GetInstance()->FindPlaylistFormat(ext) != 0);
}

void DumpStat(DSTAT& stat)
{
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "Dir Entry Info*\n");
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "path [%s]\n", stat.path);
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "LFN  [%s]\n", stat.longFileName);
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "fname[%s]\n", stat.fname);
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "fext [%s]\n", stat.fext);
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "pname[%s]\n", stat.pname);
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "pext [%s]\n", stat.pext);
}

// note this appends a *.* for file search reasons.
char* GeneratePath(DSTAT& stat)
{
    int len = strlen( stat.path ) + 1 /* \ */ + strlen( stat.fname )
        + 1 /* . */ + strlen( stat.fext ) + 5 /* \*.*\0 */;
    // dc - make sure we dont push a dir that we can't possible reference files from
    char* ret = 0;
    if( (len + 12) < EMAXPATH )
    {
        make_string( ret, len );
        if (strlen(stat.fext))
            sprintf( ret, "%s\\%s.%s\\*.*", stat.path, stat.fname, stat.fext );
        else
            sprintf( ret, "%s\\%s\\*.*", stat.path, stat.fname );
    }
    return ret;
}
