//........................................................................................
//........................................................................................
//.. File Name: id3_error.h
//.. Last Modified By: Donni Reitz-Pesek	donni@iobjects.com
//.. Modification date: 2/17/2000
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.
//..	 All rights reserved. This code may not be redistributed in source or linkable
//.. 	 object form without the express written consent of Interactive Objects.
//.. Contact Information: www.iobjects.com
//........................................................................................
//........................................................................................
//  The authors have released ID3Lib as Public Domain (PD) and claim no copyright,
//  patent or other intellectual property protection in this work.  This means that
//  it may be modified, redistributed and used in commercial and non-commercial
//  software and hardware without restrictions.  ID3Lib is distributed on an "AS IS"
//  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
//  
//  The ID3Lib authors encourage improvements and optimisations to be sent to the
//  ID3Lib coordinator, currently Dirk Mahoney (dirk@id3.org).  Approved
//  submissions may be altered, and will be included and released under these terms.
//  
//  Mon Nov 23 18:34:01 1998

#ifndef	ID3LIB_ERROR_H
#define	ID3LIB_ERROR_H

#include <util/debug/debug.h>

DEBUG_USE_MODULE(ID3V2);

#if DEBUG_LEVEL==0

#define ID3_THROW(x) { }

#else

enum ID3_Err
{
	ID3E_NoMemory		= 0,
	ID3E_NoData,
	ID3E_NoBuffer,
	ID3E_InvalidFrameID,
	ID3E_FieldNotFound,
	ID3E_UnknownFieldType,
	ID3E_TagAlreadyAttached,
	ID3E_InvalidTagVersion,
	ID3E_NoFile,
	ID3E_zlibError
};

#define ID3_THROW(x) \
({ \
    switch(x) \
    { \
        case ID3E_NoMemory: \
            DEBUG(ID3V2, DBGLEV_ERROR, "Out of memory\n"); break; \
        case ID3E_NoData: \
            DEBUG(ID3V2, DBGLEV_ERROR, "No source/dest data specified\n"); break; \
        case ID3E_NoBuffer: \
            DEBUG(ID3V2, DBGLEV_ERROR, "No buffer specified\n"); break; \
        case ID3E_InvalidFrameID: \
            DEBUG(ID3V2, DBGLEV_ERROR, "Invalid frame ID\n"); break; \
        case ID3E_FieldNotFound: \
            DEBUG(ID3V2, DBGLEV_ERROR, "Field not found\n"); break; \
        case ID3E_UnknownFieldType: \
            DEBUG(ID3V2, DBGLEV_ERROR, "Unknown field type\n"); break; \
        case ID3E_TagAlreadyAttached: \
            DEBUG(ID3V2, DBGLEV_ERROR, "Tag is already attached to a file\n"); break; \
        case ID3E_InvalidTagVersion: \
            DEBUG(ID3V2, DBGLEV_ERROR, "Invalid tag version\n"); break; \
        case ID3E_NoFile: \
            DEBUG(ID3V2, DBGLEV_ERROR, "File not found\n"); break; \
        case ID3E_zlibError: \
            DEBUG(ID3V2, DBGLEV_ERROR, "Error in zlib compression library\n"); break; \
    }; \
})
#endif

#endif  // ID3LIB_ERROR_H


