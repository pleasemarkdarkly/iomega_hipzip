/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * shell.h - Header file for Gracenote shell test application.
 */


#ifndef	_SHELL_H_
#define	_SHELL_H_


#ifdef __cplusplus
extern "C"{
#endif 

/*
 * Dependencies
 */

#include <extras/cddb/gn_platform.h>
#include GN_STDIO_H


/*
 * Variables.
 */

extern	int		gAllDone;			/* time to go yet? */
extern	int		gQuietMode;			/* no output, not yet implemented */
extern	int		gInteractive;		/* TRUE reading from the keyboard */
extern	int		gEncoding;			/* base64 decode data or not */
extern	int		gCommCallback;		/* communications callback set or not */
extern	int		gUpdateCallback;	/* update callback set or not */
extern	int		gInitECDDB;			/* initialize eCDDB */
extern	int		gTiming;			/* auto-timing set or not */
extern	int		gDumpRaw;			/* dump raw XML data */
extern	int		gRawUTF8;			/* display UTF8 data */
extern	int		gStrict;			/* case-sensitive validation */
extern	int		gVerbose;			/* print lots of output */
extern	int		gAutoSelect;		/* auto-select first fuzzy match */
extern	int		gEcho;				/* echo commands for script or redirected input */
extern	int		gPaging;			/* wait for keyboard input between lookups */
extern	int		gBranding;			/* set branding display duration */
extern	int		gRepeat;			/* repeat next command */
extern	int		gRepeatCount;		/* repeat count of next command */
extern	int		gSwapDisc;			/* swap disc for batch lookups */
extern	int		gRepeating;			/* the command line processor is not in the middle of repeating */

/*
 * Prototypes
 */

int shell(int argc, char* argv[]);
void shutdown_shell(void);
void process_commands(const char* input_file);

void set_input_redirect(char* redir_filename, FILE** redir_file, int* save_stdin);
void clear_input_redirect(char** redir_filename, FILE** redir_file, int* save_stdiin);

/* display current version */
void print_shell_version(void);

void display_bad_command_msg(const char * cmd);
void shell_sleep(int seconds);

#ifdef __cplusplus
}
#endif 

#endif	/* ifndef	_SHELL_H_ */

