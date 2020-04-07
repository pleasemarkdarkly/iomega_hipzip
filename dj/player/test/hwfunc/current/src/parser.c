// parser.c
// temancl@fullplaymedia.com 03/06/02
// (c) fullplay media 
// 
// description:
// command parser and dispatcher

#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>

#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_edb7xxx.h>

#include "parser.h"
#include "cmds.h"
#include "io.h"

#include <cyg/error/codes.h>
#include <cyg/fileio/fileio.h>
#include <pkgconf/io_serial.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/kernel/kapi.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// parser commands
int parser_repeat(char param_strs[][MAX_STRING_LEN],int* param_nums);
int parser_crepeat(char param_strs[][MAX_STRING_LEN],int* param_nums);
int parser_last(char param_strs[][MAX_STRING_LEN],int* param_nums);

// command / function table
// last entry must be {NULL, NULL, NULL}
struct monitor_test test_list[] = 
	{

		// memory manipulation
		{(void*)test_readwords,"RW","<address> <count>",0},
		{(void*)test_writewords,"WW","<address> <pattern> <count>",0}, 
		{(void*)test_readbytes,"RB","<address> <count>",0},
		{(void*)test_writebytes,"WB","<address> <pattern> <count>",0}, 
		{(void*)test_readhalfwords,"RH","<address> <count>",0},
		{(void*)test_writehalfwords,"WH","<address> <pattern> <count>",0}, 

		// internal management
		{(void*)test_setverbosity,"VERB","<verbosity>",0}, 
		{(void*)test_echo,"ECHO","<string>",0}, 
		{(void*)test_rem,"REM","anything, ignored",0}, 
		{(void*)test_help,"HELP","anything, ignored",0},
		{(void*)parser_repeat,"REP","<repeat count>",0},
		{(void*)parser_crepeat,"CREP","<conditional repeat count>",0},
		{(void*)parser_last,"L","repeat last command",0},

		// real hardware tests
		{(void*)test_gpio,"GPIO","<bank> <line> <direction> <value>",0},
		{(void*)test_memtest,"MEMTEST","<size>",0},
		{(void*)test_stress,"STRESS","<fail fast = true>",0},
		{(void*)test_hdstress,"HDSTRESS","<sector count> <fail fast = true>",0},
		{(void*)test_cdstress,"CDSTRESS","<sector count> <fail fast = true>",0},
		{(void*)test_lcd,"LCD","Run LCD test, ON, OFF",0},
		
		{(void*)test_adc,"ADC","Run ADC/DAC test for <secs> length",0},
		{(void*)test_adcbg,"ADCBG","background ADC with ON/OFF",0},
		{(void*)test_gain,"GAIN","Set Analog Gain level (0-12db)",P_DECIMAL},
		{(void*)test_boost,"BOOST","Set ADC 20db boost l = on, 0 = off",P_DECIMAL},
		{(void*)test_bypass,"BYPASS","Set DAC output 0 = .73 1 = .92",0},
		{(void*)test_dac,"DAC","ON, OFF, TEST, or nothing for the full sequence",0},
		{(void*)test_tone,"TONE","<frequency> <samplerate> <length (sec)>",P_DECIMAL},
		{(void*)test_serial2,"SERIAL","Serial 2 test",0},
		{(void*)test_reset,"RESET","Invoke hardware reset",0},
		{(void*)test_temp,"TEMP","Show current temperature",0},		
		{(void*)test_ata,"ATA","Run ATA test",0},
		{(void*)test_net,"NET","Run Ethernet chip test",0},
		{(void*)test_ir,"IR","Run IR test",P_DECIMAL},
		{(void*)test_key,"KEY","Run Keypad test",P_DECIMAL},
		{(void*)test_eject,"EJECT","Eject/Inject CD",0},
		{(void*)test_netmem,"NETMEM","Network internal memory test",0},		
		{(void*)test_net_slave,"SLAVE","Network slave test",P_STRING},		
		{(void*)test_net_master,"MASTER","Network master test",P_STRING},
		{(void*)test_flash,"FLASH","Factory flash procedure",0},
		
		{NULL, NULL, NULL}
	};



char lastline[MAX_LINE_LENGTH];
char parseline[MAX_LINE_LENGTH];


// parser state
unsigned long ulRepeatCount = 0;
int bRepeat = FALSE;
int bRepeatCond = FALSE;
int bLast = FALSE;


// parser extensions
int parser_repeat(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	printf("Entering Repeat Mode\n");
	ulRepeatCount = param_nums[0];
	bRepeat = TRUE;
	bRepeatCond = FALSE;
}

// parser extensions
int parser_crepeat(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	printf("Entering Conditional Repeat Mode\n");
	ulRepeatCount = param_nums[0];
	bRepeat = TRUE;
	bRepeatCond = TRUE;
}
// parser extensions
int parser_last(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	printf("Repeating Last Command\n");
	bLast = TRUE;
}


