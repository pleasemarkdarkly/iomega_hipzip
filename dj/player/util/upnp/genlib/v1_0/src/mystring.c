#include <util/upnp/genlib/mystring.h>

#include <string.h>

int strncasecmp(const char *szA, const char *szB, size_t n)
{
	int i;
	int res = 0;
	for (i = 0; (i < n) && *szA && *szB; ++i, ++szA, ++szB)
	{
/*
		char ua = toupper(szA[i]), ub = toupper(szB[i]);
		if (ua < ub)
			return -1;
		else if (ua > ub)
			return 1;
		else if (ua == 0)
			return 0;
*/
		res = (int)(tolower(*szA) - tolower(*szB));
		if (res != 0)
			break;
	}
//	return 0;
	if (i == n)
		return 0;

    if (!*szA) {
        if (!*szB)  return 0;
        return -1;
    }
    if (!*szB)
        return 1;

	return res;
}

int strcasecmp(const char *szA, const char *szB)
{
	int len = strlen(szA) > strlen(szB) ? strlen(szA) : strlen(szB);
	return strncasecmp(szA, szB, len);
}
