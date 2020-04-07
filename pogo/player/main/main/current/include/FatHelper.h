#ifndef _FAT_HELPER_HEADER_
#define _FAT_HELPER_HEADER_

#include <fs/fat/sdapi.h>
#include <util/datastructures/SimpleList.h>
#include <main/util/datastructures/SortList.h>

typedef SortList<const char*> StringList;
typedef SimpleListIterator<const char*> StringListIterator;

// return a pointer into fn, just past the period that demarcates the beginning of the file extension.
char* ExtensionFromFilename(char* fn);
// return a pointer into szURL pointing to just the last path segment, ie the filename.
char* FilenameFromURL(const char* szURL);
// return the length of the url, except for the last path segment (so the parent dir's path length)
int PathlenFromURL(const char* szURL);
// push the subfolder segment onto the path, assuming szPath is constrained to EMAXPATH chars.
bool PushPathSegment(char* szPath, char* szSubFolder);
// walk back from the end of szPath and place a null just before the last path segment
void PopPathSegment(char* szPath);
// trim spaces off the end of str, by placing a null in the first trailing space
void TrimInPlace(char* str);
// run TrimInPlace on the fname and fext fields of the given stat.
void TrimStatNames(DSTAT& stat);
// check if the given extension is a supported playlist file extension.
bool IsPlaylistExtension(char* ext);
// append a url segment, indicated by szSubFolder, to pszURL.  nAllocSize indicates the current allocation size, and may be re-allocated if the segment exceeds this room.
void PushURLSegment(char** pszURL, int &nAllocSize, char* szSubFolder);
// generate a full path into a new string, given a stat.
char* GeneratePath(DSTAT& stat);
// remove any trailing spaces from szAddition and append it to szBase.
void AddStrippedString(char* szBase, const char* szAddition, int iAdditionLength);
// given a stat object pointing to a file, generate a new string containing the long name, sans path.
char* MakeLongFilename(DSTAT& stat);
// given a stat object pointing to a file, compute the byte count of the short filename.
int LengthOfShortFilename(DSTAT& stat);
// given a stat obj pointing at a file, and a string in which to place the name, fill it in.
char* MakeShortFilename(DSTAT& stat, char* szName);
// dynamically allocate a new full-url string given the url of the parent dir, and a stat obj pointing to the file.
char* GenerateURL(const char* szRoot, DSTAT& statobj);
// print out a report on the given stat object.
void DumpStat(DSTAT &stat);
// given a path, separate it into segments by slashes, and return a list of the chunks, dynamically allocated, not just pointers into path.
StringList TokenizePath(char* path);
// given a fat-style path, generate a new string containing the equivalent URL.
char* URLFromPath(const char* szPath);
// given a url, generate a new string containing the full path.
char* PathFromURL(const char* url);
// check that the directory specified in path exists, and create it if it didn't already exist.
void VerifyOrCreateDirectory(char* path);
// modify the file at path to have the hidden attribute.
void HideFile(char* path);
// check that a file exists and has the normal file type.
bool FileExists(char* url);
// check that a dir exists and has the normal directory type.
bool DirExists(char* url);
// check that a directory path is within the length limits.
bool ValidateShortPathLength(DSTAT& stat);
// return a pointer into a full url where the long path begins (just past the 'file://' part)
char* LongPathFromFullURL(char* url);

#define make_string(var,len) ( var = (char*) malloc(sizeof(char)* (len)) )
#define make_wide_string(var,len) ( var = (unsigned short*) malloc(sizeof(unsigned short)* (len)) )
#define free_string(adr) free( adr )
#define shift_upper(a) (a >= 'a' && a <= 'z' ? a - ('a' - 'A') : a )

#endif // _FAT_HELPER_HEADER_