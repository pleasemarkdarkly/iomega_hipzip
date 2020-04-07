#include <stdio.h>
#include <string.h>

#if defined(PROF)
#include <util/diag/TinyProfile.h>
#define DECLARE_PROFILE(x) static profile_data _profile_data_##x
#define START_PROFILE(x) profile_start(&_profile_data_##x);
#define END_PROFILE(x,args...) diag_printf(args, profile_end(&_profile_data_##x));
#else
#define DECLARE_PROFILE(x)
#define START_PROFILE(x)
#define END_PROFILE(x,y...)
#endif


//#define SHOW_SCALEFACS
#define SHOW_SCALEFACS_COLOR 1


#include "types.h"
#include "codec.h"
#include "polyphase.h"
#include "hybrid.h"
#include "code.h"
#include "in.h"
#include "out.h"
#include "huffman.h"
#include "reservoir.h"
#include "memory.h"
#include <setjmp.h>
#if defined(DESKTOP) || defined(STANDALONE)
#include "../../../../util/eresult/include/eresult.h"
#else
#include <util/eresult/eresult.h>
#include <codec/common/codec_workspace.h>
#endif
#include "quant.h"


#ifdef SHOW_SCALEFACS
int ix_max(int, int);

static void print_mag(struct granule_t *gi, int ch)
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


ERESULT pem_init(int bitrate)
{
	int i;

	sram_p = FAST_DECODE_ADDR; // from codec_workspace.h

	memset(&dram, 0, sizeof(dram));		// automatically resets framecount, if used
	memset(sram_p, 0, sizeof(struct sram) - sizeof(struct ro_sram));
	memcpy(&sram_p->ro, &rom_ro_sram, sizeof(struct ro_sram));

	//diag_printf("sram at %08x\n", sram_p);
	//diag_printf("ro_sram at %08x\n", &sram_p->ro);
	//diag_printf("rom_ro_sram at %08x\n", &rom_ro_sram);
	//diag_printf("so sram %x\n", sizeof(struct sram));
	//diag_printf("so ro_sram %x\n", sizeof(struct ro_sram));

	for (i = 1; i < 15; i++)
		if (bitrate == bitrates[i])
			break;

	if (i == 15)
		return PEM_BAD_BITRATE;

	FASTVAR(mpeg).bitrate = bitrate;
	FASTVAR(mpeg).bitrate_index = i;

	init_polyphase();
	init_hbits();
	init_code();
	reservoir_init();
	out_init();

	return PEM_NO_ERROR;
}

#ifdef PEM_DEMO_LIMITFRAMES
static void obnoxious_noise(short *pcm)
{
	int i,j;
	j=0x3fff;
	for(i=0;i<1152;i++) {
		*pcm++ = j;
		*pcm++ = j;
		if (i%128 == 127)
			j = -j;
	}
}
#endif

ERESULT pem_work(short *pcm, int *pcm_count, int e_o_s)
{
	int ch, gr;
	int mean_bits;
	int padding;
	ERESULT err;
	DECLARE_PROFILE(polyphase);
	DECLARE_PROFILE(mdct);
	DECLARE_PROFILE(butterfly);
	DECLARE_PROFILE(code);
	DECLARE_PROFILE(out);

	err = setjmp(error_jbuf);
	if (err) {
		return err;
	}

	if (*pcm_count < 1152 && !e_o_s)
		return PEM_NOT_ENOUGH_INPUT;  /* error: need more input */

	if (*pcm_count > 1152)
		*pcm_count = 1152;

	FASTVAR(mpeg).frame_size = framesizes[FASTVAR(mpeg).bitrate_index];//(int)(1152.0f / 44.1f / 8.0f * FASTVAR(mpeg).bitrate);
		// or round it up, or divide it evenly

	FASTVAR(mpeg).ms_stereo = 1;

	mean_bits = 4 * FASTVAR(mpeg).frame_size - 144;

	reservoir_start(8 * FASTVAR(mpeg).frame_size);

	out_start();

#if defined(PEM_DEMO_LIMITFRAMES)
	if (dram.framecount++ > PEM_DEMO_LIMITFRAMES) {
//		longjmp(error_jbuf,PEM_DEMO_LIMIT_REACHED);
		obnoxious_noise(pcm);
	}
#endif

	for (gr = 0; gr < 2; gr++) {

		START_PROFILE(polyphase);
		polyphase(pcm+1152*gr, FASTVAR(dctbuf)[0][1-gr], FASTVAR(dctbuf)[1][1-gr]);
		END_PROFILE(polyphase,"polyphase:%d\n");


		for (ch = 0; ch < 2; ch++) {

			START_PROFILE(mdct);
			mdct(FASTVAR(dctbuf)[ch][gr], FASTVAR(dctbuf)[ch][1-gr], FASTVAR(xr), 32 - 16 * ch);
			END_PROFILE(mdct,"mdct%d:%d\n",ch);

			START_PROFILE(butterfly);
			butterfly(ch ? 16 : 31);
			END_PROFILE(butterfly,"butterfly%d:%d\n",ch);

//{ static int max = 0, min = 0; int i; for (i = 0; i < 576; i++) { if (FASTVAR(xr)[i] > max) max = FASTVAR(xr)[i]; if (FASTVAR(xr)[i] < min) min = FASTVAR(xr)[i]; } fprintf(stderr, "max = 0x%08x, min = 0x%08x\n", max, min); }

			START_PROFILE(code);
			code(ch, &FASTVAR(granule)[gr][ch], mean_bits);
			END_PROFILE(code,"code%d:%d\n",ch);

#ifdef SHOW_SCALEFACS
print_mag(&FASTVAR(granule)[gr][ch], ch);
#endif

			START_PROFILE(out);
			out(&FASTVAR(granule)[gr][ch]);
			END_PROFILE(out,"out%d:%d\n",ch);
		}
	}
	padding = reservoir_end();
	START_PROFILE(out);
	out_done(padding);
	END_PROFILE(out,"out_done:%d\n");

	return PEM_NO_ERROR;
}
ERESULT pem_save_state(void* buf, int len)
{
    if( len < (sizeof(dram)+sizeof(struct sram)) ) {
        return PEM_INSUFFICIENT_SPACE;
    }
    memcpy( buf, &dram, sizeof(dram) );
    buf += sizeof(dram);
    // this copies the ro section too - oh well
    memcpy( buf, sram_p, sizeof(struct sram) );
    
    return PEM_NO_ERROR;
}

ERESULT pem_restore_state(const void* buf)
{
    memcpy( &dram, buf, sizeof(dram) );
    buf += sizeof(dram);
    memcpy( sram_p, buf, sizeof(struct sram) );
    
    return PEM_NO_ERROR;
}

int pem_get_state_size(void)
{
    return (sizeof(dram) + sizeof(struct sram) );
}

void pem_fini(void)
{
	out_fini();
}

