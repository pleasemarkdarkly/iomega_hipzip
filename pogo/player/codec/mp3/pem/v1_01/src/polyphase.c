//#define PROF
#ifdef PROF
#include "TinyProfile.h"
#endif

#include <stdio.h>


#include <math.h>
#define PI 3.1415926535898
#include "polyphase.h"
#include "types.h"
#include "memory.h"



#ifdef BIGENDIAN
union ll {
	long long x;
	struct {
		long hi;
		long lo;
	} xx;
};
#else
union ll {
	long long x;
	struct {
		long lo;
		long hi;
	} xx;
};
#endif

/*
  mutiply 32bit x 32bit, discarding low 32 bits.
*/
extern inline int cfm(int a1, int a2) {
	union ll b;
	b.x = (long long)a1 * a2;
#ifdef PEM_PRECISION
	return (b.xx.hi < 0) ? b.xx.hi+1 : b.xx.hi;
#else
	return b.xx.hi;
#endif
}


#define c_4_1   0x5a827999	// 31-bit fractional
#define c_8_1   0x22a2f4f7	// 30-bit fractional
#define c_8_3   0x539eba45
#define c_16_1  0x10503ed1	// 29-bit fractional
#define c_16_7  0x52036741
#define c_16_3  0x133e37a1
#define c_16_5  0x1ccc9aef
#define c_32_1  0x0809e8ce	// 28-bit fractional
#define c_32_15 0x519e4e03
#define c_32_3  0x085c2781
#define c_32_13 0x1b8f24b0
#define c_32_5  0x091233e8
#define c_32_11 0x10f8892a
#define c_32_7  0x0a5961cc
#define c_32_9  0x0c9c4805
#define c_64_1  0x04013c25	// 27-bit fractional
#define c_64_31 0x518522fa
#define c_64_3  0x040b345b
#define c_64_29 0x1b42c833
#define c_64_5  0x041fa2d6
#define c_64_27 0x107655e3
#define c_64_7  0x043f9342
#define c_64_25 0x0bdf91b2
#define c_64_9  0x046cc1bc
#define c_64_23 0x095b0352
#define c_64_11 0x04a9d9cf
#define c_64_21 0x07c7d1db
#define c_64_13 0x04fae371
#define c_64_19 0x06b6fcf2
#define c_64_15 0x056601ea
#define c_64_17 0x05f4cf6e


void init_polyphase(void)
{
	int i, j;

	for (j = 0; j < 33*32*2; j++)
		FASTVAR(polybuf)[j] = 0;

	for(i = 0; i < 512; i++)
		FASTVAR(wwindow)[i] = wwin_init[i];
}

#ifdef PEM_PRECISION
#define SHIFT(a,b) ((a) / (1 << (b)))
#define PREC(x) x
#define FAST(x)
#else
#define SHIFT(a,b) ((a) >> (b))
#define PREC(x)
#define FAST(x) x
#endif

#if defined(PEM_ARM_ASM) && defined(__OPTIMIZE__)
#define A2(i,j) { int dummy; __asm__ __volatile__ ( \
	"\n\t\t" \
FAST("smull	%1,%2,%3,%1\n\t\t") \
PREC("smulls	%1,%2,%3,%1\n\t\t") \
PREC("addmi	%2, %2, #1\n\t\t") \
	"rsb	%1,%2,%0, asr #1\n\t\t" \
	"add	%0,%2,%0, asr #1\n" \
	: "+r"(a ## i), "+r"(a ## j), "=&r"(dummy) : "r"(c_4_1)); }
#else
#define A2(i,j) { \
	fixed16 t = a ## i, u = cfm(a ## j, c_4_1); \
	a ## i = SHIFT(t,1) + u; \
	a ## j = SHIFT(t,1) - u; \
}
#endif

#if defined(PEM_ARM_ASM) && defined(__OPTIMIZE__)
#define B2(i,j) { int dummy; __asm__ __volatile__ ( \
	"\n\t\t" \
FAST("smull	%2,%1,%3,%1\n\t\t") \
PREC("smulls	%2,%1,%3,%1\n\t\t") \
PREC("addmi	%1,%1,#1\n\t\t") \
	"rsb	%2,%1,%0, asr #1\n\t\t" \
	"add	%1,%1,%0, asr #1\n\t\t" \
