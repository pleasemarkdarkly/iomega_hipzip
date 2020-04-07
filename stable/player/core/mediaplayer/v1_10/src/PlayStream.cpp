#include <datasource/datasourcemanager/DataSourceManager.h>
#include <codec/codecmanager/CodecManager.h>
#include <datastream/filtermanager/FilterManager.h>
#include <datastream/outputmanager/OutputManager.h>
#include <codec/common/Codec.h>              // ICodec definition
#include <datastream/filter/Filter.h>        // IFilter definition
#include <datastream/input/InputStream.h>    // IInputStream
#include <datastream/output/OutputStream.h>  // IOutputStream
#include <playlist/common/Playlist.h>        // IPlaylistEntry
#include <core/mediaplayer/MediaPlayer.h>
#include <core/mediaplayer/PlayStream.h>
#include <util/datastructures/SimpleList.h>
#include <util/rbuf/rbuf.h>                // yay rbuffers
#include <util/eventq/EventQueueAPI.h>     // for main event queue
#include <util/debug/debug.h>              // for debug system
#include <datastream/outfilter/OutFilterKeys.h> // outfilter, yay

#include "MediaPlayerImp.h"

DEBUG_USE_MODULE( MP );

//
// CPlayStreamSettings
//
CPlayStreamSettings::CPlayStreamSettings() 
{
    m_szStreamName = 0;
    m_iBufferMultiplier = PLAYSTREAM_DEFAULT_BUFFER_MULTIPLIER;
    
    memset( (void*)&(m_FilterList[0]), 0, sizeof(unsigned int) * PLAYSTREAM_FILTER_LIMIT );
    memset( (void*)&(m_OutputList[0]), 0, sizeof(unsigned int) * PLAYSTREAM_OUTPUT_LIMIT );
    memset( (void*)&(m_DataList[0]),   0, sizeof(void*) * PLAYSTREAM_DATA_INDICES );
}
CPlayStreamSettings::~CPlayStreamSettings()
{
    if( m_szStreamName ) {
        delete [] m_szStreamName;
    }
}
void CPlayStreamSettings::CopySettings( const CPlayStreamSettings* pSettings ) 
{
    memcpy( (void*)&(m_FilterList[0]), (void*)&(pSettings->m_FilterList[0]), sizeof(unsigned int)*PLAYSTREAM_FILTER_LIMIT);
    memcpy( (void*)&(m_OutputList[0]), (void*)&(pSettings->m_OutputList[0]), sizeof(unsigned int)*PLAYSTREAM_OUTPUT_LIMIT);
    memcpy( (void*)&(m_DataList[0]), (void*)&(pSettings->m_DataList[0]), sizeof(void*) * PLAYSTREAM_DATA_INDICES );
    SetStreamName( pSettings->GetStreamName() );
    SetBufferMultiplier( pSettings->GetBufferMultiplier() );
}
void CPlayStreamSettings::SetStreamName(const char* szStreamName)
{
    // doing the sequence this way allows SetStreamName(0) to clear the name
    if( m_szStreamName ) {
        delete [] m_szStreamName;
    }
    if( !szStreamName ) {
        return ;
    }
    m_szStreamName = new char[strlen(szStreamName)+1];
    strcpy( m_szStreamName, szStreamName );
}

//
// CPlayStream
//

CCodecManager* CPlayStream::s_pCM;
CDataSourceManager* CPlayStream::s_pDSM;
CMediaPlayerImp* CPlayStream::s_pMediaPlayerImp;
CFilterManager* CPlayStream::s_pFM;
COutputManager* CPlayStream::s_pOM;



CPlayStream::CPlayStream() : m_szURL(0), m_pDataSource(0),
	m_pInputStream(0),  m_pMetadata(0), m_pCodec(0), m_pFilters(0),
	m_ulTrackTime(0), m_ulLastTrackTime(0), m_bCodecFromHeap(true)
{
	static bool first = true;
	if (first) {
		first = false;
		s_pCM = CCodecManager::GetInstance();
		s_pDSM = CDataSourceManager::GetInstance();
		s_pMediaPlayerImp = CMediaPlayerImp::GetInstance();
        // ditto
		s_pFM = CFilterManager::GetInstance();
        // ditto
		s_pOM = COutputManager::GetInstance();
	}
    m_filterStreamInfo.m_szStreamName[0] = 0;
}

