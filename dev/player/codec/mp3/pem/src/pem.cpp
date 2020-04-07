#include <codec/mp3/pem/pem.h>
#include <codec/mp3/pem/fpmp3.h>
#include <util/debug/debug.h>
#include <string.h>

DEBUG_MODULE(PEM);
DEBUG_USE_MODULE(PEM);
DEBUG_USE_MODULE(RBUF);

CPEMWrapper *
CPEMWrapper::_instance = 0;

CPEMWrapper *
CPEMWrapper::GetInstance()
{
	if (!_instance)
		_instance = new CPEMWrapper();

	return _instance;
}

extern void *fpmp3_base;

CPEMWrapper::CPEMWrapper()
{
	_wav_buf = new unsigned char[1152*4];
	_mp3_buf = new unsigned char[2048];
	initialized = false;
}

CPEMWrapper::~CPEMWrapper()
{
    delete [] _wav_buf;
    delete [] _mp3_buf;
}

ERESULT
CPEMWrapper::Start(int bitrate, rbuf_reader_t *in, rbuf_writer_t *out)
{
	ERESULT err;
	_in = 0;
	_out = 0;

	DEBUG(PEM, DBGLEV_INFO, "Calling fpmp3_start\n");
	err = fpmp3_start(bitrate);
	DEBUG(PEM, DBGLEV_INFO, "fpmp3_start returned\n");
	if (FAILED(err)) {
		initialized = false;
		return err;
	}
	_in = in;
	_out = out;
	initialized = true;
	return PEM_NO_ERROR;
}

#ifdef __RBUF_INPUT
ERESULT
CPEMWrapper::EncodeLastFrame()
{
	if (!initialized) return PEM_NOT_INITIALIZED;

	unsigned int avail;
	avail = rb_read_avail(_in);
	avail = (avail > 1152*4) ? 1152*4 : avail;
	if (avail) {
		ERESULT rb_res = rb_copy_read(_in, _slop, avail);
		DBASSERT(PEM, SUCCEEDED(rb_res), "rb_copy_read failed with amount <= rb_read_avail");
		int samples = avail / 4;
		pem_work((short *)_slop, &samples, 1);
	}

	pem_fini();
	initialized = false;
	return PEM_EOF;
}

ERESULT
CPEMWrapper::EncodeFrame()
{
	if (!initialized) return PEM_NOT_INITIALIZED;

	ERESULT rb_res, pem_res;
	unsigned int actual;
	int samples;
	
	rb_res = rb_read_data(_in, 1152*4, &actual);
	switch(rb_res) {
	case RBUF_NOSPACE:
		if (rb_read_eof(_in) == RBUF_EOF)
			return EncodeLastFrame();
		else
			return PEM_NO_WORK;
		break;
	case RBUF_EOF:
		return EncodeLastFrame();
		break;
	case RBUF_SUCCESS:
		samples = 1152;
		pem_res = pem_work((short *)rb_read_ptr(_in), &samples, 0);
		rb_res = rb_read_done(_in, 1152*4);
		DBASSERT(RBUF, SUCCEEDED(rb_res),
			"rb_read_done failed with amount equal to rb_read_data: 0x%08x", rb_res);
		return pem_res;
		break;
	case RBUF_SPLIT:
		rb_res = rb_copy_read(_in, _slop, 1152*4);
		DBASSERT(RBUF, SUCCEEDED(rb_res), "rb_copy_read failed with amount <= what rb_read_avail reported");
		samples = 1152;
		pem_res = pem_work((short *)_slop, &samples, 0);
		return pem_res;
	default:
		DBASSERT(RBUF, 0, "switch fell through");
	}
}
#endif

ERESULT 
CPEMWrapper::EncodeBuffer(short *pcm)
{
	if (!initialized) return PEM_NOT_INITIALIZED;
	unsigned int mp3_bytes_out;
	
	ERESULT err = fpmp3_encode(pcm, _mp3_buf, &mp3_bytes_out);
	if( FAILED(err) )
		return err;
        
    if( mp3_bytes_out ) {
        err = rb_copy_write(_out, _mp3_buf, mp3_bytes_out);
        if( FAILED(err) )
            return PEM_OUTPUT_RBUF_OVERFLOW;
    }
	
	return PEM_NO_ERROR;
}

ERESULT
CPEMWrapper::Finish()
{
	if (!initialized) return PEM_NOT_INITIALIZED;
	unsigned int mp3_bytes_out;
	
	ERESULT err = fpmp3_finish(_mp3_buf, &mp3_bytes_out);
	if( FAILED(err) )
		return err;
	err = rb_copy_write(_out, _mp3_buf, mp3_bytes_out);
	if( FAILED(err) )
		return PEM_OUTPUT_RBUF_OVERFLOW;
	
	return PEM_NO_ERROR;
}

ERESULT
CPEMWrapper::SaveState( void* buf, int len ) 
{
    return pem_save_state( buf, len );
}

ERESULT
CPEMWrapper::RestoreState( const void* buf )
{
    return pem_restore_state( buf );
}

/*
ERESULT
pem_do_out(unsigned char *x, int n)
{
	CPEMWrapper *pem = CPEMWrapper::_instance;
	
	if (n > 0) {
		ERESULT err = rb_copy_write(pem->_out, x, n);
		if (FAILED(err)) {
			diag_printf("ringbuf error: 0x%08x (only %d free, tried %d)\n",
				err, rb_write_avail(pem->_out), n);
			return PEM_OUTPUT_RBUF_OVERFLOW;
		}
	}

	return PEM_NO_ERROR;
}
*/
