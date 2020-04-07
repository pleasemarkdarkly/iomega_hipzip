#ifndef __PIS_INCLUDE_FC_DEFINED__ 
#define __PIS_INCLUDE_FC_DEFINED__

#include <main/datastream/fatfile/CircIndex.h>
#include <main/datastream/fatfile/CacheMan.h>
#include <util/debug/debug.h>

class CCacheMan;
class CCacheReferee;
class IInputStream;

class CFileCache
{
public:
    CFileCache();
    ~CFileCache();
	
    void InitForURL(const char* szFilename);
	
    void Clear();
	void Deconfigure();
    void Minimize();
	
    IInputStream* GetFile();
	void ReopenFile();
    bool FileIsCorrupt();
    bool IsEOFBuffered();
    // do the buffers start at the BOF, and is there some data there?
    bool IsBOFBuffered();
    // do the buffers start at the BOF (maybe with no data)?
    bool AreBuffersBOFRooted();

    void PrependBuffersForBOF();
    int GetStartChar();
    void SetFileEOF();
    int GetPosition();

    void RewindCacheToBOF();

    char* GetURL();
    void SetURL(const char* szURL);

    void OpenFile();
    bool IsFileOpen();
    void CloseFile();

    int FileLength();

    int  CacheCeiling();
    void NormalizeFileCursor();

    bool BackFillForSeek(int nFileOffset);
	bool ForwardFillForSeek(int nFileOffset);
    bool OffsetIsBuffered (int nFileOffset);
    bool ShiftReadPoint(int nFileOffset, int nCharsToShift, short iUnitDirection, bool bBorrowingEmpty);
    bool FullRebufferForSeek(int nFileOffset);
    
    bool IsSkipEOF();
    void RestoreSkippedEOF();
    bool ReachedBuffersToSkip();
    void SkipBuffers(CCacheReferee* pRef);
    
    int  HandleHardError(unsigned char* pBuffer);
    bool InsertBufferDeadSpace(unsigned char* pBuffer, int &nBytesToRead, int &nBytesAlreadyRead);
    bool ReReadBeforeHardError(unsigned char* pBuffer, int nPreReadPos, int &nNextGoodLBAStart);
    
    void FillBuffer();
    bool SetFile(const char* szFilename);
    int GiveUpBuffers(int nRequested, bool bFromHead = false);
    int CountFullBuffers();
    BufferListIterator GetReadBufferIterator();

    void IncrementNextFullBuffer();
    void DecrementNextFullBuffer();
    void IncrementNextEmptyBuffer();
    void DecrementNextEmptyBuffer();

    bool IsCullable();
    void SetCullable(bool bCullable);

    bool IsCurrentCache();

    void ResetNextFullPointers();
    void ResetNextEmptyPointers();
    
    BufferList* GetBufferList();
    
    int GetFileOffset();
    int GetBufferOffset();
    void SetFileOffset(int);
    void SetBufferOffset(int);

    int GetPartialBufferIndex();
    int GetPartialBufferChars();
    int GetCharCount();

    CCircIndex* GetNextFullBufferCircIndex();
    CCircIndex* GetNextEmptyBufferCircIndex();

    int CountUnneededBuffers();
    int CountPreReadPointChars();
    void FixNextEmptyBuffer();

    bool AreAllBuffersFull();
    int FilePositionFromBufferListIndex(int nListIndex);
private:

    CCircIndex* m_nNextFullBuffer;
	CCircIndex*	m_nNextEmptyBuffer;
    bool m_bNextEmptyOffEnd;
    bool m_bNextFullOffEnd;

	int m_nFileOffset;
	int	m_nBufferOffset;

    CCacheMan* m_pCacheMan;
    IInputStream* m_pFile;

    BufferListIterator m_itNextFullBuffer;
    BufferListIterator m_itNextEmptyBuffer;

    int m_nStartChar;
	int	m_nChars;

    int	m_nPartialIndex;
	int	m_nPartialChars;
	
    short m_nSkipStart;
	short m_nSkipExtent;
	bool m_bSkipEOF;
	int	m_nSkipEOFChars;

	bool m_bValid;					
	bool m_bEOF;
	bool m_bFileCorrupt;
    int m_nHardErrors;
	bool m_bHardErrorEncountered;
    int  m_nLastDeadSpace;
    void Initialize();
    
    BufferList m_lstBuffers;

    char* m_szURL;
    bool m_bFileOpen;
    bool m_bCullable;
    int m_nFileLength;

};

#include "FileCache.inl"

#endif // __PIS_INCLUDE_FC_DEFINED__