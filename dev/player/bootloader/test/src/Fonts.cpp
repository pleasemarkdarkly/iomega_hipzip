//........................................................................................
//........................................................................................
//.. File Name: Fonts.cpp															..
//.. Date: 09/24/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: declarations of fonts used in the player					..
//.. Usage: These fonts are included in screens throughout the player					..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/24/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include "Fonts.h"

static short Latin_Font_9_offset_table[256] =
{
0, 0, 0, 0, 0, 0, 0, 0,	// 0 - 7
0, 0, 0, 0, 0, 0, 0, 0,	// 8 - 15
0, 0, 0, 0, 0, 0, 0, 0,	// 16 - 23
0, 0, 0, 0, 0, 0, 0, 0,	// 24 - 31
0,	// space // 5 5-0
5,	// ! // 2 7-5
7,	// " // 4 11-7
11,	// # // 6 17-11
17,	// $ // 4 21-17
21,	// %
28,	// &
34,	// '
36,	// (
40,	// )
44,	// *
48,	// +
54,	// ,
57,	// -
62,	// .
64,	// /
69,	// 0
74,	// 1
79,	// 2
84,	// 3
89,	// 4
94,	// 5
99,	// 6
104,	// 7
109,	// 8
114,	// 9
119,	// :
121,	// ;
124,	// <
128,	// =
133,	// >
137,	// ?
143,	// @
151,	// A
157,	// B
163,	// C
169,	// D
175,	// E
180,	// F
185,	// G
191,	// H
197,	// I
199,	// J
205,	// K
211,	// L
216,	// M
222,	// N
228,	// O
234,	// P
240,	// Q
246,	// R
252,	// S
258,	// T
264,	// U
270,	// V
276,	// W
284,	// X
290,	// Y
296,	// Z
302,	// [
305,	// backslash
311,	// ]
314,	// ^
320,	// _
326,	// `
329,	// a
334,	// b
339,	// c
344,	// d
349,	// e
354,	// f
358,	// g
363,	// h
368,	// i
370,	// j
373,	// k
378,	// l
381,	// m
387,	// n
392,	// o
397,	// p
402,	// q
407,	// r
411,	// s
416,	// t
420,	// u
425,	// v
430,	// w
436,	// x
442,	// y
447,	// z
453,	// {
457,	// |
459,	// }
463,	// ~
468,	// box (127)
474, 474, 474, 474, 474, 474, 474, 474,			// 128 - 135
474, 474, 474, 474, 474, 474, 474, 474, 474,	// 136 - 144
475,	// �
478,	// �
481, 481, 481, 481, 481,						// 147 - 151
481, 481, 481, 481, 481, 481, 481, 481,			// 152 - 159
481,	// no break space (160)
486,	// �
488,	// �
493,	// �
499,	// �
505,	// �
513,	// �
515,	// �
521,	// �
525,	// �
533,	// �
538,	// �
545,	// �
550,	// �
555,	// �
563,	// �
568,	// �
572,	// �
578,	// �
582,	// �
585,	// �
588,	// �
594,	// �
600,	// �
602,	// �
605,	// �
608,	// �
612,	// �
619,	// �
628,	// �
637,	// �
646,	// �
652,	// �
658,	// �
664,	// �
670,	// �
676,	// �
682,	// �
688,	// �
698,	// �
704,	// �
709,	// �
714,	// �
719,	// �
724,	// �
727,	// �
730,	// �
734,	// �
738,	// �
745,	// �
751,	// �
757,	// �
763,	// �
769,	// �
775,	// �
781,	// �
787,	// �
793,	// �
799,	// �
805,	// �
811,	// �
817,	// �
823,	// �
828,	// �
833,	// �
838,	// �
843,	// �
848,	// �
853,	// �
858,	// �
863,	// �
871,	// �
876,	// �
881,	// �
886,	// �
891,	// �
896,	// �
899,	// �
902,	// �
906,	// �
910,	// �
916,	// �
921,	// �
926,	// �
931,	// �
936,	// �
941,	// �
946,	// �
952,	// �
958,	// �
963,	// �
968,	// �
973,	// �
978,	// �
983,	// �
988		// �
};


