//
// MediaPlayerImp.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

// 07/11/02 dc
//   restructuring of the usage of the MP thread control flag
//   previously this flag was used in a hodge-podge way, where someone would set bits and
//   someone else might clear them. the usage was actually incorrect, and it was open to race conditions.
//   there is still the potential for race conditions, but they should be non malicious (glaring example is the caller
//   issuing 'play' and 'stop' before the MP wakes up).
//
//   the new form is for the API to only make calls to cyg_flag_setbits(), i.e. the API only tells the MP what it wants
//   to do. from there, the MP thread is responsible for checking the bits for state changes (i.e. PLAYING when it is stopped,
//   STOPPED or TERMINATE when it is playing) and clearing stale bits for the transition. the only time the MP thread calls setbits
//   is when an EOF happens and no autochange is possible.

// 07/15/02 dc
//   again restructuring, in order to dodge the problems of the above solution
//   now the flag is used in a command->response way. the media player thread waits for command bits (PLAY, STOP, TERMINATE)
//   to be set in the flag; when they are set, it attempts to process the command, and sets either PLAYING, STOPPED, TERMINATED, or
//   error bits, while clearing the command bits. since the flag does not provide any sense of time, it is still possible to issue
//   STOP and PLAY commands in quick succession and get mixed results from the mediaplayer, but only if the calls are made from seperate
//   threads. this is because commands issued to the mediaplayer are now properly synchronized, such that when PLAY is issued, the
//   caller waits on the flag until PLAYING or ERROR comes back. this should resolve the previous nasty race conditions. Longer term
//   it should be possible to get rid of the m_ePlayState variable and have that simply be a check of the flag state.

#include "MediaPlayerImp.h"

#include <pkgconf/kernel.h>  // kernel configuration

#include <core/events/SystemEvents.h>
#include <core/mediaplayer/PlayStream.h>

// data access managers
#include <datasource/datasourcemanager/DataSourceManager.h>

#ifdef DDOMOD_DJ_BUFFERING
#include <main/buffering/BufferInStream.h>
#endif

// interfaces
#include <codec/common/Codec.h>              // ICodec definition
#include <datastream/filter/Filter.h>        // IFilter definition
#include <datastream/input/InputStream.h>    // IInputStream
#include <playlist/common/Playlist.h>        // IPlaylistEntry

#include <util/eventq/EventQueueAPI.h>     // for main event queue
#include <util/debug/debug.h>              // for debug system

#include <util/utils/utils.h>              // strdup_new

DEBUG_MODULE( MP );
DEBUG_USE_MODULE( MP );

#define PARANOID_LOCK()   cyg_mutex_lock(&m_PlayStreamMutex)
#define PARANOID_UNLOCK() cyg_mutex_unlock(&m_PlayStreamMutex)
//#define PARANOID_LOCK()
//#define PARANOID_UNLOCK()

// Create a simple, unbuffered input stream
IInputStream* CreateSimpleInputStream( IMediaContentRecord* pMediaContentRecord );

CMediaPlayerImp* CMediaPlayerImp::s_pSingleton = 0;

CMediaPlayerImp* CMediaPlayerImp::GetInstance() 
{
    if( s_pSingleton == 0 ) {
        s_pSingleton = new CMediaPlayerImp;
    }
    return s_pSingleton;
}
void CMediaPlayerImp::Destroy() 
{
    if( s_pSingleton ) delete s_pSingleton;
    s_pSingleton = NULL;
}

