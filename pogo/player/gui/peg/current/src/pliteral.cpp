/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pliteral.cpp - Literal strings used by PEG. Defined for either PEG_UNICODE
//  or normal 8-bit ASCII encoding.
//
// Author: Kenneth G. Maxwell
//
// Copyright (c) 1997-2000 Swell Software 
//              All Rights Reserved.
//
// Unauthorized redistribution of this source code, in whole or part,
// without the express written permission of Swell Software
// is strictly prohibited.
//
// Notes:
//
// This file contains every string literal used by PEG. Each string is
// encoded both in 8-bit and in 16-bit form.
// 
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#include <gui/peg/peg.hpp>

#ifdef PEG_UNICODE

TCHAR lsOK[] = {'O','K','\0'};
TCHAR lsNO[] = {'N','o','\0'};
TCHAR lsYES[] = {'Y','e','s','\0'};
TCHAR lsABORT[] = {'A','b','o','r','t','\0'};
TCHAR lsCANCEL[] = {'C','a','n','c','e','l','\0'};
TCHAR lsRETRY[] =  {'R','e','t','r','y','\0'};
TCHAR lsCLOSE[] = {'C','l','o','s','e','\0'};
TCHAR lsAPPLY[] = {'A','p','p','l','y', 0};
TCHAR lsTEST[]   = {'A','\0'};
TCHAR lsMINIMIZE[] = {'M','i','n','i','m','i','z','e','\0'};
TCHAR lsMAXIMIZE[] = {'M','a','x','i','m','i','z','e','\0'};
TCHAR lsNULL[]     = {'\0'};
TCHAR lsRESTORE[]  = {'R','e','s','t','o','r','e','\0'};
TCHAR lsSPACE[]    = {' ','\0'};
TCHAR lsHELP[]     = {'H','e','l','p', 0};

#ifdef  PEG_IMAGE_CONVERT
TCHAR lsNotAllowed[] = {'I','n','l','i','n','e', ' ','C','o','n','v','e','r','s','i','o','n',' ',
    'o','f', ' ','2','4','b','p','p',' ','i','m','a','g','e','s',' ',
    'i','s',' ','n','o','t',' ','s','u','p','p','o','r','t','e','d','\0'};
TCHAR lsBadMagic[] = {'E','r','r','o','r',' ','r','e','a','d','i',
    'n','g',' ','m','a','g','i','c',' ','n','u','m','b','e','r','.','\0'};
TCHAR lsNotAGif[] = {'N','o','t',' ','a',' ','G','I','F',' ','f',
    'i','l','e','.','\0'};
TCHAR lsBadGifVer[] = {'B','a','d',' ','v','e','r','s','i','o','n',
    ' ','n','u','m','b','e','r',',',' ','n','o','t',' ',
    '\'','8','7','a','\'',' ','o','r',' ','\'','8','9','a','\'','.','\0'};
TCHAR lsScrnDescFail[] = {'F','a','i','l','e','d',' ','t','o',
    ' ','r','e','a','d',' ','s','c','r','e','e','n',' ',
    'd','e','s','c','r','i','p','t','o','r','.','\0'};
TCHAR lsGifRdErr[] = {'R','e','a','d',' ','e','r','r','o','r',' ',
    'o','n',' ','i','m','a','g','e',' ','d','a','t','a','.','\0'};
TCHAR lsBadColormap[] = {'B','a','d',' ','C','o','l','o','r','m','a','p','\0'};
TCHAR lsOutOfMem[] = {'O','u','t',' ','O','f',' ',
    'M','e','m','o','r','y','\0'};
TCHAR lsUnsupported[] = {'U','n','s','u','p','p','o','r','t','e','d',
    ' ','I','m','a','g','e',' ','T','y','p','e',0};
#endif

#else

TCHAR lsOK[] = "OK";
TCHAR lsNO[] = "No";
TCHAR lsYES[] = "Yes";
TCHAR lsABORT[] = "Abort";
TCHAR lsCANCEL[] = "Cancel";
TCHAR lsRETRY[] = "Retry";
TCHAR lsCLOSE[] = "Close";
TCHAR lsAPPLY[] = "Apply";
TCHAR lsTEST[]   = "A";
TCHAR lsMINIMIZE[] = "Minimize";
TCHAR lsMAXIMIZE[] = "Maximize";
TCHAR lsNULL[]     = "";
TCHAR lsRESTORE[]  = "Restore";
TCHAR lsSPACE[]    = " ";
TCHAR lsHELP[] = "Help";

#ifdef PEG_IMAGE_CONVERT
TCHAR lsNotAllowed[] = "Inline Conversion of 24bpp images is not allowed";
TCHAR lsBadMagic[] = "Error reading magic number.";
TCHAR lsNotAGif[] = "Not a GIF file.";
TCHAR lsBadGifVer[] = "Bad version number, not '87a' or '89a'.";
TCHAR lsScrnDescFail[] = "Failed to read screen descriptor.";
TCHAR lsGifRdErr[] = "Read error on image data.";
TCHAR lsBadColormap[] = "Bad Colormap";
TCHAR lsOutOfMem[] = "Out Of Memory";
TCHAR lsUnsupported[] = "Unsupported Image Type";
#endif

