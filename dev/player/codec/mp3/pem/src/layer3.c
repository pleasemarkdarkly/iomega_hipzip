//#include "TinyProfile.h"
#include <math.h>
#define pow2(x) pow(2.0f, x)
#include <stdio.h>

//#define SHOW_SCALEFACS
//#define SHOW_SCALEFACS_COLOR 1
#define DEBUG(s...) diag_printf(##s)

//#define ZERO_TEST

#ifdef PROGRESS_REPORT
#include <time.h>
#include <sys/times.h>
#include <stdio.h>
#endif

//#include <string.h>

#include "types.h"
#include "layer3.h"
#include "polyphase.h"
#include "hybrid.h"
#include "code.h"
#include "in.h"
#include "out.h"
#include "tables.h"
#include "huffman.h"
#include "reservoir.h"
#include "quant.h"


#ifdef SHOW_SCALEFACS
static void print_mag(graninfo *gi, int ch)
{
	int i;

#ifdef SHOW_SCALEFACS_COLOR
{
	static int count = 0;
	static char *kl[16] = {
		"0;30;40", "0;34;40", "1;34;40", "1;34;44",
///		"1;31;41", "0;34;40", "1;34;40", "1;34;44",
////		"0;37;40", "0;34;40", "1;34;40", "1;34;44",
		"0;36;44", "1;36;44", "1;36;46", "1;37;46",
		"1;37;47", "1;37;47", "1;37;47", "1;37;47",
		"1;37;47", "1;37;47", "1;37;47", "1;37;47"
	};
	static char hex[16] = "0123456789abcdef";
	if (count == SHOW_SCALEFACS_COLOR) {
		if (ch)
			count = 0;
		fprintf(stderr, "|");
		for (i = 0; i < 21; i++)
//			fprintf(stderr, "\e[%sm%c", kl[scalefac[i]], ix_max(sbo[i], sbo[i+1]) ? '#' : '-');
			fprintf(stderr, "\e[%sm%c", kl[scalefac[i]], ix_max(sbo[i], sbo[i+1]) ? hex[scalefac[i]] : '-');
		fprintf(stderr, "\e[0m| %2d %4d %3d%c", gi->part2_length, gi->part3_length, gi->global_gain, ch ? '\n' : '\t');
	}
	if (ch)
		count++;
}
#else
	fprintf(stderr, "(%d)", ch);
	for (i = 0; i < 21; i++)
		fprintf(stderr, "%3d", scalefac[i]);
	fprintf(stderr, "  %3d  %2d %4d\n", gi->global_gain, gi->part2_length, gi->part3_length);
#endif
}
#endif

static fixed16 last_time_factor[2] = {0, 0};
static fixed16 last_time_s[2] = {0x10000, 0x10000};
fixed16 time_factor;
void calc_time_factor(fixed16 *x, int ch)
{
	fixed16 s1, s2;
	fixed16 t;

	int i;

	s1 = s2 = 0;
	for (i = 0; i < 288; i++)
		if (fm16(x[i], x[i]) > s1) s1 = fm16(x[i], x[i]);
	for ( ; i < 576; i++)
		if (fm16(x[i], x[i]) > s1) s2 = fm16(x[i], x[i]);

	time_factor = s1 ? fd16(s2, s1) : 32000 << 16;
	time_factor = 0x10000 + time_factor;

s1 += s2;

	t = (last_time_s[ch] ? fd16(s1, last_time_s[ch]) : 32000 << 16);
	t = 0x10000 + t;
	time_factor = fm16(time_factor, t);

t = time_factor;
time_factor = fm16(time_factor, last_time_factor[ch]);

last_time_s[ch] = s1;
last_time_factor[ch] = t;

	if (time_factor > 3 << 16)
		time_factor = 3 << 16;
	if (time_factor < 26000) // 0.4
		time_factor = 26000;
}


static fixed8 samples[2][1152];
static fixed16 dctbuf[2][2][18][32];
// DD: pulled si out of L3_compress for new interface
static sideinfo si;
fixed16 xr[576];
int ix[576];
char sx[576];


