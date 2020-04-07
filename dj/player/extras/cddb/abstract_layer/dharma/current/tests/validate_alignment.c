/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * validate_alignment.c - Routines to validate the size and alignment of data structures
 */


/* These routines will display the offset in bytes from the start of
 * data structures that are kept both in files and memory so we can
 * determine if a platform will have issues reading them directly from
 * a file.
 */

#include "validate_alignment.h"

#include <cyg/kernel/kapi.h>
#include <util/debug/debug.h>

/* prnt should take the same variable arg list as printf, but it is up to */
/* the developer to log the information sent to it in some way */
#define prnt	diag_printf

#define STACKSIZE (32 * 4096)

static cyg_handle_t thread;
static cyg_thread thread_obj;
static char stack[STACKSIZE];


static void hex_print(unsigned char *ptr, int size)
{
	int	i;

	for(i = 0; i < size; i++)
	{
		prnt("%.2x ", ptr[i]);
	}
}

/* Leave some room for comments we may add later when we save the output */
void print_alignment_header()
{
	prnt("# Date    : %s\n", __DATE__);
	prnt("# Time    : %s\n", __TIME__);
	prnt("# CPU     :\n");
	prnt("# OS      :\n");
	prnt("# Compiler:\n");
	prnt("# Flags   :\n");
	prnt("# Endian  :\n");
	prnt("# Other   :\n");
	prnt("#\n");
	prnt("#\n");
	prnt("#\n");
	prnt("\n");
}


/* see what kind of endian-ness this platform is */
void validate_endian()
{
	gn_uchar_t	endian8 = 0x12;
	gn_uint16_t	endian16 = 0x1234;
	gn_uint32_t	endian32 = 0x12345678;

	prnt("endian8  : ");
	hex_print((unsigned char *)&endian8, sizeof(endian8));
	prnt("\nendian16 : ");
	hex_print((unsigned char *)&endian16, sizeof(endian16));
	prnt("\nendian32 : ");
	hex_print((unsigned char *)&endian32, sizeof(endian32));
	prnt("\n");
}

/* Determine the size of basic variables, and the number of bits in each*/
void validate_sizes()
{
	int		i;
	unsigned char	bit_char;
	unsigned short	bit_short;
	unsigned int	bit_int;
	unsigned long	bit_long;

	prnt("sizeof(unsigned char) : %d\n", sizeof(unsigned char));
	prnt("sizeof(unsigned short): %d\n", sizeof(unsigned short));
	prnt("sizeof(unsigned int)  : %d\n", sizeof(unsigned int));
	prnt("sizeof(unsigned long) : %d\n", sizeof(unsigned long));
	prnt("sizeof(void *)        : %d\n", sizeof(void *));

	bit_char = ~0;
	i = 0;
	do
	{
		bit_char >>= 1;
		i++;
	}
	while(bit_char);
	prnt("Number of bits in unsigned char : %d\n", i);

	bit_short = ~0;
	i = 0;
	do
	{
		bit_short >>= 1;
		i++;
	}
	while(bit_short);
	prnt("Number of bits in unsigned short: %d\n", i);

	bit_int = ~0;
	i = 0;
	do
	{
		bit_int >>= 1;
		i++;
	}
	while(bit_int);
	prnt("Number of bits in unsigned int  : %d\n", i);

	bit_long = ~0;
	i = 0;
	do
	{
		bit_long >>= 1;
		i++;
	}
	while(bit_long);
	prnt("Number of bits in unsigned long : %d\n", i);

}

void validate_alignment_1()
{
	stru1		st1;
	stru2		st2;
	stru3		st3;
	unsigned	head;

	head = (unsigned)&st1;
	prnt("stru1  (sizeof = %3d): %3d %3d\n", sizeof(st1),
		(unsigned)&(st1.e1) - head,
		(unsigned)&(st1.e2) - head);

	head = (unsigned)&st2;
	prnt("stru2  (sizeof = %3d): %3d\n", sizeof(st2),
		(unsigned)&(st2.e1) - head);

	head = (unsigned)&st3;
	prnt("stru3  (sizeof = %3d): %3d %3d %3d %3d %3d\n", sizeof(st3),
		(unsigned)&(st3.e1) - head,
		(unsigned)&(st3.e2) - head,
		(unsigned)&(st3.info.e3) - head,
		(unsigned)&(st3.info.e4) - head,
		(unsigned)&(st3.info.e5) - head);
}


