/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * configuration.h - Declaration of functions called to process configuration commands.
 */

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_configmgr.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Prototypes.
 */

void disp_conf(int argc, char* argv[]);

gn_error_t
set_configuration(gn_config_param_t param, gn_str_t param_val);

void disp_configuration(void);
void save_configuration(void);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _CONFIGURATION_H_ */

