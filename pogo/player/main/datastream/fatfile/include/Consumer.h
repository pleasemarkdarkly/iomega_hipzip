#ifndef __PIS_INCLUDE_CONSUMER_DEFINED__ 
#define __PIS_INCLUDE_CONSUMER_DEFINED__

#include <datastream/input/InputStream.h>
#include <main/datastream/fatfile/FileCache.h>

class CConsumer
{
public:
	CConsumer();
	~CConsumer();
	
	int Seek(IInputStream::InputSeekPos Origin, int Offset);
	int Read(void* pBuffer,int dwBytes);
	static CFileCache* GetReadCache() { return m_pCurrentReadCache; }
	void SetCurrentReadCache(CFileCache* pNewCache);
private:
    bool LockBufferForReading();
    int GetSafeFileLength(CFileCache* pCache);
    bool FileOffsetFromOriginOffset(CFileCache* pCache, int nFileSize, IInputStream::InputSeekPos eOrigin, int nOffset, int &nFileOffset);
    void ReadBufferLimits(CFileCache *pCache, int &nBufferFloor, int &nBufferCeiling);

	int FullRebuffer( unsigned int nSoughtFileOffset );
	bool ShiftReadPoint(int nSoughtFileOffset, int nCharsToShift, short iUnitDirection, bool bBorrowingEmpty);
	void SetReadCache(CFileCache* pCache) { m_pCurrentReadCache = pCache; }
	bool	m_bReadBufferLocked;
	static CFileCache*	m_pCurrentReadCache;		// track this so the producer has a clue about where is more important to fill buffers.
};

#endif // __PIS_INCLUDE_CONSUMER_DEFINED__
