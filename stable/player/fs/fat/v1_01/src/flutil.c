//........................................................................................
//........................................................................................
//.. Last Modified By: Dan Bolstad	danb@iobjects.com									..	
//.. Modification date: 8/25/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/******************************************************************************
 * FileName:  FLUTIL.C - Contains string manipulation routines
 *
 * SanDisk Host Developer's Toolkit
 *
 * Copyright (c) 1997 - 1999 SanDisk Corporation
 * All rights reserved.
 * This code may not be redistributed in source or linkable object form
 * without the consent of its author.
 *
 ******************************************************************************/

#include "pcdisk.h"


#if (USE_FILE_SYSTEM)


// local routines
SDLOCAL SDBOOL valid_char( TEXT c );

/*****************************************************************************
 * Name: PC_ALLSPACE - Test if size characters in a string are spaces
 *
 * Description
 *        Test if the first size spaces of string are ' ' characters.
 *
 * Entries:
 *       TEXT    *p        Buffer data
 *       INT16   i         Max. number of bytes to be tested
 *
 * Returns
 *       YES if all spaces.
 *       NO otherwise
 *
 ******************************************************************************/
#if (RTFS_SUBDIRS)
SDBOOL pc_allspace ( TEXT *p, INT16 i ) /* __fn__*/
{
	while ( i-- )
	{
		if ( *p++ != ' ' )
			return (NO);
	}

	return (YES);
}
#endif  /* (RTFS_SUBDIRS) */


#if (RTFS_SUBDIRS)
/*****************************************************************************
 * Name:   PC_ISDOT  - Test if a filename is exactly '.'
 *                                         
 * Description:
 *       Test to see if fname is exactly '.' followed by seven spaces and
 *       file extesion is exactly three spaces.
 *
 * Entries:
 *       TEXT    *fname    File Name
 *       TEXT    *fext     File extension
 *
 * Returns
 *       YES if file:ext == '.'
 *       NO otherwise
 *
 ******************************************************************************/
SDBOOL pc_isdot ( TEXT *fname, TEXT *fext ) /* __fn__*/
{
	return ((*fname == '.') &&
		pc_allspace(fname+1, 7) && pc_allspace(fext, 3) );
}


/******************************************************************************
 * Name: PC_ISDOTDOT  - Test if a filename is exactly {'.','.'};
 *
 * Description:
 *       Test to see if fname is exactly '..' followed by six spaces and
 *       file extension is exactly three spaces.
 *
 * Entries:
 *       TEXT    *fname    File name
 *       TEXT    *fext     File extension
 *
 * Returns
 *       YES if file:ext == {'.','.'}
 *
 ******************************************************************************/
SDBOOL pc_isdotdot ( TEXT *fname, TEXT *fext ) /* __fn__*/
{
	return ( (*fname == '.') && (*(fname+1) == '.') && 
		pc_allspace(fname+2, 6) && pc_allspace(fext, 3) );
}
#endif  /* (RTFS_SUBDIRS) */




/*****************************************************************************
 * Name: PC_MFILE  - Build a file spec (xxx.yyy) from a file name and extension
 *
 * Description:
 *       Fill in to with a concatenation of file and ext. File and ext are
 *       not assumed to be null terminated but must be blank filled to [8,3]
 *       chars respectively. 'to' will be a null terminated string file.ext.
 *
 * Entries:
 *       TEXT    *to
 *       TEXT    *filename
 *       TEXT    *ext
 *
 * Returns:
 *       A pointer to 'to'.
 *
 ******************************************************************************/
TEXT *pc_mfile ( TEXT *to, TEXT *filename, TEXT *ext ) /* __fn__*/
{
	TEXT    *p;
	TEXT    *retval;
	INT16   i;

	p = filename;
	retval = to;
	i = 0;
	while ( *p )
	{
		if ( *p == ' ' )
			break;
		else
		{
			*to++ = *p++;
			i++;
		}

		if ( i == 8 )
			break;
	}

	if ( p != filename )
	{
		*to++ = '.';
		p = ext;
		i = 0;
		while( *p )
		{
			if ( *p == ' ' )
				break;
			else
			{
				*to++ = *p++;
				i++;
			}
			if ( i == 3 )
				break;
		}
	}

	/* Get rid of trailing '.' s */
	if ( (to > retval) && *(to-1) == '.')
		to--;
    *to = 0;

	return (retval);
}



/*****************************************************************************
 * Name: PC_MPATH  - Build a path sppec from a filename and pathname
 *
 * Description:
 *       Fill in "to" with a concatenation of path and filename. If path 
 *       does not end with a path separator, one will be placed between
 *       path and filename.
 *
 *       "TO" will be null terminated.
 *
 * Entries:
 *       UTEXT *to
 *       UTEXT *path
 *       TEXT *filename
 *
 * Returns:
 *        A pointer to 'to'.
 *
 ******************************************************************************/
TEXT *pc_mpath ( TEXT *to, TEXT *path, TEXT *filename ) /* __fn__*/
{
	TEXT *retval;
	TEXT *p;
    TEXT c = 0;

	retval = to;
	p = path;
	while(*p)
	{
		if (*p == ' ')
			break;
		else
			*to++ = (c =  *p++);
	}

    if ( (c != (TEXT)char_backslash) ||  (c != (TEXT)char_forwardslash) )
		*to++ = (TEXT)char_backslash;

	p = filename;
	while(*p)
		*to++ = *p++;

    *to = 0;

	return (retval);
}



