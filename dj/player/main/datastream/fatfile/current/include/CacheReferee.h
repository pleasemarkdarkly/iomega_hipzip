#if !defined(CACHEREFERREE_HEADER_FILE__4DE88F9A_92ED_4AF8_8488_123456789023232__INCLUDED_)
#define CACHEREFERREE_HEADER_FILE__4DE88F9A_92ED_4AF8_8488_123456789023232__INCLUDED_

#include <cyg/kernel/kapi.h>

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
    // make sure noone is waiting to get an empty buffer
	void JogEmpty();			
    // make sure noone is waiting to get a full buffer
	void JogFull();				
    void SetAllBuffersEmpty();
    void Report();
private:
	inline void Inc(int &nCirc);
	inline void Dec(int &nCirc);
    // how many empty buffers are there to fill with decoded data
	cyg_sem_t m_semEmpty;										
    // how many full buffers are there already full of decoded data
	cyg_sem_t m_semFull;								
    // circular write point
	int m_nCurrentEmptyBuffer;							
    // circular read point
	int	m_nCurrentFullBuffer;							
    // is an empty being filled?
	bool m_bEmptyInUse;									
    // is a full being consumed?
	bool m_bFullInUse;									
	bool m_bPutFullBarred; 
	bool m_bPutEmptyBarred;
	short m_nFakeEmpty;
	short m_nFakeFull;
	short m_nBuffers;
};

#endif //def CACHEREFERREE_HEADER_FILE__4DE88F9A_92ED_4AF8_8488_123456789023232__INCLUDED_
