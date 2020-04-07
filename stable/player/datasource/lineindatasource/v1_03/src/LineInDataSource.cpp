//
// LineInDataSource.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <datasource/lineindatasource/LineInDataSource.h>
#include "LineInDataSourceImp.h"

CLineInDataSource::CLineInDataSource()
    : IDataSource( LINE_IN_DATA_SOURCE_CLASS_ID )
{
    m_pImp = new CLineInDataSourceImp;
}

CLineInDataSource::~CLineInDataSource() 
{
    delete m_pImp;
}

void CLineInDataSource::SetInstanceID( int iDataSourceID )
{
    m_pImp->SetInstanceID(iDataSourceID);
}

int CLineInDataSource::GetInstanceID() const
{
    return m_pImp->GetInstanceID();
}

// Copies the string the data source uses to prefix its URLs into the given string provided.
bool
CLineInDataSource::GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const
{
    return m_pImp->GetRootURLPrefix(szRootURLPrefix, iMaxLength);
}

//! Asks the source to open this URL for reading.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IInputStream for this file type.
IInputStream* CLineInDataSource::OpenInputStream(const char* szURL)
{
    return m_pImp->OpenInputStream(szURL);
}

IMediaContentRecord* CLineInDataSource::GenerateEntry( IContentManager* pContentManager, const char* pURL ) 
{
    return m_pImp->GenerateEntry(pContentManager, pURL);
}