/******************************************************************************
 * Name: PC_PARSEDRIVE -  Get a drive number from a path specifier
 *
 * Description:
 *       Take a path specifier in path and extract the drive number from it.
 *       If the second character in path is ':' then the first char is assumed
 *       to be a drive specifier and 'A' is subtracted from it to give the
 *       drive number. If the drive number is valid, driveno is updated and 
 *       a pointer to the text just beyond ':' is returned. Otherwise null
 *       is returned.
 *       If the second character in path is not ':' then the default drive
 *       number is put in driveno and path is returned.
 *
 * Note: Extract drive no from D: or use default. return the rest of the
 *       string or NULL if a bad drive no is requested.
 *
 * Entries:
 *       INT16 *driveno
 *       TEXT  *path
 *
 * Returns:
 *        Returns NULL on a bad drive number otherwise a pointer to the first
 *        character in the rest of the path specifier.
 *
 ******************************************************************************/
TEXT *pc_parsedrive ( INT16 *driveno, TEXT *path ) /* __fn__*/
{
	TEXT    *p;
	COUNT   dno;

	p = path;

	/* get drive no */
	if ( *p && (*(p+1) == ':'))
	{
		dno = (COUNT)(pc_byte2upper(*p) - 'A');
		p += 2;
	}
	else
		dno = pc_getdfltdrvno();

	if ( !pc_validate_driveno(dno) )
		return (SDNULL);
	else
	{
		*driveno = dno;
		return (p);
	}
}


/*****************************************************************************
 * Name: PC_FILEPARSE -  Parse a file xxx.yyy into filename/pathname
 *
 *
 * Description
 *       Take a file named "XXX.YY" and return SPACE padded NULL terminated 
 *       filename "XXX       " and fileext "YY " components. If the name or
 *       ext are less than [8,3] characters the name/ext is space filled and
 *       null termed.
 *       If the name/ext is greater  than [8,3] the name/ext is truncated.
 *       '.' is used to seperate file from ext, the special cases of "." and
 *       ".." are also handled.
 *
 *       Take a string "xxx[.yy]" and put it into filename and fileext.
 * Note: add a check legal later.
 *
 * Entries:
 *       TEXT *filename
 *       TEXT *fileext
 *       TEXT *p
 *
 * Returns:
 *       Returns YES
 *
 ******************************************************************************/
SDBOOL pc_fileparse ( TEXT *filename, TEXT *fileext, TEXT *p ) /* __fn__*/
{
	INT16   i,j,k,count,len,iPeriod,iName,iExt;
    //	TEXT    *namePat;
    //	TEXT    *extPat;
    //	TEXT    *tmpPtr;
    //	SDBOOL  longName;

    /* Defaults */
    pc_memfill( filename, (8 * sizeof(UTEXT)), 0x20);
    filename[8] = 0;
    pc_memfill( fileext, (3 * sizeof(UTEXT)), 0x20);
    fileext[3] = 0;

	/* Special cases of . and .. */
	if ( *p == '.' )
	{
		*filename = '.';
		if ( *(p+1) == '.' )
		{
			*(++filename) = '.';
			return (YES);
		}
        else if ( *(p + 1) == 0 )
			return (YES);

		return (NO);
	}	

	longFileName = SDNULL;

	// short filename test

	// if there more than one period

	count = 0;
	iPeriod = pc_strlen(p);
	for(i = 0; i < pc_strlen(p); i++)
	{
		if(p[i] == '.')
		{
			iPeriod = i;
			count++;
		}
	}

	
	// test for short filename, copy if so
	if(count <= 1)
	{

		len = 0;
		if(iPeriod <= 8)
		{	

			for(i = iPeriod + 1; i < pc_strlen(p); i++)
			{			
				len++;
			}

			if(len <= 3)
			{

				// test strings now				
				j = 0;
				for(i = 0; i < iPeriod; i++)
				{
					filename[j] = p[i];
					j++;
				}

				j = 0;
				for(i = iPeriod + 1; i < pc_strlen(p); i++)
				{
					fileext[j] = p[i];	
					j++;
				}

				if(validate_filename(filename,8) && (validate_filename(fileext,3) || pc_allspace(fileext,3)))
				{
					pc_str2upper(filename, filename);
					pc_str2upper(fileext, fileext);
					return (YES);
				}
				else if(filename[0] == '*')
				{
					// temancl - TODO:
					// not sure this is completely safe, 
					// but better than LFN madness 
					pc_str2upper(filename, filename);
					pc_str2upper(fileext, fileext);
					return (YES);
				}
			}
		}
		
	}

	// otherwise, yup, we are in long filename territory
	// try and conform a little bit with the whitepaper, please
	
	/* reset defaults */
    pc_memfill( filename, (8 * sizeof(UTEXT)), 0x20);
    filename[8] = 0;
    pc_memfill( fileext, (3 * sizeof(UTEXT)), 0x20);
    fileext[3] = 0;

	longFileName = p;

	len = pc_strlen(p);
	
	j = len - 1;
	i = 0;

	// strip leading, trailing periods, spaces
	while(p[i] == '.' || p[i] == ' ')	
		i++;	

	while(p[j] == '.' || p[j] == ' ')
		j--;
	
	// try and find last period (if any)
	
	// i is not a period, j is not a period
	k = j - 1;
	
	while(k > i) 
	{
		if(p[k] == '.')
			break;

		k--;
	}


	// if there is no extra period, put k back at the end
	if(k == i)
	{
		k = j;
	}


	iName = 0;

	// construct 8 character name
	for(; i <= k && iName < 8; i++)
	{

		if(p[i] == ' ' || p[i] == '.')
		{
			// skip spaces and periods
			continue;
		}
		else if(valid_char(p[i]))
		{
			// allow valid_chars
			filename[iName] = p[i];
		}
		else
		{
			// underscore the rest
			filename[iName] = '_';
		}
		
		iName++;
	}

	// put magic ~ thingy on the end
	for (i = 0; i < 8; i++)
	{
       if ( (UTINY)filename[i] == ' ' )
			break;
	}

    if ( i > 6 )
	{
		filename[6] = '~';
		filename[7] = '1';
	}
	else
	{
		filename[i] = '~';
		filename[i+1] = '1';
	}
	

	// phew, all done with 8

	
	// do we have a 3 to do?
	if(k != j)
	{

		iExt = 0;
		
		// advance past period
		k++;


		// construct 3 character extension
		for(;  k <= j && iExt < 3; k++)
		{

			if(p[k] == ' ' || p[k] == '.')
			{
				// skip spaces and periods
				continue;
			}
			else if(valid_char(p[k]))
			{
				// allow valid_chars
				fileext[iExt] = p[k];
			}
			else
			{
				// underscore the rest
				fileext[iExt] = '_';
			}
			
			iExt++;
		}
	}
	pc_str2upper(filename, filename);
	pc_str2upper(fileext, fileext);

	return (YES);
}