void L3_compress(short* pSamples, int iNumSamples)
{
  //static int qcount[8192];

 fixed8 *in[2];
 int ch, gr;
 int i;
 int iSamplesRead;
 // DD: made sideinfo si global, like dctbuf
 // sideinfo si;
 
 int mean_bits;
 int padding;
#ifdef PROGRESS_REPORT
 static int frame_no = 0;
#endif
 
 
 memset((char *)&si, 0, sizeof(si));
 memset((char *)&dctbuf, 0, sizeof(dctbuf));
 
 init_polyphase();
 init_mdct();
 init_hbits();
 init_code();
 init_34();
 
 while (iSamplesRead = in_get(samples, pSamples, iNumSamples)) {
   iNumSamples -= iSamplesRead;

   //fprintf(stderr, "\n{F: %d} ", frame_no);
   
   mpeg.frame_size = (int)(1152.0f / 44.1f / 8.0f * mpeg.bitrate);
   // or round it up, or divide it evenly
   
   mean_bits = 4 * mpeg.frame_size - 144;
   
   reservoir_start(8 * mpeg.frame_size);
   
   out_start();
   
   in[0] = samples[0];
   in[1] = samples[1];
   
   //fprintf(stderr, "\n");
   
   for (gr = 0; gr < 2; gr++) {
     for (ch = 0; ch < 2; ch++) {
       
       
       //profile_data pd;
       //profile_start(&pd);
       calc_time_factor(in[ch], ch);
       //profile_end(&pd, "timefac");
       //profile_start(&pd);
       //       for (i = 0; i < 9; i++) {
       //	 polyphaseA(ch, &in[ch], &dctbuf[ch][1][2*i][0]);
       //	 polyphaseB(ch, &in[ch], &dctbuf[ch][1][2*i+1][0]);
       //       }
       polyphase(ch, &in[ch], &dctbuf[ch][1][0][0]);

       //				if (si.gr[gr].ch[ch].tt.block_type != 2) {
       for (i = 0; i < 32; i++)
	 mdct(&dctbuf[ch][0][0][i], &dctbuf[ch][1][0][i], &xr[18 * i], i & 1);
       
       butterfly(); // xr gets from .16 to .13
       
       //				} else {
       //					for (i = 0; i < 576; i++) xr[i] = 0;
       //				}
       
       memcpy(dctbuf[ch][0], dctbuf[ch][1], 576 * sizeof(fixed16));
				//profile_end(&pd, "xform");
				//profile_start(&pd);
       code(ch, &(si.gr[gr].ch[ch].tt), mean_bits);
				//profile_end(&pd, "code");
       
       
       
       //{ int i; for (i = 0; i < 576; i++) qcount[ix[i]]++; }
       
       
       
#ifdef SHOW_SCALEFACS
       //if (gr == 0 && ch == 0) print_mag(&si.gr[gr].ch[ch].tt);
       //if (ch == 0) print_mag(&si.gr[gr].ch[ch].tt);
       print_mag(&si.gr[gr].ch[ch].tt, ch);
#endif
       
       // profile_start(&pd);
       out(&(si.gr[gr].ch[ch].tt));
       // profile_end(&pd, "out");
     }

   }
   padding = reservoir_end(&si);
   out_done(&si, padding);

   diag_printf("pdata = %x\n", pSamples);
   diag_printf("iData = %x\n", iSamplesRead);
   pSamples += iSamplesRead;
   diag_printf("pdata = %x\n\n", pSamples);

 }    
 L3_FlushBitstream();
 
#ifdef PROGRESS_REPORT
 fprintf(stderr, "\n");
#endif
 
 
 
 //{ int i; for (i = 0; i < 8192; i++) fprintf(stderr, "%4d: %d \t(%d)\n", i, qcount[i], i * qcount[i]); }
 

}

