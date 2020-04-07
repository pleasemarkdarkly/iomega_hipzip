/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: arm7 and later wide math functions

 ********************************************************************/

#ifdef _ARM_ASSEM_
#ifndef _V_WIDE_MATH
#define _V_WIDE_MATH

static inline ogg_int32_t MULT32(ogg_int32_t x, ogg_int32_t y) {
  int lo,hi;
  asm volatile("smull  %0,%1,%2,%3;\n"
               : "=&r"(lo),"=&r"(hi)
               : "%r"(x),"r"(y));
  return(hi);
}

static inline ogg_int32_t MULT31(ogg_int32_t x, ogg_int32_t y) {
  return MULT32(x,y)<<1;
}

static inline ogg_int32_t MULT30(ogg_int32_t x, ogg_int32_t y) {
  return MULT32(x,y)<<2;
}

static inline ogg_int32_t MULT31_SHIFT15(ogg_int32_t x, ogg_int32_t y) {
  int lo,hi;
  asm volatile("smull  %0,%1,%2,%3;\n"
	       "mov    %0,%0, lsr #15;\n"
	       "orr    %1,%0,%1, lsl #17;\n"
               : "=&r"(lo),"=&r"(hi)
               : "%r"(x),"r"(y));
  return(hi);
}

static inline ogg_int32_t CLIP_TO_15(ogg_int32_t x) {
  asm volatile("adds   r0,%0,#32768;\n"
	       "movmi  %0,#0x8000;\n"
	       "subs   r0,%0,#32768;\n"
	       "movpl  %0,#0x7f00;\n"
	       "orrpl  %0,%0,#0xff;\n"
	       : "+r"(x)
	       :
	       : "r0","cc");
  return(x);
}

#endif
#endif