/*****************************************************************************
 * Name: PC_NIBBLEPARSE -  Nibble off the left most part of a pathspec
 *
 * Description:
 *       Take a pathspec (no leading D:). and parse the left most element into
 *       filename and file ext. (SPACE right filled.).
 *
 * NOTE: Parse a path. Return NULL if problems or a pointer to the "next".
 *
 * Entries:
 *       TEXT *filename
 *       TEXT *fileext
 *       TEXT *path
 *
 * Returns
 *        Returns a pointer to the rest of the path specifier beyond file.ext
 ******************************************************************************/
TEXT *pc_nibbleparse ( TEXT *filename, TEXT *fileext, TEXT *path ) /* __fn__*/
{
	TEXT *p;
	TEXT *t;
	TEXT *tbuf;

	p = path;
	tbuf = (TEXT *)fspath;
	t = &tbuf[0];

	if ( !p ) /* Path must exist */
		return (SDNULL);

	while ( *p )
	{
        if ( (*p == (TEXT)char_backslash) || *p == (TEXT)char_forwardslash )
		{
			p++;
			break;
		}
		else
			*t++ = *p++;
	}
    *t = 0;

	if ( pc_fileparse(filename, fileext, &tbuf[0]) )
		return (p);

	return (SDNULL);
}


/******************************************************************************
 * Name:   PC_PARSEPATH -  Parse a path specifier into path,file,ext
 *
 * Description:
 *        Take a path specifier in path and break it into three null terminated
 *        strings topath,filename and file ext.
 *        The result pointers must contain enough storage to hold the results.
 *        Filename and fileext are BLANK filled to [8,3] spaces.
 *
 *        Rules:
 *
 *        SPEC           PATH                FILE          EXT
 *        B:JOE           B:              'JOE        '   '   '
 *        B:\JOE          B:\             'JOE        '   '   '
 *        B:\DIR\JOE      B:\DIR          'JOE        '   '   '
 *        B:DIR\JOE       B:DIR           'JOE        '   '   '
 *
 * Entries:
 *       TEXT *topath
 *       TEXT *filename
 *       TEXT *fileext
 *       TEXT *path
 *
 * Returns:
 *       YES    if Successful
 *       NO     if Failed
 ******************************************************************************/
// calculate a limit for the number of subdirectories based on EMAXPATH
// subtract 4 for 'A:\\' and '\0', div 12 for chars per directory, less one for the final file, 
#define DIRLIMIT (((EMAXPATH-4) / 12) - 1)
SDBOOL pc_parsepath ( TEXT *topath, TEXT *filename, TEXT *fileext, TEXT *path ) /*__fn__*/
{
	TEXT    *pfile;        
	TEXT    *pto;      
	TEXT    *pfr;      
	TEXT    *pslash;       
    int     dircount = 0;
    SDBOOL  twiddle = NO;
	SDBOOL  colon;
    

	pto = topath;
	pfr = path;
	pslash = SDNULL;

	if ( path[0] && path[1] == ':' )
		colon = YES;
	else
		colon = NO;

	*pto = 0;
	while ( *pfr )
	{
		*pto = *pfr++;
        if ((*pto == (TEXT)char_backslash)  || (*pto == (TEXT)char_forwardslash) ) {
			pslash = pto;
            if( twiddle ) {
                dircount++;
                if( dircount == DIRLIMIT )
                    return NO;
                twiddle = 0;
            }
        }
        else {
            twiddle = 1;
        }
		pto++; 
	}
    *pto = 0;

	if ( pslash )
		pfile = pslash+1;
	else
	{
		if ( colon )
			pfile = topath + 2;
		else
			pfile = topath;
	}

	if ( !pc_fileparse(filename, fileext, pfile) )
		return (NO);


	if ( !pslash )
	{
		if ( colon )
            *(topath+2) = 0;
		else
            *topath = 0;
	}
	else
	{
		if ( (colon && (pslash == topath + 2)) ||
            (!colon && (pslash == topath)) )
            *(pslash+1) = 0;
		else
            *pslash = 0;
	}
	return (YES);
}