FAST("smull	%2,%0,%5,%2\n\t\t") \
PREC("smulls	%2,%0,%5,%2\n\t\t") \
PREC("addmi %0,%0,#1\n\t\t") \
FAST("smull	%2,%1,%4,%1\n") \
PREC("smulls	%2,%1,%4,%1\n") \
PREC("addmi	%1,%1,#1\n\t\t") \
	: "+r"(a ## i), "+r"(a ## j), "=&r"(dummy) : "r"(c_4_1), "r"(c_8_1), "r"(c_8_3)); }
#else
#define B2(i,j) { \
	fixed16 t = a ## i, u = cfm(a ## j, c_4_1); \
	a ## j = cfm((SHIFT(t,1) + u), c_8_1); \
	a ## i = cfm((SHIFT(t,1) - u), c_8_3); \
}
#endif

#if defined(PEM_ARM_ASM) && defined(__OPTIMIZE__)
#define A4(i,j) __asm__ __volatile__ ( \
	"\n\t\t" \
PREC("cmp	%2,#0\n\t\t") \
PREC("addmi	%2,%2,#3\n\t\t") \
	"add	%0,%1,%2, asr #2\n\t\t" \
	"rsb	%1,%1,%2, asr #2\n" \
	: "=&r"(a ## i), "+r"(a ## j) : "r"(a ## i));
#else
#define A4(i,j) { \
	fixed16 t = a ## i, u = a ## j; \
	a ## i = SHIFT(t,2) + u; \
	a ## j = SHIFT(t,2) - u; \
}
#endif

#if defined(PEM_ARM_ASM) && defined(__OPTIMIZE__)
#define B4(i,j,k,l) { int dummy; __asm__ __volatile__ ( \
	"\n\t\t" \
PREC("cmp	%2,#0\n\t\t") \
PREC("addmi	%2,%2,#3\n\t\t") \
	"rsb	%2,%1,%0, asr #2\n\t\t" \
	"add	%1,%1,%0, asr #2\n\t\t" \
FAST("smull	%2,%0,%4,%2\n\t\t") \
PREC("smulls	%2,%0,%4,%2\n\t\t") \
PREC("addmi	%0,%0,#3\n\t\t") \
FAST("smull	%2,%1,%3,%1\n") \
PREC("smulls	%2,%1,%3,%1\n") \
PREC("addmi	%1,%1,#3\n\t\t") \
	: "+r"(a ## i), "+r"(a ## j), "=&r"(dummy) : "r"(c_16_ ## k), "r"(c_16_ ## l)); }
#else
#define B4(i,j,k,l) { \
	fixed16 t = a ## i, u = a ## j; \
	a ## j = cfm((SHIFT(t,2) + u), c_16_ ## k); \
	a ## i = cfm((SHIFT(t,2) - u), c_16_ ## l); \
}
#endif

#if defined(PEM_ARM_ASM) && defined(__OPTIMIZE__)
#define A8(i,j) __asm__ __volatile__ ( \
	"\n\t\t" \
PREC("cmp %2,#0\n\t\t") \
PREC("addmi %2,%2,#7\n\t\t") \
	"add	%0,%1,%2, asr #3\n\t\t" \
	"rsb	%1,%1,%2, asr #3\n" \
	: "=&r"(a ## i), "+r"(a ## j) : "r"(a ## i));
#else
#define A8(i,j) { \
	fixed16 t = a ## i, u = a ## j; \
	a ## i = SHIFT(t,3) + u; \
	a ## j = SHIFT(t,3) - u; \
}
#endif

#if defined(PEM_ARM_ASM) && defined(__OPTIMIZE__)
#define B8(i,j,k,l) { int dummy; __asm__ __volatile__ ( \
	"\n\t\t" \
PREC("cmp %0,#0\n\t\t") \
PREC("addmi %0,%0,#7\n\t\t") \
	"rsb	%2,%1,%0, asr #3\n\t\t" \
	"add	%1,%1,%0, asr #3\n\t\t" \
	"smull	%2,%0,%4,%2\n\t\t" \
	"smull	%2,%1,%3,%1\n" \
	: "+r"(a ## i), "+r"(a ## j), "=&r"(dummy) : "r"(c_32_ ## k), "r"(c_32_ ## l)); }
#else
#define B8(i,j,k,l) { \
	fixed16 t = a ## i, u = a ## j; \
	a ## j = cfm((SHIFT(t,3) + u), c_32_ ## k); \
	a ## i = cfm((SHIFT(t,3) - u), c_32_ ## l); \
}
#endif

#ifdef PEM_ARM_ASM
#define A16(i,j) __asm__ __volatile__ ( \
	"\n\t\t" \
PREC("cmp %2,#0\n\t\t") \
PREC("addmi %2,%2,#15\n\t\t") \
	"add	%0,%1,%2, asr #4\n\t\t" \
	"rsb	%1,%1,%2, asr #4\n" \
	: "=&r"(a ## i), "+r"(a ## j) : "r"(a ## i));
#else
#define A16(i,j) { \
	fixed16 t = a ## i, u = a ## j; \
	a ## i = SHIFT(t,4) + u; \
	a ## j = SHIFT(t,4) - u; \
}
#endif

