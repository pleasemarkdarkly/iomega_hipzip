//........................................................................................
//........................................................................................
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 8/16/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/*****************************************************************************
* FileName:  UTIL.C - Contains string manipulation and byte order conversion
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1996-1999 SanDisk Corporation
* Copyright EBS Inc. 1996
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
******************************************************************************/
/* UTIL.C - Contains string manipulation and byte order conversion routines */

#include <fs/fat/sdapi.h>



/*****************************************************************************
* Name: COPYBUFF  - Copy one buffer to another
*
* Description
*       Essentially memcpy. Copy number of BYTES from source to target
*       buffer.
*
* Entries:
*       SDVOID *vto       Target buffer
*       SDVOID *vfrom     Source buffer
*       INT16 size      Number of bytes to copy
*
*Returns
*        None
*
******************************************************************************/
#ifndef copybuff                /* might be using memcpy */
SDVOID copybuff(SDVOID *vto, SDVOID *vfrom, INT16 size) /* __fn__*/
{
        UINT16 *to;
        UINT16 *from;
        UINT16 dsize;

        dsize = size >> 1;

        to = (UINT16 *) vto;
        from = (UINT16 *) vfrom;

        while ( dsize-- )
                *to++ = *from++;

        if (size & 1)
                *((UCHAR *)to) = *((UCHAR *)from);
}
#endif


/*****************************************************************************
* Name: COMPBUFF  - Compare two buffers
*
* Description
*       Compare n characters of Source and Target string buffers.
*
* Entries:
*       SDVOID *vfrom     Source buffer
*       SDVOID *vto       Target buffer
*       INT16 size        Number of bytes to compare
*
* Returns
*       -1      Source < Target
*        0      Source = Target
*       +1      Source > Target
*
******************************************************************************/
#ifndef compbuff                /* might be using memcmp */
INT16 compbuff(SDVOID *vfrom, SDVOID *vto, INT16 size) /* __fn__*/
{
        UTINY *str1 = (UTINY *) vfrom;
        UTINY *str2 = (UTINY *) vto;

        while ( size-- )
        {
                if ( *str1 < *str2 )
                        return -1;
                if ( *str1 > *str2 )
                        return  1;
                str1++;
                str2++;
        }

        return 0;
}
#endif



/*****************************************************************************
* Name: PC_MEMFILL  - Fill a buffer with a character
*
* Description
*        Fill the target buffer with size instances of c
*
* Entries:
*       SDVOID *vto       Target buffer
*       INT16 size      Number of bytes to do
*       UTINY c         Value to be filled
*
* Returns:
*        None
*
******************************************************************************/
#ifndef pc_memfill              /* might be using memfill */
SDVOID pc_memfill(SDVOID *vto, INT16 size, UTINY c) /* __fn__*/
{
        UTINY *to = (UTINY *) vto;

        while ( size-- )
                *to++ = c;
}
#endif


UINT16 swap_hi_low_byte (UINT16 inword)
{
        UINT16 tmpWord;

        tmpWord = inword;
        inword <<= 8;
        tmpWord = (tmpWord >> 8) | inword;

        return (tmpWord);
}


/*****************************************************************************
* Name: pc_strcat - strcat
*
* Description
*       Essential strcat function. Copy one buffer to another.  The source
*       buffer must be ended with a NULL.
*
* Entries:
*       TEXT *to        Target buffer
*       TEXT *from      Source buffer
*
* Returns
*        Nothing
*
******************************************************************************/
#ifndef pc_strcat               /* might be using strcat */
SDVOID pc_strcat(TEXT *to, TEXT *from) /* __fn__*/
{
        while ( *to )
                to++;

        while ( *from )
                *to++ = *from++;
        *to = 0;
}
#endif



/*****************************************************************************
PC_CNVRT -  Convert intel byte order to native byte order.


Summary
    #include "pcdisk.h"

    ULONG to_DWORD (from)  Convert intel style 32 bit to native 32 bit
        UTINY *from;

    UINT16 to_WORD (from)  Convert intel style 16 bit to native 16 bit
        UTINY *from;

    SDVOID fr_WORD (to,from) Convert native 16 bit to 16 bit intel
        UTINY *to;
        UCOUNT from;

    SDVOID fr_DWORD (to,from) Convert native 32 bit to 32 bit intel
        UTINY *to;
        ULONG from;

 Description
    This code is known to work on 68K and 808x machines. It has been left
    as generic as possible. You may wish to hardwire it for your CPU/Code
    generator to shave off a few bytes and microseconds, be careful though
    the addresses are not guaranteed to be word aligned in fact to_WORD AND
    fr_WORD's arguments are definately NOT word alligned when working on odd
    number indeces in 12 bit fats. (see pc_faxx and pc_pfaxx().

    Note: Optimize at your own peril, and after everything else is debugged.

    Bit shift operators are used to convert intel ordered storage
    to native. The host byte ordering should not matter.

Returns

Example:
    See other sources.

******************************************************************************/