CMediaPlayerImp::CMediaPlayerImp() 
{
    DBEN( MP );
	
    m_pEventQueue  = CEventQueue::GetInstance();
	
    m_pCurrentStream = NULL;
    m_pNextStream = NULL;
    m_ePlayState   = CMediaPlayer::NOT_CONFIGURED;
    
    m_pfnCreateMetadata = NULL;
    m_pfnCreatePlayStream = NULL;
    m_pfnCreateInputStream = &CreateSimpleInputStream;
    
    m_pCodecPoolAddress = 0;
    m_iCodecPoolSize = 0;
    
    m_iSRCBlendLevel = 44100;  // default to all software SRC
    m_pSettings = new CPlayStreamSettings;
    
    // create our flag
    cyg_flag_init( &m_Flag );
    
    cyg_mutex_init( &m_PlayStreamMutex );
    // set the current mode to stopped
    ClearBits( ~(0x00) );
    SetBits( FLAG_STOPPED );
    
    // create our thread
    cyg_thread_create( DECODER_THREAD_PRIORITY,
                       CMediaPlayerImp::MediaPlayerEntryPoint,
                       (cyg_addrword_t) this,
                       "Decode thread",
                       (void*) m_DecoderThreadStack,
                       DECODER_THREAD_STACK_SIZE,
                       &m_hDecoderThread,
                       &m_DecoderThreadData );
    
    cyg_thread_resume( m_hDecoderThread );
    
    DBEX( MP );
}

CMediaPlayerImp::~CMediaPlayerImp() 
{
    DBEN( MP );
	
    this->Deconfigure();

    SetBits( FLAG_TERMINATE );
	
    // reverse the flag so we are now waiting on it
    WaitForBits( FLAG_TERMINATED );
	
    // now that the thread is killed, destroy the flag and release resources
    cyg_flag_destroy( &m_Flag );
	
    cyg_thread_suspend( m_hDecoderThread );
    while( !cyg_thread_delete( m_hDecoderThread ) ) {
        // this should never have to execute
        cyg_thread_delay( 1 );
    }

    delete m_pSettings;
    //    delete m_pPlaystream;
    //    delete m_pStreamInfo;
    //    delete m_pMetadata;
        
    DBEX( MP );
}
// Static entry point
void CMediaPlayerImp::MediaPlayerEntryPoint( cyg_addrword_t data ) 
{
    DBEN( MP );
	
    reinterpret_cast<CMediaPlayerImp*>(data)->MediaPlayerThread();
	
    // the following line should never execute
    DBEX( MP );
}

