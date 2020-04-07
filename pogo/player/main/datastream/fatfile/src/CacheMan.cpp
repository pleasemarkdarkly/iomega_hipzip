//........................................................................................
//........................................................................................
//.. File Name: CacheMan.cpp															..
//.. Date: 01/25/2001																	..
//.. Author(s): Eric Gibbs																..
//.. Description of content:															..
//.. Usage:																				..
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 03/27/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

/*	CCacheMan	*/
#include <main/datastream/fatfile/CacheMan.h>

#include <datastream/fatfile/FileInputStream.h>
#include <main/datastream/fatfile/ProducerMsgPump.h>
#include <main/datastream/fatfile/ConsumerMsgPump.h>
#include <main/datastream/fatfile/FileCache.h>
#include <main/datastream/fatfile/CacheReferee.h>

#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>

CCacheMan*	CCacheMan::m_pInstance = 0;

CCacheMan* CCacheMan::GetInstance()
{
	if (!m_pInstance)
		m_pInstance = new CCacheMan();
	return m_pInstance;
}

void CCacheMan::Destroy()
{
	delete m_pInstance;
}

CCacheMan::CCacheMan() : m_nRebufferPastBuffersToFill(0)
{
	m_pCacheReferee = new CCacheReferee(BUFFER_COUNT);
	m_bJustSeekedFromEOF = false;
	m_bFirstSpinup = true;
	m_bConsumerBorrowingEmpty = false;
}

CCacheMan::~CCacheMan()
{
	delete m_pCacheReferee;
}

bool CCacheMan::IsBufferToFill() 
{	
	CBufferedFatFileInputStreamImp* pPIS = CBufferedFatFileInputStreamImp::GetInstance();
	CFileCache* pCache = pPIS->GetCache();
	if (pCache->m_bEOF)
		return false;
	if (pCache->m_nChars != pCache->m_nBuffers * BUFFER_SIZE)
		return true;
	return false;
}

void CCacheMan::SetPastBuffersToFill(int nPast)
{   
    m_nRebufferPastBuffersToFill = nPast;

}

CCacheReferee* CCacheMan::GetCacheReferee()
{
    return m_pCacheReferee;
}

void CCacheMan::SetBorrowingEmpty(bool borrowing)
{
    m_bConsumerBorrowingEmpty = borrowing;
}

bool CCacheMan::IsBorrowingEmpty()
{
    return m_bConsumerBorrowingEmpty;
}