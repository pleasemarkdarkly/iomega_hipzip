#include "codec.h"
#include "memory.h"

long
fpmp3_start(int bitrate)
{
	return pem_init(bitrate);
}

long
fpmp3_encode(short *pcm, unsigned char *mp3out, unsigned int *bytesout)
{
	int foo = 1152;
	long res;
	sram_p->outbuf = mp3out;
	sram_p->outcount = 0;
	res = pem_work(pcm,&foo,0);
	*bytesout = sram_p->outcount;
	return res;
}

long
fpmp3_finish(unsigned char *mp3out, unsigned int *bytesout)
{
	sram_p->outbuf = mp3out;
	sram_p->outcount = 0;
	pem_fini();
	*bytesout = sram_p->outcount;
	return 0;
}
