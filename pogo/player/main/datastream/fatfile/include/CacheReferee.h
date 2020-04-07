#if !defined(CACHEREFERREE_HEADER_FILE__4DE88F9A_92ED_4AF8_8488_123456789023232__INCLUDED_)
#define CACHEREFERREE_HEADER_FILE__4DE88F9A_92ED_4AF8_8488_123456789023232__INCLUDED_

#include <cyg/kernel/kapi.h>

#ifdef _DEBUG_CACHE_REFEREE
#define DEBUG_CACHE_REFEREE(s...) diag_printf(##s)
#define DEBUG_CACHE_REFEREE_NAMES
#else
#undef DEBUG_CACHE_REFEREE_NAMES
#define DEBUG_CACHE_REFEREE(s...) /**/
#endif

class CCacheReferee
{
public:
	CCacheReferee(short nBuffers);
#ifdef DEBUG_CACHE_REFEREE_NAMES
	void SetDebugName(char* pName);
#endif

	~CCacheReferee();
	
	int GetFull();
	void PutFull();
	void ReturnFull();
	
	int GetEmpty();
	void PutEmpty();
	void ReturnEmpty();

	int PeekFull();
	int PeekEmpty();

	void Reset(short nFull);
	void JogEmpty();			// make sure noone is waiting to get an empty buffer
	void JogFull();				// make sure noone is waiting to get a full buffer
    void SetAllBuffersEmpty();
private:
#ifdef DEBUG_CACHE_REFEREE_NAMES
	char				m_InstanceName[255];
#endif
	inline void Inc(int &nCirc);
	inline void Dec(int &nCirc);
	cyg_sem_t			m_semEmpty;										// how many empty buffers are there to fill with decoded data
	cyg_sem_t			m_semFull;										// how many full buffers are there already full of decoded data
	int					m_nCurrentEmptyBuffer;							// circular write point
	int					m_nCurrentFullBuffer;							// circular read point
	bool				m_bEmptyInUse;									// is an empty being filled?
	bool				m_bFullInUse;									// is a full being consumed?
	bool				m_bPutFullBarred; 
	bool				m_bPutEmptyBarred;
	short				m_nFakeEmpty;
	short				m_nFakeFull;
	short				m_nBuffers;
};

#endif //def CACHEREFERREE_HEADER_FILE__4DE88F9A_92ED_4AF8_8488_123456789023232__INCLUDED_