#ifdef PEM_ARM_ASM
#define B16(i,j,k,l) { int dummy; __asm__ __volatile__ ( \
	"\n\t\t" \
PREC("cmp %0,#0\n\t\t") \
PREC("addmi %0,%0,#15\n\t\t") \
	"rsb	%2,%1,%0, asr #4\n\t\t" \
	"add	%1,%1,%0, asr #4\n\t\t" \
FAST("smull	%2,%0,%4,%2\n\t\t") \
PREC("smulls	%2,%0,%4,%2\n\t\t") \
PREC("addmi	%0,%0,#1\n\t\t") \
FAST("smull	%2,%1,%3,%1\n") \
PREC("smulls	%2,%1,%3,%1\n") \
PREC("addmi	%1,%1,#1\n\t\t") \
	: "+r"(a ## i), "+r"(a ## j), "=&r"(dummy) : "r"(c_64_ ## k), "r"(c_64_ ## l)); }
#else
#define B16(i,j,k,l) { \
	fixed16 t = a ## i, u = a ## j; \
	a ## j = cfm((SHIFT(t,4) + u), c_64_ ## k); \
	a ## i = cfm((SHIFT(t,4) - u), c_64_ ## l); \
}
#endif

#ifdef PEM_ARM_ASM
#define A32(i,j) __asm__ __volatile__ ( \
	"\n\t\t" \
PREC("cmp %2,#0\n\t\t") \
PREC("addmi %2,%2,#31\n\t\t") \
	"add	%0,%1,%2, asr #5\n\t\t" \
	"rsb	%1,%1,%2, asr #5\n" \
	: "=&r"(a ## i), "+r"(a ## j) : "r"(a ## i));
#else
#define A32(i,j) { \
	fixed16 t = a ## i, u = a ## j; \
	a ## i = SHIFT(t,5) + u; \
	a ## j = SHIFT(t,5) - u; \
}
#endif

#ifdef PEM_ARM_ASM
#define A32_S(i,j) __asm__ __volatile__ ( \
	"\n\t\t" \
PREC("cmp %0,#0\n\t\t") \
PREC("addmi %0,%0,#31\n\t\t") \
	"add	%0,%1,%0, asr #5\n\t\t" \
	: "+r"(a ## i) : "r"(a ## j));
#else
#define A32_S(i,j) { \
	fixed16 t = a ## i, u = a ## j; \
	a ## i = SHIFT(t,5) + u; \
}
#endif



#define X2(x0,x1,x2,x3,x4,x5,x6,x7) A2(x0,x1) B2(x2,x3) A2(x4,x5) B2(x6,x7)
#define X4(x0,x1,x2,x3,x4,x5,x6,x7) A4(x0,x3) A4(x1,x2) B4(x4,x7,1,7) B4(x5,x6,3,5)
#define XA8(x0,x1,x2,x3,x4,x5,x6,x7) A8(x0,x7) A8(x1,x6) A8(x2,x5) A8(x3,x4)
#define XB8(x0,x1,x2,x3,x4,x5,x6,x7) B8(x0,x7,1,15) B8(x1,x6,3,13) B8(x2,x5,5,11) B8(x3,x4,7,9)


#ifdef PEM_ARM_ASM

#define ALL8 \
	register fixed16 a0 __asm__("r2"); \
	register fixed16 a1 __asm__("r3"); \
	register fixed16 a2 __asm__("r4"); \
	register fixed16 a3 __asm__("r5"); \
	register fixed16 a4 __asm__("r6"); \
	register fixed16 a5 __asm__("r7"); \
	register fixed16 a6 __asm__("r8"); \
	register fixed16 a7 __asm__("r9")

#define STR8(x, a, b, c, d, e, f, g, h) do { \
	__asm__ __volatile__( \
		"stmia	%0, {%1, %2, %3, %4, %5, %6, %7, %8}" \
		: : "r"(x), "r"(a), "r"(b), "r"(c), "r"(d), \
			     "r"(e), "r"(f), "r"(g), "r"(h) \
	); \
} while (0)

#define LDR8(x, a, b, c, d, e, f, g, h) do { \
	__asm__ __volatile__( \
		"ldmia	%8, {%0, %1, %2, %3, %4, %5, %6, %7}" \
		: "=r"(a), "=r"(b), "=r"(c), "=r"(d), \
		  "=r"(e), "=r"(f), "=r"(g), "=r"(h) \
		: "r"(x) \
	); \
} while (0)

