#include <stdlib.h>
#include <string.h>
#include <util/upnp/genlib/encode.h>

char Encode(BYTE b, bool fURLFriendly)
{
	if (b >= 0 && b < 26) return 'A' + b;
	if (b > 25 && b < 52) return 'a' + b - 26;
	if (b > 51 && b < 62) return '0' + b - 52;
	if (b == 62 && !fURLFriendly) return '+';
    if (b == 62 && fURLFriendly) return '!';
	if (b == 63 && !fURLFriendly) return '/';
    if (b == 63 && fURLFriendly) return '*';
	return '\0';
}

bool Encode3(BYTE bIn[], char cOut[], bool fURLFriendly)
{
	if (!bIn || !cOut)
		return false;
	cOut[0] = Encode(bIn[0] >> 2, fURLFriendly);
	cOut[1] = Encode(((bIn[0] & 3) << 4) | (bIn[1] >> 4), fURLFriendly);
	cOut[2] = Encode(((bIn[1] << 2) & 0x3c) | (bIn[2] >> 6), fURLFriendly);
	cOut[3] = Encode(bIn[2] & 0x3f, fURLFriendly);
	return true;
}

bool Encode2(BYTE bIn[], char cOut[], bool fURLFriendly)
{
	if (!bIn || !cOut)
		return false;
	cOut[0] = Encode(bIn[0] >> 2, fURLFriendly);
	cOut[1] = Encode(((bIn[0] & 3) << 4) | (bIn[1] >> 4), fURLFriendly);
	cOut[2] = Encode((bIn[1] << 2) & 0x3c, fURLFriendly);
	cOut[3] = '=';
	return true;
}

bool Encode1(BYTE bIn, char cOut[], bool fURLFriendly)
{
	if (!cOut)
		return false;
	cOut[0] = Encode(bIn >> 2, fURLFriendly);
	cOut[1] = Encode((bIn & 3) << 4, fURLFriendly);
	cOut[2] = cOut[3] = '=';
	return true;
}

bool HrBlobTo64Sz(BYTE bIn[], DWORD cbIn, char** cOut)
{
	long j, ccOut;
	DWORD i;

	if (!bIn || !cOut)
		return false;
	ccOut = (cbIn / 3) * 4;
	if (cbIn % 3) ccOut += 4;
	if (!(*cOut = new char[ccOut + 1]))
		return false;

	for (i = 0, j = 0; i < cbIn; i += 3, j += 4)
	{
		if (i + 3 <= cbIn)
		{
			if (!Encode3(&bIn[i], &(*cOut)[j], false))
				return false;
		}
		else if (i + 2 == cbIn)
		{
			if (!Encode2(&bIn[i], &(*cOut)[j], false))
				return false;
		}
		else if (!Encode1(bIn[i], &(*cOut)[j], false))
			return false;
	}
	(*cOut)[ccOut] = '\0';

	return true;;
}

#ifdef USE_DECODE

BYTE Decode(const char c, bool fURLFriendly)
{
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a' + 26;
	if (c >= '0' && c <= '9') return c - '0' + 52;
    // To be compatible with DRM v1,
    // it will accept all +, !, /, * for the case of DRM
	if (c == '+') return 62;
    if (c == '!' && fURLFriendly) return 62;
	if (c == '/') return 63;
    if (c == '*' && fURLFriendly) return 63;
	return 127;
}

bool Decode4(const char cIn[], BYTE *bOut, bool fURLFriendly)
{
	BYTE cOut[4];
	int i;

	if (!cIn || !bOut)
	{
		return false;
	}
	for (i = 0; i < 4; i++)
	{
		cOut[i] = Decode(cIn[i], fURLFriendly);
		if (cOut[i] == 127)
		{
			return false;
		}
	}
	bOut[0] = ((cOut[0] << 2) | (cOut[1] >> 4));
	bOut[1] = ((cOut[1] << 4) | (cOut[2] >> 2));
	bOut[2] = ((cOut[2] << 6) | cOut[3]);

	return true;
}

bool DecodeLast4(const char cIn[], BYTE *bOut, DWORD *pcbOut, bool fURLFriendly)
{
	BYTE cOut[4];
	int i;

	if (!cIn || !bOut || !pcbOut)
	{
		return false;
	}
	for (i = 0; i < 4; i++)
	{
		if (cIn[i] == '=') continue;
		cOut[i] = Decode(cIn[i], fURLFriendly);
		if (cOut[i] == 127)
		{
			return false;
		}
	}

	if (cIn[0] == '=' || cIn[1] == '=')
	{
		return false;
	}

	if (cIn[2] == '=' && cIn[3] == '=')
	{
		bOut[0] = ((cOut[0] << 2) | (cOut[1] >> 4));
		*pcbOut = 1;
	}
	else if (cIn[3] == '=')
	{
		bOut[0] = ((cOut[0] << 2) | (cOut[1] >> 4));
		bOut[1] = ((cOut[1] << 4) | (cOut[2] >> 2));
		*pcbOut = 2;
	}
	else
	{
		bOut[0] = ((cOut[0] << 2) | (cOut[1] >> 4));
		bOut[1] = ((cOut[1] << 4) | (cOut[2] >> 2));
		bOut[2] = ((cOut[2] << 6) | cOut[3]);
		*pcbOut = 3;
	}

	return true;
}

bool Hr64SzToBlob(char* cIn, BYTE **pbOut, DWORD *pcbOut)
{
	DWORD dwCount = 0, ccIn = 0;
	DWORD i = 0;

    if (!cIn) return false;
    if (!pcbOut) return false;
    if (!pbOut) return false;

    *pcbOut = 0;
    *pbOut = NULL;

	ccIn = strlen(cIn);
	if (ccIn < 4 || (ccIn % 4))
	{
		return false;
	}
	*pcbOut = (ccIn / 4) * 3;
	if (!(*pbOut = new BYTE[*pcbOut]))
	{
		return false;
	}
	memset((void*)*pbOut, 0, *pcbOut);
	*pcbOut = 0;
	for (i = 0; i < ccIn - 4; i += 4)
	{
		if( !Decode4(&cIn[i], &(*pbOut)[*pcbOut], false) )
        {
            goto Error;
        }
        *pcbOut += 3;
	}
    if( !DecodeLast4(&cIn[i], &(*pbOut)[*pcbOut], &dwCount, false) )
    {
        goto Error;
    }

	*pcbOut += dwCount;

	return true;

Error:

	if (*pbOut) delete *pbOut;
	*pbOut = NULL;
	*pcbOut = 0;
	return false;
}

#endif	// USE_DECODE
