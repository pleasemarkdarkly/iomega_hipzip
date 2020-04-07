#include <stdio.h>
#include <cyg/infra/diag.h>
#include <devs/audio/dai.h>
#include <util/debug/debug.h>
#include <util/eresult/eresult.h>
#include <util/rbuf/rbuf.h>
#include <util/thread/ThreadedObject.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <codec/mp3/pem/codec.h>
#include <codec/mp3/pem/pem.h>


DEBUG_MODULE(LINEINTEST);

#define VERIFY(x) \
	{ \
		ERESULT __tmp_err = x; \
		if (FAILED(__tmp_err)) { \
			diag_printf( "%s:%d\n   %s failed: 0x%08x\n", __FILE__, __LINE__, #x, __tmp_err); \
		} else { \
			diag_printf( "%s:%d\n   %s successful: 0x%08x\n", __FILE__, __LINE__, #x, __tmp_err); \
		} \
	}

short silence[1152*2];

static unsigned int bitrates[] = {64, 96, 112, 128, 160, 192, 224, 256};

class LineInTest : public IThreadedObject {
public:
	short dummy_buf[1152*2];
	rbuf_t *inbuf;
	rbuf_reader_t *inbuf_r;
	rbuf_writer_t *inbuf_w;
	rbuf_t *outbuf;
	rbuf_reader_t *outbuf_r;
	rbuf_writer_t *outbuf_w;

	IOutputStream *out;
	CPEMWrapper *pem;


	LineInTest() : IThreadedObject(10, "LineInTest", 32768)
	{
	}

	void DoOutput(bool flush = false)
	{
		unsigned int desired = rb_read_avail(outbuf_r);
		ERESULT err;

		if (!flush)
			desired = (desired >= 16384) ? 16384 : 0;

		if (desired) {
			unsigned int actual;
			err = rb_read_data(outbuf_r, desired, &actual);
			switch(err) {
			case RBUF_SPLIT:
				out->Write(rb_read_ptr(outbuf_r), actual);
				rb_read_done(outbuf_r, actual);
				desired -= actual;
				rb_read_data(outbuf_r, desired, &actual);
				// fall through
			case RBUF_SUCCESS:
				out->Write(rb_read_ptr(outbuf_r), actual);
				rb_read_done(outbuf_r, actual);
				break;
			default:
				DBASSERT(LINEINTEST, false, "rbuf failure");
			}
		}
		
	}


	void ThreadBody()
	{
		out = new CFatFileOutputStream();
		pem = CPEMWrapper::GetInstance();
		
		#define OUTBUF_SIZE (32768)
		
		for (int pem_rate_index = 0;
			true; pem_rate_index = (pem_rate_index + 1) % (sizeof(bitrates) / sizeof(bitrates[0]))) {
			int pem_rate = bitrates[pem_rate_index];
			char filename[256];

			outbuf = rb_new(OUTBUF_SIZE);
			outbuf_r = rb_new_reader(outbuf);
			outbuf_w = rb_new_writer(outbuf);

			snprintf(filename, 256, "a:\\linein-%d.mp3", pem_rate);
			diag_printf("Dumping to %s\n", filename);
			VERIFY(out->Open(filename));
			
			diag_printf("Initializing PEM for %d kbit/sec operation...\n", pem_rate);
			VERIFY(pem->Start(pem_rate, inbuf_r, outbuf_w));
			
			DAIOverflowCounter = 0;
			DAIInit();
			DAIResetRecord();
			DAISetSampleFrequency( 44100 );
			DAIEnable();
			diag_printf("\n");
			cyg_tick_count_t start = cyg_current_time();
			cyg_tick_count_t stop = start + 12000;
			cyg_tick_count_t last = start;
			cyg_tick_count_t now;
			unsigned int last_overflow = 0;
			short *dai_buffer;
			unsigned int dai_avail, dai_remaining;
			dai_remaining = DAIRead(&dai_buffer, &dai_avail);
			int minbuf,maxbuf, buf;
			minbuf = 1024;
			maxbuf = 0;
			while (cyg_current_time() < stop) {
/*				diag_printf("%3d ", pem_rate);
				diag_printf("%3d", now / 100);
				diag_printf(".%02d", now % 100);
				diag_printf("%6d ", (volatile long)DAIOverflowCounter);
				diag_printf("%3d ", DAIFreeRecordBufferCount()); */
				if (cyg_current_time() >= last+100) {
					last = cyg_current_time();
					if (last_overflow < (volatile long)DAIOverflowCounter) {
						last_overflow = (volatile long)DAIOverflowCounter;
						diag_printf("!");
					} else {
						diag_printf(".");
					}
				}
				buf = DAIFreeRecordBufferCount();
				if (maxbuf < buf) maxbuf = buf;
				if (minbuf > buf) minbuf = buf;
				ERESULT err;
				int samples;
				
				DoOutput();	// Dump any pending output
				dai_remaining = DAIRead(&dai_buffer, &dai_avail);
				if (dai_remaining == 0) {
					samples = 1152;
					err = pem->EncodeBuffer(dai_buffer);
					DBASSERT(LINEINTEST, SUCCEEDED(err), "PEM Failed: 0x%08x\n", err);
					DAIReleaseBuffer();
//					diag_printf("PEM\n");
				} else {
//					diag_printf("(%4d)\n", dai_remaining);
					cyg_thread_delay(5);
				}
			}
			pem->EncodeBuffer(silence);
			pem->Finish();
			DoOutput(true);
			out->Close();
			diag_printf("\n%3d kbps, ", pem_rate);
			diag_printf("%d dropped ", DAIOverflowCounter);
			diag_printf("%d/", minbuf);
			diag_printf("%d (min/max) free buffers\n", maxbuf);
			rb_free_writer(outbuf_w);
			rb_free_reader(outbuf_r);
			rb_free(outbuf);
		}
	}
};

RUN_AS_STARTUP_THREAD(LineInTest)
