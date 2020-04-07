//
// DJ Serial 2 Test
//

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
#include "cmds.h"
#include "parser.h"

int test_serial2echo(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	
    int ser2,ser2w,i,done,iread;

	char inbuff[64];



	// is serial port connected?
	if((*(cyg_uint8*)PBDR & 0x20) == 0)
	{
		diag_printf("serial not connected\n");
		return;
	}
	else
		diag_printf("serial detected\n");

	
	// 9600 8N1 (done by default)
	// COM2

    diag_printf( "calling open(/dev/ser2)\n");
    ser2 = open("/dev/ser2", O_RDONLY | O_NONBLOCK);
	ser2w = open("/dev/ser2", O_WRONLY);
    
    if( ser2 < 0 || ser2w < 0) 
	{
		diag_printf("open(/dev/ser2) returned error");
		return;
	}

	// echo, look for carrige return
	do
	{

		if(read(ser2,inbuff,1) > 0)
		{

			// debug display data read over serial - yes, this does work
			diag_printf("read %c\n",inbuff[0]);

			write( ser2w, (char*)inbuff, 1);
		}

		if(isbreak())
		{
			close(ser2);
			return TEST_ERR_FAIL;
		}

		
	
	} while(inbuff[0] != '\r');

	close(ser2);
	return TEST_OK_PASS;
}

const char *szTest = "test";
const char *szHelp = "type 'test' to continue\n";
int test_serial2(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
    int ser2,ser2w,i,done,iread;

	char inbuff[64];



	// is serial port connected?
	if((*(cyg_uint8*)PBDR & 0x20) == 0)
	{
		diag_printf("serial not connected\n");
		return TEST_ERR_FAIL;
	}
	else
		diag_printf("serial detected\n");

	
	// 9600 8N1 (done by default)
	// COM2

    ser2 = open("/dev/ser2", O_RDONLY | O_NONBLOCK);
	ser2w = open("/dev/ser2", O_WRONLY);
    
    if( ser2 < 0 || ser2w < 0) 
	{
		diag_printf("open(/dev/ser2) returned error");
		return TEST_ERR_FAIL;
	}
    

	write( ser2w, szHelp, strlen(szHelp));


	// echo, look for carrige return

	for(i = 0; i < strlen(szTest); i++)
	{
		while(read(ser2,inbuff,1) < 1)
		{
			if(isbreak())
			{
				close(ser2);
				return TEST_ERR_FAIL;
			}

			cyg_thread_delay(5);
		}


		write( ser2w, (char*)inbuff, 1);
		if(inbuff[0] != szTest[i])
		{
			close(ser2);
			return TEST_ERR_FAIL;
		}

	}

	close(ser2);
	return TEST_OK_PASS;


}