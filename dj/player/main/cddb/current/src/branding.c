/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * branding.c - Example fulfillment of Gracenote Branding Requirements
 */


/*
 * Dependencies
 */

#include "branding.h"
#include "shell.h"
#include <stdio.h>
#include <util/debug/debug.h>


/*
 * Constants
 */

const char	powerup_brand[]	= "Powered by Gracenote (display for 3 seconds upon power up)";
const char	lookup_brand[]	= "Accessing Gracenote CDDB (display for 3 seconds upon lookup)";
const char	update_brand[]	= "Updating Gracenote CDDB (display for 3 seconds upon update)";

static int	powerup_duration = 0	/* 3 in a synchronous display environment, 3 seconds is way too long */;
static int	lookup_duration = 0		/* 3 */;
static int	update_duration = 0		/* 3 */;


/*
 * Implementations
 */

void
brand_display_on_powerup(void)
{
	diag_printf("\n%s\n",powerup_brand);
	diag_printf("===========================================================\n");
	shell_sleep(powerup_duration);
}


void
brand_display_on_lookup(void)
{
	diag_printf("\n%s\n",lookup_brand);
	diag_printf("===========================================================\n");
	shell_sleep(lookup_duration);
}


void
brand_display_on_update(void)
{
	diag_printf("\n%s\n",update_brand);
	diag_printf("===========================================================\n");
	shell_sleep(update_duration);
}


void
brand_set_duration_on_powerup(int delay)
{
	powerup_duration = delay;
}


void
brand_set_duration_on_lookup(int delay)
{
	lookup_duration = delay;
}


void
brand_set_duration_on_update(int delay)
{
	update_duration = delay;
}

