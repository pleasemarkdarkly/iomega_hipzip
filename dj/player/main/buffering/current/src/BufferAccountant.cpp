#include <main/buffering/BufferDebug.h>
#include <util/debug/debug.h>
#include <main/buffering/BufferAccountant.h>
//#include "WaveOut.h"

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_BUF_ACCOUNTANT, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_BUF_ACCOUNTANT);  // debugging prefix : (0) ba

CBufferAccountant::CBufferAccountant(short nBlocks) :
    m_nCurrentInvalidBlock(-1),
    m_nCurrentValidBlock(-1),
    m_bInvalidCheckedOut(false),
    m_nFakeInvalid(0),
    m_nFakeValid(0),
    m_nBlocks(nBlocks)
{
    DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_HIGH_LEVEL_TRACE, "ba:ctor\n"); 
    cyg_semaphore_init( &m_semInvalid, nBlocks );
	cyg_semaphore_init( &m_semValid, 0 );
}

CBufferAccountant::~CBufferAccountant()
{
    DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_HIGH_LEVEL_TRACE, "ba:dtor\n"); 
	cyg_semaphore_destroy( &m_semInvalid );
	cyg_semaphore_destroy( &m_semValid );
}

void CBufferAccountant::Reset(short nValid)
{
    DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_HIGH_LEVEL_TRACE, "ba:Reset %d/%d\n",nValid,m_nBlocks);
	JogInvalid();
	m_nFakeInvalid = 0;
	m_nFakeValid   = 0;
	cyg_semaphore_init( &m_semInvalid, m_nBlocks-nValid );
	cyg_semaphore_init( &m_semValid, nValid );
	m_nCurrentInvalidBlock = nValid - 1;
	m_nCurrentValidBlock = -1;
    m_bInvalidCheckedOut = false;
}

int CBufferAccountant::GetValid()
{
    //DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_LOW_LEVEL_TRACE, "ba:GetValid\n"); 
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
    //DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_TRACE, "ba:GetInval\n"); 
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
    m_bInvalidCheckedOut = true;
	this->Inc( m_nCurrentInvalidBlock);
	return m_nCurrentInvalidBlock;
}

void CBufferAccountant::ReturnInvalid()
{
    DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_HIGH_LEVEL_TRACE, "ba:RtnInval\n"); 
	cyg_semaphore_post( &m_semInvalid );
	this->Dec( m_nCurrentInvalidBlock );
}

void CBufferAccountant::ReturnValid()
{
    DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_HIGH_LEVEL_TRACE, "ba:RtnVal\n"); 
	cyg_semaphore_post( &m_semValid );
	this->Dec( m_nCurrentValidBlock );
}

void CBufferAccountant::PutInvalid()
{
    DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_LOW_LEVEL_TRACE, "ba:PtInv\n"); 
	cyg_semaphore_post( &m_semInvalid );
}

bool CBufferAccountant::PutValid()
{
    // if we haven't really requested an invalid to fill since the last reset, 
    // then this is outdated data and should be ignored.
    if (!m_bInvalidCheckedOut)
        return false;
    DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_LOW_LEVEL_TRACE, "ba:PVal\n"); 
	cyg_semaphore_post( &m_semValid );
    return true;
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
    //DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_LOW_LEVEL_TRACE, "ba:Valid %d/%d\n",nValid,m_nBlocks); 
	return (int)nValid;
}

int CBufferAccountant::PeekInvalid()
{
	cyg_count32 nInvalid;
	cyg_semaphore_peek ( &m_semInvalid, &nInvalid );
    DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_LOW_LEVEL_TRACE, "ba:PeekInval %d/%d\n",nInvalid,m_nBlocks); 
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
    DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_HIGH_LEVEL_TRACE, "ba:SetAllInval\n");
    while (PeekValid() > 0)
		if (GetValid() >= 0)
			PutInvalid();
}

void CBufferAccountant::Report()
{
    DEBUGP( DBG_BUF_ACCOUNTANT, DBGLEV_LOW_LEVEL_TRACE, "ba:%d\n",PeekValid());
}
