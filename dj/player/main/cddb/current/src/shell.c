/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * shell.c:	Main routines for driving the shell implementation.
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STDIO_H
#include GN_STRING_H
#include GN_CTYPE_H
#include GN_TIME_H
#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_configmgr.h>
#include <extras/cddb/gn_build.h>
#include <extras/cddb/gn_system.h>
#include "command_table.h"

#if	defined(PLATFORM_WIN32)
	#include GN_IO_H
	#include <windows.h>
#elif	defined(PLATFORM_UNIX)
	#include GN_UNISTD_H
#endif

#include "shell.h"
#include "branding.h"
#include <util/debug/debug.h>

const char	gPrompt[]	= "ecddb * > ";


/*
 * Variables.
 */

int		gAllDone		= 0;		/* set when quitting */
int		gQuietMode		= 0;		/* no output, not yet implemented */
int		gInteractive	= 0;		/* TRUE reading from the keyboard */
FILE*	gInputFile		= NULL;		/* input file or redirected stdin */
int		gEncoding		= 0;		/* base64 decode data or not */
int		gCommCallback	= 0;		/* communications callback set or not */
int		gUpdateCallback	= 0;		/* update callback set or not */
int		gInitECDDB		= GN_TRUE;	/* initialize eCDDB */
int		gTiming			= GN_FALSE;	/* auto-timing set or not */
int		gDumpRaw		= GN_FALSE;	/* dump raw XML data */
int		gRawUTF8		= GN_FALSE;	/* display UTF8 data */
int		gStrict			= GN_FALSE;	/* case-sensitive validation */
int		gVerbose		= GN_FALSE;	/* print lots of output */
int		gAutoSelect		= GN_TRUE;	/* auto-select first fuzzy match */
int		gEcho			= GN_FALSE; /* echo commands for script or redirected input */
int		gPaging			= GN_FALSE;	/* wait for keyboard input between lookups */
int		gBranding		= GN_TRUE;	/* set branding display duration */
int		gRepeat			= GN_FALSE;	/* repeat next command */
int		gRepeatCount	= 0;		/* repeat count of next command */
int		gSwapDisc		= GN_FALSE;	/* swap disc for batch lookups */
int		gRepeating		= GN_FALSE;	/* the command line processor is not in the middle of repeating */


/*
 *	Local functions
 */

static int			init_shell(void);
static int			parse_commandline(char* commandline, char* argv[], char** output_redir_filename);
static int			get_command(char* buffer, int buffer_size, FILE* input_file);
static void			execute_command_line(int argc, char* argv[], char* redir_filename);

static int start_timing(time_t* start);
static void stop_timing(time_t start, int* time_set);
static void set_output_redirect(char* redir_filename, FILE** redir_file, int* save_stdout);
static void clear_output_redirect(char** redir_filename, FILE** redir_file, int* save_stdout);


/*
 * Implementations
 */

#if 0
int main(int argc, char* argv[])
{
	return shell(argc,argv);
}
#endif


int shell(int argc, char* argv[])
{
	int				result = 0;
	gn_error_t		error = SUCCESS;

	result = init_shell();

	argc--;
	argv++;
	execute_command_line(argc,argv,NULL);

	/* check to see if the command line (or a redirect from the command line) contained a 'quit' command */
	if (gAllDone)
	{
		shutdown_shell();
		return result;
	}

	brand_display_on_powerup();
	print_shell_version();

	if (gInitECDDB == GN_TRUE)
		error = initialize_ecddb(GN_HEAP_SIZE_DEFAULT);
	else
		diag_printf("eCDDB is not initialized\n\n");

	process_commands(NULL);

	if (gInitECDDB == GN_TRUE)
		error = shutdown_ecddb();

	shutdown_shell();
	return result;
}


void process_commands(const char* input_file_name)
{
	char				commandline[1024] = "";
	char*				output_redir_filename = NULL;
	int					internal_argc = 0;
	char*				internal_argv[256] = {""};
	FILE*				input_file = stdin;

	if (input_file_name != NULL)
	{
		input_file = fopen(input_file_name,"r");
		if (input_file == NULL)
			return;
	}

	while (!gAllDone)
	{
		commandline[0] = '\0';

		/* Get the next command, parse it, and attempt to dispatch it */
		if (!get_command(commandline, sizeof(commandline),input_file))
			break;

#ifdef	_GN_LOG_MEM_
		gnmem_add_mem_logging_info(commandline);
#endif /* #ifndef _GN_LOG_MEM_ */
#ifndef _BLD_PROTO_
		gnfs_add_fs_logging_info("Shell Command:");
		gnfs_add_fs_logging_info(commandline);
#endif /* #ifndef _BLD_PROTO_ */

		/* skip lines starting with '#' */
		if (commandline[0] == '#')
			continue;

		/* break up a string into separate commands */
		output_redir_filename = NULL;
		internal_argc = parse_commandline(commandline, internal_argv, &output_redir_filename);
		if (internal_argc == 0)
			continue;

		execute_command_line(internal_argc,internal_argv,output_redir_filename);
	}

	if ((input_file != stdin) && (input_file != NULL))
		fclose(input_file);
}


