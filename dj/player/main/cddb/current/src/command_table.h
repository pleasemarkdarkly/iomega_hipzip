/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * command_table.h - Definitions for command "table" for the shell app.
 */

#ifndef	_COMMAND_TABLE_H_
#define	_COMMAND_TABLE_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Structures and typedefs.
 */

/* Signature for actual command handlers */
typedef void (*command_handler_t)(int argc, char* argv[]);

/* Information for each command */
typedef struct {
	command_handler_t	handler;
	const char* 		command;
	const char*			syntax;
	const char* 		desc;
	const char* 		fulldesc;
} command_record_t;

/* Information for shell options */
typedef struct {
	command_handler_t	handler;
	const char* 		option;
	int*				value;
} option_record_t;


/*
 * Constants
 */

#define	GN_HEAP_SIZE_DEFAULT	(384*1024)

/* responses to user prompts */
#define	PROCEED		1
#define QUIT		0


/*
 * Prototypes.
 */

/* command handler prototypes (NOTE: ADD NEW COMMANDS HERE) */
void 	quit(int argc, char* argv[]);
void 	show_commands(int argc, char* argv[]);
command_handler_t find_command(const char* str);

gn_error_t initialize_ecddb(gn_size_t heap_size);

gn_error_t shutdown_ecddb(void);

int		prompt_user_proceed(const char* str);


#ifdef __cplusplus
}
#endif 

#endif	/* ifndef	_COMMAND_TABLE_H_ */