#else

#define ALL8 \
	fixed16 a0, a1, a2, a3, a4, a5, a6, a7

#define STR8(x, a, b, c, d, e, f, g, h) do { \
	x[0] = a; x[1] = b; x[2] = c; x[3] = d; \
	x[4] = e; x[5] = f; x[6] = g; x[7] = h; \
} while (0)

#define LDR8(x, a, b, c, d, e, f, g, h) do { \
	a = x[0]; b = x[1]; c = x[2]; d = x[3]; \
	e = x[4]; f = x[5]; g = x[6]; h = x[7]; \
} while (0)

#endif


static /*inline*/ void dctI_0(const fixed16 *z, fixed16 *d)
{
	ALL8;

// ldm 56..63, compute, ldm 24..31, compute
	_F_	a0 = z[61];
	_F_	a1 = z[63] - z[59];
	_F_	a2 = z[62] + z[60];
	_F_	a3 = z[56] - z[58];
	_F_	a4 = z[30] + z[29];
	_F_	a5 = z[24] - z[27];
	_F_	a6 = z[31] + z[28];
	_F_	a7 = z[25] - z[26];

	_F_	a7 += a5;
	_F_	a5 += a6;
	_F_	a6 += a4;
	_F_	a3 += a2;
	_F_	a7 += a6;

	_F_	X2(0, 1, 2, 3, 4, 5, 6, 7)
	_F_	X4(0, 1, 2, 3, 4, 5, 6, 7)
	_F_	XA8(0, 1, 2, 3, 4, 5, 6, 7)

// stm 0..7
	_F_	STR8(d, a0, a1, a2, a3, a4, a5, a6, a7);
	_F_
}


static /*inline*/ void dctI_1(const fixed16 *z, fixed16 *d)
{
#ifndef PEM_ARM_ASM
	fixed16 a0, a1, a2, a3, a4, a5, a6, a7;
#else
	register fixed16 a0 __asm__("r2");
	register fixed16 a1 __asm__("r3");
	register fixed16 a2 __asm__("r4");
	register fixed16 a3 __asm__("r5");
	register fixed16 a4 __asm__("r6");
	register fixed16 a5 __asm__("r7");
	register fixed16 a6 __asm__("r8");
	register fixed16 a7 __asm__("r9");
#endif

// ldm 8..15, ldm 40..43, compute, ldm 44..47, compute
	_F_	a0 = z[14] + z[45];
	_F_	a1 = z[ 8] - z[43];
	_F_	a2 = z[15] + z[44];
	_F_	a3 = z[ 9] - z[42];
	_F_	a4 = z[46] + z[13];
	_F_	a5 = z[40] - z[11];
	_F_	a6 = z[47] + z[12];
	_F_	a7 = z[41] - z[10];

	_F_	a7 += a3;
	_F_	a3 += a5;
	_F_	a5 += a1;
	_F_	a1 += a6;
	_F_	a6 += a2;
	_F_	a2 += a4;
	_F_	a4 += a0;
	_F_	a7 += a5;
	_F_	a5 += a6;
	_F_	a6 += a4;
	_F_	a3 += a2;
	_F_	a7 += a6;

	_F_	X2(0, 1, 2, 3, 4, 5, 6, 7)
	_F_	X4(0, 1, 2, 3, 4, 5, 6, 7)
	_F_	XB8(0, 1, 2, 3, 4, 5, 6, 7)

// stm 8..15
	_F_	d += 8; STR8(d, a0, a1, a2, a3, a4, a5, a6, a7);
	_F_
}


