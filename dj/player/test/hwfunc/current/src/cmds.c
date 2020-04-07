// cmds.c
// temancl@fullplaymedia.com 03/06/02
// (c) fullplay media 
// 
// description:
// internal monitor commands

#include "parser.h"
#include "io.h"
#include "cmds.h"
#include <cyg/hal/hal_edb7xxx.h>


// definitions, limits
#define MIN_VERBOSITY 0
#define MAX_VERBOSITY 3
#define DEFAULT_VERBOSITY 3

// read 64 bytes by default
#define DEFAULT_READ 64
#define DEFAULT_WRITE 1

#define MAX_WRITE_PRINT 64

#define WORDSIZE 4
#define HALFSIZE 2
#define BYTESIZE 1


extern cyg_uint8 I2CRead(cyg_uint8 Address, cyg_uint8 Register);
// state variables
cyg_uint8 verbosity = DEFAULT_VERBOSITY;

// display temperature
int test_temp(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	char temp;
	temp = I2CRead(0x9A,0);
	DEBUG1("Current Temp = %d C\n",temp);


	return TEST_OK_PASS;
}

int test_reset(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	DEBUG1("Resetting...\n");
	*(volatile cyg_uint8 *)PDDR &= ~0x1;
	*(volatile cyg_uint8 *)PDDDR &= ~0x1;
    *(volatile cyg_uint8 *)PDDR |= 0x1;

	// shouldn't reach this code.. 
	return TEST_ERR_FAIL;
}


// useful commands
int test_gpio(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	DEBUG2("GPIO Set\n");

	if(!(((param_nums[0] >= 0xA) && (param_nums[0] <= 0xE)) && 
	   ((param_nums[1] >= 0) && (param_nums[1] <= 7)) && 
	   ((param_nums[2] == 0) || (param_nums[2] == 1)) &&
	   ((param_nums[3] == 0) || (param_nums[3] == 1))))	   
	{
		printf("error: invalid parameters\n");
		return TEST_ERR_PARAMS;
	}

	volatile cyg_uint8 *pvalbank;
	volatile cyg_uint8 *pdirbank; 
	cyg_uint8 banknum = param_nums[0];
	cyg_uint8 line = param_nums[1];
	cyg_uint8 dir = param_nums[2];
	cyg_uint8 val = param_nums[3];
	char cbank;


	switch(banknum)
	{
		case 0xA:
			cbank = 'A';
			pvalbank = (volatile cyg_uint8 *)PADR;
			pdirbank = (volatile cyg_uint8 *)PADDR;
			break;
	
		case 0xB:
			cbank = 'B';
			pvalbank = (volatile cyg_uint8 *)PBDR;
			pdirbank = (volatile cyg_uint8 *)PBDDR;
			break;

		case 0xD:
			cbank = 'D';
			pvalbank = (volatile cyg_uint8 *)PDDR;
			pdirbank = (volatile cyg_uint8 *)PDDDR;
			break;

		case 0xE:
			cbank = 'E';
			pvalbank = (volatile cyg_uint8 *)PEDR;
			pdirbank = (volatile cyg_uint8 *)PEDDR;
			break;

		default:
			return TEST_ERR_PARAMS;
			break;
	}

	DEBUG3("OLD GPIO Bank %c Data = %2.2x Dir = %2.2x\n",cbank,*(volatile cyg_uint8 *)pvalbank,*(volatile cyg_uint8 *)pdirbank);

	if(dir)
		*(volatile cyg_uint8 *)pdirbank |= (1 << line);
	else
		*(volatile cyg_uint8 *)pdirbank &= ~(1 << line);

	if(val)
		*(volatile cyg_uint8 *)pvalbank |= (1 << line);
	else
		*(volatile cyg_uint8 *)pvalbank &= ~(1 << line);
		
	DEBUG3("NEW GPIO Bank %c Data = %2.2x Dir = %2.2x\n",cbank,*(volatile cyg_uint8 *)pvalbank,*(volatile cyg_uint8 *)pdirbank);

	return TEST_OK_PASS;
}



// commands