static void execute_command_line(int argc, char* argv[], char* redir_filename)
{
	command_handler_t	command = NULL;
	int					time_set = 0;
	time_t				start;
	int					save_stdout = -1;
	FILE*				output_redir_file = NULL;
	gn_bool_t			repeater = GN_FALSE;


	if (argc == 0)
		return;

	command = find_command(argv[0]);

	set_output_redirect(redir_filename,&output_redir_file,&save_stdout);

	if (gRepeating == GN_TRUE)
	{
		if (gRepeat == GN_TRUE)
			repeater = GN_TRUE;
	}

	while (1)
	{
		if (gEcho)
		{
			int	i = 0;

			diag_printf("%s",gPrompt);
			for (i = 0; i < argc; i++)
				diag_printf("%s ",argv[i]);
			diag_printf("\n");
		}

		if (command == NULL)
		{
			display_bad_command_msg(argv[0]);
			gRepeatCount = 0; /* don't display the "bad command message" repeatedly */
		}
		else
		{
			time_set = start_timing(&start);

			/* invoke the command handler with the rest of the arguments */
			command(argc - 1, argv + 1);

			stop_timing(start,&time_set);
		}

		if (repeater == GN_TRUE)
		{
			gRepeatCount--;
			if (gRepeatCount <= 0)
			{
				gRepeat = GN_FALSE;
				repeater = GN_FALSE;
			}
		}

		if (repeater == GN_FALSE)
		{
			clear_output_redirect(&redir_filename, &output_redir_file,&save_stdout);
			break;
		}
	}
}


static int init_shell(void)
{
	if (isatty(fileno(stdin)))
		gInteractive = 1;

	return 0;
}


void shutdown_shell(void)
{
}


void print_shell_version(void)
{
	gn_str_t	build_str;

	build_str = get_build_summary();
	if (build_str != NULL)
		diag_printf("%s\n\n", build_str);
}


static int parse_commandline(char* commandline, char* argv[], char** output_redir_filename)
{
	/* break up the commandline into separate arguments, return # of args */
	int		num_args = 0;
	char*	ptr = NULL;
	char*	tail_ptr = NULL;
	char*	tmp = NULL;

	/* arguments are demarcated by white space, unless white space is nested within double quotes */

	/* first, look for output redirection '>' */
	tail_ptr = strchr(commandline, '>');
	if (tail_ptr)
	{
		tmp = tail_ptr;
		*tail_ptr++ = 0;

		/* get rid of whitespace between '>' and filename */
		while (*tail_ptr && isspace(*tail_ptr))
			tail_ptr++;
		tmp--;
		while(tmp > commandline && isspace(*tmp))
			tmp--;
		if (tmp == commandline)
			return 0;
		tmp++;
		*tmp = 0;

		if (*tail_ptr)
		{
			*output_redir_filename = tail_ptr;
			while (*tail_ptr && !isspace(*tail_ptr))
				tail_ptr++;
		}
		tail_ptr = NULL;
	}

	/* next, trim leading white space */
	ptr = commandline;
	while (ptr != NULL)
	{
		while (*ptr && isspace(*ptr))
			ptr++;

		if (*ptr == 0)
			/* nothing bug nothing */
			break;

		/* now, see if we've got a quoted string */
		if (*ptr == '\"')
		{
			char*	head_ptr = ptr;
			head_ptr++;
			tail_ptr = strstr(head_ptr,"\"");

			if (tail_ptr != NULL)
				/* move past the leading quote */
				ptr = head_ptr;
		}

		argv[num_args] = ptr;
		num_args++;

		if (tail_ptr == NULL)
		{
			tail_ptr = strstr(ptr," ");
			if (tail_ptr == NULL)
				tail_ptr = strstr(ptr,"\t");
		}

		if (tail_ptr != NULL)
		{
			*tail_ptr = 0; /* remove the trailing quote or trailing space*/
			ptr = tail_ptr + 1; /* reset the head of the command line */
		}
		else
			ptr = NULL;

		tail_ptr = NULL;
	}

	/* no more arguments, let's go */
	argv[num_args] = NULL;
	return num_args;
}


static int get_command(char * buffer, int buffer_size, FILE* input_file)
{
	char *	p = NULL;

	if (input_file == stdin)
	{
		if (gPrompt)
			diag_printf("%s",gPrompt);
		else
			diag_printf("* > ");
	}

	if (fgets(buffer, buffer_size, input_file) == NULL)
	{
	/* let's find out if we're at the end of the file */
		int		fresult = 0;

		fresult = feof(input_file);
		if (fresult != 0)
		{
			/* this is the end of file */
			if (input_file != stdin)
				/* this condition should always test true */
				fclose(input_file);
			return 0;
		}
		else
		{
			/* we're not at the end of the file --> the NULL fgets result indicates an error */
			fresult = ferror(input_file);
			perror(NULL);

			return -1;
		}
	}

	if (input_file == stdin)
		diag_printf("\n");
	
	/* remove trailing newline, if present */
	p = strchr(buffer, '\n');
	if (p != NULL)
		*p = 0;

	/* remove trailing carriage return, if present */
	p = strchr(buffer, '\r');
	if (p != NULL)
		*p = 0;

	return 1;
}