// DD: begin new interface
void L3_open( void ) {

 DEBUG("+%s\n", __FUNCTION__);
 memset((char *)&si, 0, sizeof(si));
 memset((char *)&dctbuf, 0, sizeof(dctbuf));
 
 init_polyphase();
 init_mdct();
 init_hbits();
 init_code();
 init_34();

 // DD: important mpeg settings!
 mpeg.bitrate = 128;
 mpeg.bitrate_index = 9;
 mpeg.ms_stereo = 1;

 // DD: resetting state, very important... 
 DEBUG("*******%s  last_time_factor[0] = %d \n", __FUNCTION__, last_time_factor[0]);
 DEBUG("*******%s  last_time_factor[1] = %d \n", __FUNCTION__, last_time_factor[1]);
 DEBUG("*******%s  last_time_s[0]      = %d \n", __FUNCTION__, last_time_s[0]);
 DEBUG("*******%s  last_time_s[1]      = %d \n", __FUNCTION__, last_time_s[1]);
 DEBUG("*******%s  time_factor         = %d \n", __FUNCTION__, time_factor);

 last_time_factor[0] = 0;
 last_time_factor[1] = 0;
 last_time_s[0] = 0x10000;
 last_time_s[0] = 0x10000;
 time_factor = 0;

 DEBUG("-%s\n", __FUNCTION__);
}

void L3_compressFrame( short *pOutBuf, unsigned int* pOutBufLen, short *pInBuf ) {

 fixed8 *in[2];
 int ch, gr;
 int i;

 int j;
 int tmpAddress;

 fixed16 *b0 = samples[0], *b1 = samples[1];
/*   fixed16 *b0, *b1; */
/*   fixed16 t0, t1; */

 // DD: made sideinfo si global, like dctbuf
 // sideinfo si;
 
 int mean_bits;
 int padding;
#ifdef PROGRESS_REPORT
 static int frame_no = 0;
#endif

 // DD: need to set the outputBuffer pointer and length in out.c
 setOutputBufferPointer( pOutBuf );
 setOutputBufferLengthPointer( pOutBufLen );

 // do the in_get shifting right here.
 tmpAddress = ((int) pInBuf);
 for (j = 0; j < 1152; j++) {
   fixed16 t0 = ((int)*pInBuf++) << 14;
   fixed16 t1 = ((int)*pInBuf++) << 14;
   *b0++ = (t0 + t1);
   *b1++ = (t0 - t1);
 }

   // need to massage in_get... or do the work in PEMOutputFilter::Write
   
   mpeg.frame_size = (int)(1152.0f / 44.1f / 8.0f * mpeg.bitrate);
   // or round it up, or divide it evenly
   
   mean_bits = 4 * mpeg.frame_size - 144;
   
   reservoir_start(8 * mpeg.frame_size);
   
   out_start();
   
   in[0] = samples[0];
   in[1] = samples[1];
   
   //fprintf(stderr, "\n");
   
   for (gr = 0; gr < 2; gr++) {
     for (ch = 0; ch < 2; ch++) {
       
       
       //profile_data pd;
       //profile_start(&pd);
       calc_time_factor(in[ch], ch);

       //profile_end(&pd, "timefac");
       //profile_start(&pd);
       
       polyphase(ch, &in[ch], &dctbuf[ch][1][0][0]);
       
       for (i = 0; i < 32; i++)
	 mdct(&dctbuf[ch][0][0][i], &dctbuf[ch][1][0][i], &xr[18 * i], i & 1);
       
       butterfly(); // xr gets from .16 to .13

       memcpy(dctbuf[ch][0], dctbuf[ch][1], 576 * sizeof(fixed16));

				//profile_end(&pd, "xform");
				//profile_start(&pd);
       code(ch, &(si.gr[gr].ch[ch].tt), mean_bits);

				//profile_end(&pd, "code");
       
#ifdef SHOW_SCALEFACS
       print_mag(&si.gr[gr].ch[ch].tt, ch);
#endif
       
       // profile_start(&pd);
       out(&(si.gr[gr].ch[ch].tt));

       // profile_end(&pd, "out");
     }

   }

   padding = reservoir_end(&si);
   out_done(&si, padding);


}

void L3_close( void ) {
 DEBUG("+%s\n", __FUNCTION__);
 L3_FlushBitstream();
 DEBUG("-%s\n", __FUNCTION__);
}
// DD: end new interface
