#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "types.h"
#include "ro.h"
#include <setjmp.h>

#if 0
#ifndef PEM_USE_SRAM
#define sram_p (&the_fake_sram)
extern struct sram the_fake_sram;
#else
#include <codec/common/codec_workspace.h>
#define sram_p ((struct sram *)FAST_DECODE_ADDR)
#endif
#endif

extern struct sram *sram_p;



struct sram {
	
	struct granule_t granule[2][2];
	
	fixed16 dctbuf[2][2][576];
	fixed16 xr[576];
	short ix[576];
	unsigned int sxs[18];
	
	int limit_nonzero;
	int limit_big;
	
	char hbits[34][16][16];
	
	int slen0, slen1;
	int scalefac[21];
	
	fixed64_32 eeb[2];		// sum of energy in all (used) bands
	fixed64_32 eb[21];		// energy in band
	fixed64_32 heb[2][21];	// last granule's energy in band
	
	fixed16 sf[21];
	fixed16 old_sf[2][21];
	
	fixed16 leb[21];		// logarithm of energy band
	fixed16 lebs[21];		// leb, spreading function applied
	fixed16 lthr[21];		// log threshold (lebs-0x90000)
	fixed16 enm[144];
	
	short wwindow[512];
	short polybuf[2112];
	
	struct mpeg_t mpeg;
	
	
	int ResvSize;
	int ResvMax;
	
	
	int last_gain[2];
	int last_bits[2];
	
	
	int bytes_to_do;
	int skew;
	int mainlength[8];
	int sideno;
	
	int main_data_begin;
	
	unsigned long *sbuffer, *dbuffer;
	unsigned long sbufferword, dbufferword;
	int sbits_left, dbits_left;
	
	unsigned int sxss;
	int sxsn;
	int sxsi;
	
	unsigned char *outbuf;
	unsigned long outcount;
	
	struct ro_sram ro;
	
};

extern struct dram {
#if defined(PEM_DEMO_LIMITFRAMES)
	unsigned long framecount;
#endif
	unsigned long mainbitter[4096];
	unsigned long sidebitter[72];
	jmp_buf error_jbuf;
} dram;

#define FASTVAR(x) (sram_p->x)
#define FASTRO(x) (sram_p->ro.x)
#define VAR(x) (dram.x)

#if 0
#define granule sram_p->granule
                                       
#define dctbuf sram_p->dctbuf
#define xr sram_p->xr
#define ix sram_p->ix
#define sxs sram_p->sxs
                                       
#define limit_nonzero sram_p->limit_nonzero
#define limit_big sram_p->limit_big
                                       
#define hbits sram_p->hbits
                                       
#define slen0 sram_p->slen0
#define slen1 sram_p->slen1
#define scalefac sram_p->scalefac
                                       
#define eeb sram_p->eeb
#define eb sram_p->eb
#define heb sram_p->heb
                                       
#define sf sram_p->sf
#define old_sf sram_p->old_sf
                                       
#define leb sram_p->leb
#define lebs sram_p->lebs
#define lthr sram_p->lthr
#define enm sram_p->enm
                                       
#define wwindow sram_p->wwindow
#define polybuf sram_p->polybuf
                                       
#define mpeg sram_p->mpeg
                                       
                                       
#define ResvSize sram_p->ResvSize
#define ResvMax sram_p->ResvMax
                                       
                                       
#define last_gain sram_p->last_gain
#define last_bits sram_p->last_bits
                                       
                                       
#define bytes_to_do sram_p->bytes_to_do
#define skew sram_p->skew

#define mainlength sram_p->mainlength
#define sideno sram_p->sideno
                                       
#define main_data_begin sram_p->main_data_begin
                                       
#define sbuffer sram_p->sbuffer
#define dbuffer sram_p->dbuffer
#define sbufferword sram_p->sbufferword
#define dbufferword sram_p->dbufferword
#define sbits_left sram_p->sbits_left
#define dbits_left sram_p->dbits_left
                                       
#define sxss sram_p->sxss
#define sxsn sram_p->sxsn
#define sxsi sram_p->sxsi


#endif

#define sidebitter dram.sidebitter
#define mainbitter dram.mainbitter
#define error_jbuf dram.error_jbuf



#define enen sram_p->ro.enen
#define maxs sram_p->ro.maxs
#define maxpb sram_p->ro.maxpb
#define sbo sram_p->ro.sbo
#define sbew sram_p->ro.sbew
#define bark sram_p->ro.bark
#define afac sram_p->ro.afac
#define mfac sram_p->ro.mfac
#define l2 sram_p->ro.l2
#define subdv_table sram_p->ro.subdv_table
#define huftab0 sram_p->ro.huftab0
#define huftab1 sram_p->ro.huftab1
#define huftab2 sram_p->ro.huftab2
#define iwinn sram_p->ro.iwinn
#define itan0 sram_p->ro.itan0
#define trig sram_p->ro.trig
#define ca sram_p->ro.ca
#define cs sram_p->ro.cs
#define costab sram_p->ro.costab
#define enfac sram_p->ro.enfac
#define slen sram_p->ro.slen
#define sf0 sram_p->ro.sf0
#define sf1 sram_p->ro.sf1
#define gaintab sram_p->ro.gaintab
#define qq0tab sram_p->ro.qq0tab
#define t1HB sram_p->ro.t1HB
#define t2HB sram_p->ro.t2HB
#define t3HB sram_p->ro.t3HB
#define t5HB sram_p->ro.t5HB
#define t6HB sram_p->ro.t6HB
#define t7HB sram_p->ro.t7HB
#define t8HB sram_p->ro.t8HB
#define t9HB sram_p->ro.t9HB
#define t10HB sram_p->ro.t10HB
#define t11HB sram_p->ro.t11HB
#define t12HB sram_p->ro.t12HB
#define t16HB sram_p->ro.t16HB
#define t24HB sram_p->ro.t24HB
#define t32HB sram_p->ro.t32HB
#define t33HB sram_p->ro.t33HB
#define t1l sram_p->ro.t1l
#define t2l sram_p->ro.t2l
#define t3l sram_p->ro.t3l
#define t5l sram_p->ro.t5l
#define t6l sram_p->ro.t6l
#define t7l sram_p->ro.t7l
#define t8l sram_p->ro.t8l
#define t9l sram_p->ro.t9l
#define t10l sram_p->ro.t10l
#define t11l sram_p->ro.t11l
#define t12l sram_p->ro.t12l
#define t13l sram_p->ro.t13l
#define t15l sram_p->ro.t15l
#define t16l sram_p->ro.t16l
#define t24l sram_p->ro.t24l
#define t32l sram_p->ro.t32l
#define t33l sram_p->ro.t33l
#define ht sram_p->ro.ht
#define hlen sram_p->ro.hlen

#endif//__MEMORY_H__