static /*inline*/ void dctI_2(const fixed16 *z, fixed16 *d)
{
	ALL8;
#ifndef PEM_ARM_ASM
	fixed16 tt;
#else
	register fixed16 tt __asm__("r10");
#endif

	_F_	tt = z[49] - z[ 2];
	_F_	a7 = z[33] - z[18];
	_F_	d[31] = tt + a7;
	_F_	tt = z[17] - z[34];
	_F_	a7 += tt;
	_F_	a3 = z[ 1] - z[50];
	_F_	d[27] = tt + a3;
	_F_	tt = z[48] - z[ 3];
	_F_	a3 += tt;
	_F_	a5 = z[32] - z[19];
	_F_	d[29] = tt + a5;
	_F_	tt = z[16] - z[35];
	_F_	a5 += tt;
	_F_	a1 = z[ 0] - z[51];
	_F_	d[25] = tt + a1;
	_F_	tt = z[55] + z[ 4];
	_F_	a1 += tt;
	_F_	a6 = z[39] + z[20];
	_F_	d[30] = tt + a6;
	_F_	tt = z[23] + z[36];
	_F_	a6 += tt;
	_F_	a2 = z[ 7] + z[52];
	_F_	d[26] = tt + a2;
	_F_	tt = z[54] + z[ 5];
	_F_	a2 += tt;
	_F_	a4 = z[38] + z[21];
	_F_	d[28] = tt + a4;
	_F_	tt = z[22] + z[37];
	_F_	a4 += tt;
	_F_	a0 = z[ 6] + z[53];
	_F_	d[24] = tt + a0;

	_F_	a7 += a5;
	_F_	a5 += a6;
	_F_	a6 += a4;
	_F_	a3 += a2;
	_F_	a7 += a6;

	_F_	X2(0, 1, 2, 3, 4, 5, 6, 7)
	_F_	X4(0, 1, 2, 3, 4, 5, 6, 7)
	_F_	XA8(0, 1, 2, 3, 4, 5, 6, 7)

	_F_	d += 16; STR8(d, a0, a1, a2, a3, a4, a5, a6, a7);
	_F_
}


static /*inline*/ void dctI_3(fixed16 *d)
{
	ALL8;

	_F_	d += 24;
		LDR8(d, a0, a1, a2, a3, a4, a5, a6, a7);

	_F_	a7 += a3;
	_F_	a3 += a5;
	_F_	a5 += a1;
	_F_	a1 += a6;
	_F_	a6 += a2;
	_F_	a2 += a4;
	_F_	a4 += a0;
	_F_	a7 += a5;
	_F_	a5 += a6;
	_F_	a6 += a4;
	_F_	a3 += a2;
	_F_	a7 += a6;

	_F_	X2(0, 1, 2, 3, 4, 5, 6, 7)
	_F_	X4(0, 1, 2, 3, 4, 5, 6, 7)
	_F_	XB8(0, 1, 2, 3, 4, 5, 6, 7)

	_F_	STR8(d, a0, a1, a2, a3, a4, a5, a6, a7);
	_F_
}



#define bfII(off, c0, c1, c2, c3) do { \
	ALL8; \
	a0 = d[     off]; \
	a1 = d[ 1 + off]; \
	a2 = d[14 - off]; \
	a3 = d[15 - off]; \
	a4 = d[16 + off]; \
	a5 = d[17 + off]; \
	a6 = d[30 - off]; \
	a7 = d[31 - off]; \
	_F_	A16(0, 3) \
	_F_	A16(1, 2) \
	_F_	B16(4, 7, c0, c3) \
	_F_	B16(5, 6, c1, c2) \
	_F_	A32(0, 7) \
	_F_	A32(1, 6) \
	_F_	A32(2, 5) \
	_F_	A32(3, 4) \
	d[     off] = a0; \
	d[ 1 + off] = a1; \
	d[14 - off] = a2; \
	d[15 - off] = a3; \
	d[16 + off] = a4; \
	d[17 + off] = a5; \
	d[30 - off] = a6; \
	d[31 - off] = a7; \
} while (0)

#define bfII_S(off, c0, c1, c2, c3) do { \
	ALL8; \
	a0 = d[     off]; \
	a1 = d[ 1 + off]; \
	a2 = d[14 - off]; \
	a3 = d[15 - off]; \
	a4 = d[16 + off]; \
	a5 = d[17 + off]; \
	a6 = d[30 - off]; \
	a7 = d[31 - off]; \
	_F_	A16(0, 3) \
	_F_	A16(1, 2) \
	_F_	B16(4, 7, c0, c3) \
	_F_	B16(5, 6, c1, c2) \
	_F_	A32_S(0, 7) \
	_F_	A32_S(1, 6) \
	_F_	A32_S(2, 5) \
	_F_	A32_S(3, 4) \
	d[     off] = a0; \
	d[ 1 + off] = a1; \
	d[14 - off] = a2; \
	d[15 - off] = a3; \
	d[16 + off] = a4; \
	d[17 + off] = a5; \
	d[30 - off] = a6; \
	d[31 - off] = a7; \
} while (0)