void CMediaPlayerImp::MediaPlayerThread() 
{
    DBEN( MP );
	
    cyg_flag_value_t flgRes;
    bool bEOF;
    ERESULT res = MAKE_ERESULT(SEVERITY_SUCCESS,0,0);
	
    for(;;) {
        flgRes = WaitForBits( FLAG_INTERNAL_WAIT_ALL );

        // Clear any residual error condititions
        ClearBits( FLAG_ERROR );

        // Check for terminate command
        if( flgRes & FLAG_TERMINATE ) {
            // assumption: Deconfigure() was called prior to setting TERMINATE bit
            break;
        }

        // Check for stop command
        if( flgRes & FLAG_STOP ) {
            // This is a race condition that happens occaisonally; the player is actively playing,
            //  but the user issues a stop command during an autoset transition. Since we dont know
            //  which command was chronologically issued first (i.e. is this a stale stop?) the safest thing
            //  to do is come out of play mode and go into stop mode.

            ClearBits( FLAG_STOP | FLAG_PLAY | FLAG_PLAYING );
            SetBits( FLAG_STOPPED );

            // Continue through the loop, waiting for the next command
            continue;
        }
        
        // Check for play command or for the playing bit; on autoset transitions this bit is not cleared
        //  and no new commands are sent to the MP
        if( (flgRes & FLAG_PLAY) || (flgRes & FLAG_PLAYING) ) {            

            PARANOID_LOCK();
            
            // Verify configured to play
            if( m_ePlayState == CMediaPlayer::NOT_CONFIGURED && m_pCurrentStream ) {
                PARANOID_UNLOCK();
                
                // Clear the play command, issue an error, and loop back to WaitForBits()
                ClearBits( FLAG_PLAY | FLAG_PLAYING );
                SetBits( FLAG_STOPPED | FLAG_ERROR );
                continue;
            }

            // these unequal, which causes the condition in the below do{}while to pass
            m_pCurrentStream->m_ulLastTrackTime = ~0;
            m_pCurrentStream->m_ulTrackTime     = 0;
            
            PARANOID_UNLOCK();
            
            // Synchronize 'STOPPED' state bit
            if( flgRes & FLAG_STOPPED ) {
                ClearBits( FLAG_STOPPED );
            }

            SetBits( FLAG_PLAYING );
            
            // Set soft state
            m_ePlayState = CMediaPlayer::PLAYING;

            m_pEventQueue->PutEvent( EVENT_STREAM_PLAYING, 0 );
			
            bEOF = false;
			
            while( !bEOF ) {
                // Check to see if there are any new flags
                flgRes = CheckBits( FLAG_INTERNAL_WAIT_ALL );

                // Check for PLAY command and clear, since we are already playing
                if( flgRes & FLAG_PLAY ) {
                    ClearBits( FLAG_PLAY );
                }

                // Check for STOP command
                PARANOID_LOCK();                
                if( flgRes & FLAG_STOP || !m_pCurrentStream ) {
                    DEBUG( MP, DBGLEV_WARNING, "play loop stopping\n");
                    PARANOID_UNLOCK();
                    break;
                }
                PARANOID_UNLOCK();

                // Decode pump
                PARANOID_LOCK();
                do {
                    unsigned long ulTrackTime;
                    
                    if (m_pCurrentStream) {
                        res = m_pCurrentStream->m_pCodec->DecodeFrame( m_pCurrentStream->m_ulTrackTime );
					
                        // TODO analyze a precalculated track progress rate indicator
                        //  determine whether or not we should trigger an event
                        if( m_pCurrentStream->m_ulTrackTime != m_pCurrentStream->m_ulLastTrackTime ) {
                            // send an update message
                            m_pEventQueue->TryPutEvent( EVENT_STREAM_PROGRESS, (void *)m_pCurrentStream->m_ulTrackTime );
                            m_pCurrentStream->m_ulLastTrackTime = m_pCurrentStream->m_ulTrackTime;
                        }
                    }
                    else {
                        // No m_pCurrentStream, say there was a decode error
                        DEBUG(MP, DBGLEV_ERROR, "m_pCurrentStream is NULL\n");
                        res = CODEC_DECODE_ERROR;
                    }
                } while( res == CODEC_NO_ERROR );
                PARANOID_UNLOCK();
				
                // Check the decode result
                if( res == CODEC_END_OF_FILE ) {
                    bEOF = true;
                }
                else if( res != CODEC_NO_WORK ) {
                    DEBUG(MP, DBGLEV_ERROR, "codec gave back error result 0x%08x\n", res);
                    break;
                }
				
                // Check again to see if STOP was issued during decode phase
                flgRes = CheckBits( FLAG_INTERNAL_WAIT_ALL );
				
                if( flgRes & FLAG_STOP ) {
                    DEBUG( MP, DBGLEV_WARNING, "play loop stopping (flag triggered)\n");
                    break;
                }
				
                if( bEOF ) {
                    // Flushing loop - force all audio out of rbufs in the filter chain
                    PARANOID_LOCK();
                    if (m_pCurrentStream) {
                        for( int i = 0; m_pCurrentStream->m_pFilters[i]; ) {
                            for( int z = i; m_pCurrentStream->m_pFilters[z]; z++ ) {
                                do {
                                    res = m_pCurrentStream->m_pFilters[z]->DoWork(true);
                                } while( res == FILTER_NO_ERROR );
                                
                                if( res == FILTER_EOF && i == z) {
                                    i++;
                                }
                                else if( res != FILTER_NO_WORK ) {
                                    DEBUG(MP, DBGLEV_ERROR, "filter %d gave back error result 0x%08x\n", i, res );
                                    break;
                                }
                            }
                        }
                    }
                    else {
                        // No m_pCurrentStream, so assume no more work to do
                    }
                    PARANOID_UNLOCK();
                } else {
                    PARANOID_LOCK();
                    if (m_pCurrentStream) {
                        for( int i = 0; m_pCurrentStream->m_pFilters[i]; i++ ) {
                            do {
                                res = m_pCurrentStream->m_pFilters[i]->DoWork();
                            } while( res == FILTER_NO_ERROR );
                            
                            // this may not be correct - filters can potentially give back EOF. maybe this
                            // behavior should change
                            if( res != FILTER_NO_WORK && res != FILTER_EOF ) {
                                DEBUG(MP, DBGLEV_ERROR, "filter %d gave back error result 0x%08x\n", i, res );
                                break;
                            }
                        }
                    }
                    else {
                        // No m_pCurrentStream, so assume no more work to do
                    }
                    PARANOID_UNLOCK();
                }
            }
			
            // at this point, either we aborted out of the loop, or bEOF is true
            // first, check the flag results
            if( flgRes & FLAG_STOP ) {
                // STOP command was issued; clear STOP command, stale PLAY command, and PLAYING state
                ClearBits( FLAG_STOP | FLAG_PLAYING );
                // Set STOPPED state
                SetBits( FLAG_STOPPED );
                // Synchronize soft state
                m_ePlayState = CMediaPlayer::STOPPED;
            } else {
                // No flag command issued, so decode step broke out
                if( bEOF ) {
                    PARANOID_LOCK();
                    if (m_pNextStream) {
                        // Stay in play mode, but switch to the next stream
						
                        // Copy the necessary fields to the change_stream_event_data_t
                        if (m_pCurrentStream)
                            m_pCurrentStream->CleanupPlayStream();

#ifdef DDOMOD_DJ_BUFFERING
                        CBuffering::GetInstance()->NotifyStreamAutoset();
#endif

                        change_stream_event_data_t* pCSED = new change_stream_event_data_t;
                        pCSED->pPreviousStream = m_pCurrentStream;
                        pCSED->szURL = strdup_new(m_pNextStream->GetURL());
                        pCSED->pMediaPlayerMetadata = m_pNextStream->m_pMetadata;
                        pCSED->streamInfo = m_pNextStream->m_streamInfo;

                        m_pCurrentStream = m_pNextStream;
                        m_pNextStream    = 0;
                        
                        PARANOID_UNLOCK();
                        m_pEventQueue->PutEvent( EVENT_STREAM_AUTOSET, pCSED );
                    } else {
                        // Go to deconfigured state
                        DEBUG( MP, DBGLEV_INFO, "EOF, stopping\n");

                        // On an EOF, we clear the playing bit and set the stopped bit
                        ClearBits( FLAG_PLAYING );
                        SetBits( FLAG_STOPPED );

                        change_stream_event_data_t* pCSED = 0;
                        if( m_pCurrentStream ) {
                            m_pCurrentStream->CleanupPlayStream();
                            pCSED = new change_stream_event_data_t;
                            pCSED->szURL = strdup_new(m_pCurrentStream->GetURL());
                            pCSED->pMediaPlayerMetadata = m_pCurrentStream->m_pMetadata;
                            pCSED->streamInfo = m_pCurrentStream->m_streamInfo;
                            pCSED->pPreviousStream = m_pCurrentStream;
                            m_pCurrentStream = 0;
                        }
                        m_ePlayState = CMediaPlayer::NOT_CONFIGURED;
                        
                        PARANOID_UNLOCK();
                        m_pEventQueue->PutEvent( EVENT_STREAM_END, pCSED );
                    }
                } else {
                    // No EOF, so we got an error. Dont set the ERROR flag since this wasn't
                    //  in response to an external call
                    ClearBits( FLAG_PLAYING );
                    SetBits( FLAG_STOPPED );
					
                    // generate a STREAM_FAIL message here
                    // we can analyze the error a little bit here
                    change_stream_event_data_t* pCSED = 0;
                    PARANOID_LOCK();
                    if( m_pCurrentStream ) {
                        m_pCurrentStream->CleanupPlayStream();
                        pCSED                       = new change_stream_event_data_t;
                        pCSED->szURL                = strdup_new(m_pCurrentStream->GetURL());
                        pCSED->pMediaPlayerMetadata = m_pCurrentStream->m_pMetadata;
                        pCSED->streamInfo           = m_pCurrentStream->m_streamInfo;
                        pCSED->pPreviousStream      = m_pCurrentStream;
                        m_pCurrentStream            = 0;
                    }
                    PARANOID_UNLOCK();
                    m_ePlayState = CMediaPlayer::NOT_CONFIGURED;
                    m_pEventQueue->PutEvent( EVENT_STREAM_FAIL, pCSED );
                }
            }
        }
    }
    
    // exit code
    
    DBEX( MP );
    
    SetBits( FLAG_TERMINATED );
    
    // the following line should never execute
    // since the scheduler will wake on the above setbits
    DBEX( MP );
}


