#ifndef __PLAYSTREAM_H__
#define __PLAYSTREAM_H__

#include <codec/common/Codec.h>
#include <datastream/filter/Filter.h>

class CMediaPlayerImp;
class CPlayStream;

// CPlayStreamSettings
//! The sequence of codecs->filters->outputstream is considered
//! to be the playstream. Currently, the CPlayStreamSettings
//! class allows the user to specify a list of filters to
//! put in the playstream, an outputstream to use for playback,
//! a stream name in the form of a URL, and some user configurable
//! data.
//
#define PLAYSTREAM_DATA_INDICES 8
#define PLAYSTREAM_FILTER_LIMIT 3
#define PLAYSTREAM_OUTPUT_LIMIT 3
#define PLAYSTREAM_DEFAULT_BUFFER_MULTIPLIER 2
class CPlayStreamSettings
{
public:
    CPlayStreamSettings();
    ~CPlayStreamSettings();

    void CopySettings( const CPlayStreamSettings* pSettings );
    // functions for user configurable data indices
    void SetData( int Index, void* p )
        { m_DataList[Index] = p;    }
    void* GetData( int Index ) const
        { return m_DataList[Index]; }

    // functions to control stream name
    void SetStreamName( const char* szStreamName );
    const char* GetStreamName() const
        { return m_szStreamName;    }

    // functions to control buffer multiplier
    int GetBufferMultiplier() const
        { return m_iBufferMultiplier; }
    void SetBufferMultiplier(int x)
        { m_iBufferMultiplier = x;  }
    
    // expose these lists for now
    unsigned int m_FilterList[PLAYSTREAM_FILTER_LIMIT];
    unsigned int m_OutputList[PLAYSTREAM_OUTPUT_LIMIT];
    
protected:
    int m_iBufferMultiplier;
    char* m_szStreamName;
    void* m_DataList[PLAYSTREAM_DATA_INDICES];
};

class CPlayStream : public CPlayStreamSettings 
{
public:
	CPlayStream();
	~CPlayStream();	

    // Returns the URL of the currently set track, or 0 if no track is set.
    const char* GetURL()
        { return m_szURL; }
    
protected:
	ERESULT SetSong(IPlaylistEntry *);
	void CleanupPlayStream();
	
	ERESULT FindCodec(IMediaContentRecord* pContentRecord);
	ERESULT TestCodec(IMediaContentRecord* pContentRecord);
	void DeleteCodec();
    
    char* m_szURL;

	class IDataSource* m_pDataSource;
	class IInputStream* m_pInputStream;
	class IMetadata*  m_pMetadata;
	class ICodec* m_pCodec;
	class IFilter** m_pFilters;
	
	stream_info_t m_streamInfo;
	filter_stream_info_t m_filterStreamInfo;

	unsigned long m_ulTrackTime;
	unsigned long m_ulLastTrackTime;

	static class CCodecManager* s_pCM;
	static class CDataSourceManager* s_pDSM;
	static class CMediaPlayerImp* s_pMediaPlayerImp;
	static class CFilterManager* s_pFM;
	static class COutputManager* s_pOM;

	friend class CMediaPlayerImp;
	bool m_bCodecFromHeap;
};

#endif // __PLAYSTREAM_H__