/******************************************************************************
 * Name: PC_PATCMP  - Compare a pattern with a string
 *
 * Description:
 *       Compare size bytes of p against pattern. Applying the following rules.
 *               If size == 8. 
 *                       (To handle the way dos handles deleted files)
 *                       if p[0] = DELETED, never match
 *                       if pattern[0] == DELETED, match with 0x5 
 *
 *               '?' in pattern always matches the current char in p.
 *               '*' in pattern always matches the rest of p.
 *
 * Entries:
 *       TEXT    *p
 *       TEXT    *pattern
 *       INT16   size
 *
 * Returns
 *        Returns YES if they match
 *
 ******************************************************************************/
SDBOOL pc_patcmp ( TEXT *p, TEXT *pattern, INT16 size ) /* __fn__*/
{

	/* Kludge. never match a deleted file */
	if ( size == 8 ) 
	{
		if ( *(UTINY *)p == PCDELETE )
			return (NO);
		else if ( *(UTINY *)pattern == PCDELETE ) /* But E5 in the Pattern matches 0x5 */
		{
			if ( (*(UTINY *)p == 0x5) || (*(UTINY *)p == '?') )
			{
				size -= 1;
#if SANDISK_IS_INTELLIGENT
				*p++;
				*pattern++;
#else
				p++;
				pattern++;
#endif
			}
			else
				return (NO);
		}
	}

	while ( size-- )
	{
		if ( *(UTINY *)pattern == '*' ) /* '*' matches the rest of the name */
			return (YES);

		if ( pc_byte2upper(*pattern) != *p )
			if ( *(UTINY *)pattern != '?' )
				return (NO);
		pattern++;
		p++;
	}

	return (YES);
}


INT16  pc_strlen (TEXT *name) /* __fn__*/
{
	TEXT *path;
	INT16 strlength;

	strlength = 0;
	path = name;
	while (*path++)
		strlength++;

	return (strlength);
}




/*****************************************************************************
 * Name: PC_BYTE2UPPER  - Convert a lower case to upper case character.
 *
 * Description
 *        Change a lower to upper case character
 *
 * Entries:
 *       TEXT    c     Character to be changed to upper case
 *
 * Returns
 *       TEXt    c     Upper case character
 *
 *****************************************************************************/
TEXT pc_byte2upper( TEXT c ) /* __fn__*/
{
	if  ( (c >= 'a') && (c <= 'z') )
		c = (TEXT) ('A' + c - 'a');

	return (c);
}



/*****************************************************************************
 * Name: PC_STR2UPPER  - Copy a string and make sure the dest is in Upper case
 *
 * Description
 *        Copy a null terminated string. Change all lower case chars to upper case
 *
 * Entries:
 *       TEXT    *to     Target string
 *       TEXT    *from   Source string
 *
 * Returns
 *        None
 *
 *****************************************************************************/
SDVOID pc_str2upper( TEXT *to, TEXT *from ) /* __fn__*/
{
	TEXT c;

	while( *from )
	{
		c = *from++;
		if  ( (c >= 'a') && (c <= 'z') )
			c = (TEXT) ('A' + c - 'a');
		*to++ = c;          
	}

    *to = 0;
}


/*****************************************************************************
 * Name: PC_STRN2UPPER  - Copy n characters of a string and make sure the dest
 *                        is in Upper case.
 *
 * Description
 *       Copy n characters of a string. Change all lower case characters
 *       to upper case characters.
 *
 * Entries:
 *       TEXT    *to     Target string
 *       TEXT    *from   Source string
 *       INT16    n      n characters to copy
 *
 * Returns
 *        None
 *
 *****************************************************************************/
SDVOID pc_strn2upper(TEXT *to, TEXT *from, INT16 n) /* __fn__*/
{
	INT16   i;
	TEXT    c;

	for (i = 0; i < n; i++)
	{
		c = *from++;
		if  ( (c >= 'a') && (c <= 'z') )
			c = (TEXT) ('A' + c - 'a');
		*to++ = c;          
	}
}


/*****************************************************************************
 * Name: VALID_CHAR  - Test for valid filename character
 *
 * Description
 *       Check for valid filename character.
 *
 * Entries:
 *       TEXT    c       filename character
 *
 * Returns
 *       YES     if character is a valid filename character
 *       NO      if character is unvalid
 *
 *****************************************************************************/
SDLOCAL SDBOOL valid_char( TEXT c ) /* __fn__*/
{
    UTEXT byte = (UTEXT)c;
    
    /* Test for valid chars. Note: we accept lower case because that 
       is patched in pc_ino2dos() at a lower level.
    */
    if (byte < 0x20) {
        if (byte == 0x05)
            return (YES);
        else
            return (NO);
    }
    if (byte == 0x22 || // '"'
        byte == 0x2a || // '*'
        //	byte == 0x2b ||  // '+'
        //	byte == 0x2c || // '''
        //	byte == 0x2e || // '.'
        byte == 0x2f || // '/'
        byte == 0x3a || // ':'
        //	byte == 0x3b || // ';'
        byte == 0x3c || // '<'
        //	byte == 0x3d || // '='
        byte == 0x3e || // '>'
        byte == 0x3f || // '?'
        //	byte == 0x5b || // '['
        byte == 0x5c || // '\'
        //	byte == 0x5d || // ']'
        byte == 0x7c)   // '|'
        return (NO);

    return (YES);
}


/*****************************************************************************
 * Name: VALIDATE_FILENAME  - Test for a valid filename
 *
 * Description
 *       Check for valid filename
 *
 * Entries:
 *       TEXT    *name       filename
 *       INT16    len        length of filename
 *
 * Returns
 *       YES     if filename is valid
 *       NO      if filename is unvalid
 *
 *****************************************************************************/
