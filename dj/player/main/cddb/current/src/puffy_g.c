/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * puffy_g.c:	Couldn't be puffier than this.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STDIO_H
#include <util/debug/debug.h>
                     

char*	puffy_g[24] = {		 
"						   ",
"                     ***  ",
"                    ***** ",
"                    ***** ",
"            ****     ***  ",
"          ********        ",
"        ************      ",
"       **************     ",
"       **************     ",
"       **************     ",
"       **************     ",
"        ************      ",
"          ********        ",
"            ****          ",
"                          ",
"          ********        ",
"       **************     ",
"          ********        ",
"                          ",
"       gracenote (tm)     ",
"                          ",
"                          ",
0 };


void	print_puffy(void)
{
	char**	p = &puffy_g[0];

	while (p && *p)
	{
		diag_printf("%s\n", *p);
		p++;
	}
}