CPlayStream::~CPlayStream()
{
	CleanupPlayStream();
    delete m_pMetadata;
    delete [] m_szURL;
}

ERESULT
CPlayStream::SetSong(IPlaylistEntry *pNewSong)
{
	ERESULT res;
	// attach to the new song

	IMediaContentRecord* pContentRecord = pNewSong->GetContentRecord();
	if (pContentRecord == NULL) {
		DEBUG( MP, DBGLEV_ERROR, "Couldn't get a contentrecord pointer\n");
	}
    DEBUG( MP, DBGLEV_INFO, "Opening URL %s\n", pContentRecord->GetURL());

    delete [] m_szURL;
    m_szURL = new char[strlen(pContentRecord->GetURL()) + 1];
    strcpy(m_szURL, pContentRecord->GetURL());

    // find the source
	m_pDataSource = s_pDSM->GetDataSourceByID(pContentRecord->GetDataSourceID());
	
	if( m_pDataSource == NULL ) {
		DEBUG( MP, DBGLEV_ERROR, "Couldn't get a datasource pointer\n" );
		return MP_ERROR;
	}
	// create the in stream
	m_pInputStream = s_pMediaPlayerImp->m_pfnCreateInputStream( pContentRecord );

	if( !m_pInputStream ) {
		DEBUG( MP, DBGLEV_ERROR, "failed to open input stream, URL: %s\n", pContentRecord->GetURL() );
		return MP_ERROR;
	}
	// if we need metadata and the medplyr is offering, get it
	if( !m_pMetadata && s_pMediaPlayerImp->m_pfnCreateMetadata ) {
		m_pMetadata = (*s_pMediaPlayerImp->m_pfnCreateMetadata)();
	}
	
	// if we can create a custom playstream, do so.
    bool bFoundSettings = false;
	if( s_pMediaPlayerImp->m_pfnCreatePlayStream ) {
		bFoundSettings = s_pMediaPlayerImp->m_pfnCreatePlayStream( pContentRecord, (CPlayStreamSettings*)this );
	}
	if( !bFoundSettings ) {
		if( !s_pMediaPlayerImp->m_pSettings ) {
			DEBUG( MP, DBGLEV_ERROR, "no play settings\n");
			return MP_ERROR;
		}
		else {
            this->CopySettings( s_pMediaPlayerImp->m_pSettings );
		}
	}

	res = this->FindCodec(pContentRecord);
	if (FAILED(res)) {
		return res;
	}
	
	//
	///////////////////////////////////////////////////////////////////////////
	// at this point, we know for sure that we have an input stream and a codec
	// set up our filters
	///////////////////////////////////////////////////////////////////////////
	//
	
	// set up the filter_stream_info_t structure
	m_filterStreamInfo.m_bIsPcm = true;
	m_filterStreamInfo.un.pcm.Channels          = m_streamInfo.Channels;
	m_filterStreamInfo.un.pcm.SamplingFrequency = m_streamInfo.SamplingFrequency;
	m_filterStreamInfo.un.pcm.Bitrate           = m_streamInfo.Bitrate;
	m_filterStreamInfo.un.pcm.Duration          = m_streamInfo.Duration;

    if( GetStreamName() ) {
        strcpy( m_filterStreamInfo.m_szStreamName, GetStreamName() );
    } else {
        // just start the string with a null
        m_filterStreamInfo.m_szStreamName[0] = 0;
    }
	
	
	// count the number of filters in the output chain
	int count = 0;
	while( m_FilterList[count] ) {
		count++;
	}
	// now count the number of outfilters in the output chain
	while( m_OutputList[count] ) {
		count++;
	}
	
#if defined(SUPPORT_SOFTWARE_SRC)
	// If we need to use software SRC, factor in a spot for it
	if( m_streamInfo.SamplingFrequency != 44100 ) {
        if( m_streamInfo.SamplingFrequency < (unsigned) s_pMediaPlayerImp->m_iSRCBlendLevel ) {
            count++;
        }
	}
#endif // SUPPORT_SOFTWARE_SRC
	
	// use count+1 for the list terminator
	this->m_pFilters = new (IFilter*)[count+1];
	memset( (void*) this->m_pFilters, 0, (count+1) * sizeof( IFilter* ) );
	
	
	
	//
	// This section of setting up the playstream used to be much more
	// ugly. it was written (before) as follows:
	// 1) create codec RB
	// 2) see if we need SRC
	//    if yes, create the src filter and a new RB
	// 3) for each filter, create the filter and a new RB
	// 4) create one outfilter and attach it
	// 5) create the output stream and attach it
	// This caused duplication of code and limited us to one outfilter.
	// Since outfilter has been rewritten to support chaining, the following
	// is a more sensible version:
	// 1) create codec RB
	// 2) see if we need SRC
	//    if yes, make sure we create the SRC filter first
	//    if no, make sure we create the first filter in the list first
	// 3) for each filter in the list, create the filter and a new RB
	//    if we run out of filters, switch to output streams and make outfilters
	//    if we are making outfilters, create an output stream and attach it
	//
	
	rbuf_t* rb;
	rbuf_reader_t* reader;
	rbuf_writer_t* writer;
	int in_unit, out_unit, unit_size;
	
	// set up the codec by getting handles to an unallocated buffer...:>
	rb = rb_new( 0 );
	writer = rb_new_writer( rb );
	(this->m_pCodec)->SetWriteBuf( writer );
	
	// determine how much data the codec will write out to this thing
	out_unit = (this->m_pCodec)->GetOutputUnitSize();
	
	// if we have to do SRC, do it first
	const unsigned int* ListPtr;
	int iSrcIndex, iDestIndex = 0;
	int iFilterID;
	
	// Prime the loop
	// if there are filters
	if( m_FilterList[0] ) {
		// set the list pointer
		ListPtr = m_FilterList;
		iFilterID = ListPtr[0];
		// point to next item
		iSrcIndex = 1;
	} else {
		// set the list pointer
		ListPtr = m_OutputList;
		iFilterID = OUTFILTER_KEY;
		// point to tip
		iSrcIndex = 0;
	}
	
#if defined(SUPPORT_SOFTWARE_SRC)
	// modify the above priming if we have to do src
	if( m_streamInfo.SamplingFrequency != 44100 ) {
        if( m_streamInfo.SamplingFrequency < (unsigned)s_pMediaPlayerImp->m_iSRCBlendLevel ) {
            iFilterID = SRC_FILTER_ID;
            // switch the loop back into filter mode
            ListPtr   = m_FilterList;
            // abuse this variable
            iSrcIndex = -1;
        }
	}
#endif  // SUPPORT_SOFTWARE_SRC
	
	// Filter load and setup loop. This is actually simpler than the previous
	// implementation and has less duplicated code.
	// The trick is that we use a pointer into the filter list, create all the
	// filters, then use a pointer into the output list, and create OutFilters
	bool bLastWasReadOnly = false;
	do {
		ERESULT fres;
		// Load
		this->m_pFilters[iDestIndex] = s_pFM->LoadFilter( iFilterID );
		if( this->m_pFilters[iDestIndex] == NULL ) {
			DEBUG(MP, DBGLEV_ERROR, "mediaplayer couldn't load filter %x\n", iFilterID );
			res = MP_ERROR;
			goto bad_playstream;
		}
		
		// If we are doing outfilters, then set up the output stream
		if( iFilterID == OUTFILTER_KEY ) {
			IOutputStream* pOutputStream = s_pOM->LoadOutputStream( ListPtr[ iSrcIndex ] );
			
			if( !pOutputStream  ) {
				DEBUG( MP, DBGLEV_ERROR, "mediaplayer couldn't find output stream %x\n", ListPtr[ iSrcIndex ] );
				res = MP_ERROR;
				goto bad_playstream;
			}
#ifdef SUPPORT_HARDWARE_SRC
            // 12/04/01 dc  special case to handle hardware SRC via configuration in waveout
            if( ListPtr[ iSrcIndex ] == WAVEOUT_KEY ) {
                ERESULT r = pOutputStream->Ioctl( KEY_WAVEOUT_SET_SAMPLERATE, (void*)&m_filterStreamInfo.un.pcm.SamplingFrequency );
                if( FAILED( r ) ) {
                    res = MP_ERROR;
                    goto bad_playstream;
                }
            }
#endif  // SUPPORT_HARDWARE_SRC
			
			// set the output stream _then_ configure the outfilter; this allows the filter stream info to
			// be propogated down if needed
			(this->m_pFilters[iDestIndex])->Ioctl( KEY_OUTFILTER_SET_OUTPUTSTREAM, (void*)pOutputStream );
        }
#ifdef SUPPORT_SOFTWARE_SRC
		else if( iFilterID == SRC_FILTER_ID ) {
            DEBUG( MP, DBGLEV_WARNING, "setting software src for sr %d blendlevel %d\n",
                m_streamInfo.SamplingFrequency,s_pMediaPlayerImp->m_iSRCBlendLevel );
            (this->m_pFilters[iDestIndex])->Ioctl( SRC_FILTER_IOCTL_SET_MIN_OUTPUT, (void*) s_pMediaPlayerImp->m_iSRCBlendLevel );
        }
#endif  // SUPPORT_SOFTWARE_SRC
		
		// Configure the filter
		fres = (this->m_pFilters[iDestIndex])->Configure( m_filterStreamInfo );
		if( FAILED( fres ) ) {
			DEBUG( MP, DBGLEV_ERROR, "could not configure filter 0x%x\n", iFilterID );
			res = MP_ERROR;
			goto bad_playstream;
		}
		
		// Figure out unit sizes
		in_unit = (this->m_pFilters[iDestIndex])->GetInputUnitSize();
		unit_size = m_iBufferMultiplier * (in_unit > out_unit ? in_unit : out_unit );
		
		// Adjust the buffer we are reading from
		// TODO might want to spend time thinking about buffer sizing
		//  with multiple readers..
		if( !bLastWasReadOnly ) {
			rb_resize( rb, unit_size );
		}
		reader = rb_new_reader( rb );
		(this->m_pFilters[iDestIndex])->SetReadBuf( reader );
		
		// See if we have any more work to do
		// Point to the next item to process
		iSrcIndex++;
		// If we are doing filters
		if( ListPtr == m_FilterList ) {
			// see if we have run out
			if( ListPtr[ iSrcIndex ] == 0 ) {
				// switch to output streams if we have
				iSrcIndex = 0;
				ListPtr = m_OutputList;
				iFilterID = OUTFILTER_KEY;
			}
			else {
				// otherwise grab the next filter
				iFilterID = ListPtr[ iSrcIndex ];
			}
		}  else {
			// since we are doing output streams, see if we have
			// any more. otherwise break out of the loop
			if( ListPtr[ iSrcIndex ] == 0 ) {
				break;
			}
		}
		
		// At this point we must have more work. If the last
		// filter wasn't read only, then set up a new write buffer here
		if( fres != FILTER_READONLY ) {
			rb = rb_new( 0 );
			writer = rb_new_writer( rb );
			(this->m_pFilters[iDestIndex])->SetWriteBuf( writer );
			out_unit = (this->m_pFilters[iDestIndex])->GetOutputUnitSize();
			
			bLastWasReadOnly = false;
		} else {
			bLastWasReadOnly = true;
		}
		
		
		iDestIndex++;
		// TODO should this have a bounds check?
	} while( 1 );
	
	pContentRecord->SetStatus(IContentRecord::CR_OKAY);
	return MP_NO_ERROR;
	
bad_playstream:
	CleanupPlayStream();
	return MP_ERROR;
}

