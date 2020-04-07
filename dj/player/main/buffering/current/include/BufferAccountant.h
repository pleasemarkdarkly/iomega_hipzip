#if !defined(CACHEREFERREE_HEADER_FILE__4DE88F9A_92ED_4AF8_8488_123456789023232__INCLUDED_)
#define CACHEREFERREE_HEADER_FILE__4DE88F9A_92ED_4AF8_8488_123456789023232__INCLUDED_

#include <cyg/kernel/kapi.h>

class CBufferAccountant
{
public:
	CBufferAccountant(short nBlocks);
#ifdef DEBUG_CACHE_REFEREE_NAMES
	void SetDebugName(char* pName);
#endif

	~CBufferAccountant();
	
	int GetValid();
	bool PutValid();
	void ReturnValid();
	
	int GetInvalid();
	void PutInvalid();
	void ReturnInvalid();

	int PeekValid();
	int PeekInvalid();

	void Reset(short nValid);
    // make sure noone is waiting to get an empty buffer
	void JogInvalid();			
    // make sure noone is waiting to get a full buffer
	void JogValid();				
    void SetAllBlocksInvalid();
    void Report();
private:
	inline void Inc(int &nCirc);
	inline void Dec(int &nCirc);
    // how many empty blocks are there to fill with decoded data
	cyg_sem_t m_semInvalid;										
    // how many full blocks are there already full of decoded data
	cyg_sem_t m_semValid;								
    // circular write point
	int m_nCurrentInvalidBlock;							
    // circular read point
	int	m_nCurrentValidBlock;							
   // is a full being consumed?
	bool m_bInvalidCheckedOut;									
	short m_nFakeInvalid;
	short m_nFakeValid;
	short m_nBlocks;
};

#endif //def CACHEREFERREE_HEADER_FILE__4DE88F9A_92ED_4AF8_8488_123456789023232__INCLUDED_