void CMediaPlayerImp::StopPlayback() 
{
    if( !( CheckBits( FLAG_STOPPED ) & FLAG_STOPPED ) ) {
        SetBits( FLAG_STOP );
        // TODO: error flag is ignored here
        WaitForBits( FLAG_EXTERNAL_WAIT_STOP );
    }

    // If there's a playsteam, then pass an event to the play manager so it can be deleted.

    // This variable exists so that messages are not Put() while the mutex
    // is locked.  Doing so causes deadlock when the message queue is near full.
    bool                        bPutEvent = false;
    change_stream_event_data_t* pCSED;
        
    cyg_mutex_lock(&m_PlayStreamMutex);
    if( m_pCurrentStream ) {
        m_pCurrentStream->CleanupPlayStream();
        pCSED                       = new change_stream_event_data_t;
        pCSED->szURL                = strdup_new(m_pCurrentStream->GetURL());
        pCSED->pMediaPlayerMetadata = m_pCurrentStream->m_pMetadata;
        pCSED->streamInfo           = m_pCurrentStream->m_streamInfo;
        pCSED->pPreviousStream      = m_pCurrentStream;
        m_pCurrentStream            = 0;
        bPutEvent                   = true;
    }
    cyg_mutex_unlock(&m_PlayStreamMutex);
    if (bPutEvent)
        m_pEventQueue->PutEvent( EVENT_STREAM_STOPPED, pCSED );
}

