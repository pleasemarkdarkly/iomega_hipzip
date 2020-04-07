#include <util/debug/debug.h>
#include <main/datastream/fatfile/CacheReferee.h>
//#include "WaveOut.h"

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_CACHE_REF, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(DBG_CACHE_REF);

CCacheReferee::CCacheReferee(short nBuffers) :	m_nCurrentEmptyBuffer(-1),
									m_nCurrentFullBuffer(-1),
									m_bEmptyInUse(false),
									m_bFullInUse(false),
									m_bPutFullBarred(false),
									m_bPutEmptyBarred(false),
									m_nFakeEmpty(0),
									m_nFakeFull(0),
									m_nBuffers(nBuffers)
{
    cyg_semaphore_init( &m_semEmpty, nBuffers ); 
	cyg_semaphore_init( &m_semFull, 0 );			
}

CCacheReferee::~CCacheReferee()
{
	cyg_semaphore_destroy( &m_semEmpty );
	cyg_semaphore_destroy( &m_semFull );
}

void CCacheReferee::Reset(short nFull)
{
    DEBUGP( DBG_CACHE_REF, DBGLEV_TRACE, "cr:reset %d\n",nFull);    
    if (m_bEmptyInUse)
	{	
		this->m_bPutEmptyBarred = true;
		ReturnEmpty();
	}
	if (m_bFullInUse)
	{
		m_bPutFullBarred = true;
		ReturnFull();
	}
	JogEmpty();
	m_nFakeEmpty = 0;
	m_nFakeFull = 0;
	m_bFullInUse = false;
	m_bEmptyInUse = false;
	cyg_semaphore_init( &m_semEmpty, m_nBuffers-nFull ); 
	cyg_semaphore_init( &m_semFull, nFull );			
	m_nCurrentEmptyBuffer = nFull - 1;
	m_nCurrentFullBuffer = -1;
}

int CCacheReferee::GetFull()
{
	cyg_semaphore_wait( &m_semFull );
	if (m_nFakeFull>0)
	{
		m_nFakeFull--;
		return -1;
	}
	this->Inc(m_nCurrentFullBuffer);
	return m_nCurrentFullBuffer;
}

int CCacheReferee::GetEmpty()
{
	cyg_semaphore_wait( &m_semEmpty );
	if (m_nFakeEmpty>0)
	{
		m_nFakeEmpty--;
		int nEmpty = PeekEmpty();
		while (nEmpty > 0 && m_nFakeEmpty > 0)
		{
			cyg_semaphore_wait( &m_semEmpty );
			nEmpty--;
			m_nFakeEmpty--;
		}
		if (nEmpty > 0)
		{
			cyg_semaphore_wait( &m_semEmpty );
			this->Inc( m_nCurrentEmptyBuffer);
			return m_nCurrentEmptyBuffer;
		}
		else
			return -1;
	}
	this->Inc( m_nCurrentEmptyBuffer);
	return m_nCurrentEmptyBuffer;
}

void CCacheReferee::ReturnEmpty()
{
	if (!m_bEmptyInUse)
		return;
	m_bEmptyInUse = false;
	cyg_semaphore_post( &m_semEmpty );
	this->Dec( m_nCurrentEmptyBuffer );
}

void CCacheReferee::ReturnFull()
{
	if (!m_bFullInUse)
		return;
	m_bFullInUse = false;
	cyg_semaphore_post( &m_semFull );
	this->Dec( m_nCurrentFullBuffer );
}

void CCacheReferee::PutEmpty()
{
	if (m_bPutEmptyBarred)
	{
		m_bPutEmptyBarred = false;
		return;
	}
	m_bFullInUse = false;
	cyg_semaphore_post( &m_semEmpty );	
}

void CCacheReferee::PutFull()
{
	if (m_bPutFullBarred)
	{
		m_bPutFullBarred = false;
		return;
	}
	m_bEmptyInUse = false;
	cyg_semaphore_post( &m_semFull );
}

void CCacheReferee::Inc(int &nCirc)
{
	if (++nCirc == this->m_nBuffers)
		nCirc -= this->m_nBuffers;
}
void CCacheReferee::Dec(int &nCirc)
{
	if (--nCirc == -1)
		nCirc += m_nBuffers;
}

int CCacheReferee::PeekFull()
{
	cyg_count32 nFull;
	cyg_semaphore_peek ( &m_semFull, &nFull );
	return (int)nFull;
}

int CCacheReferee::PeekEmpty()
{
	cyg_count32 nEmpty;
	cyg_semaphore_peek ( &m_semEmpty, &nEmpty );
	return (int)nEmpty;
}

void CCacheReferee::JogEmpty()
{
	m_nFakeEmpty++;
	cyg_semaphore_post( &m_semEmpty );
}

void CCacheReferee::JogFull()
{
	m_nFakeFull++;
	cyg_semaphore_post( &m_semFull );
}

#ifdef DEBUG_CACHE_REFEREE_NAMES
void CCacheReferee::SetDebugName(char* pName)
{
	strcpy (m_InstanceName,pName);
}
#endif

void CCacheReferee::SetAllBuffersEmpty()
{
    DEBUGP( DBG_CACHE_REF, DBGLEV_TRACE, "cr:setall empty\n");
    while (PeekFull() > 0)
		if (GetFull() >= 0)
			PutEmpty();
}

void CCacheReferee::Report()
{
    DEBUGP( DBG_CACHE_REF, DBGLEV_TRACE, "cr:%d,%d\n",PeekFull(),PeekEmpty());
}