static unsigned char Latin_Font_9_data_table[1125] = {
0x05,0x4a,0x27,0x24,0x22,0x80,0x00,0x00,0x13,0x08,0xc6,0x0b,0xcc,0xf3,0x18,0x00,
0x00,0x38,0x78,0x47,0x8f,0x79,0xef,0x3d,0x14,0x14,0x50,0x8a,0x27,0x3c,0x73,0xc7,
0xbe,0x8a,0x28,0x28,0xa2,0xfb,0x21,0x88,0x02,0x02,0x00,0x10,0x18,0x10,0x94,0x20,
0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x01,0x50,0x0f,0x93,0x00,0x01,0x80,0x41,
0x4e,0x51,0xc7,0x00,0x00,0x07,0x1e,0x42,0x13,0x00,0x1e,0x02,0x40,0x08,0x44,0x26,
0x10,0x02,0x08,0x20,0x82,0x08,0x0f,0x9e,0xf7,0xbd,0xe8,0x91,0x1e,0x44,0xe3,0x8e,
0x38,0xe0,0x0e,0x45,0x14,0x51,0x45,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x89,0x11,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x5f,0x75,
0x4e,0x24,0x4a,0x20,0x00,0x24,0x99,0x29,0x1a,0x10,0x14,0xa4,0x02,0x04,0x44,0x84,
0xa4,0x50,0x45,0x08,0x41,0x14,0x14,0x90,0xda,0x28,0xa2,0x8a,0x28,0x08,0x8a,0x28,
0x28,0xa2,0x0a,0x10,0x94,0x01,0x02,0x00,0x10,0x20,0x10,0x04,0x20,0x00,0x00,0x00,
0x00,0x40,0x00,0x00,0x00,0x02,0x48,0x08,0x99,0x00,0x22,0x40,0x22,0x51,0x02,0x20,
0x80,0x00,0x08,0x80,0xa2,0x29,0x29,0x3e,0x06,0xa0,0x18,0x8c,0x42,0x20,0x85,0x14,
0x51,0x45,0x14,0x14,0x20,0x84,0x21,0x08,0x91,0x11,0x45,0x14,0x51,0x45,0x14,0x51,
0x45,0x14,0x51,0x45,0xc9,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x0a,0x47,0x88,0x28,0x24,
0x20,0x00,0x24,0x88,0x21,0x2b,0x9c,0x14,0xa5,0x04,0xf2,0x05,0x35,0x14,0x50,0x45,
0x08,0x41,0x14,0x15,0x10,0xab,0x28,0xa2,0x8a,0x28,0x08,0x8a,0x28,0x25,0x22,0x12,
0x10,0xa2,0x00,0x73,0x8e,0x73,0x31,0xdc,0x94,0xa7,0x9c,0x67,0x1d,0x4e,0xe9,0x4a,
0xa8,0xa5,0xf2,0x48,0xa8,0x80,0x02,0x72,0x11,0x22,0x50,0x04,0x97,0x89,0x7b,0xd6,
0x40,0x4f,0x8b,0x49,0x3e,0x02,0xa9,0x08,0x84,0x46,0x20,0x08,0xa2,0x8a,0x28,0xa2,
0x14,0x20,0x84,0x21,0x08,0x91,0x11,0x65,0x14,0x51,0x45,0x12,0x91,0x45,0x14,0x51,
0x45,0x29,0x73,0x9c,0xe7,0x39,0xd8,0xe6,0x31,0x8c,0x89,0x11,0x4e,0x31,0x8c,0x63,
0x08,0x72,0x52,0x94,0xa5,0x09,0x00,0x04,0x0a,0x71,0x05,0x88,0x2a,0xf8,0x78,0x44,
0x88,0x42,0x48,0x52,0x23,0x24,0x08,0x01,0x09,0x55,0x17,0x90,0x45,0xce,0x4d,0xf4,
0x16,0x10,0x8a,0xa8,0xa2,0x8a,0x27,0x08,0x89,0x45,0x42,0x14,0x22,0x08,0x80,0x00,
0x0a,0x50,0x94,0xa2,0x52,0x95,0x25,0x52,0x94,0xa5,0x90,0x49,0x4a,0xa5,0x24,0x24,
0x45,0x48,0x80,0x00,0x87,0x8e,0x7f,0x0e,0x05,0x54,0x92,0x08,0x15,0x40,0x02,0x11,
0x09,0x3e,0x82,0x44,0x89,0x04,0x82,0x40,0x88,0xa2,0x8a,0x28,0xa2,0x27,0x20,0xe7,
0x39,0xc8,0x91,0x39,0x55,0x14,0x51,0x45,0x11,0x11,0x45,0x14,0x51,0x29,0x2e,0x08,
0x42,0x10,0x84,0x25,0x09,0x4a,0x52,0x89,0x10,0x29,0x4a,0x52,0x94,0x80,0x9a,0x52,
0x94,0xa5,0xc9,0x00,0x04,0x1f,0x12,0xe9,0x08,0x20,0x20,0x00,0x44,0x88,0x81,0x78,
0x52,0x24,0x9c,0x24,0xf2,0x11,0x39,0xf4,0x50,0x45,0x08,0x45,0x14,0x15,0x10,0x8a,
0x68,0xbc,0xab,0xc0,0x88,0x89,0x45,0x45,0x08,0x42,0x08,0x80,0x00,0x7a,0x50,0x97,
0xa2,0x52,0x96,0x25,0x52,0x94,0xa5,0x0c,0x49,0x4a,0xa2,0x24,0x42,0x48,0x08,0x80,
0x02,0x82,0x0a,0x14,0x0a,0x05,0x13,0xa4,0x08,0x16,0x40,0x02,0x3b,0x09,0x1e,0x22,
0x02,0x4a,0x45,0x26,0x91,0x0f,0xbe,0xfb,0xef,0xbe,0x7c,0x20,0x84,0x21,0x08,0x91,
0x11,0x4d,0x14,0x51,0x45,0x12,0x91,0x45,0x14,0x51,0x11,0x29,0x7b,0xde,0xf7,0xbd,
0xfd,0x0f,0x7b,0xde,0x89,0x11,0xe9,0x4a,0x52,0x94,0xbe,0xaa,0x52,0x94,0xa5,0x29,0x00,
0x00,0x0a,0x74,0xa9,0x04,0x40,0x20,0x00,0x84,0x89,0x09,0x0a,0x52,0x44,0x85,0x02,
0x04,0x00,0x81,0x14,0x50,0x45,0x08,0x45,0x15,0x14,0x90,0x8a,0x28,0xa0,0x92,0x20,
0x88,0x89,0x45,0x48,0x88,0x82,0x04,0x80,0x00,0x4a,0x50,0x94,0x22,0x52,0x95,0x25,
0x52,0x94,0xa5,0x02,0x49,0x52,0xa5,0x24,0x82,0x48,0x08,0x80,0x02,0x82,0x0e,0x7f,
0x0e,0x05,0x50,0x12,0x00,0x15,0x40,0x00,0x00,0x0f,0x06,0x10,0xe4,0x82,0xc1,0x50,
0xb2,0x08,0xa2,0x8a,0x28,0xa2,0x44,0x1e,0x84,0x21,0x08,0x91,0x11,0x45,0x14,0x51,
0x45,0x14,0x51,0x45,0x14,0x51,0x11,0x29,0x4a,0x52,0x94,0xa5,0x20,0xe8,0x42,0x10,
0x89,0x12,0x29,0x4a,0x52,0x94,0x80,0xca,0x52,0x94,0xa5,0x29,0x00,0x04,0x00,0x20,
0xee,0x02,0x80,0x03,0x02,0x83,0x09,0xe6,0x09,0x8c,0x43,0x18,0x60,0x00,0x10,0x79,
0x17,0x8f,0x79,0xe8,0x39,0x14,0xe4,0x5e,0x8a,0x27,0x20,0x6a,0x2f,0x08,0x70,0x82,
0x88,0x88,0xfb,0x05,0x80,0x00,0x3b,0x8e,0x73,0xa1,0xd2,0x94,0x95,0x52,0x67,0x1d,
0x1c,0x27,0x21,0x48,0x9d,0xf1,0x50,0x0f,0x80,0x02,0x77,0xd1,0x08,0x41,0x04,0x97,
0x89,0x00,0x15,0x40,0x0f,0x80,0x08,0x86,0x20,0x09,0x05,0x42,0x11,0x52,0x28,0xa2,
0x8a,0x28,0xa2,0x87,0x88,0xf7,0xbd,0xe8,0x91,0x1e,0x44,0xe3,0x8e,0x38,0xe0,0x0e,
0x38,0xe3,0x8e,0x11,0xc9,0x39,0xce,0x73,0x9c,0xdc,0x47,0x39,0xce,0x89,0x11,0xc9,
0x31,0x8c,0x63,0x08,0x71,0xce,0x73,0x9d,0xc7,0x00,0x00,0x00,0x00,0x04,0x00,0x00,
0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0xf8,0x00,0x00,0x00,0x00,0x40,0x10,0x00,0x00,0x04,0x04,0x00,0x00,0x00,
0x00,0x04,0x00,0x00,0x00,0x00,0x02,0x20,0x00,0x08,0x51,0x02,0x20,0x00,0x00,0x08,
0x80,0x00,0x00,0x08,0x06,0x10,0x00,0x09,0xe4,0x22,0x79,0xc0,0x00,0x00,0x00,0x00,
0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x01,0x0e,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x05,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x01,0x80,0x20,0x00,0x00,0x04,0x04,0x00,0x00,0x00,0x00,0x18,0x00,
0x00,0x00,0x00,0x02,0x00,0x00,0x08,0x4e,0x01,0xc0,0x00,0x00,0x07,0x00,0x00,0x00,
0x00,0x06,0x00,0x00,0x08,0x44,0x72,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,
0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x18,0x06,0x00,};

CBmpFont Latin_Font_9(9, Latin_Font_9_data_table,Latin_Font_9_offset_table,0,125);