void CMediaPlayerImp::PausePlayback( bool bPassEvent ) 
{
    if( !(CheckBits( FLAG_STOPPED ) & FLAG_STOPPED ) )
    {
        SetBits( FLAG_STOP );
        // TODO: error flag is ignored here
        WaitForBits( FLAG_EXTERNAL_WAIT_STOP );
    }

    if( bPassEvent ) {
        // generate a PAUSED event
        m_pEventQueue->PutEvent( EVENT_STREAM_PAUSED, 0 );
    }
}

ERESULT CMediaPlayerImp::SetPlayStreamSettings( const CPlayStreamSettings* pSettings ) 
{
    // TODO actually check the settings being passed in
    if( pSettings != NULL ) {
        m_pSettings->CopySettings( pSettings );
    }
    
    return MP_NO_ERROR;
}

void CMediaPlayerImp::SetCreateMetadataFunction(FNCreateMetadata* pfnCreateMetadata)
{
    m_pfnCreateMetadata = pfnCreateMetadata;
    // Clear out any cached metadata that may be of a different type.
}

void CMediaPlayerImp::SetCreateInputStreamFunction(FNCreateInputStream* pfnCreateInputStream)
{
    if (pfnCreateInputStream == 0)
        m_pfnCreateInputStream = &CreateSimpleInputStream;
    else
        m_pfnCreateInputStream = pfnCreateInputStream;
}

void CMediaPlayerImp::SetCreatePlayStreamFunction(FNCreatePlayStream* pfnCreatePlayStream)
{
    m_pfnCreatePlayStream = pfnCreatePlayStream;
}

void CMediaPlayerImp::Deconfigure() 
{
    if( m_ePlayState == CMediaPlayer::NOT_CONFIGURED ) return ;
    
    this->StopPlayback();

    cyg_mutex_lock(&m_PlayStreamMutex);

    m_pCurrentStream = 0;
    
    delete m_pNextStream;
    m_pNextStream = 0;

    cyg_mutex_unlock(&m_PlayStreamMutex);
    
    m_ePlayState = CMediaPlayer::NOT_CONFIGURED;
}

void CMediaPlayerImp::SetCodecPool( unsigned int* pPoolAddress, unsigned int iPoolSize ) 
{
    // This will take effect for the next codec the MP instantiates
    m_pCodecPoolAddress = pPoolAddress;
    m_iCodecPoolSize = iPoolSize;
}

