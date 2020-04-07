/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_errors.h - Definitions for error handling with embedded CDDB.
 */

#ifndef	_GN_ERRORS_H_
#define _GN_ERRORS_H_

/*
 * Background.
 */

/*
 * ERROR CODE FORMAT VERSION 1
 */
/*
 *  Gracenote embedded CDDB error codes (gn_error_t) [version 1]
 *  are 32-bit values layed out as follows:
 *
 *   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-----+-------+---------------+---------------+---------------+
 *  |s| ver | rsvd  |  package/lib  |          error code           |
 *  +-+-----+-----------------------+---------------+---------------+
 *
 *  where
 *
 *      s - severity - indicates success or failure
 *
 *          0 - success
 *          1 - failure
 *
 *      ver - version bits (value = 001).
 *
 *      rsvd - reserved portion of the error code.
 *
 *      package/lib - which library or package is the source of the error.
 *
 *      errror code - as defined in gn_error_codes.h.
 */


/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_error_codes.h>

#ifdef __cplusplus
extern "C"{
#endif 

/*
 * Constants.
 */

/* Severity values */
#define	GN_SUCCESS		0
#define	GN_FAILURE		1

/* Offsets into error code */
#define	GNERR_CODE_OFFSET		0	/* new */
#define	GNERR_PKG_OFFSET		16
#define	GNERR_VER_OFFSET		28
#define	GNERR_SEV_OFFSET		31

/* Masks for fields within error code */
#define	GNERR_CODE_MASK			0xFFFF	/* new */
#define	GNERR_PKG_MASK			0xFF0000
#define	GNERR_VER_MASK			0x70000000

/* Sizes (in bits) of fields within error code */
#define	GNERR_CODE_SIZE			16	/* new */
#define	GNERR_PKG_SIZE			8
#define	GNERR_VER_SIZE			3
#define	GNERR_SEV_SIZE			1

#define	GNERR_MAX_PACKAGE_STRING	32
#define	GNERR_MAX_CODE_STRING		64
#define	GNERR_MAX_MESSAGE_STRING	(GNERR_MAX_PACKAGE_STRING + GNERR_MAX_CODE_STRING + 64)


/*
 * Macros.
 */

/* Form error value from package id and error code */
#define		GNERR_MAKE_VALUE(package_id,error_code)	\
\
				((gn_error_t)(((gn_uint32_t)0x01 << GNERR_SEV_OFFSET)		|		\
							((gn_uint32_t)0x001 << GNERR_VER_OFFSET)		|		\
							((gn_uint32_t)package_id << GNERR_PKG_OFFSET)	|		\
							((gn_uint32_t)error_code << GNERR_CODE_OFFSET)))

/* Extract error code */
#define		GNERR_ERROR_CODE(error_code)	\
\
				((gn_uint16_t)((((gn_uint32_t)error_code & GNERR_CODE_MASK) >> GNERR_CODE_OFFSET) &	\
				(((gn_uint32_t)0x01 << (GNERR_CODE_SIZE + 1)) - 1)))

/* Extract package id */
#define		GNERR_PKG_ID(package_id)	\
\
				((gn_uint16_t)((((gn_uint32_t)package_id & GNERR_PKG_MASK) >> GNERR_PKG_OFFSET) &		\
				(((gn_uint32_t)0x01 << (GNERR_PKG_SIZE + 1)) - 1)))


#ifndef GN_NO_LOGGING

#define		GNERR_ERR_CODE(code)		gnerr_log_error(code,__LINE__,__FILE__)
#define		GNERR_LOG_ERR(code)			gnerr_log_error(code,__LINE__,__FILE__)
#define		GNERR_LOG_ERR_CNTXT(code,context_info)		gnerr_log_error_cntxt(code,context_info,__LINE__,__FILE__)

#else

#define		GNERR_ERR_CODE(code)					(code)
#define		GNERR_LOG_ERR(code)						(code)
#define		GNERR_LOG_ERR_CNTXT(code,context_info)	(code)

#endif


/*
 * Backward Compatibility
 */

#include <extras/cddb/gn_error_values.h>


/*
 * Prototypes
 */

/* API to log or display the error information */
gn_error_t
gnerr_log_error(gn_error_t err,gn_int32_t line, gn_char_t filename[]);

/* API to log or display the context specific error information */
gn_error_t
gnerr_log_error_cntxt(gn_error_t code,gn_str_t context_error_info,gn_int32_t line, gn_char_t filename[]);

/* get a descriptive message reflecting the package id and error value */
gn_str_t
gnerr_get_error_message(gn_error_t error_value);


#ifdef __cplusplus
}
#endif 


#endif /* #ifndef _GN_ERRORS_H_ */
