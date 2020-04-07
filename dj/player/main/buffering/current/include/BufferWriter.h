#ifndef __PIS_INCLUDE_PRODUCER_DEFINED__ 
#define __PIS_INCLUDE_PRODUCER_DEFINED__ 

class CBufferWorker;
typedef enum eBWResult { BW_OK, BW_EOF, BW_FAIL, BW_DONE };

class CBufferWriter
{
public:
	CBufferWriter();
	~CBufferWriter();

    void SetWorker(CBufferWorker* pWkr);
	eBWResult WriteBlock();
private:
    CBufferWorker* m_pWorker;
};

#endif // __PIS_INCLUDE_PRODUCER_DEFINED__ _
