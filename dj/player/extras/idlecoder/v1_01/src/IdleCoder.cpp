#include <cyg/kernel/kapi.h>
#include <codec/mp3/pem/pem.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <extras/idlecoder/IdleCoder.h>
#include <util/eresult/eresult.h>
#include <util/rbuf/rbuf.h>
#include <util/registry/Registry.h>
#include <util/debug/debug.h>
#include <util/utils/utils.h>
#include <stdlib.h>
#include <string.h>
#include <_modules.h>
#ifdef DDOMOD_DJ_METADATAFILETAG
#include <main/metadata/metadatafiletag/MetadataFileTag.h>
#endif

DEBUG_MODULE_S( IDLECODER, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( IDLECODER );

// Registry key settings
#define IDLE_CODER_REGISTRY_KEY_TYPE 0x1C           // 1D1E C0DE
#define IDLE_CODER_COUNT_REGISTRY_KEY_NAME  0x01    // Job count
static const RegKey IdleCoderJobCountRegKey = REGKEY_CREATE( IDLE_CODER_REGISTRY_KEY_TYPE, IDLE_CODER_COUNT_REGISTRY_KEY_NAME );

#define CHUNK_SIZE  (1152*4)
#define CHUNK_COUNT (16)
#define INPUT_BUF_SIZE (CHUNK_SIZE * CHUNK_COUNT)

#define OUTPUT_CHUNK_SIZE (16384)
#define OUTPUT_BUF_SIZE   (OUTPUT_CHUNK_SIZE*4)

const char * const
CIdleCoder::state_string[] = {
    "Not Ready",
    "Halted",
    "Idle",
    "Paused",
    "Starting",
    "Encoding",
    "Finishing",
    "Finished",
    "Error",
    "Exit"
};

// Job rec

CIdleCoder::job_rec_t::job_rec_t(const char *in_url, const char *out_url, int bitrate)
    : in(0), out(0), bitrate(bitrate), 
    time_started(0), last_saved(0), bytes_encoded(0),
    last_result(0), active(false)
{
    this->in_url = strdup_new(in_url);
    this->out_url = strdup_new(out_url);
}

CIdleCoder::job_rec_t::~job_rec_t()
{
    delete [] in_url;
    delete [] out_url;
}


// Statics

CIdleCoder *
CIdleCoder::GetInstance()
{
    if (!CIdleCoder::_instance) {
        CIdleCoder::_instance = new CIdleCoder(16, 16384);
        CIdleCoder::_instance->Start();
    }
    return CIdleCoder::_instance;			
}

void
CIdleCoder::Destroy()
{
    if( CIdleCoder::_instance ) delete CIdleCoder::_instance;
    CIdleCoder::_instance = NULL;
}

CIdleCoder *
CIdleCoder::_instance = 0;

// Members

CIdleCoder::CIdleCoder(cyg_addrword_t priority, cyg_ucount32 stacksize)
    : IThreadedObject(priority, "IdleEncoderThread", stacksize)
{
    cyg_mutex_init(&_busy);
    cyg_cond_init(&_wake, &_busy);
    cyg_mutex_init(&_jobs_lock);
    _eof = true;
    _pem = CPEMWrapper::GetInstance();
    _state = kNotReady;
    _job = 0;
    _callback = 0;
    
    //    _pem_state = NULL;
}

CIdleCoder::~CIdleCoder()
{
    cyg_mutex_lock(&_busy);
    _state = kExit;
    cyg_mutex_unlock(&_busy);
    cyg_cond_signal(&_wake);

    // IThreadedObject destructor will wait for our thread to exit
}


void
CIdleCoder::Enqueue(const char *in_url, const char *out_url, int bitrate)
{
    Enqueue(new job_rec_t(in_url, out_url, bitrate));
}

void
CIdleCoder::Enqueue(job_rec_t *j)
{
    cyg_mutex_lock(&_jobs_lock);
    _jobs.PushBack(j);
    DEBUG( IDLECODER, DBGLEV_INFO, "Job enqueued\n");
    // todo double check this
    if (_state != kPaused)
        cyg_cond_signal(&_wake);
    cyg_mutex_unlock(&_jobs_lock);
}

bool
CIdleCoder::RemoveJob(const char *in_url)
{
    cyg_mutex_lock(&_jobs_lock);
    
    for (SimpleListIterator<job_rec_t *> it = _jobs.GetHead(); it != _jobs.GetEnd(); ++it) {
        if (!strcmp(in_url, (*it)->in_url))
        {
            // handle the case where this is the current running job and we are paused
            if(!strcmp(_job->in_url, in_url) ) {
                DEBUG( IDLECODER, DBGLEV_WARNING, "Deleting active job\n");
                AbortStream();
                _job = 0;
                _state = kHalted;
            }
            
            delete _jobs.Remove(it);
            cyg_mutex_unlock(&_jobs_lock);
            return true;
        }
    }
    
    cyg_mutex_unlock(&_jobs_lock);
    return false;
}

void
CIdleCoder::RemoveAllJobs()
{
    cyg_mutex_lock(&_jobs_lock);
    
    while (!_jobs.IsEmpty())
        delete _jobs.PopFront();
    
    cyg_mutex_unlock(&_jobs_lock);
}

CIdleCoder::job_rec_t *
CIdleCoder::Peek()
{
    DBEN( IDLECODER );
    job_rec_t* j;
    cyg_mutex_lock(&_jobs_lock);
    if(_jobs.IsEmpty()) {
        j = 0;
    } else {
        // yes this looks wrong, but GetHead() returns an iterator, and * is the accessor
        j = *_jobs.GetHead();
    }
    cyg_mutex_unlock(&_jobs_lock);
    
    DBEX( IDLECODER );
    return j;
}

CIdleCoder::job_rec_t *
CIdleCoder::Dequeue()
{
    DBEN( IDLECODER );
    job_rec_t *j;
    cyg_mutex_lock(&_jobs_lock);
    if (_jobs.IsEmpty()) {
        j = 0;
    } else {
        j = _jobs.PopFront();
    }
    cyg_mutex_unlock(&_jobs_lock);
    
    DBEX( IDLECODER );
    return j;
}


void
CIdleCoder::Run()
{
    DEBUG( IDLECODER, DBGLEV_INFO, "Starting (or resuming) IdleCoder\n");
    cyg_mutex_lock(&_busy);
    _state = kStarting;
    //    if( _pem_state ) {
        //        _pem->RestoreState( _pem_state );
    //    }
    cyg_mutex_unlock(&_busy);
    cyg_cond_signal(&_wake);
}

void
CIdleCoder::Pause()
{
    DEBUG( IDLECODER, DBGLEV_INFO, "Pausing IdleCoder\n");
	cyg_mutex_lock(&_busy);
	_state = kPaused;
    // save the state of the encoder
    //    if( !_pem_state ) {
    //        _pem_state = (void*) new char[_pem->GetStateSize()];
    //    }
    //    _pem->SaveState( _pem_state, _pem->GetStateSize() );
	cyg_mutex_unlock(&_busy);
}

void
CIdleCoder::Halt()
{
    cyg_mutex_lock(&_busy);
    _state = kHalted;
    if (_job) {
        AbortStream();
        // the job will still be on the head of the queue
        //        Enqueue(_job);
        _job = 0;
    }
    //    if( _pem_state ) {
    //        delete _pem_state;
    //        _pem_state = NULL;
    //    }
    cyg_mutex_unlock(&_busy);
    DEBUG( IDLECODER, DBGLEV_INFO, "IdleCoder Suspended\n");
}

#define IC_REGKEY_IN_URL(x) REGKEY_CREATE(IDLE_CODER_REGISTRY_KEY_TYPE, 3 * (x) + IDLE_CODER_COUNT_REGISTRY_KEY_NAME + 1)
#define IC_REGKEY_OUT_URL(x) REGKEY_CREATE(IDLE_CODER_REGISTRY_KEY_TYPE, 3 * (x) + 1 + IDLE_CODER_COUNT_REGISTRY_KEY_NAME + 1)
#define IC_REGKEY_BITRATE(x) REGKEY_CREATE(IDLE_CODER_REGISTRY_KEY_TYPE, 3 * (x) + 2 + IDLE_CODER_COUNT_REGISTRY_KEY_NAME + 1)

void
CIdleCoder::SaveToRegistry()
{
    //  cyg_mutex_lock(&_busy);
    
    // Get the old job count so extra jobs can be deleted.
    CRegistry* pRegistry = CRegistry::GetInstance();
    unsigned int iPreviousJobCount = (unsigned int)pRegistry->FindByKey(IdleCoderJobCountRegKey);
    DEBUG( IDLECODER, DBGLEV_INFO, "Previous count: %d jobs\n", iPreviousJobCount);

    // Save the job count.
    pRegistry->RemoveItem(IdleCoderJobCountRegKey);
    DEBUG( IDLECODER, DBGLEV_INFO, "%d jobs\n", _jobs.Size());
    pRegistry->AddItem(IdleCoderJobCountRegKey, (void*)_jobs.Size(), REGFLAG_PERSISTENT, sizeof(int));
    
    SimpleListIterator<job_rec_t *> it = _jobs.GetHead();
    unsigned int i = 0;
    for (; it != _jobs.GetEnd(); ++i, ++it)
    {
        // Erase any previous jobs in the registry.
        if (char* job_url = (char*)pRegistry->FindByKey(IC_REGKEY_IN_URL(i)))
        {
            pRegistry->RemoveItem(IC_REGKEY_IN_URL(i));
            delete [] job_url;
        }
        if (char* job_url = (char*)pRegistry->FindByKey(IC_REGKEY_OUT_URL(i)))
        {
            pRegistry->RemoveItem(IC_REGKEY_OUT_URL(i));
            delete [] job_url;
        }
        pRegistry->RemoveItem(IC_REGKEY_BITRATE(i));
        
        // Save this job.
        job_rec_t* job = *it;
        
        DEBUG( IDLECODER, DBGLEV_INFO, "Job %d in url: %s\n", i, job->in_url);
        pRegistry->AddItem(IC_REGKEY_IN_URL(i), (void*)strdup_new(job->in_url), REGFLAG_PERSISTENT, strlen(job->in_url) + 1);
        DEBUG( IDLECODER, DBGLEV_INFO, "Job %d out url: %s\n", i, job->out_url);
        pRegistry->AddItem(IC_REGKEY_OUT_URL(i), (void*)strdup_new(job->out_url), REGFLAG_PERSISTENT, strlen(job->out_url) + 1);
        DEBUG( IDLECODER, DBGLEV_INFO, "Job %d bitrate: %d\n", i, job->bitrate);
        pRegistry->AddItem(IC_REGKEY_BITRATE(i), (void*)job->bitrate, REGFLAG_PERSISTENT, sizeof(int));
    }

    // Erase old jobs.
    while (i < iPreviousJobCount)
    {
        if (char* job_url = (char*)pRegistry->FindByKey(IC_REGKEY_IN_URL(i)))
        {
            pRegistry->RemoveItem(IC_REGKEY_IN_URL(i));
            delete [] job_url;
        }
        if (char* job_url = (char*)pRegistry->FindByKey(IC_REGKEY_OUT_URL(i)))
        {
            pRegistry->RemoveItem(IC_REGKEY_OUT_URL(i));
            delete [] job_url;
        }
        pRegistry->RemoveItem(IC_REGKEY_BITRATE(i));
        ++i;
    }

    //  cyg_mutex_unlock(&_busy);
}

void
CIdleCoder::LoadFromRegistry()
{
    //    cyg_mutex_lock(&_busy);

    // Load the job count.
    CRegistry* pRegistry = CRegistry::GetInstance();
    unsigned int job_count = (unsigned int)pRegistry->FindByKey(IdleCoderJobCountRegKey);
    DEBUG( IDLECODER, DBGLEV_INFO, "%d jobs\n", job_count);
    if (!job_count)
        // No jobs, so stop loading.
        return;
    
    // Clear the old job list.
    while (!_jobs.IsEmpty())
        delete _jobs.PopFront();
    
    // Load the jobs.
    for (unsigned int i = 0; i < job_count; ++i)
    {
        // Grab the input url
        char* job_in_url = (char*)pRegistry->FindByKey(IC_REGKEY_IN_URL(i));
        if (!job_in_url)
        {
            DEBUG( IDLECODER, DBGLEV_ERROR, "Error reading in_url for job %d\n", i);
            continue;
        }
        DEBUG( IDLECODER, DBGLEV_INFO, "Job %d in url: %s\n", i, job_in_url);
        
        // Grab the output url
        char* job_out_url = (char*)pRegistry->FindByKey(IC_REGKEY_OUT_URL(i));
        if (!job_out_url)
        {
            DEBUG( IDLECODER, DBGLEV_ERROR, "Error reading out_url for job %d\n", i);
            continue;
        }
        DEBUG( IDLECODER, DBGLEV_INFO, "Job %d out url: %s\n", i, job_out_url);
        
        // Grab the bitrate
        int job_bitrate = (int)pRegistry->FindByKey(IC_REGKEY_BITRATE(i));
        if (!job_bitrate)
        {
            DEBUG( IDLECODER, DBGLEV_ERROR, "Error reading bitrate for job %d\n", i);
            continue;
        }
        DEBUG( IDLECODER, DBGLEV_INFO, "Job %d bitrate: %d\n", i, job_bitrate);
        
        // Add the job to the queue.
        Enqueue(job_in_url, job_out_url, job_bitrate);
        
    }
    //    cyg_mutex_unlock(&_busy);
}

void
CIdleCoder::DebuggingCallback(CIdleCoder::job_rec_t *j, CIdleCoder::state_t s)
{
    DEBUG( IDLECODER, DBGLEV_INFO, "DebuggingCallback: \"%s\" mode: %s\n", j->in_url, CIdleCoder::state_string[s]);
}

void 
CIdleCoder::StartStream()
{
    CDataSourceManager *dsm = CDataSourceManager::GetInstance();
    ERESULT err;

    _job->active = true;
    _job->bytes_encoded = 0;
    _job->in = dsm->OpenInputStream(_job->in_url); // open the file;
#ifdef DDOMOD_DJ_METADATAFILETAG
    CMetadataFileTag::GetInstance()->CheckStreamSafeLength(_job->in);
#endif
    _job->out = dsm->OpenOutputStream(_job->out_url);
    _job->last_saved = 0;
    _job->time_started = cyg_current_time();
    
    // the validity of the opened file handles is checked by the caller
    // the processing function will exit out and junk the stream if one
    //  or both handles failed to open
    _eof = false;
    // Prebuffer input
    _input_buf = rb_new( INPUT_BUF_SIZE );
    _input_reader = rb_new_reader(_input_buf);
    _input_writer = rb_new_writer(_input_buf);

    _output_buf = rb_new( OUTPUT_BUF_SIZE );
    _output_reader = rb_new_reader(_output_buf);
    _output_writer = rb_new_writer(_output_buf);
    
    err = _pem->Start(_job->bitrate, 0, _output_writer);
	if( FAILED(err) ){
		DEBUG( IDLECODER, DBGLEV_ERROR, "PEM error: 0x%8x. Job canceled\n", err);
	}
 }


long
CIdleCoder::EncodeFrame()	// Called from within locked mutex
{
    ERESULT err;
    _chunk = CHUNK_SIZE;
#if 1
    unsigned int actual;
    rb_read_data( _input_reader, _chunk, &actual );
    if( actual == 0 ) {
        this->FillReadBuffer();
        rb_read_data( _input_reader, _chunk, &actual );
    }
	_job->bytes_encoded += actual;
    if( actual == _chunk ) {
        err = _pem->EncodeBuffer( (short*)rb_read_ptr( _input_reader ));
    } else {
        err = _pem->EncodeBuffer( (short*)rb_read_ptr( _input_reader ));
		_pem->Finish();
        _eof = true;
    }
    rb_read_done( _input_reader, actual );
    
    if( FAILED(err) ) {
        DEBUG( IDLECODER, DBGLEV_ERROR, "PEM error: 0x%8x. Job canceled\n", err);
        _state = kError;
    }
#else
    if (!_job->in) {
        DEBUG( IDLECODER, DBGLEV_ERROR, "_job->in == NULL; that ain't good\n");
    }
    unsigned int actual = _job->in->Read(_buf, _chunk);
    int samples = actual >> 2; // Bytes to stereo samples
    if (actual == _chunk) {
        err = _pem->EncodeBuffer(_buf, &samples, 0);
    } else {
        err = _pem->EncodeBuffer(_buf, &samples, 1);
        _eof = true;
    }
    if (FAILED(err)) {
        DEBUG( IDLECODER, DBGLEV_ERROR, "PEM error: 0x%8x. Job canceled\n", err);
        _state = kError;
    }
#endif
    return actual;
}
void
CIdleCoder::FillReadBuffer()
{
    if(!_job->in) {
        DEBUG( IDLECODER, DBGLEV_ERROR, "_job->in == NULL\n");
    }
    unsigned int actual;
    rb_write_data( _input_writer, INPUT_BUF_SIZE, &actual );
    
    // make sure the rbuf is empty
    // the trick here is the buffer is sized in terms of chunks, so we do full fills
    // then process all chunks until the rbuf is empty, then full fill, etc
    if( actual < INPUT_BUF_SIZE ) {
        DEBUG( IDLECODER, DBGLEV_WARNING, "Expected empty rbuf (%d)\n",actual);
        return ;
    }
    
    void* buf = rb_write_ptr( _input_writer );
    actual = _job->in->Read(buf, actual);
    if( actual < 0 ) {
        DEBUG( IDLECODER, DBGLEV_ERROR, "read error\n");
    }
    rb_write_done( _input_writer, actual );
}

void
CIdleCoder::DoOutput(bool flush)
{
    unsigned int desired = rb_read_avail(_output_reader);
    ERESULT err;
    
    if (!flush)
        desired = (desired >= OUTPUT_CHUNK_SIZE) ? OUTPUT_CHUNK_SIZE : 0;
    
    if (desired) {
        unsigned int actual;
        err = rb_read_data(_output_reader, desired, &actual);
        switch(err) {
        case RBUF_SPLIT:
            _job->out->Write(rb_read_ptr(_output_reader), actual);
            rb_read_done(_output_reader, actual);
            desired -= actual;
            rb_read_data(_output_reader, desired, &actual);
            // fall through
        case RBUF_SUCCESS:
            _job->out->Write(rb_read_ptr(_output_reader), actual);
            rb_read_done(_output_reader, actual);
            break;
        default:
            DEBUG( IDLECODER, DBGLEV_ERROR, "rbuf failure");
            //			DBASSERT(LINEINTEST, false, "rbuf failure");
        }
    }
    
}

void
CIdleCoder::AbortStream()
{
    if (_job) {
        // TODO verify - this should be addressed by not dequeueing until jobs are completed
        _eof = true;

        rb_free_writer(_output_writer);
        rb_free_reader(_output_reader);
        rb_free(_output_buf);
        
        rb_free_writer(_input_writer);
        rb_free_reader(_input_reader);
        rb_free(_input_buf);

        if( _job->out ) {
            delete _job->out;
            _job->out = NULL;
        }
        if( _job->in ) {
            delete _job->in;
            _job->in = NULL;
        }
        _job->active = false;
    }
}

void 
CIdleCoder::FinishStream()
{
    rb_free_writer(_output_writer);
    rb_free_reader(_output_reader);
    rb_free(_output_buf);
    
    rb_free_writer(_input_writer);
    rb_free_reader(_input_reader);
    rb_free(_input_buf);
    
    _eof = true;
    delete _job->out;
    delete _job->in;
    
    //    if( _pem_state ) {
    //        delete _pem_state;
    //        _pem_state = NULL;
    //    }
    
    DEBUG( IDLECODER, DBGLEV_INFO, "Waking client thread\n");
    cyg_cond_broadcast(&_wake);
}

void
CIdleCoder::ThreadBody()
{
    DEBUG( IDLECODER, DBGLEV_INFO, "CIdleCoder::ThreadBody: Idle encoder worker thread starting...\n");
    cyg_mutex_lock(&_busy);
    // this really invalidates external state control.
    //    _state = kIdle;
    _buf = new short[1152*2];
    cyg_cond_broadcast(&_wake);
    bool bExit = false;
    
    cyg_tick_count_t start_time = 0;
    
    while(!bExit) {
        // Yield so Enqueue(), Run(), and Halt() can do their damage.
        DEBUG( IDLECODER, DBGLEV_TRACE, "Unlock\n");
        cyg_mutex_unlock(&_busy);
        DEBUG( IDLECODER, DBGLEV_TRACE, "Yield\n");
        cyg_thread_yield();
        DEBUG( IDLECODER, DBGLEV_TRACE, "Lock\n");
        cyg_mutex_lock(&_busy);
        
        DEBUG( IDLECODER, DBGLEV_TRACE, "Switch: %d (%s)\n", _state, state_string[_state]);
        switch (_state) {
        case kIdle:
            DEBUG( IDLECODER, DBGLEV_INFO, "CIdleCoder::ThreadBody: Waiting for a job...\n");
            cyg_cond_wait(&_wake);
            _state = kStarting;
            break;
        case kPaused:
            DEBUG( IDLECODER, DBGLEV_INFO, "kPaused\n");
            cyg_cond_wait(&_wake);
            break;
        case kHalted:
            DEBUG( IDLECODER, DBGLEV_INFO, "kHalted\n");
            cyg_cond_wait(&_wake);
            break;
        case kStarting:
            DEBUG( IDLECODER, DBGLEV_INFO, "kStarting\n");
			/* If there's already a job, then we were just resumed. This means setup already occured, so we should
			   skip to kEncoding
			 */
			if (_job) {		
				_state = kEncoding;
				break;
			}
            /* Fetch a new job; dont remove it from the queue (thus avoiding the need to push the job back on the
               queue later
            */
			_job = Peek();
            if (_job) {
                StartStream();
                if (!_job->in || !_job->out) {
					DEBUG( IDLECODER, DBGLEV_ERROR, "StartStream failed. (in: %08x out: %08x) Job canceled\n",
						_job->in, _job->out);
                   _state = kError;
                    break;
                }
                if (_callback) {
                    (*_callback)(_job, kStarting);
                }
                _state = kEncoding;
                start_time = cyg_current_time();
            } else {
                _state = kHalted;
            }
            break;
        case kEncoding:
            if (_callback) {
                (*_callback)(_job, kEncoding);
            }
            EncodeFrame();
            DoOutput();
            if (_eof)
                _state = kFinishing;
            break;
        case kFinishing:
            DEBUG( IDLECODER, DBGLEV_INFO, "Encoded %d bytes in %d time\n", _job->in->Length(), cyg_current_time() - start_time);
            DEBUG( IDLECODER, DBGLEV_INFO, "kFinishing\n");
            DoOutput(true);
            if (_callback) {
                (*_callback)(_job, kFinishing);
            }
            FinishStream();
            // since we are finishing this job, remove it from the queue and delete it
            //  do this before saving to the registry
            Dequeue();
//            SaveToRegistry(); // Move this to the callback to be thread-safe.
            DEBUG( IDLECODER, DBGLEV_INFO, "kFinished\n");
            _state = kFinished;
            if (_callback) {
                (*_callback)(_job, kFinished);
            }
            delete _job;
            _job = 0;
            _state = kStarting;
            break;
        case kError:
            DEBUG( IDLECODER, DBGLEV_ERROR, "kError\n");
            if (_callback) {
                (*_callback)(_job, kError);
            }
            AbortStream();
            Dequeue();
            delete _job;
            _job = 0;
            _state = kStarting;
            break;
        case kExit:
            AbortStream();
            bExit = true;
            break;
        default:
            ;
        }
    }
    cyg_mutex_unlock(&_busy);
    
    delete[] _buf;
    _buf = 0;
    cyg_cond_destroy(&_wake);
    cyg_mutex_destroy(&_busy);
}