SDBOOL validate_filename( TEXT *name, INT16 len ) /* __fn__*/
{
	INT16   i;
	INT16   strLength;
	TEXT    *filename;

	strLength = pc_strlen(name);
	filename = name;

	if ( name[0] == ' ' )
	{
		i = 0;
		while (i < strLength)       /* Skip leading Zeros */
		{
			if (name[i++] == ' ')
				filename++;
			else
				break;
		}

        if ( (i == strLength) && (len == 8) )   /* filename can't be all spaces */
            return (NO);

		/* Validate individual characters */
		for (; i < strLength; i++)
			if ( !valid_char(name[i]) )
				return (NO);

		name = filename;
	}                
	else
	{
        for (i = 0; i < strLength; i++)
            /* Validate individual characters */
			if ( !valid_char(name[i]) )
				return (NO);

	}

	return (YES);
} 



//#if (CHAR_16BIT)


/******************************************************************************
 * Name: b_pack
 *
 * Description
 *       Pack a byte data buffer to a portable word data buffer (i.e. the
 *       target string will have all the leading byte as 0x00 removed).
 *
 * Entries:
 *       UTINY  *to      Target data buffer 
 *       UINT16 *from    Source data buffer
 *       UINT16 length   Length of string to convert
 *       UINT16 offset   Byte offset of target data buffer 
 *
 * Returns:
 *        16-bit target data buffer.
 *
 ******************************************************************************/
SDVOID b_pack(UINT16 *to, UTINY *from, UINT16 length, UINT16 offset) /* __fn__*/
{
    UINT16 woffset, i;

    woffset = offset >> 1;
    for (i = 0; i < length; i++, offset++)
	{
        if ( offset & 1 )
		{
#if (LITTLE_ENDIAN)
            to[woffset] &= 0x00FF;
            to[woffset] |= (((UINT16)from[i]) << 8);
#else
            to[woffset] &= 0xFF00;
            to[woffset] |= (from[i] & 0xFF);
#endif
			woffset++;
		}
		else
		{
#if (LITTLE_ENDIAN)
			to[woffset] &= 0xFF00;
            to[woffset] |= (from[i] & 0xFF); /* note: known to be zero in upper byte */
#else
            to[woffset] &= 0x00FF;
            to[woffset] |= (((UINT16)from[i]) << 8);   /* note: known to be zero in upper byte */
#endif
		}
	}
}


/******************************************************************************
 * Name: w_pack
 *
 * Description
 *       Pack a word at offset/2 to a portable word data buffer.
 *
 * Entries:
 *       UINT16 *to      Target word data buffer 
 *       UINT16 from     Word to be packed into data buffer
 *       UINT16 offset   Byte offset of target data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID w_pack(UINT16 *to, UINT16 from, UINT16 offset) /* __fn__*/
{
	UINT16 woffset=offset/2, w1;

    if ( offset & 1 )
	{
#if (LITTLE_ENDIAN)
        w1 = to[woffset] & 0x00FF;
        w1 |= (from << 8);
		to[woffset] = w1;
		w1 = to[woffset+1] & 0xFF00;
        w1 |= (from >> 8);
		to[woffset+1] = w1;
#else
        w1 = to[woffset] & 0xFF00;
        w1 |= (from >> 8);
		to[woffset] = w1;
        w1 = to[woffset+1] & 0x00FF;
        w1 |= (from << 8);
		to[woffset+1] = w1;
#endif
	}
	else
		to[woffset] = from;
}


/******************************************************************************
 * Name: w_unpack
 *
 * Description
 *       Convert a word data buffer at offset/2 to a portable word data buffer
 *
 * Entries:
 *       UTINY  *to      Target data buffer 
 *       UINT16 *from    Source data buffer
 *       UINT16 offset   Byte offset of target data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID w_unpack(UINT16 *to, UINT16 *from, UINT16 offset) /* __fn__*/
{
	UINT16 woffset = offset/2;

    if ( offset & 1 )
	{
#if (LITTLE_ENDIAN)
        // dc- dont assume anything about ordering on data read...
        UTINY* p = ((UTINY*)from)+offset;
        *to = (*p & 0xff) | ((*(p+1) & 0xff) << 8);
            //		w1 = from[woffset] & 0xFF00;
            //        w2 = from[woffset+1] & 0x00FF;
            //        *to = (w1 >> 8) | (w2 << 8);
#else
        UINT16 w1, w2;
        w1 = from[woffset] & 0x00FF;
        w2 = from[woffset+1] & 0xFF00;
        *to = (w1 << 8) | (w2 >> 8);
#endif
	}
	else
	{
		*to = from[woffset];
	}
}

/******************************************************************************
 * Name: l_pack
 *
 * Description
 *       Pack two words of a word data buffer to a portable word data buffer.
 *
 * Entries:
 *       UINT16 *to      Target data buffer 
 *       ULONG  from     Dword to put into data buffer
 *       UINT16 offset   Byte offset of target data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID l_pack(UINT16 *to, ULONG from, UINT16 offset) /* __fn__*/
{
#if LITTLE_ENDIAN
    w_pack(to, (UINT16)((ULONG)from & 0x0FFFF), offset);
    w_pack(to, (UINT16)((ULONG)from>>16), offset+2);
#else
    w_pack(to, (UINT16)((ULONG)from>>16), offset);
    w_pack(to, (UINT16)((ULONG)from & 0x0FFFF), offset+2);
#endif
}


