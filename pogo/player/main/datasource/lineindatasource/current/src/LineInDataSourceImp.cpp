//
// LineInDataSourceImp.cpp
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "LineInDataSourceImp.h"
#include <content/common/ContentManager.h>
#include <main/datastream/lineinput/LineInputStream.h>
#include <codec/codecmanager/CodecManager.h>

#include <stdlib.h> /* malloc */

CLineInDataSourceImp::CLineInDataSourceImp()
    : IDataSource( LINE_IN_DATA_SOURCE_CLASS_ID )
{
}

CLineInDataSourceImp::~CLineInDataSourceImp() 
{
}

// Copies the string the data source uses to prefix its URLs into the given string provided.
bool
CLineInDataSourceImp::GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const
{
    if (iMaxLength > 8)
    {
        strcpy(szRootURLPrefix, "line://");
        return true;
    }
    else
        return false;
}

//! Asks the source to open this URL for reading.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IInputStream for this file type.
IInputStream* CLineInDataSourceImp::OpenInputStream(const char* szURL)
{
    if( strnicmp( szURL, "line", 4 ) == 0 ) {
        CLineInputStream* pStream = new CLineInputStream;
        if( pStream->Open( szURL ) < 0 ) {
            delete pStream;
            pStream = NULL;
        }
        return pStream;
    } else {
        return NULL;
    }
}

/*
IMediaContentRecord* CLineInDataSourceImp::GenerateEntry( IContentManager* pContentManager, const char* pURL ) 
{
    media_record_info_t mediaContent;
    memset((void*)&mediaContent, 0, sizeof(media_record_info_t));

    if( ParseURL( pURL, mediaContent ) == 0 )
        return(pContentManager->AddMediaRecord( mediaContent ));
    else
        return 0;
}

int CLineInDataSourceImp::ParseURL( const char* pURL, media_record_info_t& mediaContent ) 
{
    if( strnicmp( pURL, "line", 4 ) == 0 ) {
        mediaContent.szURL = (char*)malloc(strlen(pURL) + 1);
        strcpy( mediaContent.szURL, pURL );
        mediaContent.iDataSourceID = GetInstanceID();
        mediaContent.iCodecID = CCodecManager::GetInstance()->FindCodecID("pcm");
        
        return 0;
    }
    return -1;
}
*/