#endif

#ifdef PEG_STRLIB

/*--------------------------------------------------------------------------*/
//
// The PEG string library- Not a full ANSI string manipulation library!! This
// library provides all of the string functions used by PEG, in a format
// capable of supporting both 8 and 16 bit characters.
//
/*--------------------------------------------------------------------------*/
TCHAR *PegStrCat(TCHAR *s1, const TCHAR *s2)
{
    TCHAR *sr = s1;
    while(*s1)
    {
        s1++;
    }
    while(*s2)
    {
        *s1++ = *s2++;
    }
    *s1 = *s2;
    return sr;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
TCHAR *PegStrnCat(TCHAR *s1, const TCHAR *s2, int iMax)
{
    TCHAR *sr = s1;
    while(*s1)
    {
        s1++;
    }
    while(*s2 && iMax--)
    {
        *s1++ = *s2++;
    }
    if (iMax > 0)
    {
        *s1 = *s2;
    }
    return sr;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
TCHAR *PegStrCpy(TCHAR *s1, const TCHAR *s2)
{
    TCHAR *sr = s1;
    while(*s2)
    {
        *s1++ = *s2++;
    }
    *s1 = *s2;
    return sr;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
TCHAR *PegStrnCpy(TCHAR *s1, const TCHAR *s2, int iMax)
{
    TCHAR *sr = s1;
    while(*s2 && iMax--)
    {
        *s1++ = *s2++;
    }
    if (iMax > 0)
    {
        *s1 = *s2;
    }
    return sr;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int PegStrCmp(const TCHAR *s1, const TCHAR *s2)
{
    TCHAR cVal2 = *s2++;
    while(cVal2)
    {
        if (*s1++ != cVal2)
        {
            if (s1[-1] < cVal2)
            {
                return -1;
            }
            return 1;
        }
        cVal2 = *s2++;
    }

    // if we get to here, then s2 ended
    if (*s1)            // s1 is still going??
    {
        return 1;
    }
    return 0;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int PegStrnCmp(const TCHAR *s1, const TCHAR *s2, int iMax)
{
    int i, j;
    i = PegStrLen(s1);
    j = PegStrLen(s2);
    
    if (iMax > j)
    {
        iMax = j;
    }
    if (iMax > i)
    {
        iMax = i;
    }
    while(iMax--)
    {
        if (*s1++ != *s2++)
        {
            if (s1[-1] < s2[-1])
            {
                return -1;
            }
            return 1;
        }
    }
    if (*s1 && *s2)
    {
        return 0;
    }
    if (*s1)
    {
        return 1;
    }
    return -1;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int PegStrLen(const TCHAR *s1)
{
    int iLen =0;
    while(*s1++)
    {
        iLen++;
    }
    return iLen;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
long PegAtoL(const TCHAR *s1)
{
    long j = 0;
    int sign = 0;

    if (*s1 == '+')
    {
	    s1++;
    }
    else
    {
        if (*s1 == '-')
        {
	        s1++;
	        sign--;
	    }
    }
    while (*s1 >= '0' && *s1 <= '9')
    {
	    j = j * 10 + (*s1 - '0');
	    s1++;
    }
	if(sign)
	return(-j);
    else
	return (j);
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int PegAtoI(const TCHAR *s1)
{
    int j = 0;
    int sign = 0;

    if (*s1 == '+')
    {
	    s1++;
    }
    else
    {
        if (*s1 == '-')
        {
	        s1++;
	        sign--;
	    }
    }
    while (*s1 >= '0' && *s1 <= '9')
    {
	    j = j * 10 + (*s1 - '0');
	    s1++;
    }
	if(sign)
	return(-j);
    else
	return (j);
}

#ifdef PEG_UNICODE

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int PegStrCmp(const char *s1, const char *s2)
{
    char cVal2 = *s2++;
    while(cVal2)
    {
        if (*s1++ != cVal2)
        {
            if (s1[-1] < cVal2)
            {
                return -1;
            }
            return 1;
        }
        cVal2 = *s2++;
    }

    if (*s1)        // if we get to here, then s2 terminated
    {
        return 1;   // s1 is still going
    }
    return 0;       // they both terminated
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int PegStrLen(const char *s1)
{
    int iLen =0;
    while(*s1++)
    {
        iLen++;
    }
    return iLen;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
int PegAtoI(const char *s1)
{
    int j = 0;
    int sign = 0;

    while(*s1 == ' ')
    {
        s1++;
    }
    if (*s1 == '+')
    {
	    s1++;
    }
    else
    {
        if (*s1 == '-')
        {
	        s1++;
	        sign--;
	    }
    }
    while (*s1 >= '0' && *s1 <= '9')
    {
	    j = j * 10 + (*s1 - '0');
	    s1++;
    }
	if(sign)
	return(-j);
    else
	return (j);

}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
long PegAtoL(const char *s1)
{
    long j = 0;
    int sign = 0;

    while(*s1 == ' ')
    {
        s1++;
    }

    if (*s1 == '+')
    {
	    s1++;
    }
    else
    {
        if (*s1 == '-')
        {
	        s1++;
	        sign--;
	    }
    }
    while (*s1 >= '0' && *s1 <= '9')
    {
	    j = j * 10 + (*s1 - '0');
	    s1++;
    }
	if(sign)
	return(-j);
    else
	return (j);
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
TCHAR *PegStrCpy(char *s1, const TCHAR *s2)
{
    UnicodeToAscii(s1, s2);
    return (TCHAR *) s1;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
TCHAR *PegStrCpy(TCHAR *s1, const char *s2)
{
    AsciiToUnicode(s1, s2);
    return s1;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
char *PegStrCpy(char *s1, const char *s2)
{
    char *sr = s1;
    while(*s2)
    {
        *s1++ = *s2++;
    }
    *s1 = *s2;
    return sr;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
char *PegStrCat(char *s1, const char *s2)
{
    char *sr = s1;
    while(*s1)
    {
        s1++;
    }
    while(*s2)
    {
        *s1++ = *s2++;
    }
    *s1 = *s2;
    return sr;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
char *PegStrCat(char *s1, const TCHAR *s2)
{
    char *sr = s1;
    while(*s1)
    {
        s1++;
    }
    while(*s2)
    {
        *s1++ = (char) *s2++;
    }
    *s1 = (char) *s2;
    return sr;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
TCHAR *PegStrCat(TCHAR *s1, const char *s2)
{
    TCHAR *sr = s1;

    while(*s1)
    {
        s1++;
    }
    while(*s2)
    {
        *s1++ = (TCHAR) *s2++;
    }
    *s1 = (TCHAR) *s2;

    return sr;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
char *PegStrnCat(char *s1, const TCHAR *s2, int iMax)
{
    char *sr = s1;
    while(*s1)
    {
        s1++;
    }
    while(*s2 && iMax--)
    {
        *s1++ = (char) *s2++;
    }
    *s1 = 0;

    return sr;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void UnicodeToAscii(char *s1, const TCHAR *s2)
{
    while(*s2)
    {
        *s1++ = (char) *s2++;
    }
    *s1 = '\0';
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void AsciiToUnicode(TCHAR *s1, const char *s2)
{
    while(*s2)
    {
        *s1++ = (TCHAR) *s2++;
    }
    *s1 = 0;
}

#endif      // PEG_UNICODE

#endif      // PEG_STRLIB

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#if defined(PEG_UNICODE) && defined(PEG_SJIS_CONVERSION)

extern TCHAR SJMap[];

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
TCHAR SJISToUnicode(TCHAR c1)
{
    // the table is in order of SJIS values, so we can find the correct
    // area of the table quickly, then search for the exact value:

    if (!c1)
    {
        return c1;
    }

    TCHAR *pTest = SJMap;
    WORD wIndex = SJIS_MAPTABLE_ENTRIES;
    pTest += wIndex;
    wIndex /= 2;

    if (*pTest < c1)
    {
        pTest += wIndex;

        if (*pTest > c1)
        {
            pTest -= wIndex;
        }
    }
    else
    {
        pTest -= wIndex;
        if (*pTest > c1)
        {
            pTest = SJMap;
        }
    }

    while(*pTest)
    {
        if (*pTest == c1)
        {
            return (*(pTest + 1));
        }
        pTest += 2;
    }
    return 0;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void SJISToUnicode(TCHAR *s1)
{
    TCHAR JisVal;
    JisVal = *s1;

    while (JisVal)
    {
        *s1++ = SJISToUnicode(JisVal);
        JisVal = *s1;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void SJISToUnicode(TCHAR *s1, const TCHAR *s2)
{
    TCHAR JisVal;

    JisVal = *s2++;

    while (JisVal)
    {
        *s1++ = SJISToUnicode(JisVal);
        JisVal = *s2++;
    }
    *s1 = (TCHAR) 0;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
TCHAR UnicodeToSJIS(TCHAR c1)
{
    if (!c1)
    {
        return c1;
    }
    TCHAR *pTest = SJMap;
    pTest++;
	
	while(*pTest)
	{
	    if (*pTest == c1)
	    {
            return (*(pTest - 1));
        }
	    pTest += 2;
	}
    return 0;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void UnicodeToSJIS(TCHAR *s1)
{
    TCHAR wUnicode = *s1;

    while (wUnicode)
    {
        *s1++ = UnicodeToSJIS(wUnicode);
        wUnicode = *s1;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void UnicodeToSJIS(TCHAR *s1, const TCHAR *s2)
{
    TCHAR wUnicode = *s2++;

    while(wUnicode)
    {
        *s1++ = UnicodeToSJIS(wUnicode);
        wUnicode = *s2++;
    }
    *s1 = 0;
}
 

#endif      // PEG_SJIS_MAP


