/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_registrar.h - defines an abstract interface to a device 
 * registration package. First, create a registrar reference with one of 
 * the creation functions, then use it by calling one of its function 
 * pointers, and when you are done, delete it by calling its destroy 
 * function. Depending on which kind of registrar you create, you will 
 * get different behavior.
 */

#ifndef GN_REGISTRAR_H
#define GN_REGISTRAR_H

/*
 * Dependencies
 */

#include <extras/cddb/gn_defines.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Constants
 */

#define ONLINE_REGISTRAR	0
#define PC_REGISTRAR		1


/*
 * Typedefs
 */

typedef gn_uint32_t	gn_registrar_type;

typedef struct gn_device_reg_t {
	gn_char_t	id[64];
	gn_char_t	tag[64];
} gn_device_reg_t;

typedef struct gn_registrar_t {
	gn_size_t			size;	/* this size might be greater than sizeof(gn_registrar_t) */
	gn_registrar_type	type;	/* will be the registrar's true type */

	void				(*destroy)(struct gn_registrar_t* registrar);

	gn_error_t			(*obtain_reg_data)(struct gn_registrar_t* registrar, gn_device_reg_t* reg_data);
} gn_registrar_t;


/*
 * Creation Functions
 */

gn_registrar_t*
gn_create_online_registrar(gn_transporter_t* transporter);

gn_registrar_t*
gn_create_pc_registrar(void);

#ifdef __cplusplus
}
#endif

#endif
