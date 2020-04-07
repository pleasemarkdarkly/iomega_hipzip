#include <main/main/IsoHelper.h>

#include <string.h>

#include <util/debug/debug.h>

bool IsCDDA(const char* szURL)
{
    // This only works for DJ.
    return !strncmp(szURL, "file://cdda?", 12);
}

// Return a pointer to just the filename part of the url
const char* FilenameFromIsoURLInPlace(const char* szURL)
{
    char* pch = strrchr(szURL, '/');
    return pch ? pch + 1 : szURL;
}

