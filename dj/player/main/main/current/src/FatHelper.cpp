#include <main/main/FatHelper.h>
#include <core/playmanager/PlayManager.h>
#include <fs/fat/sdapi.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <main/main/AppSettings.h>     // PLAYLIST_STRING_SIZE
#include <util/datastructures/SimpleList.h>

#include <ctype.h>      /* isspace */
#include <stdlib.h>     /* malloc */
#include <stdio.h>      /* sprintf */

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_FAT_HELPER, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_FAT_HELPER);

#define make_string(var,len) ( var = new char[len] )
#define make_wide_string(var,len) ( var = new unsigned short[len] )
#define free_string(adr) delete [] adr
#define shift_upper(a) (a >= 'a' && a <= 'z' ? a - ('a' - 'A') : a )

// return a pointer to just the filename part of the url
const char* FilenameFromURLInPlace(const char* szURL)
{
    char* pch = strrchr(szURL, '/');
    return pch ? pch + 1: szURL;
}

// return a pointer to just the filename part of the url, in the same string
const char* FullFilenameFromURLInPlace(const char* szURL)
{
    return szURL + 7 /* file:// */;
}

// trim trailing spaces from the string elements in pstat
void TrimStat(DSTAT* pstat)
{   
    TrimInPlace(pstat->fname);
    TrimInPlace(pstat->fext);
    TrimInPlace(pstat->path);
}

// add a subfolder to a full path sequence (while moving down into subfolders)
bool
PushPathSegment(char* szPath, char* szSubFolder)
{
    // if the folder is so deep that its path is unusable, then fail
    if (strlen(szPath) + strlen(szSubFolder) + 5 > PLAYLIST_STRING_SIZE)
        return false;
    strcat(szPath,szSubFolder);
    strcat(szPath,"/");
    return true;
}

// push a path segment onto pszURL
void PushURLSegment(char** pszURL, int &nAllocSize, char* szSubFolder)
{
    if (!*pszURL)
    {
        nAllocSize += 64;
        *pszURL = new char[nAllocSize];
        *pszURL[0] = 0;
    }
    if (strlen(*pszURL) + strlen(szSubFolder) + 2 > nAllocSize)
    {
        while (strlen(*pszURL) + strlen(szSubFolder) + 2 > nAllocSize)
            nAllocSize += 64;
        char* temp = new char[nAllocSize];
        strcpy (temp,*pszURL);
        delete [] *pszURL;
        *pszURL = temp;
    }
    strcat (*pszURL,szSubFolder);
    strcat (*pszURL,"/");
}

// remove a subfolder from a full path (while moving up towards the folder root)
void
PopPathSegment(char* szPath)
{
    int index = strlen(szPath) - 1;
    while (szPath[index-1] != '/')
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
        make_string( filename, strlen(szRoot) + strlen( p ) + (statobj.fattribute & ADIRENT ? 1 /* / */ : 0) + 1);
		sprintf( filename, "%s%s%s", szRoot, p, statobj.fattribute & ADIRENT ? "/" : "");
    }
	else
    {
        if (!isspace(statobj.fext[0]) && !isspace(statobj.fext[1]) && !isspace(statobj.fext[2]))
        {
            make_string( filename, strlen(szRoot) + strlen(statobj.fname) + 1 /* . */  + strlen(statobj.fext) + (statobj.fattribute & ADIRENT ? 1 /* / */ : 0) + 1);
            strcpy(filename, szRoot);
            // Remove trailing spaces.
            AddStrippedString(filename, statobj.fname, 8);
            strcat(filename, ".");
            AddStrippedString(filename, statobj.fext, 3);
            if (statobj.fattribute & ADIRENT)
                strcat(filename, "/");
        }
        else
        {
            make_string( filename, strlen(szRoot) + strlen(statobj.fname) + (statobj.fattribute & ADIRENT ? 1 /* / */ : 0) + 1);
            strcpy(filename, szRoot);
            // Remove trailing spaces.
            AddStrippedString(filename, statobj.fname, 8);
            if (statobj.fattribute & ADIRENT)
                strcat(filename, "/");
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
    int len = strlen( stat.path ) + 1 /* / */ + strlen( stat.fname )
        + 1 /* . */ + strlen( stat.fext ) + 5 /* /*.*\0 */;
    // dc - make sure we dont push a dir that we can't possibly reference files from
    char* ret = 0;
    if( (len + 12) < EMAXPATH )
    {
        make_string( ret, len );
        if (strlen(stat.fext))
            sprintf( ret, "%s/%s.%s/*.*", stat.path, stat.fname, stat.fext );
        else
            sprintf( ret, "%s/%s/*.*", stat.path, stat.fname );
    }
    return ret;
}

typedef SimpleList<const char*> StringList;
typedef SimpleListIterator<const char*> StringListIterator;

bool IsValidPathRootSegment(const char* segment)
{
    if ( (segment[0] != 'A' && segment[0] != 'a')
       ||(segment[1] != ':'))
       return false;
    return true;
}

// given a partial URL, and a path segment that goes on the end, convert the
//  path-based segment to a url version, and append it to the partial url.
bool PushPathSegmentOntoURL(const char* segment, char* &url, int &nURLLen)
{
    DSTAT stat;
    int masklen = 0;
    if (url)
        masklen = strlen(url) - 7 + strlen(segment) + 1;
    else
        masklen = strlen(segment) + 2;
    char mask[masklen];
    // deal with the root case
    if (url == 0)
    {
        if (!IsValidPathRootSegment(segment))
            return false;
        // put the root onto the url.
        PushURLSegment(&url,nURLLen,ROOT_URL_PATH);
        return true;
    }
    strcpy (mask, url+7);
    strcat (mask, segment);
    if (!pc_gfirst(&stat, mask))
    {
        pc_gdone(&stat);
        return false;
    }
    // if this entry matches our segment, succeed.
    // (epg,11/1/2001): TODO: check that we don't need case insensitivity here!
    char* lfn = MakeLongFilename(stat);
    pc_gdone(&stat);
    PushURLSegment(&url,nURLLen,lfn);
    delete [] lfn;
    return true;
}

void PrintStringList(StringList lst)
{
    int i = 0;
    for (StringListIterator str = lst.GetHead(); str != lst.GetEnd(); ++str)
    {
        DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "%02d:%s\n",i,(*str));
        ++i;
    }
}

