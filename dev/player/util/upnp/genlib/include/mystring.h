#ifndef MYSTRING_H_
#define MYSTRING_H_

#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

int strncasecmp(const char *, const char *, size_t);
int strcasecmp(const char *, const char *);

#ifdef __cplusplus
}
#endif

#endif	// MYSTRING_H_
