//........................................................................................
//........................................................................................
//.. File Name: FontData.c																..
//.. Date: 7/21/2000																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: Declartion of font.						 				..
//.. Usage: Used in bootloader because peg is not available.							..
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 9/5/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................


#include "gui/font/BmpFont.h"

short IomegaMediumFont_offset_table[128] =
{
0, 0, 0, 0, 0, 0, 0, 0,	// 0 - 7
0, 0, 0, 0, 0, 0, 0, 0,	// 8 - 15
0, 0, 0, 0, 0, 0, 0, 0,	// 16 - 23
0, 0, 0, 0, 0, 0, 0, 0,	// 24 - 31
463,	// space
325,	// "
329,	// !
331,	// #
337,	// $
341,	// %
348,	// &
354,	// ' 32 - 39
356,	// (
359,	// )
363,	// *
368,	// +
374,	// ,
378,	// -
383,	// .
385,	// /
320,	// 0
276,	// 1
280,	// 2
285,	// 3
290,	// 4
295,	// 5
300,	// 6
305,	// 7
310,	// 8
315,	// 9
390,	// :
392,	// ;
395,	// <
399,	// =
405,	// >
409,	// ?
414,	// @
0,	// A
6,	// B
12,	// C
18,	// D
24,	// E
29,	// F
34,	// G
40,	// H
46,	// I
48,	// J
54,	// K
60,	// L
65,	// M
71,	// N
77,	// O
83,	// P
89,	// Q
95,	// R
101,	// S
107,	// T
113,	// U
119,	// V
125,	// W
133,	// X
139,	// Y
145,	// Z
422,	// [
424,	// backslash
430,	// ]
433,	// ^
437,	// _
444,	// `
151,	// a
156,	// b
161,	// c
166,	// d
171,	// e
176,	// f
180,	// g
185,	// h
190,	// i
192,	// j
196,	// k
201,	// l
204,	// m
210,	// n
215,	// o
220,	// p
225,	// q
230,	// r
234,	// s
239,	// t
243,	// u
248,	// v
253,	// w
259,	// x
265,	// y
270,	// z
447,	// {
451,	// |
453,	// }
457,	// ~
0,
};

UChar IomegaMediumFont_width_table[128] =
{
0, 0, 0, 0, 0, 0, 0, 0,	// 0 - 7
0, 0, 0, 0, 0, 0, 0, 0,	// 8 - 15
0, 0, 0, 0, 0, 0, 0, 0,	// 16 - 23
0, 0, 0, 0, 0, 0, 0, 0,	// 24 - 31
4,	// space
4,	// "
2,	// !
6,	// #
5,	// $
7,	// %
6,	// &
2,	// ' 32 - 39
4,	// (
4,	// )
4,	// *
6,	// +
3,	// ,
5,	// -
2,	// .
5,	// /
5,	// 0
4,	// 1
5,	// 2
5,	// 3
5,	// 4
5,	// 5
5,	// 6
5,	// 7
5,	// 8
5,	// 9
2,	// :
3,	// ;
4,	// <
6,	// =
4,	// >
5,	// ?
8,	// @
6,	// A
6,	// B
6,	// C
6,	// D
5,	// E
5,	// F
6,	// G
6,	// H
2,	// I
6,	// J
6,	// K
5,	// L
6,	// M
6,	// N
6,	// O
6,	// P
6,	// Q
6,	// R
6,	// S
6,	// T
6,	// U
6,	// V
8,	// W
6,	// X
6,	// Y
6,	// Z
3,	// [
5,	// backslash
3,	// ]
4,	// ^
7,	// _
4,	// `
5,	// a
5,	// b
5,	// c
5,	// d
5,	// e
3,	// f
5,	// g
5,	// h
2,	// i
4,	// j
5,	// k
3,	// l
6,	// m
5,	// n
5,	// o
5,	// p
5,	// q
4,	// r
5,	// s
4,	// t
5,	// u
5,	// v
6,	// w
6,	// x
5,	// y
6,	// z
4,	// {
2,	// |
4,	// }
4,	// ~
0,
};

