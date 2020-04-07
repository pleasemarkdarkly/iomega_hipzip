/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * configuration.c
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include "configuration.h"
#include <extras/cddb/gn_errors.h>
#include <util/debug/debug.h>

#include GN_STRING_H
#include GN_STDLIB_H


/*
 * Implementations
 */

gn_error_t
set_configuration(gn_config_param_t param, gn_str_t param_val)
{
	gn_error_t	error = GNERR_NoError;

	error = gnconf_set_str(param,param_val);

	if (error == ABSTERR_CONF_Wrongdata)
		diag_printf("Invalid data input.\n");

	else if (error == ABSTERR_InvalidArg)
		diag_printf("Invalid configuration parameter.\n");

	else if (error == SUCCESS)
		diag_printf("Successful setting.\n");

	else
		diag_printf("Failure on setting the value.\n");

	return error;
}


void disp_configuration(void)
{
	gn_char_t*			svalue = NULL;
	gn_config_param_t	key = NULL;
	gn_size_t			i = 0;
	gn_error_t			error = SUCCESS;

	for (i = 0; 1; i++)
	{
		error = gnconf_get_str_element(i,&key,&svalue);
		if (error != SUCCESS)
			break;

		if (svalue && *svalue)
			diag_printf("%-32s: %s\n", key,svalue);
		else
			diag_printf("%-32s: (null)\n",key);
	}

	diag_printf("\n");
}


void save_configuration(void)
{
	gnconf_store_params(NULL);
}