void validate_alignment_2()
{
	stru4			st1;
	stru7			st2;
	stru8			st3;
	stru5			st4;
	stru6			st5;
	unsigned			head;

	head = (unsigned)&st1;
	prnt("stru4  (sizeof = %3d): %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
		sizeof(st1),
		(unsigned)&(st1.e1) - head,
		(unsigned)&(st1.e2) - head,
		(unsigned)&(st1.e3) - head,
		(unsigned)&(st1.e4) - head,
		(unsigned)&(st1.e5) - head,
		(unsigned)&(st1.e6) - head,
		(unsigned)&(st1.e7) - head,
		(unsigned)&(st1.e8) - head,
		(unsigned)&(st1.e9) - head);

	head = (unsigned)&st2;
	prnt("stru7  (sizeof = %3d): %3d %3d %3d %3d %3d %3d %3d\n",
		sizeof(st2),
		(unsigned)&(st2.e1) - head,
		(unsigned)&(st2.e2) - head,
		(unsigned)&(st2.e3) - head,
		(unsigned)&(st2.e4) - head,
		(unsigned)&(st2.e5) - head,
		(unsigned)&(st2.e6) - head,
		(unsigned)&(st2.e7) - head);

	head = (unsigned)&st3;
	prnt("stru8  (sizeof = %3d): %3d %3d %3d %3d %3d\n", sizeof(st3),
		(unsigned)&(st3.e1) - head,
		(unsigned)&(st3.e2) - head,
		(unsigned)&(st3.e3) - head,
		(unsigned)&(st3.e4) - head,
		(unsigned)&(st3.e5) - head);

	head = (unsigned)&st4;
	prnt("stru5  (sizeof = %3d): %3d %3d %3d\n", sizeof(st4),
		(unsigned)&(st4.e1) - head,
		(unsigned)&(st4.e2) - head,
		(unsigned)&(st4.e3) - head);

	head = (unsigned)&st5;
	prnt("stru6  (sizeof = %3d): %3d %3d %3d\n", sizeof(st5),
		(unsigned)&(st5.e1) - head,
		(unsigned)&(st5.e2) - head,
		(unsigned)&(st5.e3) - head);
}


void validate_alignment_3()
{
	stru9		st1;
	unsigned	head;

	head = (unsigned)&st1;
	prnt("stru9  (sizeof = %3d): %3d %3d %3d %3d %3d\n", sizeof(st1),
		(unsigned)&(st1.e1) - head,
		(unsigned)&(st1.e2) - head,
		(unsigned)&(st1.e3) - head,
		(unsigned)&(st1.e4) - head,
		(unsigned)&(st1.e5) - head);
}

void validate_alignment_4()
{
	stru10		st1;
	unsigned			head;

	head = (unsigned)&st1;
	prnt("stru10 (sizeof = %3d): %3d %3d %3d %3d %3d\n", sizeof(st1),
		(unsigned)&(st1.e1) - head,
		(unsigned)&(st1.e2) - head,
		(unsigned)&(st1.e3) - head,
		(unsigned)&(st1.e4) - head,
		(unsigned)&(st1.e5) - head);
}


void validate_alignment_5()
{
	stru11		st1;
	stru12		st2;
	stru13		st3;
	unsigned	head;

	head = (unsigned)&st1;
	prnt("stru11 (sizeof = %3d): %3d %3d %3d %3d %3d %3d %3d\n",
		sizeof(st1),
		(unsigned)&(st1.e1) - head,
		(unsigned)&(st1.e2) - head,
		(unsigned)&(st1.e3) - head,
		(unsigned)&(st1.e4) - head,
		(unsigned)&(st1.e5) - head,
		(unsigned)&(st1.e6) - head,
		(unsigned)&(st1.e7) - head);

	head = (unsigned)&st2;
	prnt("stru12 (sizeof = %3d): %3d %3d %3d %3d %3d %3d %3d %3d\n",
		sizeof(st2),
		(unsigned)&(st2.e1) - head,
		(unsigned)&(st2.e2) - head,
		(unsigned)&(st2.e3) - head,
		(unsigned)&(st2.e4) - head,
		(unsigned)&(st2.e5) - head,
		(unsigned)&(st2.e6) - head,
		(unsigned)&(st2.e7) - head,
		(unsigned)&(st2.e8) - head);

	head = (unsigned)&st3;
	prnt("stru13 (sizeof = %3d): %3d %3d %3d %3d %3d\n", sizeof(st3),
		(unsigned)&(st3.e1) - head,
		(unsigned)&(st3.e2) - head,
		(unsigned)&(st3.e3) - head,
		(unsigned)&(st3.e4) - head,
		(unsigned)&(st3.e5) - head);
}