// get the function pointer to call
void* getfp(char* name)
{
	int i = 0;

	while(test_list[i].func != NULL)
	{
		if(strncmpci(test_list[i].name,name,MAX_CMD_LEN) == 0)
		{
			return test_list[i].func;
		}

		i++;
	}

	return NULL;
}

int getdec(char *name)
{
	int i = 0;

	while(test_list[i].func != NULL)
	{
		if(strncmpci(test_list[i].name,name,MAX_CMD_LEN) == 0)
		{
			return test_list[i].dec;
		}

		i++;
	}

	return NULL;
}
	

// entry point/shell
void
shell(void)
{
	// parameter buffers
	char param_strs[MAX_CMDS][MAX_STRING_LEN];
	cyg_uint32 param_nums[MAX_CMDS];

	char *paramlist,*param, *newline;
	int bInfinite;
	int result;
	voidfunc test;
	int dec;
	cyg_uint8 i;  

	shell_io_init();

	printf("\nFullplay DJ rev 03/04 Functionality Tests\n");	
#if (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000)
	printf("90mhz ");
#else
	printf("74mhz ");
#endif
	printf("16mb configuration\n");
	printf("Verbosity set to %d\n",verbosity);
	printf("Type HELP for command list.\n");

	// command loop
	while(1)
	{


		// if we are repeating the last command
		if(bLast)
		{
			strncpy(parseline,lastline,MAX_LINE_LENGTH);
		}
		else
		{
			if(bRepeat)
			{
				if(ulRepeatCount > 0)
					printf("\n%cREP %u> ",bRepeatCond ? 'C' : ' ',ulRepeatCount);
				else
					printf("\n%cREP INF> ",bRepeatCond ? 'C' : ' ');
			}
			else
			{
				printf("\n> ");
			}

			// get a line
			newline = getline();

			// copy for destructive parser
			strncpy(parseline,newline,MAX_LINE_LENGTH);
			
		}


		// get cmd
		paramlist = strtoken(parseline," \t");

		// find function pointer 
		// printf("cmd = '%s', param = '%s'\n",line,paramlist);
		if((test = (voidfunc)getfp(parseline)) == NULL)
		{
			printf("Invalid Command\n");
			continue;
		}

		if(!bLast && (voidfunc)test != (voidfunc)parser_last)
		{
			// save command, as long as it isn't L
			
			strncpy(lastline,newline,MAX_LINE_LENGTH);
			// printf("saving '%s'\n",lastline);
		}

		bLast = FALSE;
	

		dec = getdec(parseline);

	
		// clear params
		for(i = 0; i < MAX_CMDS; i++)
		{	
			param_strs[i][0] = '\0';
			param_nums[i] = 0;
		}


		// get params
		for(i = 0; i < MAX_CMDS; i++)
		{			

			//printf("param %d\n",i);
		
			// go to next non-space/tab
			while(*paramlist == ' ' || *paramlist == '\t')
				paramlist++;

			if(*paramlist == '\0')
			{
				//printf("eol\n");
				break;
			}

			

			switch(*paramlist)
			{
				// parse a string
				case '\"':

					// skip quote
					paramlist++;

					// save as param
					param = paramlist;

					// advance parameter list
					paramlist = strtoken(param,"\"");
					strncpy(param_strs[i],param,MAX_STRING_LEN);

					// print results
					//printf("param string %d = %s\n",i,param_strs[i]);					
					break;

				// try and parse a number
				default:
					// save as param
					param = paramlist;

					// advance parameter list
					paramlist = strtoken(param," \t");

					// interpret numbers as hex or decimal?
					if(dec & P_DECIMAL)
						param_nums[i] = atoi(param);
					else
						param_nums[i] = axtoi(param);
					
					//printf("param num %d = %x\n",i,param_nums[i]);

					// zeros become strings
					if(!param_nums[i] || (dec & P_STRING))
					{
						strncpy(param_strs[i],param,MAX_STRING_LEN);
						//printf("param string %d = %s\n",i,param_strs[i]);
					}

					break;
			} // switch

		} // for
	

		set_keypress();

		if(bRepeat)
		{

			if(verbosity > 0)
			{
				printf("Repeating Command, Ctrl-C to cancel\n");
			}
			else
			{
				printf("Repeating Command\n");
			}

			bInfinite = FALSE;

			if(ulRepeatCount == 0)
				bInfinite = TRUE;
			
			while(ulRepeatCount > 0 || bInfinite)
			{
				// repeating test
				result = test(param_strs,param_nums);
				printf("return val = %d\n",result);

				if(bRepeatCond && result != TEST_OK_PASS)
				{
					printf("failure, exiting loop\n");
					break;
				}


				// if verbosity > 0, ctrl-c works
				if(verbosity > 0)
				{

					if(isbreak())
					{
						printf("Ctrl-C detected, breaking\n");
						ulRepeatCount = 0;
						break;
					}

				}

				ulRepeatCount--;
			}

			bRepeat = FALSE;
		}
		else
		{		
			result = test(param_strs,param_nums);
			printf("return val = %d\n",result);

		}



		reset_keypress();

	} // while(1)
	

}