int test_read(char param_strs[][MAX_STRING_LEN],int* param_nums,int width)
{

	unsigned long ulStart = 0;
	cyg_uint32 *wstart = 0;
	cyg_uint32 wtemp = 0;
	cyg_uint16 *hstart = 0;
	cyg_uint16 htemp = 0;
	cyg_uint8  *bstart = 0;
	cyg_uint8  btemp = 0;
	unsigned long len,i;
	
	if(param_nums[0] == 0)
	{
		printf("error: invalid parameters\n");
	}


	// setup start pointer, align if neccesary
	switch(width)
	{
		case WORDSIZE:
			wstart = (cyg_uint32*)param_nums[0];
			(cyg_uint32)wstart -= ((cyg_uint32)wstart % WORDSIZE);
			ulStart = (cyg_uint32)wstart;
			break;
		case HALFSIZE:
			hstart = (cyg_uint16*)param_nums[0];
			(cyg_uint32)hstart -= ((cyg_uint32)hstart % HALFSIZE);
			ulStart = (cyg_uint32)hstart;
			break;
		case BYTESIZE:
			bstart = (cyg_uint8*)param_nums[0];				
			ulStart = (cyg_uint32)bstart;
			break;
		default:
			printf("error: invalid data width\n");
			return TEST_ERR_PARAMS;
	}

	if(param_nums[1] == 0)
	{
		len = DEFAULT_READ / width;
	}
	else
	{	
		len = param_nums[1];
	}

	switch(verbosity)
	{
		case 3:
			pprint_start((void*)ulStart);
		case 2:
			printf("Reading %u %d-byte values starting at 0x%x\n",len,width,ulStart);
			break;
		default:
			break;
	}
		

	switch(width)
	{
		case WORDSIZE:
			for(i = 0; i < len; i++)
			{
				wtemp = wstart[i];
				if(verbosity == 3)
					pprint_word(wtemp);
			}
			break;
		case HALFSIZE:
			for(i = 0; i < len; i++)
			{
				htemp = hstart[i];
				if(verbosity == 3)
					pprint_halfword(htemp);
			}
			break;
		case BYTESIZE:
			for(i = 0; i < len; i++)
			{
				btemp = bstart[i];
				if(verbosity == 3)
					pprint_byte(btemp);
			}
			break;		
		default:
			printf("error: invalid data width\n");
			return TEST_ERR_PARAMS;
	}


	switch(verbosity)
	{
		case 3:
			pprint_end();
		default:
			break;
	}



}

int test_write(char param_strs[][MAX_STRING_LEN],int* param_nums,int width)
{

	unsigned long ulStart = 0;

	cyg_uint32 *wstart = 0;
	cyg_uint32 wtemp = 0;
	cyg_uint16 *hstart = 0;
	cyg_uint16 htemp = 0;
	cyg_uint8  *bstart = 0;
	cyg_uint8  btemp = 0;
	cyg_uint32 mask;
	unsigned long len,i;

	
	mask = 0;
	for(i = 0; i < width; i++)
	{
		mask = mask << 8;
		mask |= 0xFF;
	}
	
	
	if(param_nums[0] == 0)
	{
		printf("error: invalid parameters\n");
		return TEST_ERR_PARAMS;
	}


	// setup start pointer, align if neccesary, get pattern
	switch(width)
	{
		case WORDSIZE:
			wstart = (cyg_uint32*)param_nums[0];
			(cyg_uint32)wstart -= ((cyg_uint32)wstart % WORDSIZE);
			wtemp = (cyg_uint32)param_nums[1];
			ulStart = (cyg_uint32)wstart;
			break;
		case HALFSIZE:
			hstart = (cyg_uint16*)param_nums[0];
			(cyg_uint32)hstart -= ((cyg_uint32)hstart % HALFSIZE);
			htemp = (cyg_uint16)param_nums[1];
			ulStart = (cyg_uint32)hstart;
			break;
		case BYTESIZE:
			bstart = (cyg_uint8*)param_nums[0];				
			btemp = (cyg_uint8)param_nums[1];
			ulStart = (cyg_uint32)bstart;
			break;
		default:
			printf("error: invalid data width\n");
			return TEST_ERR_PARAMS;
	}


	if(param_nums[2] == 0)
	{
		len = DEFAULT_WRITE;
	}
	else
	{	
		len = param_nums[2];
	}

	switch(verbosity)
	{
		case 3:
			pprint_start((void*)ulStart);
		case 2:
			printf("Writing %x for %u %d-byte values starting at 0x%x\n",param_nums[1] & mask,len,width,ulStart);
			break;
		default:
			break;
	}


	switch(width)
	{
		case WORDSIZE:
			for(i = 0; i < len; i++)
			{
				wstart[i] = wtemp;
				if(verbosity == 3 && (i < (MAX_WRITE_PRINT / width)))
					pprint_word(wtemp);
			}
			break;
		case HALFSIZE:
			for(i = 0; i < len; i++)
			{
				hstart[i] = htemp;
				if(verbosity == 3 && (i < (MAX_WRITE_PRINT / width)))
					pprint_halfword(htemp);
			}
			break;
		case BYTESIZE:
			for(i = 0; i < len; i++)
			{
				bstart[i] = btemp;
				if(verbosity == 3 && (i < (MAX_WRITE_PRINT / width)))
					pprint_byte(btemp);
			}
			break;		
		default:
			printf("error: invalid data width\n");
			return TEST_ERR_PARAMS;
	}


	switch(verbosity)
	{
		case 3:
			pprint_end();
		default:
			break;
	}
}

