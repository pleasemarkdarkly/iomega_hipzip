#ifndef __PIS_INCLUDE_CACHEMAN_DEFINED__ 
#define __PIS_INCLUDE_CACHEMAN_DEFINED__

class CCacheReferee;

class CCacheMan
{
public:
	static CCacheMan*	GetInstance();
	static void			Destroy();
	void		SetConservativeMode();
	void		SetAggressiveMode();
	void		NotifyUserInterrupt();
    void        SetPastBuffersToFill(int nPast);
    void SetBorrowingEmpty(bool borrowing);
    bool IsBorrowingEmpty();
    CCacheReferee* GetCacheReferee();


private:
	friend class CProducer;
	friend class CBufferedFatFileInputStreamImp;
	friend class CConsumer;
	CCacheMan();
	~CCacheMan();
	bool		IsBufferToFill();

	static CCacheMan*	m_pInstance;
	bool		m_bActive;
	int		m_nFakeEmpties;
	bool		m_bFillMode;
	bool		m_bConsumerBorrowingEmpty;
	int			m_nFakeEmptyBuffers;
	int			m_nFakeFullBuffers;
	bool		m_bBufferMode;				
	bool		m_bBufferingReady;			
	bool		m_bFirstSpinup;
	bool		m_bJustSeekedFromEOF;		
	bool		m_bConservativeBuffering;	
	short		m_nRebufferPastBuffersToFill;
	CCacheReferee* m_pCacheReferee;			
};

#endif // __PIS_INCLUDE_CACHEMAN_DEFINED__
