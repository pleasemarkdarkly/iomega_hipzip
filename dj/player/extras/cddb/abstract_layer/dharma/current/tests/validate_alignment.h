/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * validate_alignment.h - Structure and type definitions for data alignment test
 */

#ifndef _VALIDATE_ALIGNMENT_H_
#define _VALIDATE_ALIGNMENT_H_

#include <extras/cddb/gn_defines.h>

typedef gn_uint16_t	ui16_t;
typedef gn_uchar_t	ui8_t;
typedef gn_uint32_t	ui32_t;
typedef gn_int16_t	i16_t;
typedef gn_int32_t	foffset_t;
typedef gn_int32_t	i32_t;

#define	DEF_STRU6_1	200
#define DEF_STRU7_1	32
#define DEF_STRU9_1	15
#define DEF_STRU13_1	16
#define DEF_STRU14_1	3

void print_alignment_header();
void validate_alignment();
void validate_sizes();
void validate_endian();


typedef struct struct1
{
	ui16_t	e1;
	ui16_t	e2[1];
}stru1;

typedef struct struct2
{
	ui8_t	e1[1];
}stru2;

typedef struct struct3
{
	ui16_t	e1;
	ui16_t	e2;
	union info {
		ui32_t	e3[1];
		stru1	e4;
		stru2	e5;
	} info;
}stru3;

typedef struct struct4
{
	gn_uint32_t	e1;
	gn_uint32_t	e2;
	gn_uint32_t	e3;
	gn_uint32_t	e4;
	gn_uint32_t	e5;
	gn_uint32_t	e6;
	gn_uint16_t	e7;
	gn_uint16_t	e8;
	stru3		e9;
}stru4;

typedef struct struct5
{
	gn_uint32_t	e1;
	gn_uint32_t	e2;
	gn_uint32_t	e3;
}stru5;

typedef struct struct6
{
	gn_uint16_t	e1;
	gn_uint16_t	e2;
	gn_uint16_t	e3[DEF_STRU6_1];

}stru6;

typedef struct struct7
{
	gn_uint16_t	e1;
	gn_char_t	e2[DEF_STRU7_1];
	gn_char_t	e3[DEF_STRU7_1];
	gn_uint32_t	e4;
	gn_uint32_t	e5;
	gn_uint16_t	e6;
	stru6		e7;
}stru7;

typedef struct struct8
{
	gn_uint32_t	e1;
	gn_uint16_t	e2;
	gn_uint16_t	e3;
	gn_uint32_t	e4;
	stru7		e5;
}stru8;

typedef struct struct9
{
	gn_uint16_t	e1;
	gn_uint16_t	e2;
	gn_uint32_t	e3;
	gn_uint32_t	e4;
	gn_char_t	e5[DEF_STRU9_1 + 1];
}stru9;

typedef struct struct10
{
	gn_uint32_t	e1;
	gn_uint32_t	e2;
	gn_uint16_t	e3;
	gn_uint16_t	e4;
	stru9		e5[1];
}stru10;

typedef struct struct11
{
	gn_uint16_t	e1;
	gn_uint16_t	e2;
	gn_uint32_t	e3;
	gn_uint32_t	e4;
	gn_uint16_t	e5;
	gn_uint16_t	e6;
	gn_uint32_t	e7;
}stru11;

typedef struct struct12
{
	gn_uint16_t	e1;
	gn_uint16_t	e2;
	gn_uint32_t	e3;
	gn_uint32_t	e4;
	gn_uint32_t	e5;
	gn_uint32_t	e6;
	gn_uint32_t	e7;
	gn_uint32_t	e8;
}stru12;

typedef struct struct14
{
	i16_t	e1;
	i16_t	e2[DEF_STRU14_1];
}stru14;

typedef struct struct13
{
	gn_uint32_t	e1;
	ui8_t		e2[DEF_STRU13_1];
	stru14		e3;
	gn_uint16_t	e4;
	gn_uint16_t	e5;
	stru3		e6;
}stru13;

typedef struct struct15
{
	ui32_t	e1;
	ui32_t	e2;
}stru15;

typedef struct struct16
{
	ui32_t	e1;
	ui32_t	e2;
	ui32_t	e3;
	stru15	e4;
}stru16;

typedef struct struct17
{
	foffset_t	e1;
}stru17;

typedef struct struct18
{
	ui32_t	e1;
	ui32_t	e2;
	ui32_t	e3;
	stru17	e4[1];
}stru18;

typedef union struct19
{
	ui32_t	e1;
	stru16	e2;
	stru18	e3;
}stru19;

typedef struct struct20
{
	ui32_t		e1;
	foffset_t	e2;
}stru20;

typedef struct struct24
{
	ui32_t		e1;
	foffset_t	e2;
}stru24;

typedef struct struct21
{
	ui32_t		e1;
	ui32_t		e2;
	foffset_t	e3;
	stru24		e4[1];
}stru21;

typedef struct struct22
{
	gn_uint32_t	e1;
	gn_uint32_t	e2;
	foffset_t	e3;
}stru22;

typedef struct struct23
{
	char		e1[4];
	ui32_t		e2;
	ui32_t		e3;
	ui32_t		e4;
	ui32_t		e5;
	i32_t		e6;
	ui32_t		e7;
	ui32_t		e8;
	foffset_t	e9;
	foffset_t	e10;
	foffset_t	e11;
	stru22		e12;
}stru23;

typedef struct struct25
{
	gn_int16_t	e1;
	gn_uint32_t	e2;
	gn_size_t	e3;
	gn_char_t	e4[1];
}stru25;


#endif  /* _VALIDATE_ALIGNMENT_H_ */