/******************************************************************************
 * Name: l_unpack
 *
 * Description
 *       Convert two words of word data buffer to a portable word data buffer.
 *
 * Entries:
 *       UINT16 *from    Source data buffer
 *       UTINY  *to      Target data buffer 
 *       UINT16 offset   Byte offset of target data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID l_unpack(ULONG *to, UINT16 *from, UINT16 offset) /* __fn__*/
{
	ULONG L;
	UINT16 w1, w2;
  
	w_unpack(&w1, from, offset);
	w_unpack(&w2, from, offset+2);

#if (LITTLE_ENDIAN)
    L = w2;
    *to = ((L << 16) | (ULONG)w1);
#else
    L = w1;
    *to = ((L << 16) | (ULONG)w2);
#endif
}


/******************************************************************************
 * Name: char_unpack_dosinode
 *
 * Description
 *       Convert a word data buffer to a portable word data buffer DOSINODE.
 *
 * Entries:
 *       DOSINODE *pdir          Target data buffer
 *       UINT16   *pbuff         Source data buffer
 *       UINT16   offset         Byte offset of source data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID char_unpack_dosinode(DOSINODE *pdir, UINT16 *pbuff, UINT16 offset) /* __fn__*/
{
    b_unpack((UTINY *)(&pdir->fname[0]), pbuff, 8, offset);
    b_unpack((UTINY *)(&pdir->fext[0]), pbuff, 3, (offset+8));
    b_unpack((UTINY *)(&pdir->fattribute), pbuff, 1, (offset+8+3));
    
    b_unpack((UTINY *)(&pdir->resarea0), pbuff, 1, (offset+8+3+1));
    b_unpack((UTINY *)(&pdir->ftime_tenths), pbuff, 1, (offset+8+3+1+1));
    
    w_unpack((UINT16 *)(&pdir->fcrttime), pbuff, (offset+8+3+1+1+1));
    w_unpack((UINT16 *)(&pdir->fcrtdate), pbuff, (offset+8+3+1+1+1+2));
    //        for (i = 0 ; i < 3; i++)
    //        {
    //                w_unpack((UINT16 *)(&pdir->resarea[i]), pbuff, (offset+8+3+1+(i<<1)));
    //        }
    
    w_unpack((UINT16 *)(&pdir->lastAccess), pbuff, (offset+8+3+1+6));
    w_unpack((UINT16 *)(&pdir->fclusterHi), pbuff, (offset+8+3+1+8));
    w_unpack((UINT16 *)(&pdir->ftime), pbuff, (offset+8+3+1+10));
    w_unpack((UINT16 *)(&pdir->fdate), pbuff, (offset+8+3+1+10+2));
    w_unpack((UINT16 *)(&pdir->fcluster), pbuff, (offset+8+3+1+10+2+2));
    l_unpack((ULONG *)(&pdir->fsize), pbuff, (offset+8+3+1+10+2+2+2));

#if (LITTLE_ENDIAN)
#else
#if (USE_HW_OPTION)
    for (i = 0 ; i < 3; i++)
    {
        pdir->resarea[i] = to_WORD((UCHAR *)&pdir->resarea[i]);
    }
    
    pdir->lastAccess = to_WORD((UCHAR *)&pdir->lastAccess);
    pdir->fclusterHi = to_WORD((UCHAR *)&pdir->fclusterHi);
    pdir->ftime = to_WORD((UCHAR *)&pdir->ftime);
    pdir->fdate = to_WORD((UCHAR *)&pdir->fdate);
    pdir->fcluster = to_WORD((UCHAR *)&pdir->fcluster);
    pdir->fsize = to_DWORD((UCHAR *)&pdir->fsize);
#else
    i  = (UINT16)(pdir->fsize >> 16);
    pdir->fsize <<= 16;
    pdir->fsize |= (ULONG)i;
#endif
#endif /* (LITTLE_ENDIAN) */
}


/******************************************************************************
 * Name: char_unpack_longinode
 *
 * Description
 *       Convert a word data buffer to a portable word data buffer DOSINODE.
 *
 * Entries:
 *       DOSINODE *pdir          Target data buffer
 *       UINT16   *pbuff         Source data buffer
 *       UINT16   offset         Byte offset of source data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID char_unpack_longinode(LONGINODE *pdir, UINT16 *pbuff, UINT16 offset) /* __fn__*/
{
    UINT16 tmpINT16;

    b_unpack((UTINY *)(&pdir->sequence), pbuff, 1, offset);
    b_unpack((UTINY *)(&pdir->fname[0]), pbuff, 10, offset+1);
    b_unpack((UTINY *)(&pdir->fattribute), pbuff, 1, (offset+1+10));
    b_unpack((UTINY *)(&pdir->type), pbuff, 1, (offset+1+10+1));
    b_unpack((UTINY *)(&pdir->chksum), pbuff, 1, (offset+1+10+1+1));
    b_unpack((UTINY *)(&pdir->fname2[0]), pbuff, 12, (offset+1+10+1+1+1));
    w_unpack((UINT16 *)(&tmpINT16), pbuff, (offset+1+10+1+1+1+12));
    pdir->reserved = to_WORD((UCHAR *)&tmpINT16);
    b_unpack((UTINY *)(&pdir->fname3[0]), pbuff, 4, (offset+1+10+1+1+1+12+2));
}