void display_bad_command_msg(const char * cmd)
{
	diag_printf("not found [%s] - 'h' for help\n", cmd);
}


void shell_sleep(int seconds)
{
#if defined(PLATFORM_WINDOWS)
	Sleep(seconds*1000);
#elif defined(PLATFORM_UNIX)
	sleep(seconds);
#endif
}


static int start_timing(time_t* start)
{
#ifndef _BLD_PROTO_
	struct tm*		display_time = NULL;

	if (!start)
		return 0;

	if (gTiming == 0)
		return 0;

	time(start);
	display_time = localtime(start);
	diag_printf("%s\n",asctime(display_time));

	return 1;
#else
	return 0;
#endif
}


static void stop_timing(time_t start, int* time_set)
{
#ifndef _BLD_PROTO_
	struct tm*		display_time = NULL;
	static time_t	finish;
	double			elapsed_time;

	if (gTiming == 0)
		return;

	/* display elapsed time */
	time(&finish);
	display_time = localtime(&finish);
	diag_printf("\n%s",asctime(display_time));

	if (time_set && (*time_set == 1))
	{
		elapsed_time = difftime(finish,start);
		diag_printf("Command required %6.0f seconds.\n",elapsed_time);
		*time_set = 0;
	}
#endif
}


#if !defined(PLATFORM_WIN32)
#define		_dup		dup
#define		_dup2		dup2
#define		_close		close
#define		_fileno		fileno
#endif


static void set_output_redirect(char* redir_filename, FILE** redir_file, int* save_stdout)
{
#if defined(PLATFORM_WIN32) || defined(PLATFORM_UNIX)
	if (redir_filename == NULL)
		return;
	if (redir_file == NULL)
		return;
	if (save_stdout == NULL)
		return;

	/* open file to redirect output for command */
	*save_stdout = _dup( _fileno(stdout) );
	*redir_file = fopen(redir_filename, "w");
	if (*redir_file == NULL)
	{
		diag_printf("Error opening redir_file: %s\n", redir_filename);
		_close(*save_stdout);
	}
	else
	{
		if (-1 == _dup2(_fileno(*redir_file), _fileno(stdout)))
		{
			diag_printf("Error _dup2'ing stdout.\n");
			fclose(*redir_file);
			*redir_file = NULL;
			_close(*save_stdout);
		}
	}
#endif /* #if defined(PLATFORM_WIN32) || defined(PLATFORM_UNIX) */
}


static void clear_output_redirect(char** redir_filename, FILE** redir_file, int* save_stdout)
{
#if defined(PLATFORM_WIN32) || defined(PLATFORM_UNIX)
	if ((redir_file == NULL) || (*redir_file == NULL))
		return;
	if (save_stdout == NULL)
		return;

	if ((redir_filename != NULL) && (*redir_filename != NULL))
		*redir_filename = NULL;

	fflush(stdout);
	fclose(*redir_file);
	*redir_file = NULL;
	_dup2(*save_stdout, _fileno(stdout));
	_close(*save_stdout);
	setbuf(stdout, NULL);
#endif /* #if defined(PLATFORM_WIN32) || defined(PLATFORM_UNIX) */
}


void set_input_redirect(char* redir_filename, FILE** redir_file, int* save_stdin)
{
#if defined(PLATFORM_WIN32) || defined(PLATFORM_UNIX)
	if (redir_filename == NULL)
		return;
	if (redir_file == NULL)
		return;
	if (save_stdin == NULL)
		return;

	/* open file to redirect output for command */
	*save_stdin = _dup( _fileno(stdin) );
	*redir_file = fopen(redir_filename, "r");
	if (*redir_file == NULL)
	{
		diag_printf("Error opening redir_file: %s\n", redir_filename);
		_close(*save_stdin);
	}
	else
	{
		if (-1 == _dup2(_fileno(*redir_file), _fileno(stdin)))
		{
			diag_printf("Error _dup2'ing stdin.\n");
			fclose(*redir_file);
			*redir_file = NULL;
			_close(*save_stdin);
		}
	}
#endif /* #if defined(PLATFORM_WIN32) || defined(PLATFORM_UNIX) */
}


void clear_input_redirect(char** redir_filename, FILE** redir_file, int* save_stdin)
{
#if defined(PLATFORM_WIN32) || defined(PLATFORM_UNIX)
	if ((redir_file == NULL) || (*redir_file == NULL))
		return;
	if (save_stdin == NULL)
		return;

	if ((redir_filename != NULL) && (*redir_filename != NULL))
		*redir_filename = NULL;

	fflush(stdin);
	fclose(*redir_file);
	*redir_file = NULL;
	_dup2(*save_stdin, _fileno(stdin));
	_close(*save_stdin);
	setbuf(stdin, NULL);
#endif /* #if defined(PLATFORM_WIN32) || defined(PLATFORM_UNIX) */
}
