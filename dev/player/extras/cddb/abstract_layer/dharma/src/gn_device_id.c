/*
 * Copyright (c) 2002 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_device_id.c - APIs for device identification strings information.
 */


/*
 * Dependencies
 */

#include <extras/cddb/gn_build.h>


/*
 * Implementations
 */

/* NOTE */
/* This API must be provided by device vendor and return a device unique ID or serial number */
gn_cstr_t
gn_get_device_id(void)
{
	return "123456789A";
}
gn_cstr_t
gn_get_software_version(void)
{
	return "1";
}

gn_cstr_t
gn_get_hardware_version(void)
{
	return "1";
}