/******************************************************************************
 * Name: char_pack_dosinode
 *
 * Description
 *       Pack a FINODE data buffer to a portable word data buffer.
 *
 * Entries:
 *       UINT16   *pbuff         Target data buffer
 *       FINODE   *pdir          Source data buffer
 *       UINT16   offset         Byte offset of source data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID char_pack_dosinode(UINT16 *pbuff, FINODE *pdir, UINT16 offset) /* __fn__*/
{
#if (LITTLE_ENDIAN)
    /* Check 0xE5 to first character for deletion */
    if (pdir->fname[0] == PCDELETE)
    {
        b_pack((UINT16 *)pbuff, (UTINY *)pdir, 1, offset);
        return;
    }
#else
    ULONG  tmpLong;
    UINT16 tmpINT16;

#if (CHAR_16BIT)
    if (pdir->fname[0] == PCDELETE )
    {
#if (USE_HW_OPTION)
        b_pack((UINT16 *)pbuff, (UTINY *)pdir, 1, offset);
#else
        b_pack((UINT16 *)pbuff, (UTINY *)pdir, 1, offset+1);
#endif
        return;
    }
#endif
#endif /* (LITTLE_ENDIAN) */
    
    b_pack(pbuff, (UTINY *)(&pdir->fname[0]), 8, offset);
    b_pack(pbuff, (UTINY *)(&pdir->fext[0]), 3, (offset+8));
    b_pack(pbuff, (UTINY *)(&pdir->fattribute), 1, (offset+8+3));

    b_pack(pbuff, (UTINY *)(&pdir->resarea0), 1, (offset+8+3+1));
    b_pack(pbuff, (UTINY *)(&pdir->ftime_tenths), 1, (offset+8+3+1+1));

    w_pack(pbuff, pdir->fcrttime, (offset+8+3+1+1+1));
    w_pack(pbuff, pdir->fcrtdate, (offset+8+3+1+1+1+2));
    //    for (i = 0; i < 3; i++)
    //    {
    //        w_pack(pbuff, pdir->resarea[i], (offset+8+3+1+(i<<1)));
    //    }
    w_pack(pbuff, pdir->lastAccess, (offset+8+3+1+6));
    /* High word of starting cluster of FAT32 */
    w_pack(pbuff, pdir->fclusterHi, (offset+8+3+1+6+2));
    w_pack(pbuff, pdir->ftime, (offset+8+3+1+10));
    w_pack(pbuff, pdir->fdate, (offset+8+3+1+10+2));
    w_pack(pbuff, pdir->fcluster, (offset+8+3+1+10+2+2));
    l_pack(pbuff, pdir->fsize, (offset+8+3+1+10+2+2+2));

#if (LITTLE_ENDIAN)
#else
#if (USE_HW_OPTION)

    i = (offset >> 1) + 6;          /* Offset to the buffer */
    for (tmpLong = 0; tmpLong < 10; tmpLong++)
    {
        pbuff[i] = to_WORD((UCHAR *)&pbuff[i]);
        i++;
    }
#else

    i = offset >> 1;
    for (tmpLong = 0; tmpLong < 6; tmpLong++)
    {
        tmpINT16 = pbuff[i];
        pbuff[i] = swap_hi_low_byte(tmpINT16);
        i++;
    }
#endif
    i = (offset) >> 1;
    i += 14;                /* Word offset 14 and 15 */ 
    tmpINT16 = pbuff[i+1];
    pbuff[i+1] = pbuff[i];
    pbuff[i] = tmpINT16;
#endif /* (LITTLE_ENDIAN) */
}

/******************************************************************************
 * Name: char_pack_longinode
 *
 * Description
 *       Pack a FINODE data buffer to a portable word data buffer.
 *
 * Entries:
 *       UINT16   *pbuff         Target data buffer
 *       FINODE   *pdir          Source data buffer
 *       UINT16   offset         Byte offset of source data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID char_pack_longinode(UINT16 *pbuff, LONGINODE *pdir, UINT16 offset) /* __fn__*/
{
    UINT16 tmpINT16;

#if (LITTLE_ENDIAN)
    /* Check 0xE5 to first character for deletion */
    if (pdir->sequence == PCDELETE)
    {
        b_pack((UINT16 *)pbuff, (UTINY *)pdir, 1, offset);
        return;
    }
#else
    UINT16 i;

#if (CHAR_16BIT)
    if (pdir->sequence == PCDELETE )
    {
#if (USE_HW_OPTION)
        b_pack((UINT16 *)pbuff, (UTINY *)pdir, 1, offset);
#else
        b_pack((UINT16 *)pbuff, (UTINY *)pdir, 1, offset+1);
#endif
        return;
    }
#endif
#endif /* (LITTLE_ENDIAN) */


    b_pack(pbuff, (UTINY *)(&pdir->sequence), 1, offset);
    b_pack(pbuff, (UTINY *)(&pdir->fname[0]), 10, (offset+1));
    b_pack(pbuff, (UTINY *)(&pdir->fattribute), 1, (offset+1+10));
    b_pack(pbuff, (UTINY *)(&pdir->type), 1, (offset+1+10+1));
    b_pack(pbuff, (UTINY *)(&pdir->chksum), 1, (offset+1+10+1+1));
    b_pack(pbuff, (UTINY *)(&pdir->fname2[0]), 12, (offset+1+10+1+1+1));
#if (USE_HW_OPTION)
    fr_WORD ( (UCHAR *)&tmpINT16, pdir->reserved );
#else
    tmpINT16 = pdir->reserved;
#endif
    w_pack(pbuff, tmpINT16, (offset+1+10+1+1+1+12));
    b_pack(pbuff, (UTINY *)(&pdir->fname3[0]), 4, (offset+1+10+1+1+1+12+2));

#if (LITTLE_ENDIAN)
#else
#if (USE_HW_OPTION)
#else
    i = offset >> 1;
    for (tmpINT16 = 0; i < ((offset >> 1) + 16); i++)
    {
        tmpINT16 = pbuff[i];
        tmpINT16 >>= 8;
        pbuff[i] = (pbuff[i] << 8 ) | tmpINT16;
    }
#endif
#endif /* (LITTLE_ENDIAN) */
}



