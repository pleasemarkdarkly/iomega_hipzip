/******************************************************************************
* Filename: LONGFN.C - Long File Name support routines
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1998-1999 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description:
*       Long directory and File name support routines
*
* The following routines in this file are included:
*
*
******************************************************************************/

#include "pcdisk.h"


#if (USE_FILE_SYSTEM)


/*****************************************************************************
* Name: copyLongFileName
*
* Description:
*       Calculate the name length and copy the name.
*
* Entries:
*       INT16 offset
*       INT16 index
*       UTEXT *fNameExt
*       UTEXT *curNameExt
*       INT16 displ
*
* Returns:
*       The length of the name
*
******************************************************************************/
INT16 copyLongFileName (INT16 offset, INT16 index, UTEXT *fNameExt, UTEXT *curNameExt, INT16 displ) /*__fn__*/
{
        INT16   i;
        INT16   iPos;
        INT16   strLength;
        UTEXT   tmpChr;

        iPos = (offset * LFNLEN_PER_ENTRY) + displ;
        for (i = 0, strLength = 0; i < index; i++)
        {
                tmpChr =  *((UTEXT *)curNameExt + (i << 1));
                if ( (tmpChr == 0x00) || (tmpChr == 0xFF) )
                        break;

                *(UTEXT *)(fNameExt + iPos + i) = tmpChr;
                strLength += 1;
        }

        return (strLength);
}


/******************************************************************************
* Name: LongFilenameToEntry
*
* Description:
*       
*
* Entries:
*       leName,
*       COUNT *ind,
*       COUNT *length
*       COUNT totalChar
*       SDBOOL *NULLChar
*
* NOTE: saveFileName is a global variable pointing to the name.
*
* Returns:
*       None
*
******************************************************************************/
SDVOID  LongFilenameToEntry (UTINY *leName, COUNT *ind, COUNT length, COUNT totalChar) /* __fn__*/
{
        COUNT   offset;
        COUNT   index;
        INT16   iPos;
        TEXT    tmpword;
        TEXT    *tmpPtr;

        index = *ind;
        tmpPtr = saveFileName;

        for (offset = 0; (index < length) && (offset < totalChar); index++, offset++)
        {
                iPos = offset << 1;
                tmpword = tmpPtr[index];
                leName[iPos] = tmpword;
                leName[iPos+1] = 0;
        }

        if ( offset < totalChar)
        {
                /* Check for NULL termination of the name */
                tmpword = tmpPtr[index];
                if ( tmpword == 0 && (index <= length) )
                {
                        iPos = offset << 1;
                        leName[iPos] = tmpword;
                        leName[iPos+1] = tmpword;
                        offset++;
                }
                
                for (; offset < totalChar; offset++)
                {
                        iPos = offset << 1;
                        leName[iPos] = 0xFF;
                        leName[iPos+1] = 0xFF;
                }
        }

        *ind += offset;
}


/*****************************************************************************
* Name: pc_chksum
*
* Description: - Calculate the check sum for the name.
*
* Entries:
*       name    File name
*       ext     file name extension
*
* Returns:
*       File name check sum
*
******************************************************************************/
UINT16 pc_chksum(UTINY *name, UTINY *ext) /* __fn__*/
{
        UINT16 sum, i;
        UTINY filename[12];

        for (i = 0 ; i < 8; i++)
                filename[i] = name[i];

        for (i = 0; i < 3; i++)
                filename[i + 8] = ext[i];

        filename[11] = 0;

        for (sum = i = 0; i < 11; i++)
                sum = (UINT16)((((sum & 0x01)<<7) | ((sum & 0xFE)>>1)) + filename[i]);

        return (sum);
}


