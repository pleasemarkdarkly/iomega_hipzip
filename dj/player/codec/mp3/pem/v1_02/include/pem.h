#ifndef __PEM_H__
#define __PEM_H__

#include <util/eresult/eresult.h>
#include <util/rbuf/rbuf.h>
#include <codec/mp3/pem/codec.h>

class CPEMWrapper {
public:
	static CPEMWrapper *GetInstance();
	~CPEMWrapper();

	ERESULT Start(int bitrate, rbuf_reader_t *in, rbuf_writer_t *out);

	ERESULT EncodeBuffer(short *pcm);

	ERESULT Finish();

    // routine to handle saving internal data to external buffers, since
    //  pem can't be reentrant
    ERESULT SaveState( void* buf, int len );
    ERESULT RestoreState( const void* buf );
    int GetStateSize() 
        {  return pem_get_state_size(); }
    
protected:
	CPEMWrapper();

	rbuf_reader_t *_in;
	rbuf_writer_t *_out;
	unsigned char *_wav_buf, *_mp3_buf;
	
	static CPEMWrapper *_instance;

	bool initialized;
};

#endif // __PEM_H__