/******************************************************************************
 * Name: char_unpack_ptable_entry
 *
 * Description
 *       Convert a word data buffer to a portable word data buffer PTABLE_ENTRY.
 *
 * Entries:
 *       PTABLE_ENTRY *dst       Target data buffer
 *       UINT16       *src       Source data buffer
 *       UINT16       offset     Byte offset of source data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID char_unpack_ptable_entry(PTABLE_ENTRY *dst, UINT16 *src, UINT16 offset) /* __fn__*/
{
    b_unpack((UTINY *)(&dst->boot), src, 1, offset);
    b_unpack((UTINY *)(&dst->s_head), src, 1, (offset+1));
    w_unpack((UINT16 *)(&dst->s_cyl), src, (offset+1+1));
    dst->s_cyl = to_WORD((UCHAR *)&dst->s_cyl);
    b_unpack((UTINY *)(&dst->p_typ), src, 1, (offset+1+1+2));
    b_unpack((UTINY *)(&dst->e_head), src, 1, (offset+1+1+2+1));
    w_unpack((UINT16 *)(&dst->e_cyl), src, (offset+1+1+2+1+1));
    dst->e_cyl = to_WORD((UCHAR *)&dst->e_cyl);
    l_unpack((ULONG *)(&dst->r_sec), src, (offset+1+1+2+1+1+2));
    /*      Swap  dst->r_sec = to_DWORD((UCHAR *)&dst->r_sec);  in calling routine */
    l_unpack((ULONG *)(&dst->p_size), src, (offset+1+1+2+1+1+2+4));
    /*      Swap  dst->p_size = to_DWORD((UCHAR *)&dst->p_size);  in calling routine */
}


/******************************************************************************
 * Name: char_unpack_ptable
 *
 * Description
 *       Convert a word data buffer to a portable word data buffer PTABLE.
 *
 * Entries:
 *       PTABLE  *dst       Target data buffer
 *       UINT16  *src       Source data buffer
 *       UINT16  offset     Byte offset of source data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID char_unpack_ptable(PTABLE *dst, UINT16 *src, UINT16 offset) /* __fn__*/
{
	UINT16 i;

	for (i = 0; i < 4; i++)
	{
		char_unpack_ptable_entry(&dst->ents[i], src, offset + (16*i));
	}

	w_unpack((UINT16 *)(&dst->signature), src, offset+(16<<2));
}


/******************************************************************************
 * Name: char_pack_ptable_entry
 *
 * Description
 *       Pack a PTABLE_ENTRY data buffer to a portable word data buffer
 *
 * Entries:
 *       UINT16       *dst       Target data buffer
 *       PTABLE_ENTRY *src       Source data buffer
 *       UINT16       offset     Byte offset of source data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID char_pack_ptable_entry(UINT16 *dst, PTABLE_ENTRY *src, UINT16 offset) /* __fn__*/
{
    ULONG  tmpLong;
    UINT16 tmpINT16;

	b_pack(dst, (UTINY *)&src->boot, 1, offset);
	b_pack(dst, (UTINY *)&src->s_head, 1, (offset+1));
    fr_WORD ( (UCHAR *)&tmpINT16, src->s_cyl );
    w_pack(dst, tmpINT16, (offset+1+1));
	b_pack(dst, (UTINY *)&src->p_typ, 1, (offset+1+1+2));
	b_pack(dst, (UTINY *)&src->e_head, 1, (offset+1+1+2+1));
    fr_WORD ( (UCHAR *)&tmpINT16, src->e_cyl );
    w_pack(dst, tmpINT16, (offset+1+1+2+1+1));
    fr_DWORD ( (UCHAR *)&tmpLong, src->r_sec );
    l_pack(dst, tmpLong, (offset+1+1+2+1+1+2));
    fr_DWORD ( (UCHAR *)&tmpLong, src->p_size );
    l_pack(dst, tmpLong, (offset+1+1+2+1+1+2+4));
}


/******************************************************************************
 * Name: char_pack_ptable
 *
 * Description
 *       Pack a PTABLE data buffer to a portable word data buffer
 *
 * Entries:
 *       UINT16  *dst       Target data buffer
 *       PTABLE  *src       Source data buffer
 *       UINT16  offset     Byte offset of source data buffer 
 *
 * Returns:
 *        16-bit target data buffer
 *
 ******************************************************************************/
SDVOID char_pack_ptable(UINT16 *dst, PTABLE *src, UINT16 offset) /* __fn__*/
{
	UINT16 i;
    UINT16 tmpINT16;

	for (i = 0; i < 4; i++)
		char_pack_ptable_entry(dst, &src->ents[i], offset + (16*i));

    fr_WORD ( (UCHAR *)&tmpINT16, src->signature );
    w_pack(dst, tmpINT16, (offset+(16<<2)));
}

//#endif /* (CHAR_16BIT) */

#endif  /* (USE_FILE_SYSTEM) */