void
CPlayStream::CleanupPlayStream()
{
	if( m_pCodec == NULL ) {
        if (m_pInputStream)
        {
            // If the codec couldn't play the file (e.g., DRM issues) then this will make
            // sure that the file is closed.
		    m_pInputStream->Close();
		    delete m_pInputStream;
            m_pInputStream = 0;
        }
		return ;
	}
	
	rbuf_t* rbuf;
	rbuf_writer_t* writer;
	rbuf_reader_t* reader;
	SimpleList< rbuf_writer_t* > stk;
	
	// clean up the input stream
	if (IInputStream* pInputStream = m_pCodec->GetInputStream())
	{
		pInputStream->Close();
		delete pInputStream;
	}
    m_pInputStream = 0;
	
	// clean up the codec
	writer = m_pCodec->GetWriteBuf();
	
	DeleteCodec();
	
	if( writer ) {
		stk.PushFront( writer );
	}
	
	// start the filter cleanup loop
	for( int i = 0; m_pFilters[i]; i++ ) {
		reader = m_pFilters[i]->GetReadBuf();
		
		// release the reader now
        if( reader ) {
            rb_free_reader( reader );
        }
		
		// look for a writer
		writer = m_pFilters[i]->GetWriteBuf();
		
		delete m_pFilters[i];
		
		if( writer ) {
			stk.PushFront( writer );
		}
	}
	
	// cleanup the writers and the rbufs
	
	while( !stk.IsEmpty() ) {
		writer = stk.PopFront();
		rbuf = rb_write_rbuf( writer );
		rb_free_writer( writer );
		rb_free( rbuf );
	}
	delete [] m_pFilters;
}