// return a list of all path segments found in path.  should work for both urls and fat paths.
StringList TokenizePath(char* path)
{
    StringList tokens;
    char* tstart = path;
    char* tend = path;
    while (*tstart)
    {
        ++tend;
        if (*tend == '\\' || *tend == '/' || *tend == 0)
        {
            char* token = new char[tend - tstart + 1];
            strncpy (token,tstart,tend - tstart);
            token[tend-tstart] = 0;
            tokens.PushBack(token);
            while (*tend == '\\' || *tend == '/') ++tend;
            tstart = tend;
        }
    }
    //PrintStringList(tokens);
    return tokens;
}

// delete all member strings in the given list.
void DeleteStringListElements(StringList lst)
{
    for (StringListIterator str = lst.GetHead(); str != lst.GetEnd(); ++str)
        delete [] (*str);
}

// return a url, given the equivalent fat fs style path.
char* URLFromPath(const char* path)
{
    DEBUGP(DBG_FAT_HELPER, DBGLEV_TRACE, "getting url for %s\n",path);
    char* url = 0;
    int nURLLen = 0;
    
    // break the url into the path pieces
    StringList segments = TokenizePath((char*)path);
    // process each path segment into the url version of the same.
    while (segments.Size())
    {
        const char* segment = segments.PopFront();
        if (!PushPathSegmentOntoURL(segment, url, nURLLen))
        {
            delete [] segment;
            DeleteStringListElements(segments);
            if (url)
                delete [] url;
            return 0;
        }
        delete [] segment;
    }
    // remove the trailing backslash
    url[strlen(url) -1] = 0;
    DEBUGP(DBG_FAT_HELPER, DBGLEV_TRACE, "url is %s\n",url);
    return (char*) url;
}

// return a fat fs style path, given the equivalent url.
char* PathFromURL(const char* url)
{
    char* path = (char*) url+7;
    DSTAT stat;
    if (pc_gfirst(&stat,path))
    {
        path = GeneratePath(stat);
        // chop off the trailing \\*.* added by GenPath
        path[strlen(path) - 4] = 0;
    }
    else
        path = 0;

    pc_gdone(&stat);
    return path;

}

bool VerifyOrCreateDirectory(char* path)
{
    if (!pc_isdir(path))
    {
        // The directory doesn't exist, so parse out the individual subdirectories
        // since the sdapi is too blindingly stupid to do it for us.
        // Note: the path must have a leading slash, e.g., '/blah/blah', 'a:/blah', etc.
        if (char *pch = strchr(path, '/'))
        {
            while (char* pchn = strchr(pch + 1, '/'))
            {
                *pchn = '\0';
                if (!pc_isdir(path))
                {
                    if (!pc_mkdir(path))
                    {
                        DEBUG( DBG_FAT_HELPER, DBGLEV_ERROR, "Unable to create directory: %s\n", path);
                        return false;
                    }
                }
                *pchn = '/';
                pch = pchn;
            }

            if (pch + 1 != '\0')
            {
                // No trailing slash, so make the last directory.
                if (!pc_mkdir(path))
                {
                    DEBUG( DBG_FAT_HELPER, DBGLEV_ERROR, "Unable to create directory: %s\n", path);
                    return false;
                }
            }
        }
    }
    return true;
}

char* ExtensionFromFilename(char* fn)
{
    char* ext = fn+(strlen(fn)-1);
    while (*(ext-1) != '.' && ext > fn) ext--;
    return ext;
}

// checks that the file exists
bool FileExists(const char* fn)
{
    DSTAT dstat;
    if (pc_gfirst(&dstat, const_cast<char*>(fn)))
    {
        pc_gdone(&dstat);
        return true;
    }
    pc_gdone(&dstat);

    return false;
}

// mirror a CD path to an hd path
char* CDPathToHDPath(char *pszCDPath, char* pszHDPath)
{
	strcpy(pszHDPath, "a:");
	strcat(pszHDPath, pszCDPath);

	// diag_printf("converted %s to %s\n", pszCDPath, pszHDPath);

	return pszHDPath;
}
