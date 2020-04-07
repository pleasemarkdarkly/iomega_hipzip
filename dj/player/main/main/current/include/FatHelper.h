#ifndef _FAT_HELPER_HEADER_
#define _FAT_HELPER_HEADER_

#include <fs/fat/sdapi.h>

// return a pointer into fn, just past the period that demarcates the beginning of the file extension.
char* ExtensionFromFilename(char* fn);
// return a pointer into szURL pointing to just the last path segment, the filename.
const char* FilenameFromURLInPlace(const char* szURL);
// return a pointer into szURL that bypasses the URL prefix ('file://")
const char* FullFilenameFromURLInPlace(const char* szURL);
// push the subfolder segment onto the path, assuming szPath is constrained to EMAXPATH chars.
bool PushPathSegment(char* szPath, char* szSubFolder);
// walk back from the end of szPath and place a null just before the last path segment
void PopPathSegment(char* szPath);
// append a url segment, indicated by szSubFolder, to pszURL.  nAllocSize indicates the current allocation size, and may be re-allocated if the segment exceeds this room.
void PushURLSegment(char** pszURL, int &nAllocSize, char* szSubFolder);
// append a url segment, indicated by szSubFolder, to pszURL.  nAllocSize indicates the current allocation size, and may be re-allocated if the segment exceeds this room.
void PushURLSegment(char** pszURL, int &nAllocSize, char* szSubFolder);
// trim spaces off the end of str, by placing a null in the first trailing space
void TrimInPlace(char* str);
// trim trailing spaces from the string elements in pstat
void TrimStat(DSTAT* pstat);
// check if the given extension is a supported playlist file extension.
bool IsPlaylistExtension(char* ext);
// generate a full path into a new string, given a stat.
char* GeneratePath(DSTAT& stat);
// remove any trailing spaces from szAddition and append it to szBase.
void AddStrippedString(char* szBase, const char* szAddition, int iAdditionLength);
// given a stat object pointing to a file, generate a new string containing the long name, sans path.
char* MakeLongFilename(DSTAT& stat);
// dynamically allocate a new full-url string given the url of the parent dir, and a stat obj pointing to the file.
char* GenerateURL(const char* szRoot, DSTAT& statobj);
// print out a report on the given stat object.
void DumpStat(DSTAT &stat);
// given a fat-style path, generate a new string containing the equivalent URL.
char* URLFromPath(const char* szPath);
// given a url, generate a new string containing the full path.
char* PathFromURL(const char* url);
// check that the directory specified in path exists, and create it if it didn't already exist.
bool VerifyOrCreateDirectory(char* path);
// checks that the file exists
bool FileExists(const char* fn);
// mirror a CD path to an hd path
char* CDPathToHDPath(char *pszCDPath, char* pszHDPath);

#endif // _FAT_HELPER_HEADER_
