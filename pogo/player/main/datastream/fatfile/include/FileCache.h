#ifndef __PIS_INCLUDE_FC_DEFINED__ 
#define __PIS_INCLUDE_FC_DEFINED__

#include <main/datastream/fatfile/CircIndex.h>
#include <util/debug/debug.h>

class CFatFileInputStream;

class CFileCache
{
public:
						CFileCache(int nBuffers);
						~CFileCache();
	void				InitFromFile(const char* szFilename);
	//void				CloneFromCache(CFileCache* pActiveCache);
	//void				ResetExtended();
	void				Clear();
	void				UnloadFile();
	void				Rewind();	// (epg,2/28/2001): set all counters back to the start of the cache (only for intros)
	CFatFileInputStream* GetFile();
	bool				IsMP3();	// simple string check on filename
	void				LookupID3v1();
	void				ReopenFile();
    bool                FileIsCorrupt();
    bool                FileAtEOF();
    void                SetFileEOF();
    // calculate the highest file-offset currently in memory.
    int                 CacheCeiling();
	bool                BackFillForSeek(int nFileOffset);
	bool                ForwardFillForSeek(int nFileOffset);
    bool                OffsetIsBuffered (int nFileOffset);
    bool                ShiftReadPoint(int nFileOffset, int nCharsToShift, short iUnitDirection, bool bBorrowingEmpty);
    bool                FullRebufferForSeek(int nFileOffset);
    void                NormalizeReadBufferIndex();



    CFatFileInputStream* m_pFile;
	int				    m_nStartChar;
	int				    m_nChars;
	int					m_nBuffers;
	CCircIndex*			m_nNextFullBuffer;		// use sync mutex for access?
	CCircIndex*			m_nNextEmptyBuffer;	// use sync mutex for access?
	int				    m_nFileBookmark;
	int				    m_nBufferBookmark;
	int					m_nReadBufferIndex;
	short				m_nBuffersToSpare;		
	int					m_nPartialIndex;
	int					m_nPartialChars;
	short				m_nSkipStart;
	short				m_nSkipExtent;
	bool				m_bSkipEOF;
	int					m_nSkipEOFChars;
	unsigned char**		m_aBuffers;

	bool				m_bExtended;				// let the extended cache know how special it is.
	bool				m_bValid;					
	bool				m_bEOF;
	bool				m_bFileCorrupt;				// there is no hope, deny all requests.
	bool				m_bHardErrorEncountered;	// have we encountered a hard error?
	int					m_nHardErrors;
	bool				m_bID3v1LookupDone;
	CFileCache*			m_pNext;
	CFileCache*			m_pPrev;
};

#endif // __PIS_INCLUDE_FC_DEFINED__