/*****************************************************************************
* Name: pc_higherAlias
*
* Description: - Calculate the check sum for the name.
*
* Entries:
*       filename1    File name to change
*       filename2    Source File name
*
* Returns:
*       YES if successful
*       NO if not
*
******************************************************************************/
SDBOOL pc_higherAlias (TEXT *filename1, TEXT *filename2)
{
        INT16 i, j;
        SDBOOL retval;

        retval = NO;

        for (i = 0; i < 8; i++)
        {
                if (filename1[i] == '~')
                        break;
        }

        for (j = 0; j < 8; j++)
        {
                if (filename2[j] == '~')
                        break;
        }

        if ((j - i) < 0)
        {
                for (i = 0; i < 8; i++)
                        filename1[i] = filename2[i];
                retval = YES;
                
        }
        else if ((j-i) == 0)
        {
                for (i++; i < 8; i++)
                {
                        if (filename2[i] > filename1[i])
                        {
                                for (j = i; j < 8; j++)
                                        filename1[j] = filename2[j];
                                i = 8;
                                retval = YES;
                        }
                }
        }

        return (retval);
}


/*****************************************************************************
* Name: pc_patcmpupto8ordot
*
* Description: - Compare two long file names up to 8 characters in length.
*
* Entries:
*       pPtr1   First long name
*       pPtr2   Second long name
*
* Returns:
*       YES if the names are the same
*       NO if names not the same 
*
******************************************************************************/
SDBOOL pc_patcmpupto8ordot ( TEXT *pPtr1, TEXT *pPtr2 ) /* __fn__*/
{
        INT16 i;

        for (i = 0; i < 8; i++)
        {
                if ( pPtr1[i] != pPtr2[i] )
                        return (NO);

                if ( *pPtr2 == '.' )
                        return (YES);
        }

        return (YES);
}


/*****************************************************************************
* Name: pc_patcmplong
*
* Description: - Compare two long file names.
*
* Entries:
*       pPtr1   First long name
*       pPtr2   Second long name
*       size    Length of name
*
* Returns:
*       YES if the names are the same
*       NO if names not the same 
*
******************************************************************************/
SDBOOL pc_patcmplong ( TEXT *pPtr1, TEXT *pPtr2 ) /* __fn__*/
{
        for (;;)
        {
                if ( (*pPtr1 == 0) && (*pPtr2 == 0) )
                        return (YES);

                if ( *pPtr2 != *pPtr1 )
                        return (NO);

                pPtr2++;
                pPtr1++;
        }
}


/*****************************************************************************
* Name: convert_alias_name
*
* Description:
*       Calculate where the "~" should be and the new value after the "~"
*       for the new file name.
*       The file name should be terminated.
*       
* Entries:
*       filename        The alias name
*
* Returns:
*       NONE
*
******************************************************************************/
SDVOID  convert_alias_name(TEXT *filename) /* __fn__*/
{
	TEXT  *p;
	UINT32 result;
	INT16 i;
	INT16 value;
	UTINY tmpChr;

	p = filename;

        while( *p )
	{
                if ( *(UTINY *)p == '~' )
		{
			p++;
			break;
		}
		p++;
	}

        for (i = 0, result = 0; (p[i] != 0); i++)
	{
		tmpChr = *(p+i);
                if ( tmpChr == ' ' )
		{
                        break;
		}
                if ( tmpChr == 0 )
                        break;

                if ( (0x30 <= tmpChr) && (tmpChr <= 0x39) )
			tmpChr &= 0x0F;
                if ( (0x41 <= tmpChr) && (tmpChr <= 0x46) )
			tmpChr = (UCHAR)((tmpChr & 0x0F) + 0x09);

		value = (INT16)tmpChr;

		result <<= 4;
		result |= (UINT32)value;
	}
	result += 1L;


        i = pc_strlen(filename);
        p = filename + i;
        if ( *p == 0 )
                p--;

        while ( result )
	{
		value = (INT16)(result & 0x0F);
                if ( (0 <= value) && (value < 10) )
			tmpChr = (UCHAR)(value | 0x30);
                if ( (9 < value) && (value < 16) )
			tmpChr = (UCHAR)(value - 0xA + 0x41);

                *(UTINY *)p = tmpChr;
		result >>= 4;
                p--;
	}
        *(UTINY *)p = '~';

}

#endif  /* (USE_FILE_SYSTEM) */

