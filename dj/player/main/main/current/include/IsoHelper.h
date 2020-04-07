#ifndef _ISO_HELPER_HEADER_
#define _ISO_HELPER_HEADER_


bool IsCDDA(const char* szURL);

// Return a pointer to just the filename part of the url
const char* FilenameFromIsoURLInPlace(const char* szURL);

#endif // _ISO_HELPER_HEADER_