/******************************************************************************
* Name: to_DWORD - Convert to 32-bit portable data
*
* Description
*       Convert a 32 bit intel item to a portable 32 bit data.
*
* Entries:
*       UTINY *from     Source buffer data
*
* Returns:
*       32-bit data
*
******************************************************************************/
ULONG to_DWORD ( UTINY *from ) /*__fn__*/
{
        ULONG res;
#if 0
        res = ((ULONG) *((ULONG *)from));
#else
        ULONG t;
        t = ((ULONG) *(from + 3)) & 0x00FF;
        res = (t << 24);
        t = ((ULONG) *(from + 2)) & 0x00FF;
        res |= (t << 16);
        t = ((ULONG) *(from + 1)) & 0x00FF;
        res |= (t << 8);
        t = ((ULONG) *from) & 0x00FF;
        res |= t;
#endif
        return (res);
}


/******************************************************************************
* Name: to_WORD - Covert to a portable 16-bit data
*
* Description
*       Convert a 16 bit intel item to a portable 16 bit.
*
* Entries:
*       UTINY *from     Source buffer data
*
* Returns:
*        16-bit data
*
******************************************************************************/
UINT16 to_WORD ( UTINY *from ) /*__fn__*/
{
        UINT16 nres;

#if 0
        nres = ((UINT16) *((UINT16 *)from));
#else
        UINT16 t;
        t = (UINT16) (((UINT16) *(from + 1)) & 0x00FF);
        nres = (UINT16) (t << 8);
        t = (UINT16) (((UINT16) *from) & 0x00FF);
        nres |= t;
#endif
    return (nres);
}


#if (RTFS_WRITE)
/******************************************************************************
* Name:  fr_WORD  - Convert to 16-bit INTEL format
*
* Description
*        Convert a portable 16-bit to a 16-bit intel item.
*
* Entries:
*       UTINY *to       Target buffer      
*       UINT16 from     Source data
*
* Returns:
*        Nothing
*
******************************************************************************/
SDVOID fr_WORD ( UTINY *to, UINT16 from ) /*__fn__*/
{
#if 1
    //        *((UINT16 *)to) = from;
    *to       =   (UTINY) (from & 0x00FF);
    *(to + 1) =   (UTINY) ((from >> 8) & 0x00FF);
#else
        UINT16 *tptr;
        UINT16 t1, t2;

        tptr = (UINT16 *)to;
        t1  =   (from << 8);
        t2  =  ((from >> 8) & 0x00FF);

        *tptr = (t1 | t2);

	*(to + 1) = from 
#endif
}

/******************************************************************************
* Name:  fr_DWORD  - Convert to 32-bit INTEL format
*
* Description
*        Convert a portable 32 bit to a 32 bit intel item.
*
* Entries:
*       UTINY *to       Target buffer
*       ULONG from      Source data
*
* Returns:
*        None
*
******************************************************************************/
SDVOID fr_DWORD ( UTINY *to, ULONG from ) /*__fn__*/
{
#if 1
    //        *((ULONG *)to) = from;
    *to = (UTINY) (from & 0xFF);
    *(to + 1)   =  (UTINY) ((from >> 8) & 0x00FF);
    *(to + 2)   =  (UTINY) ((from >> 16) & 0x00FF);
    *(to + 3)   =  (UTINY) ((from >> 24) & 0x00FF);
#else
        ULONG *tptr;
        ULONG tt;

        tt = 0;
        tptr = (ULONG *)to;
        *tptr = 0;

        tt = (from & 0xFF);
        tt <<= 8;
        tt |= ((from >> 8) & 0x00FF);
        tt <<= 8;
        tt |= ((from >> 16) & 0x00FF);
        tt <<= 8;
        tt |= ((from >> 24) & 0x00FF);
        *tptr = tt;
#endif
}
#endif  /* (RTFS_WRITE) */


#if (CHAR_16BIT)

/******************************************************************************
* Name: b_unpack
*
* Description
*       Convert a byte data buffer to a portable word data buffer with
*       leading bytes as 0x00.
*
* Entries:
*       UINT16 *from    Source data buffer
*       UTINY  *to      Target data buffer 
*       UINT16 length   Length of string to convert
*       UINT16 offset   Byte offset of target data buffer 
*
* Returns:
*        16-bit target data buffer with leading upper bytes as 0x00.
*
******************************************************************************/
SDVOID b_unpack(UTINY *to, UINT16 *from, UINT16 length, UINT16 offset) /*__fn__*/
{
        UINT16 i, woffset = (offset >> 1);

        for (i = 0; i < length; i++, offset++)
	{
#if (LITTLE_ENDIAN)
                if ( offset & 1 )
		{
			to[i] = (UTINY)(from[woffset] >> 8);
			woffset++;
		}
		else
                        to[i] = (UTINY)(from[woffset] & 0x00FF);
#else
                if ( offset & 1 )
		{
                        to[i] = (UTINY)(from[woffset] & 0x00FF);
			woffset++;
		}
		else
			to[i] = (UTINY)(from[woffset] >> 8);
#endif
	}
}

#endif

