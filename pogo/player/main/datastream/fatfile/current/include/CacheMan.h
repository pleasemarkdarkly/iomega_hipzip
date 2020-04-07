#ifndef __PIS_INCLUDE_CACHEMAN_DEFINED__ 
#define __PIS_INCLUDE_CACHEMAN_DEFINED__

#include <util/datastructures/SimpleList.h>
#include <main/datastream/fatfile/BufferingConfig.h>
#include <main/util/datastructures/SortList.h>
//#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>

class CCacheReferee;
class CFileCache;
class IPlaylistEntry;
class IFileNameRef;

typedef SimpleList<unsigned char *> BufferList;
typedef SimpleListIterator<unsigned char*> BufferListIterator;
typedef SimpleList<CFileCache*> CacheList;
typedef SimpleListIterator<CFileCache*> CacheListIterator;
typedef SortList<IFileNameRef*> FileRefList;
typedef SimpleListIterator<IFileNameRef*> FileRefListItr;

int BufferCountFromCharCount(int nChars);

class CCacheMan
{
public:
	static CCacheMan* GetInstance();
	static void Destroy();
	void SetConservativeMode();
	void SetAggressiveMode();
	void NotifyUserInterrupt();
    void SetPastBuffersToFill(int nPast);
    int GetPastBuffersToFill();
    void SetBorrowingEmpty(bool borrowing);
    bool IsBorrowingEmpty();
    CCacheReferee* GetCacheReferee();
    bool IsFirstSpinup();
    void SetFirstSpinup(bool first);
    int BufferIndexFromLocation(unsigned char* pBuf);

    // reshuffle the cache and buffer lists to match playlist order.
    // caller must guarantee buffers and caches aren't being accessed asynchronously.
    void SyncCachesToPlaylistOrder(IPlaylistEntry* pCurrentEntry);

    // return the cache most in need of filling.
    CFileCache* GetNextCacheToFill();
    // return the current playfile's cache
    CacheListIterator GetCurrentCache();

    // return the first cache in the list.
    CacheListIterator GetCacheHead();
    // return a cache iterator associated with the given cache.
    CacheListIterator GetCacheByCache(CFileCache* pCache);

    // set up initial buffer pointers
    void InitUnusedBufferList();
    // append a list of buffers from a cache to the list of freely available buffers
    void AppendFreeBuffers(BufferList* plstBuffers);
    // return a list of new buffers to the caller
    int GetFreeBuffers(int nNeeded, BufferList* plstBuffers, bool bPrepend = false);
    // cull buffers from caches taht don't need them
    void CullUnneededBuffers();
    void DistributeBuffersToCaches();
    FileRefList* GetFileRefsToCache(IPlaylistEntry* pCurrentEntry);
    void SetCacheFileRefs(FileRefList* plstFileRefs);
    void MarkDistantCachesCullable();
    void ResetSemaphoreCounts();

    bool IsCurrentCacheReady();
    bool IsCurrentCacheValid();
    void ReadyCurrentCache();

    void CloseAllFiles();
    int CurrentCacheShortfall();

private:

    friend class CProducer;
	friend class CBufferedFatFileInputStreamImp;
	friend class CConsumer;
	CCacheMan();
	~CCacheMan();
	bool IsBufferToFill();
    void ReportOnCache(CFileCache* pCache, int idx);
    void ReportOnAllCaches(const char* szCaption);
    int CountUsefulBuffers();
	static CCacheMan*	m_pInstance;
	bool m_bActive;
	int m_nFakeEmpties;
	bool m_bFillMode;
	bool m_bConsumerBorrowingEmpty;
	int m_nFakeEmptyBuffers;
	int	m_nFakeFullBuffers;
	bool m_bBufferMode;				
	bool m_bBufferingReady;			
	bool m_bFirstSpinup;
	bool m_bJustSeekedFromEOF;		
	bool m_bConservativeBuffering;	
	short m_nRebufferPastBuffersToFill;
	CCacheReferee* m_pCacheReferee;			
	unsigned char m_BufferSpace[ CACHE_SPACE_BYTES + 1 ];
    int m_nPastCaches;
    int m_nNonContigFullBuffers;
    // list of pointers to physically contiguous buffers.  allows buffers to be logically rearranged.
    BufferList m_lstUnusedBuffers;
    // list of caches synced to playlist order
    CacheList m_lstCaches;
    // cache handling current entry in the playlist
    CacheListIterator m_itCurrentCache;
};

#include "CacheMan.inl"

#endif // __PIS_INCLUDE_CACHEMAN_DEFINED__