ERESULT CMediaPlayerImp::SetSRCBlending( int MinSoftwareSampleRate ) 
{
    ERESULT res;
#if defined(SUPPORT_SOFTWARE_SRC) && defined(SUPPORT_HARDWARE_SRC)
    m_iSRCBlendLevel = MinSoftwareSampleRate;
    res = MP_NO_ERROR;
#else
    res = MP_UNAVAILABLE;
#endif
    
    return res;
}

int CMediaPlayerImp::QuerySRCBlending() const 
{
    return m_iSRCBlendLevel;
}

ERESULT CMediaPlayerImp::SetSong( IPlaylistEntry* pNewSong ) 
{
    ERESULT res = MP_NO_ERROR;
    
    //
    // Make sure we have a decent song pointer
    //
    if( !pNewSong ) {
        DEBUG( MP, DBGLEV_ERROR, "null song pointer\n");
        return MP_ERROR;
    }


    // Force the previous playstream to be deleted before creating this playstream.
    // TODO: remove this.
    this->Deconfigure();

    CPlayStream *tempStream = new CPlayStream();
    
    res = tempStream->SetSong(pNewSong);
    
    if (FAILED(res)) {
        DEBUG( MP, DBGLEV_ERROR, "Call to CPlayStream::SetSong failed\n");
        delete tempStream;
        return res;
    }
    
    return SetStream( tempStream );
}

ERESULT CMediaPlayerImp::SetStream( CPlayStream* pNewStream )	
{
    bool bPlaying = ( m_ePlayState == CMediaPlayer::PLAYING );

    this->Deconfigure();	// Deletes old CPlayStream, stops player.
	
    // Don't need to aquire the mutex because the player thread is sleeping
    m_pCurrentStream = pNewStream;
	
    m_ePlayState = CMediaPlayer::STOPPED;
    if( bPlaying ) {
        this->Play();
    }
	
    DEBUG( MP, DBGLEV_INFO, "setsong succeeded\n");
	
    // flag this content record as ok
    //    pContentRecord->SetStatus(IContentRecord::CR_OKAY);
	
    //
    // create and populate the steam set event data structure
    //
	
    set_stream_event_data_t* pSSED = new set_stream_event_data_t;
    pSSED->szURL = strdup_new(pNewStream->GetURL());
    pSSED->pMediaPlayerMetadata = pNewStream->m_pMetadata;
    pSSED->streamInfo = pNewStream->m_streamInfo;
    m_pEventQueue->PutEvent( EVENT_STREAM_SET, pSSED );
	
    return MP_NO_ERROR;
}

ERESULT CMediaPlayerImp::SetNextSong( IPlaylistEntry* pNewSong ) 
{
    ERESULT res = MP_NO_ERROR;

    if( !pNewSong ) {
        DEBUG( MP, DBGLEV_ERROR, "null song pointer\n");
        return MP_ERROR;
    }

    CDataSourceManager *pDSM = CDataSourceManager::GetInstance();
    int DSID = pNewSong->GetContentRecord()->GetDataSourceID();

    if( !pDSM->GetDataSourceByID(DSID)->QueryCanPrebuffer( pNewSong->GetContentRecord() ) ) {
        DEBUG( MP, DBGLEV_INFO, "Data source does not support prebuffering, ignoring\n");
        return MP_DIDNT_ENQUEUE;
    }
	
    CPlayStream *tempStream = new CPlayStream();
	
    res = tempStream->SetSong(pNewSong);
    if (FAILED(res)) {
        delete tempStream;
        return res;
    }
	
    cyg_mutex_lock(&m_PlayStreamMutex);
    delete m_pNextStream;
    m_pNextStream = 0;
	
    // If there's a current stream, then queue this one up
    if (m_pCurrentStream) {
        DEBUG( MP, DBGLEV_INFO, "Setting next song\n");
        m_pNextStream = tempStream;
        cyg_mutex_unlock(&m_PlayStreamMutex);
    } else {                    // If not, then start playing the "next" one
        DEBUG( MP, DBGLEV_INFO, "No current song, promoting next song to current\n");
        cyg_mutex_unlock(&m_PlayStreamMutex);
        return SetStream(tempStream);
    }
    return MP_NO_ERROR;
}
ERESULT CMediaPlayerImp::InvalidateNextSong()
{
    ERESULT res = MP_NO_ERROR;
    // todo: generate error code if no next stream? probably not
    if( m_pNextStream ) {
        cyg_mutex_lock(&m_PlayStreamMutex);

        delete m_pNextStream;
        m_pNextStream = 0;
        cyg_mutex_unlock(&m_PlayStreamMutex);
    }
    return res;
}