void validate_alignment_6()
{
	stru16		st1;
	stru18		st2;
	stru21		st3;
	stru23		st4;
	stru15		st5;
	stru17		st6;
	stru19		st7;
	stru20		st8;
	stru24		st9;
	stru22		st10;
	unsigned	head;

	head = (unsigned)&st1;
	prnt("stru16 (sizeof = %3d): %3d %3d %3d %3d\n", sizeof(st1),
		(unsigned)&(st1.e1) - head,
		(unsigned)&(st1.e2) - head,
		(unsigned)&(st1.e3) - head,
		(unsigned)&(st1.e4) - head);

	head = (unsigned)&st2;
	prnt("stru18 (sizeof = %3d): %3d %3d %3d %3d\n", sizeof(st2),
		(unsigned)&(st2.e1) - head,
		(unsigned)&(st2.e2) - head,
		(unsigned)&(st2.e3) - head,
		(unsigned)&(st2.e4) - head);

	head = (unsigned)&st3;
	prnt("stru21 (sizeof = %3d): %3d %3d %3d %3d\n", sizeof(st3),
		(unsigned)&(st3.e1) - head,
		(unsigned)&(st3.e2) - head,
		(unsigned)&(st3.e3) - head,
		(unsigned)&(st3.e4) - head);

	head = (unsigned)&st4;
	prnt("stru23 (sizeof = %3d): %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
		sizeof(st4),
		(unsigned)&(st4.e1) - head,
		(unsigned)&(st4.e2) - head,
		(unsigned)&(st4.e3) - head,
		(unsigned)&(st4.e4) - head,
		(unsigned)&(st4.e5) - head,
		(unsigned)&(st4.e6) - head,
		(unsigned)&(st4.e7) - head,
		(unsigned)&(st4.e8) - head,
		(unsigned)&(st4.e9) - head,
		(unsigned)&(st4.e10) - head,
		(unsigned)&(st4.e11) - head,
		(unsigned)&(st4.e12) - head);

	head = (unsigned)&st5;
	prnt("stru15 (sizeof = %3d): %3d %3d\n", sizeof(st5),
		(unsigned)&(st5.e1) - head,
		(unsigned)&(st5.e2) - head);

	head = (unsigned)&st6;
	prnt("stru17 (sizeof = %3d): %3d\n", sizeof(st6),
		(unsigned)&(st6.e1) - head);

	head = (unsigned)&st7;
	prnt("stru19 (sizeof = %3d): %3d %3d %3d\n", sizeof(st7),
		(unsigned)&(st7.e1) - head,
		(unsigned)&(st7.e2) - head,
		(unsigned)&(st7.e3) - head);

	head = (unsigned)&st8;
	prnt("stru20 (sizeof = %3d): %3d %3d\n", sizeof(st8),
		(unsigned)&(st8.e1) - head,
		(unsigned)&(st8.e2) - head);

	head = (unsigned)&st9;
	prnt("stru24 (sizeof = %3d): %3d %3d\n", sizeof(st9),
		(unsigned)&(st9.e1) - head,
		(unsigned)&(st9.e2) - head);

	head = (unsigned)&st10;
	prnt("stru22 (sizeof = %3d): %3d %3d %3d\n", sizeof(st10),
		(unsigned)&(st10.e1) - head,
		(unsigned)&(st10.e2) - head,
		(unsigned)&(st10.e3) - head);
}

void validate_alignment_7()
{
	stru25		st1;
	unsigned	head;

	head = (unsigned)&st1;
	prnt("stru25 (sizeof = %3d): %3d %3d %3d %3d\n", sizeof(st1),
		(unsigned)&(st1.e1) - head,
		(unsigned)&(st1.e2) - head,
		(unsigned)&(st1.e3) - head,
		(unsigned)&(st1.e4) - head);

}

void validate_alignment_8()
{
	stru14		st1;
	unsigned	head;

	head = (unsigned)&st1;
	prnt("stru14 (sizeof = %3d): %3d %3d\n", sizeof(st1),
		(unsigned)&(st1.e1) - head,
		(unsigned)&(st1.e2) - head);
}

void validate_alignment()
{
	validate_alignment_1();
	validate_alignment_2();
	validate_alignment_3();
	validate_alignment_4();
	validate_alignment_5();
	validate_alignment_6();
	validate_alignment_7();
	validate_alignment_8();
}

void
validate_thread(cyg_addrword_t data)
{
    print_alignment_header();
    validate_alignment();
    validate_sizes();
    validate_endian();
}

void
cyg_user_start(void)
{
    cyg_thread_create(10, validate_thread, (cyg_addrword_t)0, "validate_thread",
		      (void *)stack, STACKSIZE, &thread, &thread_obj);
    cyg_thread_resume(thread);
}