#define bfII_S3(off, c0, c1, c2, c3) do { \
	ALL8; \
	a0 = d[     off]; \
	a1 = d[ 1 + off]; \
	a2 = d[14 - off]; \
	a3 = d[15 - off]; \
	a4 = d[16 + off]; \
	a5 = d[17 + off]; \
	a6 = d[30 - off]; \
	a7 = d[31 - off]; \
	_F_	A16(0, 3) \
	_F_	A16(1, 2) \
	_F_	B16(4, 7, c0, c3) \
	_F_	B16(5, 6, c1, c2) \
	_F_	A32_S(0, 7) \
	_F_	A32_S(1, 6) \
	_F_	A32_S(2, 5) \
	_F_	A32(3, 4) \
	d[     off] = a0; \
	d[ 1 + off] = a1; \
	d[14 - off] = a2; \
	d[15 - off] = a3; \
	d[16 + off] = a4; \
	d[17 + off] = a5; \
	d[30 - off] = a6; \
	d[31 - off] = a7; \
} while (0)



static /*inline*/ void dctII(fixed16 *d)
{
	bfII(0,  1,  3, 29, 31);
	bfII(2,  5,  7, 25, 27);
	bfII(4,  9, 11, 21, 23);
	bfII(6, 13, 15, 17, 19);

/*
	_F_	A16( 0, 15)
	_F_	A16( 1, 14)
	_F_	B16(16, 31, 1, 31)
	_F_	B16(17, 30, 3, 29)
	_F_	A32( 0, 31)
	_F_	A32( 1, 30)
	_F_	A32(14, 17)
	_F_	A32(15, 16)

	_F_	A16( 2, 13)
	_F_	A16( 3, 12)
	_F_	B16(18, 29, 5, 27)
	_F_	B16(19, 28, 7, 25)
	_F_	A32( 2, 29)
	_F_	A32( 3, 28)
	_F_	A32(12, 19)
	_F_	A32(13, 18)

	_F_	A16( 4, 11)
	_F_	A16( 5, 10)
	_F_	B16(20, 27, 9, 23)
	_F_	B16(21, 26, 11, 21)
	_F_	A32( 4, 27)
	_F_	A32( 5, 26)
	_F_	A32(10, 21)
	_F_	A32(11, 20)

	_F_	A16( 6,  9)
	_F_	A16( 7,  8)
	_F_	B16(22, 25, 13, 19)
	_F_	B16(23, 24, 15, 17)
	_F_	A32( 6, 25)
	_F_	A32( 7, 24)
	_F_	A32( 8, 23)
	_F_	A32( 9, 22)
	_F_
*/
}


static /*inline*/ void dctII_S(fixed16 *d)
{
	bfII_S3(0, 1,  3, 29, 31);
	bfII_S(2,  5,  7, 25, 27);
	bfII_S(4,  9, 11, 21, 23);
	bfII_S(6, 13, 15, 17, 19);

/*
	_F_	A16( 0, 15)
	_F_	A16( 1, 14)
	_F_	B16(16, 31, 1, 31)
	_F_	B16(17, 30, 3, 29)
	_F_	A32_S( 0, 31)
	_F_	A32_S( 1, 30)
	_F_	A32_S(14, 17)
	_F_	A32(15, 16)

	_F_	A16( 2, 13)
	_F_	A16( 3, 12)
	_F_	B16(18, 29, 5, 27)
	_F_	B16(19, 28, 7, 25)
	_F_	A32_S( 2, 29)
	_F_	A32_S( 3, 28)
	_F_	A32_S(12, 19)
	_F_	A32_S(13, 18)

	_F_	A16( 4, 11)
	_F_	A16( 5, 10)
	_F_	B16(20, 27, 9, 23)
	_F_	B16(21, 26, 11, 21)
	_F_	A32_S( 4, 27)
	_F_	A32_S( 5, 26)
	_F_	A32_S(10, 21)
	_F_	A32_S(11, 20)

	_F_	A16( 6,  9)
	_F_	A16( 7,  8)
	_F_	B16(22, 25, 13, 19)
	_F_	B16(23, 24, 15, 17)
	_F_	A32_S( 6, 25)
	_F_	A32_S( 7, 24)
	_F_	A32_S( 8, 23)
	_F_	A32_S( 9, 22)
	_F_
*/
}


static void dctI(const fixed16 *z, fixed16 *d)
{
	dctI_0(z, d);
	dctI_1(z, d);
	dctI_2(z, d);
	dctI_3(d);
}


static void dct(const fixed16 *z, fixed16 *d)
{
	dctI(z, d);
	dctII(d);
//{ int i; for (i = 0; i < 32; i++) d[i] *= 4; }
}


static void dct_S(const fixed16 *z, fixed16 *d)
{
	dctI(z, d);
	dctII_S(d);
//{ int i; for (i = 0; i < 32; i++) d[i] *= 4; }
}

// Test entry points with different names to minimize namespace pollution. 

void pem_dct(const fixed16 *z, fixed16 *d) { dct(z,d); }
void pem_dct_S(const fixed16 *z, fixed16 *d) { dct_S(z,d); }


