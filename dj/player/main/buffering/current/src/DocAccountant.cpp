#include <main/buffering/BufferDebug.h>
#include <util/debug/debug.h>
#include <main/buffering/BufferAccountant.h>
//#include "WaveOut.h"

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_CACHE_REF, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_CACHE_REF);

CBufferAccountant::CBufferAccountant(short nBlocks) :	m_nCurrentInvalidBlock(-1),
									m_nCurrentValidBlock(-1),
									m_bInvalidInUse(false),
									m_bValidInUse(false),
									m_bPutValidBarred(false),
									m_bPutInvalidBarred(false),
									m_nFakeInvalid(0),
									m_nFakeValid(0),
									m_nBlocks(nBlocks)
{
    cyg_semaphore_init( &m_semInvalid, nBlocks ); 
	cyg_semaphore_init( &m_semValid, 0 );			
}

CBufferAccountant::~CBufferAccountant()
{
	cyg_semaphore_destroy( &m_semInvalid );
	cyg_semaphore_destroy( &m_semValid );
}

void CBufferAccountant::Reset(short nValid)
{
    DEBUGP( DBG_CACHE_REF, DBGLEV_VERBOSE, "da:reset %d\n",nValid);    
    if (m_bInvalidInUse)
	{	
		this->m_bPutInvalidBarred = true;
		ReturnInvalid();
	}
	if (m_bValidInUse)
	{
		m_bPutValidBarred = true;
		ReturnValid();
	}
	JogInvalid();
	m_nFakeInvalid = 0;
	m_nFakeValid = 0;
	m_bValidInUse = false;
	m_bInvalidInUse = false;
	cyg_semaphore_init( &m_semInvalid, m_nBlocks-nValid ); 
	cyg_semaphore_init( &m_semValid, nValid );			
	m_nCurrentInvalidBlock = nValid - 1;
	m_nCurrentValidBlock = -1;
}

int CBufferAccountant::GetValid()
{
	cyg_semaphore_wait( &m_semValid );
	if (m_nFakeValid>0)
	{
		m_nFakeValid--;
		return -1;
	}
	this->Inc(m_nCurrentValidBlock);
	return m_nCurrentValidBlock;
}

int CBufferAccountant::GetInvalid()
{
	cyg_semaphore_wait( &m_semInvalid );
	if (m_nFakeInvalid>0)
	{
		m_nFakeInvalid--;
		int nInvalid = PeekInvalid();
		while (nInvalid > 0 && m_nFakeInvalid > 0)
		{
			cyg_semaphore_wait( &m_semInvalid );
			nInvalid--;
			m_nFakeInvalid--;
		}
		if (nInvalid > 0)
		{
			cyg_semaphore_wait( &m_semInvalid );
			this->Inc( m_nCurrentInvalidBlock);
			return m_nCurrentInvalidBlock;
		}
		else
			return -1;
	}
	this->Inc( m_nCurrentInvalidBlock);
	return m_nCurrentInvalidBlock;
}

void CBufferAccountant::ReturnInvalid()
{
	if (!m_bInvalidInUse)
		return;
	m_bInvalidInUse = false;
	cyg_semaphore_post( &m_semInvalid );
	this->Dec( m_nCurrentInvalidBlock );
}

void CBufferAccountant::ReturnValid()
{
	if (!m_bValidInUse)
		return;
	m_bValidInUse = false;
	cyg_semaphore_post( &m_semValid );
	this->Dec( m_nCurrentValidBlock );
}

void CBufferAccountant::PutInvalid()
{
	if (m_bPutInvalidBarred)
	{
		m_bPutInvalidBarred = false;
		return;
	}
	m_bValidInUse = false;
	cyg_semaphore_post( &m_semInvalid );	
}

void CBufferAccountant::PutValid()
{
	if (m_bPutValidBarred)
	{
		m_bPutValidBarred = false;
		return;
	}
	m_bInvalidInUse = false;
	cyg_semaphore_post( &m_semValid );
}

void CBufferAccountant::Inc(int &nCirc)
{
	if (++nCirc == this->m_nBlocks)
		nCirc -= this->m_nBlocks;
}
void CBufferAccountant::Dec(int &nCirc)
{
	if (--nCirc == -1)
		nCirc += m_nBlocks;
}

int CBufferAccountant::PeekValid()
{
	cyg_count32 nValid;
	cyg_semaphore_peek ( &m_semValid, &nValid );
	return (int)nValid;
}

int CBufferAccountant::PeekInvalid()
{
	cyg_count32 nInvalid;
	cyg_semaphore_peek ( &m_semInvalid, &nInvalid );
	return (int)nInvalid;
}

void CBufferAccountant::JogInvalid()
{
	m_nFakeInvalid++;
	cyg_semaphore_post( &m_semInvalid );
}

void CBufferAccountant::JogValid()
{
	m_nFakeValid++;
	cyg_semaphore_post( &m_semValid );
}

#ifdef DEBUG_CACHE_REFEREE_NAMES
void CBufferAccountant::SetDebugName(char* pName)
{
	strcpy (m_InstanceName,pName);
}
#endif

void CBufferAccountant::SetAllBlocksInvalid()
{
    DEBUGP( DBG_CACHE_REF, DBGLEV_VERBOSE, "da:setall empty\n");
    while (PeekValid() > 0)
		if (GetValid() >= 0)
			PutInvalid();
}

void CBufferAccountant::Report()
{
    DEBUGP( DBG_CACHE_REF, DBGLEV_VERBOSE, "da:%d,%d\n",PeekValid(),PeekInvalid());
}
