/*
 * Copyright (c) 2002 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * logging.h - Functionality to display logging status from the shell.
 */

/*
 * Typedefs
 */

/* For mapping package names into package ID's */
typedef struct gn_pkg_map_s
{       
	gn_cstr_t	str;
	gn_uint16_t	pkg;
} gn_pkg_map_t;

/* For mapping logging mask strings into mask bits */
typedef struct gn_mask_map_s
{
	gn_cstr_t	str;
	gn_uint32_t	mask;
} gn_mask_map_t;

/*
 * Prototypes
 */

void do_display_logging(int argc, char* argv[]);
void do_enable_logging(int argc, char *argv[]);
void do_disable_logging(int argc, char *argv[]);
