#include <main/main/FatHelper.h>
#include <core/playmanager/PlayManager.h>
#include <fs/fat/sdapi.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <main/main/AppSettings.h>     // PLAYLIST_STRING_SIZE

#include <ctype.h>      /* isspace */
#include <stdlib.h>     /* malloc */
#include <stdio.h>      /* sprintf */

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_FAT_HELPER, DBGLEV_DEFAULT | DBGLEV_INFO );
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
    strcat(szPath,"\\");
    return true;
}
// push a segment onto the current URL member.
void PushURLSegment(char** pszURL, int &nAllocSize, char* szSubFolder)
{
    if (!*pszURL)
    {
        nAllocSize += 64;
        *pszURL = (char*)malloc(nAllocSize);
        *pszURL[0] = 0;
    }
    if (strlen(*pszURL) + strlen(szSubFolder) + 2 > nAllocSize)
    {
        while (strlen(*pszURL) + strlen(szSubFolder) + 2 > nAllocSize)
            nAllocSize += 64;
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
        if (!isspace(stat.fext[0]) && stat.fext[0] != 0)
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
char* GenerateURL(const char* szRoot, DSTAT& stat)
{
    char* filename = 0;
    if(stat.longFileName[0] != '\0')
    {
        char* p = stat.longFileName;
        make_string( filename, strlen(szRoot) + strlen( p ) + (stat.fattribute & ADIRENT ? 1 /* \\ */ : 0) + 1);
		sprintf( filename, "%s%s%s", szRoot, p, stat.fattribute & ADIRENT ? "\\" : "");
    }
	else
    {
        if (!isspace(stat.fext[0]) && !isspace(stat.fext[1]) && !isspace(stat.fext[2]))
        {
            make_string( filename, strlen(szRoot) + strlen(stat.fname) + 1 /* . */  + strlen(stat.fext) + (stat.fattribute & ADIRENT ? 1 /* \\ */ : 0) + 1);
            strcpy(filename, szRoot);
            // Remove trailing spaces.
            AddStrippedString(filename, stat.fname, 8);
            strcat(filename, ".");
            AddStrippedString(filename, stat.fext, 3);
            if (stat.fattribute & ADIRENT)
                strcat(filename, "\\");
        }
        else
        {
            make_string( filename, strlen(szRoot) + strlen(stat.fname) + (stat.fattribute & ADIRENT ? 1 /* \\ */ : 0) + 1);
            strcpy(filename, szRoot);
            // Remove trailing spaces.
            AddStrippedString(filename, stat.fname, 8);
            if (stat.fattribute & ADIRENT)
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
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "Dir Entry Info:\n");
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "path [%s]\n", stat.path);
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "LFN  [%s]\n", stat.longFileName);
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "fname[%s]\n", stat.fname);
    DEBUGP(DBG_FAT_HELPER, DBGLEV_INFO, "fext [%s]\n", stat.fext);
}

// note this appends a *.* for file search reasons.
char* GeneratePath(DSTAT& stat)
{
    int len = strlen( stat.path ) + 1 /* \ */ + strlen( stat.fname )
        + 1 /* . */ + strlen( stat.fext ) + 5 /* \*.*\0 */;
    // dc - make sure we dont push a dir that we can't possibly reference files from
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
        return false;
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
    return path;

}

char* ExtensionFromFilename(char* fn)
{
    char* ext = fn+(strlen(fn)-1);
    while (*(ext-1) != '.' && ext > fn) ext--;
    return ext;
}

void HideFile(char* path)
{
    UINT16 attribs;
    if (pc_get_attributes(path,&attribs))
    {
        attribs |= AHIDDEN;
        pc_set_attributes(path,attribs);
    }
}

void VerifyOrCreateDirectory(char* path)
{
    if (!DirExists(path))
        pc_mkdir(path);
}

bool FileExists(char* url)
{
    UINT16 attribs;
    if (pc_get_attributes(url, &attribs) == YES)
        if (!(attribs & ADIRENT))
            return true;
    return false;
}

bool DirExists(char* url)
{
    UINT16 attribs;
    if (pc_get_attributes(METAKIT_PERSIST_PATH, &attribs) == YES)
        if (attribs & ADIRENT)
            return true;
    return false;
}

// given a stat object pointing to a file, compute the byte count of the short filename.
int LengthOfShortFilename(DSTAT& stat)
{
    TrimStatNames(stat);
    if (strlen(stat.fext))
        return strlen(stat.fname) + strlen(stat.fext) + 1;
    else
        return strlen(stat.fname);
}

// given a stat obj pointing at a file, and a string in which to place the name, fill it in.
// generally used in concert with LengthOfShortFilename.  if not, the TrimInPlace should be run on the stat.fname and fext.
char* MakeShortFilename(DSTAT& stat, char* szName)
{
    if (strlen(stat.fext))
        sprintf( szName, "%s.%s", stat.fname, stat.fext );
    else
        sprintf( szName, "%s", stat.fname );
}

// run TrimInPlace on the fname and fext fields of the given stat.
void TrimStatNames(DSTAT& stat)
{
    TrimInPlace(stat.fname);
    TrimInPlace(stat.fext);
}

// check that a short path is within the length limits
bool ValidateShortPathLength(DSTAT& stat)
{
    int len = strlen( stat.path ) + 1 /* \ */ + LengthOfShortFilename(stat) + 2 /* \\0 */;
    return ( (len + 12) <= EMAXPATH );
}

// return a pointer into a full url where the long path begins (just past the 'file://' part)
char* LongPathFromFullURL(char* url)
{
    return url + 7;
}
