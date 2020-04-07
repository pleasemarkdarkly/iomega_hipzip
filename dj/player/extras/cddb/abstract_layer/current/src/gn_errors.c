/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 *	gn_errors.c - Implementation for Error-handling APIs defined in gn_errors.h 
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_log.h>

#include GN_STDIO_H
#include GN_STRING_H


/*
 * Variables
 */

static gn_char_t	error_message_string[GNERR_MAX_MESSAGE_STRING] = "";


/*
 * Implementations
 */

/* API to log or display the error information */
gn_error_t
gnerr_log_error (gn_error_t code,gn_int32_t line, gn_char_t filename[])
{
	gn_uint32_t		mask = GNLOG_ERROR;

	if (code == SUCCESS)
		return SUCCESS;

	if (gnlog_filter(GNERR_PKG_ID(code),mask) == GN_TRUE)
	{
		gnlog_header(GNERR_PKG_ID(code),mask,line,filename);

		if (gnlog_is_verbose(GNERR_PKG_ID(code),mask))
			gnlog_write(gnlog_logf("Error %0#10X (%s)\n\n",code,gnerr_get_code_desc(GNERR_ERROR_CODE(code))));
		else
			gnlog_write(gnlog_logf("Error %0#10X\n\n",code));
	}

	return code;
}


/* API to log or display the context specific error information */
gn_error_t
gnerr_log_error_cntxt(gn_error_t code, gn_str_t context_error_info, gn_int32_t line, gn_char_t filename[])
{
	if (code != SUCCESS)
		gnerr_log_error(code, line, filename);

	gnlog_write(gnlog_logf("Context Specific Error: %s\n",context_error_info));

	return code;
}


gn_str_t
gnerr_get_error_message(gn_error_t error_value)
{
	gn_char_t	package_buffer[GNERR_MAX_PACKAGE_STRING+1] = "";
	gn_char_t	error_buffer[GNERR_MAX_CODE_STRING+1] = "";
	gn_cstr_t	msg = GN_NULL;

	if(GNERR_PKG_ID(error_value) && !GNERR_ERROR_CODE(error_value))
	{
		sprintf(error_message_string, "\"Unknown error that can not be mapped (outside of eCDDB error space)\"\n");
		return error_message_string;
	}

	/* enforce the maximum error code string size */
	msg = gnerr_get_code_desc(GNERR_ERROR_CODE(error_value));
	if (msg == GN_NULL)
		strncpy(error_buffer,"Unknown error",GNERR_MAX_CODE_STRING);
	else
		strncpy(error_buffer,msg,GNERR_MAX_CODE_STRING);

	/* enforce the maximum package id string size */
	msg = gnerr_get_package_desc(GNERR_PKG_ID(error_value));
	if (msg != GN_NULL)
		strncpy(package_buffer,msg,GNERR_MAX_PACKAGE_STRING);

	error_message_string[0] = '\0';

	if (msg == GN_NULL)
	{
		/* error occurred in generic package, so don't mention any package */
		sprintf(error_message_string,"\"%s (%0#6X)\".",error_buffer,GNERR_ERROR_CODE(error_value));
	}
	else
	{
		sprintf(error_message_string,"\"%s (%0#6X)\". Reported in \"%s (%0#4X)\".",
			error_buffer,GNERR_ERROR_CODE(error_value),package_buffer,GNERR_PKG_ID(error_value));
	}

	return error_message_string;
}
