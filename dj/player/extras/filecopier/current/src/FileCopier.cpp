//
// File Name: FileCopier.cpp
//
// Copyright (c) 1998 - 2002 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//
#include <cyg/kernel/kapi.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <extras/filecopier/FileCopier.h>
#include <util/eresult/eresult.h>
#include <util/debug/debug.h>
#include <util/utils/utils.h>
#include <stdlib.h>
#include <string.h>

DEBUG_MODULE_S( FILECOPIER, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( FILECOPIER );  // debugging prefix : (33) fc


const char * const
CFileCopier::state_string[] = {
    "Not Ready",
    "Idle",
    "Paused",
    "Starting",
    "Encoding",
    "Finishing",
    "Error",
    "Exit"
};

// Job rec

CFileCopier::job_rec_t::job_rec_t(const char *in_url, const char *out_url, void* user_data)
    : user_data(user_data),
    in(0), out(0),
    active(false), bytes_to_copy(0), bytes_copied(0)
{
    this->in_url = strdup_new(in_url);
    this->out_url = strdup_new(out_url);
}

CFileCopier::job_rec_t::~job_rec_t()
{
    delete [] in_url;
    delete [] out_url;
}


// Statics

CFileCopier *
CFileCopier::GetInstance()
{
    if (!CFileCopier::_instance) {
//        CFileCopier::_instance = new CFileCopier(10, 8 * 1024);
        CFileCopier::_instance = new CFileCopier(11, 8 * 1024);
        CFileCopier::_instance->Start();
    }
    return CFileCopier::_instance;			
}

void
CFileCopier::Destroy()
{
    if( CFileCopier::_instance ) delete CFileCopier::_instance;
    CFileCopier::_instance = NULL;
}

CFileCopier *
CFileCopier::_instance = 0;

// Members

CFileCopier::CFileCopier(cyg_addrword_t priority, cyg_ucount32 stacksize)
    : IThreadedObject(priority, "FileCopierThread", stacksize)
{
    cyg_mutex_init(&_busy);
    cyg_cond_init(&_wake, &_busy);
    cyg_mutex_init(&_jobs_lock);
    _eof = true;
    _state = kNotReady;
    _job = 0;
    _callback = 0;
    _chunk = _new_chunk = 1 << 15;
    _buf = 0;
}

CFileCopier::~CFileCopier()
{
    cyg_mutex_lock(&_busy);
    _state = kExit;
    cyg_mutex_unlock(&_busy);
    cyg_cond_signal(&_wake);

    while ( !m_bStopped )
        cyg_thread_delay( 1 );

    cyg_mutex_destroy(&_busy);
    cyg_cond_destroy(&_wake);

    RemoveAllJobs();
    cyg_mutex_destroy(&_jobs_lock);

    delete [] _buf;
}

void
CFileCopier::SetBufferSize(unsigned long buffer_size)
{
    _new_chunk = buffer_size;
}

void
CFileCopier::Enqueue(const char *in_url, const char *out_url, void* user_data)
{
    Enqueue(new job_rec_t(in_url, out_url, user_data));
}

void
CFileCopier::Enqueue(job_rec_t *j)
{
    cyg_mutex_lock(&_jobs_lock);
    _jobs.PushBack(j);
    DEBUG( FILECOPIER, DBGLEV_INFO, "Job enqueued: %s -> %s\n", j->in_url, j->out_url);
    if (_state != kPaused)
        cyg_cond_signal(&_wake);
    cyg_mutex_unlock(&_jobs_lock);
}

bool
CFileCopier::RemoveJob(const char *in_url)
{
    cyg_mutex_lock(&_jobs_lock);
    
    for (SimpleListIterator<job_rec_t *> it = _jobs.GetHead(); it != _jobs.GetEnd(); ++it)
        if (!strcmp(in_url, (*it)->in_url))
        {
            delete *it;
            _jobs.Remove(it);
            cyg_mutex_unlock(&_jobs_lock);
            return true;
        }
        
        cyg_mutex_unlock(&_jobs_lock);
        return false;
}

void
CFileCopier::RemoveAllJobs()
{
    cyg_mutex_lock(&_jobs_lock);
    
    while (!_jobs.IsEmpty())
        delete _jobs.PopFront();
    
    cyg_mutex_unlock(&_jobs_lock);
}

CFileCopier::job_rec_t *
CFileCopier::Dequeue()
{
    DBEN( FILECOPIER );
    job_rec_t *j;
    cyg_mutex_lock(&_jobs_lock);
    if (_jobs.IsEmpty()) {
        j = 0;
    } else {
        j = _jobs.PopFront();
    }
    cyg_mutex_unlock(&_jobs_lock);
    
    DBEX( FILECOPIER );
    return j;
}


void
CFileCopier::Run()
{
    DEBUG( FILECOPIER, DBGLEV_INFO, "Starting FileCopier\n");
    cyg_mutex_lock(&_busy);
    _state = kStarting;
    cyg_mutex_unlock(&_busy);
    cyg_cond_signal(&_wake);
}

void
CFileCopier::Pause()
{
    DEBUG( FILECOPIER, DBGLEV_INFO, "Pausing FileCopier\n");
    cyg_mutex_lock(&_busy);
    _state = kPaused;
    cyg_mutex_unlock(&_busy);
}

void
CFileCopier::Halt()
{
    cyg_mutex_lock(&_busy);
    if (_job) {
        AbortStream();
        _state = kPaused;
        Enqueue(_job);
        _job = 0;
    }
    else
        _state = kPaused;

    cyg_mutex_unlock(&_busy);
    DEBUG( FILECOPIER, DBGLEV_INFO, "FileCopier Suspended\n");
}

void
CFileCopier::DebuggingCallback(CFileCopier::job_rec_t *j, CFileCopier::state_t s)
{
    DEBUG( FILECOPIER, DBGLEV_INFO, "DebuggingCallback: \"%s\" mode: %s\n", j->in_url, CFileCopier::state_string[s]);
}

void 
CFileCopier::StartStream()
{
    CDataSourceManager *dsm = CDataSourceManager::GetInstance();
    
    if (_chunk != _new_chunk)
    {
        delete [] _buf;
        _chunk = _new_chunk;
        _buf = new unsigned char[_chunk];
    }
    
    _eof = true;
    _job->active = false;
    _job->bytes_copied = 0;
    _job->in = dsm->OpenInputStream(_job->in_url); // open the file;
    if (_job->in)
    {
        _job->bytes_to_copy = _job->in->Length();

        // dc- total hack here to avoid filesystem issues; dont accept files that are 2gb or larger
        if( _job->bytes_to_copy >= ((unsigned)2*1024*1024*1024) ) {
            DEBUGP( FILECOPIER, DBGLEV_TRACE, "fc: dropping large job (%u)\n", _job->bytes_to_copy );
            delete _job->in;
            _job->bytes_to_copy = 0;
            return ;
        }
        
        DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:overwriting out, old == %d\n",(int)_job->out); 
        _job->out = dsm->OpenOutputStream(_job->out_url);
        if (_job->out == NULL)
        {
            DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:failed to open output %s!\n",_job->out_url); 
        }
        else
        {
            _eof = false;
            _job->active = true;
        }
    }
    else
    {
         DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:failed to open input %s!\n",_job->in_url); 
        _job->bytes_to_copy = 0;
    }
}


long
CFileCopier::CopyChunk()    // Called from within locked mutex
{
    if (!_job->in || !_job->out) {
        // sanity
        DEBUG( FILECOPIER, DBGLEV_ERROR, "_job->in == %d, _job->out == %d; that ain't good\n", _job->in, _job->out);
        _state = kError;
        return 0;
    }
    int actual_in = _job->in->Read(_buf, _chunk);
    
    if (actual_in != _chunk) {
        _eof = true;
        if (actual_in < 0)
        {
            DEBUG( FILECOPIER, DBGLEV_ERROR, "Error in read: %d. Job canceled\n", actual_in);
            _state = kError;
        }
    }

    if (actual_in > 0)
    {
        _job->bytes_copied += actual_in;
    
        int actual_out = _job->out->Write(_buf, actual_in);
        if (actual_out != actual_in)
        {
            DEBUG( FILECOPIER, DBGLEV_ERROR, "Read %d, only wrote %d. Job canceled\n", actual_in, actual_out);
            _state = kError;
        }
    }
    return actual_in;
}

void
CFileCopier::AbortStream()
{
    DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:aborting streams state %d\n",(int)_state); 
    if (_job)
    {
        DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:state is copying, delete\n"); 
        DEBUG( FILECOPIER, DBGLEV_INFO, "Dropping current job on the floor. TODO: Reschedule for next run\n");
        _eof = true;
        
        delete _job->out;
        delete _job->in;
        _job->out = NULL;
        _job->in = NULL;
        _job->active = false;
        DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:job clean and marked inactive\n"); 
    }
    else
    {
        DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:clean skip abort, _job==0\n"); 
    }
}

void 
CFileCopier::FinishStream()
{
    _eof = true;
    DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:killing streams in finish\n"); 
    delete _job->out;
    delete _job->in;
    _job->out = NULL;
    _job->in = NULL;
    DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:streams clean\n"); 
    DEBUG( FILECOPIER, DBGLEV_INFO, "Waking client thread\n");
    cyg_cond_broadcast(&_wake);
}

void
CFileCopier::ThreadBody()
{
    DEBUG( FILECOPIER, DBGLEV_INFO, "CFileCopier::ThreadBody: Idle encoder worker thread starting...\n");
    cyg_mutex_lock(&_busy);
    _state = kPaused;
    _buf = new unsigned char[_chunk];
    cyg_cond_broadcast(&_wake);
    bool bExit = false;

    while(!bExit) {
        // Yield so Enqueue(), Run(), and Halt() can do their damage.
        DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:unlck bsy\n"); 
        cyg_mutex_unlock(&_busy);
        DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:yield\n"); 
        cyg_thread_yield();
        DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:lck bsy\n"); 
        cyg_mutex_lock(&_busy);
        
        DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:switch state %d (%s)\n", _state, state_string[_state]);
        switch (_state) {
        case kIdle:
            DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:idle: wait on wake %d\n",(int)&_wake); 
            cyg_cond_wait(&_wake);
            DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:set kStart\n"); 
            _state = kStarting;
            break;
        case kPaused:
            DEBUG( FILECOPIER, DBGLEV_INFO, "kPaused\n");
            cyg_cond_wait(&_wake);
            DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:woke, loop\n"); 
            break;
        case kStarting:
            DEBUG( FILECOPIER, DBGLEV_INFO, "kStarting\n");
            if (!_job)
            {
                DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:no job -> dequeue\n"); 
                _job = Dequeue();
                if (_job) {
                    DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:job - start stream\n"); 
                    StartStream();
                    if (!_job->in)
                        DEBUGP( FILECOPIER , DBGLEV_WARNING, "fc:in stream fail\n"); 
                    if (!_job->out)
                        DEBUGP( FILECOPIER , DBGLEV_WARNING, "fc:out stream fail\n"); 

                    if (!_job->in || !_job->out) {
                        DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:state err\n"); 
                        /* Record the type of error or something */
                        _state = kError;
                        break;
                    }
                    if (_callback) {
                        DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:callback\n"); 
                        (*_callback)(_job, kStarting);
                    }
                    DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:state copy\n"); 
                    _state = kCopying;
                } else {
                    DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:no job, state idle\n"); 
                    _state = kIdle;
                }
            }
            else
            {
                DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:job w/o dequeue, state copy\n"); 
                _state = kCopying;
            }
            break;
        case kCopying:
            DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:copying\n"); 
            if (_callback) {
                (*_callback)(_job, kCopying);
            }
            CopyChunk();
            if (_eof)
                _state = kFinishing;
            break;
        case kFinishing:
            DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:finishing\n"); 
            FinishStream();
            if (_callback) {
                (*_callback)(_job, kFinishing);
            }
            delete _job;
            _job = 0;
            _state = kStarting;
            break;
        case kError:
            DEBUG( FILECOPIER, DBGLEV_ERROR, "kError 1\n");
            DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:abort stream\n"); 
            AbortStream();
            if (_callback) {
                DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:notifying callback of error condition %d %d %d\n",(int)_callback,(int)_job,(int)kError); 
                (*_callback)(_job, kError);
            }
            else
                DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:no callbackn"); 
            DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:delete _job %d\n",(int)_job); 
            delete _job;
            _job = 0;
            DEBUGP( FILECOPIER , DBGLEV_TRACE, "fc:reset state to start, iterate\n"); 
            _state = kStarting;
            break;
        case kExit:
            DEBUG( FILECOPIER, DBGLEV_INFO, "kExit\n");
            bExit = true;
            break;
        default:
            ;
        }
    }
    cyg_mutex_unlock(&_busy);
    
    cyg_cond_destroy(&_wake);
    cyg_mutex_destroy(&_busy);
    cyg_mutex_destroy(&_jobs_lock);
    
    delete[] _buf;
    _buf = 0;

    m_bStopped = true;
}
