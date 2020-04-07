#ifndef __IDLECODER_H__
#define __IDLECODER_H__

#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <util/thread/ThreadedObject.h>
#include <util/datastructures/SimpleList.h>
#include <cyg/kernel/kapi.h>
#include <codec/mp3/pem/pem.h>
#include <util/rbuf/rbuf.h>

class CIdleCoder : public IThreadedObject {
public:
	static CIdleCoder *GetInstance();
    static void Destroy();
    
	~CIdleCoder();

	class job_rec_t;

	enum state_t {		/* If you change this, please change state_string in IdleCoder.cpp as well. */
		kNotReady = 0,
        kHalted,    // halted until Run called to resume
		kIdle,      // waiting for job
		kPaused,    // active job on hold
		kStarting,	// Used as callback_t type
		kEncoding,	//  ""
		kFinishing,	//  "" - State after encoding is finished and files haven't yet been closed.
                    //       Can tack on id3v1 info.
        kFinished,  //  "" - State after encoding is finished and files have been closed.
                    //       Can delete files.
		kError,		// Called before we drop a job because something went wrong
        kExit       // Called when we are exiting the main thread
	};

    state_t GetState(void) 
        {  return _state; }

	static const char * const state_string[kExit+1];

	typedef void(*callback_t)(job_rec_t *, state_t t);
	static void DebuggingCallback(job_rec_t *, state_t t);

	void Enqueue(const char *in_url, const char *out_url, int bitrate);
	void Enqueue(job_rec_t *j);

    //! Removes a job with the given in_url from the queue of files to encode.
    //! Returns true if a matching job was found and removed, false otherwise.
    //! This should only be called when the idle coder thread isn't running.
    bool RemoveJob(const char *in_url);

    //! Removes all jobs from the queue of files to copy.
    //! This should only be called when the idle coder thread isn't running.
    void RemoveAllJobs();
    
	//! Starts processing the job queue, or resumes current job if queue has been paused.
	void Run();

	//! Pauses current job. Resume with Run()
	void Pause();

	//! Halts processing, aborts current job, returning it to the queue. Resume with Run()
	void Halt();


    //! Sets the callback that will be invoked when there's a change in encoding status.
    //! Passing in a value of 0 diables callbacks.
    void SetCallback(callback_t fn)
        { _callback = fn; }

    //! Returns true if the job queue is empty.
    //! This should only be called synchronously from the idle coder thread
    //! or when the idle coder thread isn't running.
    bool NoJobs()
        { return _jobs.IsEmpty(); }

    //! Saves out the job queue to the registry.
    void SaveToRegistry();

    //! Loads in a job queue from the registry.
    void LoadFromRegistry();

    class CIdleCoder::job_rec_t {
    public:
        job_rec_t(const char *in_url, const char *out_url, int bitrate);
        ~job_rec_t();

        char *in_url;
        char *out_url;
        IInputStream *in;
        IOutputStream *out;
        int bitrate;
        cyg_tick_count_t time_started, last_saved;
        unsigned long bytes_encoded;
        ERESULT last_result;
        bool active;
    };

private:
	static CIdleCoder *_instance;
	CIdleCoder(cyg_addrword_t priority, cyg_ucount32 stacksize);

    job_rec_t *Peek();
	job_rec_t *Dequeue();
    
	void StartStream();
    void FillReadBuffer();
	long EncodeFrame();
	void DoOutput(bool flush = false);
	void FinishStream();
	void AbortStream();

	void ThreadBody();

	// Job information
	state_t _state;
	job_rec_t * _job;			// Current job
	SimpleList<job_rec_t *> _jobs;

    callback_t _callback;

	// Helper classes
	CPEMWrapper *_pem;

	// Synchronization
	bool _halt, _eof, _started;
	cyg_mutex_t _jobs_lock;
	cyg_mutex_t _busy;
	cyg_cond_t _wake;
	

	unsigned long _chunk; // frame size expected by codec
	short *_buf;

    rbuf_t *_input_buf;
    rbuf_reader_t *_input_reader;
    rbuf_writer_t *_input_writer;
	rbuf_t *_output_buf;
	rbuf_reader_t *_output_reader;
	rbuf_writer_t *_output_writer;

    void* _pem_state;
};

#endif // __IDLECODER_H__
