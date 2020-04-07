#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdio.h> // for debugging

struct encoder_context_t {
	struct sram *fast_rw;
	struct dram *slow_rw;
	struct ro_sram *fast_ro;
};

extern struct mpeg_t {
	int ms_stereo;
	int frame_size;
	int bitrate;
	int bitrate_index;
	int max_sb;
} mpeg;


struct granule_t {
	int part2_length;
	int part3_length;
//	int big_values;
	int block_type;
//	int count1;
	int global_gain;
	int scalefac_scale;		// Not reset to zero by code()
	int scalefac_compress;	// Not reset to zero by code()
	int subblock_gain[3];
	int table_select[3];
	int region0_count;
	int region1_count;
	int count1table_select;

//	int address1;
//	int address2;
//	int address3;

	int pos0;
	int pos1;
	int pos2;
	int pos3;
};


#ifdef __arm__
#define _F_ __asm__ __volatile__ ("");

#define _SRAM_ __attribute__ (( __section__ (".sram") ))
#define _FAR_ __attribute__ (( __long_call__ ))

#define _PRIME_CACHE_(x,n) \
	{ int dummy, dummy2, dummy3; \
		__asm__ __volatile__ ( \
			"1:\n\t" \
			"ldr	%0,[%1],#16\n\t" \
			"subs	%2,%2,#1\n\t" \
			"bne	1b" \
			: "=r"(dummy), "=r"(dummy2), "=r"(dummy3) : "1"(x), "2"(n) \
		); \
	}
#else
#define _F_
#define _SRAM_
#define _FAR_
#define _PRIME_CACHE_(x,n)
#endif

#ifdef __OPTIMIZE__
#define FAST_FUNC extern inline __attribute__ (( no_instrument_function ))
#else
#define FAST_FUNC static inline __attribute__ (( no_instrument_function ))
#endif

typedef long fixed13;
typedef long fixed16;
typedef long fixed31;
typedef long fixed40;
typedef long long fixed64_32;

#if defined(PEM_ARM_ASM) && defined(PEM_BETTER_FD16)
FAST_FUNC fixed16 fd16(fixed16 a, fixed16 b) {
	long result, t1, t2, t3;
	__asm__ (
"	@ fd16 inlined here		"
"	teq %1, #0				"
"	beq 3f					"
"	mov %2, %0				"
"	mov %3, #0				"
"	cmp %1, %2, lsr #1		"
"1:	movls %1, %1, lsl #1	"
"	addls %3, %3, #1		"
"	cmpls %1, %2, lsr #1	"
"	bls 1b					"
"	mov %0, #0				"
"	add %3, %3, #16			"
"2:	cmp %2, %1				"
"	subcs %2, %2, %1		"
"	mov %2, %2, lsl #1		"
"	adcs %0, %0, %0			"
"	bcs 4f					"
"	subs %3, %3, #1			"
"	bpl 2b					"
"	b 5f					"
"3:	mov %0, #0				"
"	b 5f					"
"4:	mvn %0, #0x80000000		"
"5:	@ end of inline			"
		: "=r"(result), "=r"(t1), "=r"(t2), "=r"(t3)
		: "0"(a), "1"(b)
		: "cc"
	);
  return result;
}
#else
FAST_FUNC fixed16 fd16(fixed16 a, fixed16 b) {
	return ((long long)a << 16) / b;
}
#endif

FAST_FUNC fixed16 fm16(fixed16 a, fixed16 b) {
	return ((long long)a * b) >> 16;
}

//extern inline fixed13 fm16_16_8(fixed16 a, fixed16 b) {
//	return ((long long)a * b) >> 24;
//}

FAST_FUNC fixed13 fm16_16_13(fixed16 a, fixed16 b) {
	return ((long long)a * b) >> 19;
}

FAST_FUNC fixed16 fm16_31_16(fixed16 a, fixed31 b) {
	return ((long long)a * b) >> 31;
}

//extern inline fixed13 fm13_16_13(fixed13 a, fixed16 b) {
//	return ((long long)a * b) >> 16;
//}

//extern inline fixed16 nfm40_8_16(long long a) {
//	return a >> 32;
//}

FAST_FUNC fixed64_32 fm16_16_64_32(fixed16 a, fixed16 b) {
	return ((long long)a * b);
}

FAST_FUNC fixed64_32 fm16_64_32_64_32(fixed16 a, fixed64_32 b) {
	return (a * b) >> 16;
}

//extern inline fixed16 ftof10(float a) {
//	return (int)(0.5f + 1024.0f * a);
//}

//extern inline fixed16 ftof13(float a) {
//	return (int)(0.5f + 8192.0f * a);
//}

FAST_FUNC fixed16 ftof16(float a) {
	return (int)(0.5f + 65536.0f * a);
}

//extern inline fixed31 ftof31(float a) {
//	return (int)(0.5f + 65536.0f * 32768.0f * a);
//}

FAST_FUNC fixed40 ftof40(float a) {
	return (int)(0.5f + 65536.0f * 65536.0f * 256.0f * a);
}

//extern inline fixed64_32 ftof64_32(float a) {
//	return (long long)(0.5f + 65536.0f * 65536.0f * a);
//}

#endif

