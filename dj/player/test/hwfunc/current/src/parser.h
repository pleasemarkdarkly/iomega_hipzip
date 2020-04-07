// parser.h
// temancl@fullplaymedia.com 03/06/02
// (c) fullplay media 
// 
// description:
// parser declarations, limits

#ifndef _MONITOR_H_
#define _MONITOR_H_

#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>

#define TRUE 1
#define FALSE 0



// parser limits
#define MAX_CMDS 6
#define MAX_CMD_LEN 8
#define MAX_STRING_LEN 32

#define P_DEFAULT 0
#define P_DECIMAL 1
#define P_STRING 2 // force string mode

// struct for command format
struct monitor_test
{
	void* func;
	char* name;
	char* help;
	int dec; // parser mode options
};

// function list declaration
extern struct monitor_test test_list[];

// new function pointer typedef
typedef int (*voidfunc)(char param_strs[][MAX_STRING_LEN],int* param_nums);

void shell();

#endif // _MONITOR_H_