ERESULT
CPlayStream::FindCodec(IMediaContentRecord* pContentRecord)
{
	ERESULT res = CODEC_FAIL;
	
	if( s_pMediaPlayerImp->m_pCodecPoolAddress ) {
		m_bCodecFromHeap = false;
	} else {
		m_bCodecFromHeap = true;
	}
	
    // create the codec in sram
	this->m_pCodec = s_pCM->FindCodec( pContentRecord->GetCodecID(),
		(void*) s_pMediaPlayerImp->m_pCodecPoolAddress, s_pMediaPlayerImp->m_iCodecPoolSize);
	
	res = TestCodec(pContentRecord);

	if (res != MP_TRY_NEXT_CODEC)
		return res;
	
	// did not work with the codec we found based on codec ID, so
	// try and parse a file extension
	const char* pURL = pContentRecord->GetURL();
	const char* pExt = pURL + strlen( pURL ) - 1;
	while( (pExt > pURL) && *pExt != '.' ) {
		pExt--;
	}
	if( *pExt == '.' ) {
		pExt++;
		this->m_pCodec = s_pCM->FindCodec( pExt, (void*) s_pMediaPlayerImp->m_pCodecPoolAddress, s_pMediaPlayerImp->m_iCodecPoolSize);
		res = TestCodec(pContentRecord);
		if (res != MP_TRY_NEXT_CODEC)
			return res;
	}
	
	
	// at this point, we have to probe through available codecs
	// and see if we can find one that will load the song
	for( int i = 0; (this->m_pCodec = s_pCM->TryCodec(i)); i++ ) {
		res = TestCodec(pContentRecord);
		
		if (res != MP_TRY_NEXT_CODEC) {
			return res;
		}
	}
	
	// I took this from CMediaPlayerImp, seems weird but I won't poke at it.
	pContentRecord->SetStatus(IContentRecord::CR_DRM_FAILURE);
	
	DEBUG( MP, DBGLEV_ERROR, "failed to find an appropriate codec\n");

	return MP_ERROR;
}