int test_readwords(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	return test_read(param_strs,param_nums,4);
}

int test_writewords(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	return test_write(param_strs,param_nums,4);
}


int test_readhalfwords(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	return test_read(param_strs,param_nums,2);
}

int test_writehalfwords(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	return test_write(param_strs,param_nums,2);
}

int test_readbytes(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	return test_read(param_strs,param_nums,1);
}

int test_writebytes(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	return test_write(param_strs,param_nums,1);
}

int test_setverbosity(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	if((param_nums[0] >= MIN_VERBOSITY) && (param_nums[0] <= MAX_VERBOSITY))
	{
		verbosity = param_nums[0];
		printf("Verbosity set to %d\n",verbosity);
	}
	else
	{
		printf("Verbosity is %d\n",verbosity);
	}

	return TEST_OK_PASS;

}

int test_help(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	int i = 0;

	printf("command list:\n");
	for(i = 0; test_list[i].func != NULL; i++)
	{
		printf("cmd: %s\t||\t%s\n",test_list[i].name, test_list[i].help);
	}
	
	return TEST_OK_PASS;

}



int test_rem(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	// do nothing, accept any parameters
	// this is the whole implementation
	return TEST_OK_PASS;
}


int test_echo(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	if(param_strs[0][0] == '\0')
	{
		printf("error: echo not passed string\n");
		return TEST_ERR_PARAMS;
	}

	printf("'%s'\n",param_strs[0]);

	return TEST_OK_PASS;

}


int test_memtest(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	if(param_nums[0] == 0)
	{
		printf("error: invalid parameters\n");
		return TEST_ERR_PARAMS;
	}

	return memtest(param_nums[0]);
}

int memtest(unsigned long nBytes)
{

	unsigned long offset;
    unsigned long nWords;
    unsigned long nErrorCount = 0;

    cyg_uint32 pattern;
    cyg_uint32 antipattern;
	cyg_uint32 test;
	cyg_uint32* baseAddress;


	// align byte count
	(cyg_uint32)nBytes -= ((cyg_uint32)nBytes % WORDSIZE);		

	// try and reserve memory for test
	baseAddress = (cyg_uint32*)malloc(nBytes);

	if(baseAddress == NULL)
	{
		DEBUG2("Couldn't reserve memory for test\n");
		return TEST_ERR_FAIL;
	}


	DEBUG2("memtest: testing %d bytes at 0x%8.8x\n",nBytes,baseAddress);
	
    nWords = nBytes / 4;

    /*
     * Fill memory with a known pattern.
     */
 
	/*
	 * Fill memory with a known pattern.
	 */
	for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
	{
		baseAddress[offset] = pattern;
	}

	/*
	 * Check each location and invert it for the second pass.
	 */
	for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
	{
		test = baseAddress[offset];
		
		if (test != pattern)
		{
			DEBUG3("memtest: error at addr 0x%8.8x read 0x%8.8x, expected 0x%8.8x\n",&baseAddress[offset],test,pattern);

			nErrorCount++;
			
			if(nErrorCount == 64)
			{
				DEBUG2("memtest: error count exceeded, exiting\n");
				
				free(baseAddress);
				return TEST_ERR_FAIL;
			}

		}

		antipattern = ~pattern;
		baseAddress[offset] = antipattern;
	}

	/*
	 * Check each location for the inverted pattern and zero it.
	 */
	for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
	{
		antipattern = ~pattern;
		test = baseAddress[offset];

		if (test != antipattern)
		{
			DEBUG3("memtest: error at addr 0x%8.8x read 0x%8.8x, expected 0x%8.8x\n",&baseAddress[offset],test,antipattern);
			
			nErrorCount++;

			if(nErrorCount == 64)
			{

				DEBUG2("memtest: error count exceeded, exiting\n");
				free(baseAddress);
				return TEST_ERR_FAIL;
			}

		}
	}

	if(nErrorCount > 0)
	{
		DEBUG2("memtest: failed - %d errors detected\n",nErrorCount);
	}
	else
	{
		DEBUG2("memtest: pass");
	}

	free(baseAddress);
    return TEST_OK_PASS;

	

}