#if 0
StreamNode_t* CMediaPlayerImp::GetRoot() const 
{
    IDataStreamNode* res = NULL;
	
    if( m_ePlayState != CMediaPlayer::NOT_CONFIGURED ) {
        res = m_pPlaystream->pNodes[0];
    }
	
    return res;
    return NULL;
}

StreamNode_t* CMediaPlayerImp::AddNode( StreamNode_t* pRoot, unsigned int ChildID ) 
{
    return NULL;
}

ERESULT CMediaPlayerImp::RemoveNode( unsigned int NodeID ) 
{
    return CODEC_FAIL;
}

ERESULT CMediaPlayerImp::RemoveNode( StreamNode_t* pNode ) 
{
    return CODEC_FAIL;
}
#endif

ERESULT CMediaPlayerImp::Play() 
{
    cyg_flag_value_t flgRes;

    if( !(CheckBits( FLAG_PLAYING ) & FLAG_PLAYING )) {
        do {
            SetBits( FLAG_PLAY );
            // Put a quick timeout here in case the media player has cleared the PLAY flag between
            // when we set it and when the media player waits on it.
            flgRes = TimedWaitForBits( FLAG_EXTERNAL_WAIT_PLAY, cyg_current_time() + 10 );
        } while ( flgRes == 0 );
        
        // Handle failed play requests by returning MP_ERROR
        if( flgRes & FLAG_ERROR ) {
            return MP_ERROR;
        }
    }

    return MP_NO_ERROR;
}

ERESULT CMediaPlayerImp::Pause() 
{
    PausePlayback();
    return MP_NO_ERROR;
}

ERESULT CMediaPlayerImp::Stop() 
{
    if( m_ePlayState != CMediaPlayer::STOPPED && m_ePlayState != CMediaPlayer::NOT_CONFIGURED ) {
        this->StopPlayback();
    }
	
    return MP_NO_ERROR;
}

ERESULT CMediaPlayerImp::Seek( unsigned long secSeek ) 
{
    CMediaPlayer::PlayState ePS = m_ePlayState;

    if( ePS == CMediaPlayer::NOT_CONFIGURED ) {
        return MP_NOT_CONFIGURED;
    }
	
    if( ePS != CMediaPlayer::STOPPED ) {
        this->PausePlayback(false);
    }
	
    unsigned long sec = secSeek;
	
    PARANOID_LOCK();
    
    if (m_pCurrentStream)
        m_pCurrentStream->m_pCodec->Seek( sec );
    else
    {
        PARANOID_UNLOCK();
        return MP_NOT_CONFIGURED;
    }
    
    PARANOID_UNLOCK();

	
    ERESULT res = MP_NO_ERROR;
    if( sec != secSeek ) {
        DEBUG( MP, DBGLEV_WARNING, "Seek(%d) got back %d\n", secSeek, sec );
        res = MP_ERROR;
    }
	
    if( ePS == CMediaPlayer::PAUSED ) {
        this->Pause();
    }
    else if( ePS == CMediaPlayer::PLAYING ) {
        this->Play();
    }
	
    return MP_NO_ERROR;
}

CMediaPlayer::PlayState CMediaPlayerImp::GetPlayState() const 
{
    return m_ePlayState;
}

unsigned long CMediaPlayerImp::GetTrackTime() const 
{
    return m_pCurrentStream->m_ulTrackTime;
}

unsigned long CMediaPlayerImp::GetTrackLength() const 
{
    if( !m_pCurrentStream ) {
        return 0;
    }
    return m_pCurrentStream->m_streamInfo.Duration;
}

// Create a simple, unbuffered input stream
IInputStream* CreateSimpleInputStream( IMediaContentRecord* pMediaContentRecord )
{
    CDataSourceManager* pDSM = CDataSourceManager::GetInstance();
    return pDSM->OpenInputStream( pMediaContentRecord );
}