void
CPlayStream::DeleteCodec()
{
	if( m_bCodecFromHeap ) {
		delete this->m_pCodec;
	}
	else {
		this->m_pCodec->~ICodec();
	}
	this->m_pCodec = NULL;
}


// Attempts to configure the current codec with the current input stream. Possible returns:
//	MP_NO_ERROR		Codec recognized the stream
//	MP_TRY_NEXT_CODEC	Codec didn't recognize the stream
//	MP_ERROR		Something went badly enough wrong that we needn't bother
//					trying anything else. 
ERESULT
CPlayStream::TestCodec(IMediaContentRecord* pContentRecord)
{
	ERESULT res;
	
	if( this->m_pCodec ) {
		res = (this->m_pCodec)->SetSong( m_pDataSource, m_pInputStream, m_streamInfo, m_pMetadata );
		if( FAILED(res) ) {
			DeleteCodec();
			if (res == CODEC_DRM_FAIL) {
				// if this failed because of drm, then give up
				pContentRecord->SetStatus(IContentRecord::CR_DRM_FAILURE);
				return MP_ERROR;
			}
			if( m_pInputStream->CanSeek() ) {
				m_pInputStream->Seek( IInputStream::SeekStart, 0 );
				return MP_TRY_NEXT_CODEC;
			} else {
				DEBUG( MP, DBGLEV_ERROR, "codec->SetSong fail on a non seekable input stream\n");
				// TODO the input stream is now really fucked
				return MP_ERROR;
			}
		}
		return MP_NO_ERROR;
	}
	return MP_TRY_NEXT_CODEC;
}