UChar IomegaMediumFont_data_table[531] = {
0x23,0xc7,0xbc,0xf7,0x9e,0x8a,0x0a,0x28,0x45,0x13,0x9e,0x39,0xe3,0xdf,0x45,0x14,
0x14,0x51,0x7c,0x08,0x00,0x40,0x60,0x42,0x28,0x40,0x00,0x00,0x00,0x00,0x80,0x00,
0x00,0x00,0x04,0x63,0x05,0xe6,0x79,0x8c,0x65,0x4a,0x27,0x24,0x22,0x80,0x00,0x00,
0x08,0x00,0x00,0x30,0xf3,0x43,0x20,0x08,0x54,0x00,0x00,0x52,0x28,0x22,0x84,0x20,
0x8a,0x0a,0x48,0x6d,0x14,0x51,0x45,0x14,0x04,0x45,0x14,0x14,0x51,0x04,0x08,0x00,
0x40,0x80,0x40,0x08,0x40,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x0c,0x94,0x8d,
0x08,0x0a,0x52,0x95,0x5f,0x75,0x4e,0x24,0x4a,0x20,0x00,0x10,0x04,0x04,0x49,0x0a,
0x21,0x50,0x04,0x92,0x00,0x00,0x8a,0x28,0x22,0x84,0x20,0x8a,0x0a,0x88,0x55,0x94,
0x51,0x45,0x14,0x04,0x45,0x14,0x12,0x91,0x09,0xce,0x39,0xcc,0xc7,0x72,0x29,0x4f,
0x38,0xce,0x3a,0x9d,0xd2,0x95,0x51,0x4b,0xe4,0x10,0x95,0xce,0x0a,0x52,0x95,0x4a,
0x47,0x88,0x28,0x24,0x20,0x00,0x12,0x09,0xf2,0x0a,0x6a,0x21,0x00,0x00,0x92,0x28,0x00,
0x8b,0xc8,0x22,0xe7,0x26,0xfa,0x0b,0x08,0x45,0x54,0x51,0x45,0x13,0x84,0x44,0xa2,
0xa1,0x0a,0x10,0x29,0x42,0x52,0x89,0x4a,0x2a,0x4a,0xa5,0x29,0x4b,0x20,0x92,0x95,
0x4a,0x48,0x44,0x21,0x24,0x29,0x11,0x92,0x90,0x4a,0x71,0x05,0x88,0x2a,0xf8,0x7c,
0x20,0x10,0x01,0x12,0xaa,0x11,0x00,0x01,0x11,0x50,0x00,0xfa,0x28,0x22,0x84,0x22,
0x8a,0x0a,0x88,0x45,0x34,0x5e,0x55,0xe0,0x44,0x44,0xa2,0xa2,0x84,0x21,0xe9,0x42,
0x5e,0x89,0x4a,0x2c,0x4a,0xa5,0x29,0x4a,0x18,0x92,0x95,0x44,0x48,0x84,0x40,0xbc,
0x29,0x12,0x4e,0x90,0x5f,0x12,0xe9,0x08,0x20,0x20,0x00,0x20,0x49,0xf2,0x22,0x72,
0x11,0x00,0x00,0x92,0x00,0x00,0x8a,0x28,0x22,0x84,0x22,0x8a,0x8a,0x48,0x45,0x14,
0x50,0x49,0x10,0x44,0x44,0xa2,0xa4,0x44,0x41,0x29,0x42,0x50,0x89,0x4a,0x2a,0x4a,
0xa5,0x29,0x4a,0x04,0x92,0xa5,0x4a,0x49,0x04,0x84,0x85,0x29,0x22,0x42,0x90,0x0a,
0x74,0xa9,0x04,0x40,0x20,0x00,0x42,0x04,0x04,0x01,0x02,0x09,0x00,0x00,0x92,0x00,0x00,
0x8b,0xc7,0xbc,0xf4,0x1c,0x8a,0x72,0x2f,0x45,0x13,0x90,0x35,0x17,0x84,0x38,0x41,
0x44,0x44,0x7c,0xee,0x39,0xce,0x87,0x4a,0x29,0x2a,0xa4,0xce,0x3a,0x38,0x4e,0x42,
0x91,0x3b,0xe4,0xf3,0x04,0xc6,0x21,0x8c,0x60,0x40,0x20,0xee,0x02,0x80,0x03,0x01,
0x40,0xc0,0x00,0x20,0xf3,0x0b,0x00,0x00,0x54,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x01,0x00,0x20,0x00,0x00,0x08,0x08,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x01,0x00,0x00,0x40,0x00,0x00,0x00,
0x00,0x07,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0xc0,0x00,
0x00,0x08,0x08,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,};

/*
typedef struct Font{
  UCHAR uType;
  UCHAR uAscent;
  UCHAR uDescent;
  UCHAR uHeight;
  WORD  wBytesPerLine;
  WORD  wFirstChar;
  WORD  wLastChar;
  WORD  *pOffsets;
  UCHAR *pWidths;
  UCHAR *pData;
} GumiFont;
*/

CBmpFont  IomegaMediumFont (9, IomegaMediumFont_data_table, IomegaMediumFont_offset_table, IomegaMediumFont_width_table, 59);
