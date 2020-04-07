#ifndef __PEM_CODEC_H__
#define __PEM_CODEC_H__
#if defined(DESKTOP) || defined(STANDALONE)
#include "../../../../util/eresult/include/eresult.h"
#else
#include <util/eresult/eresult.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define PEM_TYPE_ID 0x99
#define MAKE_PEMRESULT(name, severity, id) \
	/*const ERESULT*/ PEM_ ## name = \
		MAKE_ERESULT(SEVERITY_ ## severity, PEM_TYPE_ID, id)

enum {
MAKE_PEMRESULT(NO_ERROR, SUCCESS, 0x0000),
MAKE_PEMRESULT(BAD_BITRATE, FAILED, 0x0001),
MAKE_PEMRESULT(NOT_ENOUGH_INPUT, FAILED, 0x0002),
MAKE_PEMRESULT(EOF, SUCCESS, 0x0003),
MAKE_PEMRESULT(NO_WORK, SUCCESS, 0x0004),
MAKE_PEMRESULT(OUTPUT_RBUF_OVERFLOW, FAILED, 0x0005),
MAKE_PEMRESULT(NOT_INITIALIZED, FAILED, 0x0006),
MAKE_PEMRESULT(MAIN_DATA_OUT_OF_RANGE, FAILED, 0x0100),
MAKE_PEMRESULT(PAD_DBITTER_NEGATIVE, FAILED, 0x0102),
MAKE_PEMRESULT(ILLEGAL_SCALEFAC_COMPRESS, FAILED, 0x0103),
MAKE_PEMRESULT(SBITTER_LEFTOVER_BITS, FAILED, 0x0104),
MAKE_PEMRESULT(INSUFFICIENT_SPACE, FAILED, 0x0105),
MAKE_PEMRESULT(DEMO_LIMIT_REACHED, FAILED, 0x0200)
};

#define ASSERT_SUCCESS(x) { ERESULT __tmperr; __tmperr = x; if (FAILED(__tmperr)) longjmp(error_jbuf, __tmperr); }


#define PEM_FAIL(x) { 

//	init
//	returns zero on success

ERESULT pem_init(int bitrate);


//	cleanup, and flush output

void pem_fini(void);


//	main work function
//	call with pcm in, number of pcm samples (stereo),
//	and end of stream flag
//	returns number of pcm samples consumed, or < 0 on error

ERESULT pem_work(short *pcm, int *pcm_count, int e_o_s);

//  state management routines 
ERESULT pem_save_state( void* buf, int len );
ERESULT pem_restore_state( const void* buf );
int pem_get_state_size(void);
 
//	"callback" from out.c, user should define it, should never fail
//	(or deal with it yourself)
//	should output n bytes from x

//extern ERESULT pem_do_out(unsigned char *x, int n);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__PEM_CODEC_H__
