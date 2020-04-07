/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * template.h - brief description of contents of this header file.
 */

#ifndef	_TEMPLATE_H_
#define _TEMPLATE_H_


/*
 * Dependencies.
 */

#include	<std_types.h>
#include	"our_types.h"


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants.
 */

#define		TEMPLATE_SIZE		2		/* what this constant defines */


/*
 * Enums ARE NOT SUPPORTED!
 */


/*
 * Structures and typedefs.
 */

/* brief description of structure */
typedef struct temp_dummy {
	type_t		td_field_name;
	type2_t		td_field_name2;
} temp_dummy_t;

typedef int temp_type_t;

/* brief description of structure (alt style) */
typedef struct temp_dummy_alt
{
	type_t		tda_field_name;
	type2_t		tda_field_name2;
}
temp_dummy_alt_t;


/*
 * Macros.
 */

/* brief description of macro */
#define		TEMPLATE_COMP(x)	(x * sizeof(temp_dummy_t))		


/*
 * Variables.
 */

extern	temp_dummy_t	*dummy;		/* please don't use global variables */


/*
 * Prototypes.
 */

/* brief description of function */
int template_func(temp_type_t param1, temp_type_t *param2);


#ifdef __cplusplus
}
#endif 


#endif /* _TEMPLATE_H_ */