#ifdef PEM_ARM_ASM
#define GET2h(x, a, b) do { \
	__asm__ __volatile__( \
		"\tldr	%1, [%0], #4\n\t" \
		"\tmov	%2, %1, asl #16\n\t" \
		"\tmov	%1, %1, asr #16\n\t" \
		"\tmov	%2, %2, asr #16\n\t" \
		: "+r"(x), "=r"(a), "=r"(b) \
	); \
} while(0)
#else
#ifdef BIGENDIAN
#define GET2h(x, a, b) do { \
if(1) { \
	a = *((int *)x)++; b = a >> 16; a = (a << 16) >> 16; \
} else { \
	a = *x++; b = *x++; \
}\
} while(0)
#else
#define GET2h(x, a, b) do { \
if(1) { \
	b = *((int *)x)++; a = b >> 16; b = (b << 16) >> 16; \
} else { \
	a = *x++; b = *x++; \
}\
} while(0)
#endif
#endif

static void windowAB(const short *XX, fixed16 *za, fixed16 *zb)
{
	int h, i;
	//const fixed40 *ww;
	const short *ww;
	const short *xx;

	ww = FASTVAR(wwindow);
	xx = XX;
	for (i = 4; i > 0; i--) {
		for (h = 8; h > 0; h--) {
		#ifdef NO__arm__
			register int w __asm__("r3");
			register int xa __asm__("r4");
			register int xb __asm__("r5");
			register int xA0a __asm__("r6");
			register int xB0a __asm__("r7");
			register int xA1a __asm__("r8");
			register int xB1a __asm__("r9");
			register int xA0b __asm__("r10");
			register int xB0b __asm__("r11");
			register int xA1b __asm__("r12");
			register int xB1b __asm__("r13");
		#else
			int w;
			int xa;
			int xb;
			int xA0a;
			int xB0a;
			int xA1a;
			int xB1a;
			int xA0b;
			int xB0b;
			int xA1b;
			int xB1b;
		#endif
/*
	(pr, jan 15 2002)
	Summing over i=0..7, x[AB][01][ab] are computed as follows:

	xA0a = xa[2i]   * w[2i]		xA0b = xb[2i]   * w[2i]
	xB0a = xa[2i+1] * w[2i]		xB0b = xb[2i+1] * w[2i]
	xA1a = xa[2i+1] * w[2i+1]	xA1b = xb[2i+1] * w[2i+1]
	xB1a = xa[2i+2] * w[2i+1]   xB1b = xb[2i+2] * w[2i+1]

	where xa and xb are the two channels of a stereo pcm sample

        (dd, 03 feb 2002)
        a,b --> left / right stereo sample
        A,B --> 0-63 / 64-128 of za/zb buffer
        0,1 --> bit flipped by window coeff increment
 
*/
			_F_	GET2h(xx, xa, xb);
			_F_	w = *ww++;
			_F_	xA0a  = xa * w; xA0b  = xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB0a  = xa * w; xB0b  = xb * w;
			_F_	w = *ww++;
			_F_	xA1a  = xa * w; xA1b  = xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB1a  = xa * w; xB1b  = xb * w;
			_F_	w = *ww++;
			_F_	xA0a += xa * w; xA0b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB0a += xa * w; xB0b += xb * w;
			_F_	w = *ww++;
			_F_	xA1a += xa * w; xA1b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB1a += xa * w; xB1b += xb * w;
			_F_	w = *ww++;
			_F_	xA0a += xa * w; xA0b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB0a += xa * w; xB0b += xb * w;
			_F_	w = *ww++;
			_F_	xA1a += xa * w; xA1b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB1a += xa * w; xB1b += xb * w;
			_F_	w = *ww++;
			_F_	xA0a += xa * w; xA0b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB0a += xa * w; xB0b += xb * w;
			_F_	w = *ww++;
			_F_	xA1a += xa * w; xA1b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB1a += xa * w; xB1b += xb * w;
			_F_	w = *ww++;
			_F_	xA0a += xa * w; xA0b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB0a += xa * w; xB0b += xb * w;
			_F_	w = *ww++;
			_F_	xA1a += xa * w; xA1b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB1a += xa * w; xB1b += xb * w;
			_F_	w = *ww++;
			_F_	xA0a += xa * w; xA0b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB0a += xa * w; xB0b += xb * w;
			_F_	w = *ww++;
			_F_	xA1a += xa * w; xA1b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB1a += xa * w; xB1b += xb * w;
			_F_	w = *ww++;
			_F_	xA0a += xa * w; xA0b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB0a += xa * w; xB0b += xb * w;
			_F_	w = *ww++;
			_F_	xA1a += xa * w; xA1b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB1a += xa * w; xB1b += xb * w;
			_F_	w = *ww++;
			_F_	xA0a += xa * w; xA0b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB0a += xa * w; xB0b += xb * w;
			_F_	w = *ww++;
			_F_	xA1a += xa * w; xA1b += xb * w;
			_F_	GET2h(xx, xa, xb);
			_F_	xB1a += xa * w; xB1b += xb * w;

			_F_	xx += 16*2;

			_F_	za[ 0] = SHIFT((xA0a + xA0b), 1);
			_F_	za[64] = SHIFT((xB0a + xB0b), 1);
			_F_	za[ 4] = SHIFT((xA1a + xA1b), 1);
			_F_	za[68] = SHIFT((xB1a + xB1b), 1);
			_F_	zb[ 0] = SHIFT((xA0b - xA0a), 1);
			_F_	zb[64] = SHIFT((xB0b - xB0a), 1);
			_F_	zb[ 4] = SHIFT((xA1b - xA1a), 1);
			_F_	zb[68] = SHIFT((xB1b - xB1a), 1);

			_F_	za += 8;
			_F_	zb += 8;
			_F_
		}
		za -= 63;
		zb -= 63;
	}
}


