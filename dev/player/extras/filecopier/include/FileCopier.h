//
// File Name: FileCopier.h
//
// Copyright (c) 1998 - 2002 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//
#ifndef FILECOPIER_H_
#define FILECOPIER_H_

#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <util/thread/ThreadedObject.h>
#include <util/datastructures/SimpleList.h>
#include <cyg/kernel/kapi.h>

class CFileCopier : private IThreadedObject {
public:
    static CFileCopier *GetInstance();
    static void Destroy();
    
    ~CFileCopier();
    
    class job_rec_t;
    
    enum state_t {		/* If you change this, please change state_string in FileCopier.cpp as well. */
        kNotReady = 0,
        kIdle,
        kPaused,
        kStarting,	// Used as callback_t type
        kCopying,	//  ""
        kFinishing,	//  ""
        kError,		// Called before we drop a job because something went wrong
        kExit
    };
    
    static const char * const state_string[kExit+1];
    
    typedef void(*callback_t)(job_rec_t *, state_t t);
    static void DebuggingCallback(job_rec_t *, state_t t);
    
    //! Set the size of the read/write buffer in bytes.
    void SetBufferSize(unsigned long buffer_size);
    
    void Enqueue(const char *in_url, const char *out_url, void* user_data);
    void Enqueue(job_rec_t *j);

    //! Removes the first job from the queue.
    //! This should only be called when the idle coder thread isn't running.
    job_rec_t *Dequeue();
    
    //! Removes a job with the given in_url from the queue of files to copy.
    //! Returns true if a matching job was found and removed, false otherwise.
    //! This should only be called when the idle coder thread isn't running.
    bool RemoveJob(const char *in_url);
    
    //! Removes all jobs from the queue of files to copy.
    //! This should only be called when the idle coder thread isn't running.
    void RemoveAllJobs();
    
    void Run();
    void Pause();
    void Halt();
    
    //! Sets the callback that will be invoked when there's a change in copying status.
    //! Passing in a value of 0 diables callbacks.
    void SetCallback(callback_t fn)
        { _callback = fn; }

    //! Returns a pointer to the callback function.
    callback_t GetCallback() const
        { return _callback; }
    
    //! Returns true if the job queue is empty.
    //! This should only be called synchronously from the file copier thread
    //! or when the file copier thread isn't running.
    bool NoJobs()
        { return _jobs.IsEmpty(); }
    
    class job_rec_t {
        public:
            job_rec_t(const char *in_url, const char *out_url, void* user_data);
            ~job_rec_t();
        
            char *in_url;
            char *out_url;
            void *user_data;
            IInputStream *in;
            IOutputStream *out;
            bool active;
            unsigned long bytes_to_copy, bytes_copied;
        };
    
private:
    static CFileCopier *_instance;
    CFileCopier(cyg_addrword_t priority, cyg_ucount32 stacksize);

    void StartStream();
    long CopyChunk();
    void FinishStream();
    void AbortStream();

    void ThreadBody();

    // Job information
    state_t _state;
    job_rec_t * _job;			// Current job
    SimpleList<job_rec_t *> _jobs;

    callback_t _callback;

    // Synchronization
    bool _halt, _eof, _started;
    cyg_mutex_t _jobs_lock;
    cyg_mutex_t _busy;
    cyg_cond_t _wake;

    unsigned long _chunk;       // buffer size
    unsigned long _new_chunk;   // buffer size to be set
    unsigned char *_buf;
};

#endif // FILECOPIER_H_