void polyphase(const short *x, fixed16 *sa, fixed16 *sb)
{
	fixed16 za[128], zb[128];
	int i, j;

#ifdef PROF
static profile_data p1, p2, p3, p4;
profile_start(&p1);
#endif

	{
		int *p;
		const int *q;

/*
	(dd, 03 feb 2002)
	why are there 15 ints of zeros in the polybuf?
        [.15,*18] X 32 ints --> 2112 
	these 15 ints get filled (by shifting in old values) at the
	end of this function call
*/
		p = (int *)(&FASTVAR(polybuf)[15*2]);
		q = (const int *)x;

		for (j = 0; j < 32; j++) {
			*p++ = q[32*0];
			*p++ = q[32*1];
			*p++ = q[32*2];
			*p++ = q[32*3];
			*p++ = q[32*4];
			*p++ = q[32*5];
			*p++ = q[32*6];
			*p++ = q[32*7];
			*p++ = q[32*8];
			*p++ = q[32*9];
			*p++ = q[32*10];
			*p++ = q[32*11];
			*p++ = q[32*12];
			*p++ = q[32*13];
			*p++ = q[32*14];
			*p++ = q[32*15];
			*p++ = q[32*16];
			*p++ = q[32*17];
			p += 15;
			q++;
		}
	}

#ifdef PROF
profile_end(&p1, "copy in ");
#endif

	for (i = 0; i < 18; i += 2) {

#ifdef PROF
profile_start(&p2);
#endif
/*
	(dd, 03 feb 2002)
	windowAB applies the same window coefficients
	to za(0-63) and za(64-128)

	note that za <-- mid channel, zb <-- side channel
	i think

	also note that windowAB fills short arrays za[128], zb[128]
        9 times, we have (2 bytes)(256 za+zb)(9 times) = 4608 bytes
	naively meaning that we haven't "lost information" yet... 
*/
		windowAB(&FASTVAR(polybuf)[2*i], za, zb);

#ifdef PROF
profile_end(&p2, "window  ");
profile_start(&p3);
#endif
		dct(za, sa);

#ifdef PROF
profile_end(&p3, "dct     ");
profile_start(&p3);
#endif
		dct(za+64, sa+32);

#ifdef PROF
profile_end(&p3, "dct     ");
profile_start(&p3);
#endif

		dct_S(zb, sb);

#ifdef PROF
profile_end(&p3, "dct_S   ");
profile_start(&p3);
#endif

		dct_S(zb+64, sb+32);

#ifdef PROF
profile_end(&p3, "dct_S   ");
#endif

		sa += 64;
		sb += 64;
	}


#ifdef PROF
profile_start(&p4);
#endif

	{
		int *p;

/*
	(dd, 03 feb 2002)
	why is this polybuf shift here?
	it fills in the blank 15 ints that are left when 
	polybuf is populated at the top of this function...
	so these samples are (re)used during the second call
	to polyphase...
*/
/*
	(per, 2002 Apr 17)
	I think we can profitably employ multiple load/stores here. gcc doesn't
	seem to know how to do this. 
	See, eg, ARM^2 9.4.1.
 */
		p = (int *)FASTVAR(polybuf);
		for (j = 0; j < 32; j++) {
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			*p = p[18]; p++;
			p += 18;
		}
	}

#ifdef PROF
profile_end(&p4, "copy out");
#endif
}

