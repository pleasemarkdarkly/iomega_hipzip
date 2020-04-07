// device_io.c
// temancl@fullplaymedia.com 03/06/02
// (c) fullplay media 
// 
// description:
// low level io routines, printf, string functions


#include "io.h"


#include <string.h>

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


#if 0
#include <termios.h>
#include <sys/time.h> 
#include <sys/types.h> 
#include <sys/select.h> 

static struct termios stored_settings;
#endif
// pretty print
//

// serial control
int ser1;


char szHex[128];
char szAscii[128];
char szTemp[64];

cyg_uint32 uiLineAddr;
cyg_uint32 uiDataAddr;

void shell_io_init()
{

	// set up serial input
#ifdef INTERNAL
	diag_printf( "calling open(/dev/ser1)\n");
    ser1 = open("/dev/ser1", O_RDONLY | O_NONBLOCK );
#else
	diag_printf( "calling open(/dev/ser2)\n");
    ser1 = open("/dev/ser2", O_RDONLY | O_NONBLOCK );
#endif


}


#if 0
int isprint(int c)
{
	if(c >= 32 && c <= 126 && c != 92)
	{
		return 1;
	}

	return 0;
}
#endif

// find next toks point, put a null there, return string for after it
char* strtoken(char* str, const char * toks)
{

	int i,j;

	// until the end of the string

	i = 0;
	while(str[i] != '\0')
	{

		// until there are no more tokens
		j = 0;
		while(toks[j] != '\0')
		{
			if(str[i] == toks[j])
			{
				// token match
				str[i] = '\0';
				return &(str[i+1]);
			}

			j++;
		}

		i++;
	}

	// return a pointer to '\0'
	return &str[i];
}

// pretty print functions
void pprint_start(void* buffer)
{

	uiDataAddr = (cyg_uint32)buffer;
	uiLineAddr = uiDataAddr - (uiDataAddr % 16);

	szAscii[0] = '\0';
	szHex[0] = '\0';

}

void pprint_pad()
{

	// insert spaces in middle
	strcat(szHex,"   ");
	strcat(szAscii," ");
}

void pprint_check()
{
	if((uiDataAddr - uiLineAddr) == 8)
	{
		strcat(szHex," ");
		strcat(szAscii," ");
	}
	else if(uiDataAddr == (uiLineAddr + 16))
	{
		pprint_end();
	}
}


void pprint_end()
{

	if(uiDataAddr == uiLineAddr)
	{
		// no data on this line
		return;
	}


	// are we at the end? pad if we are not
	while(uiDataAddr < (uiLineAddr + 16))
	{
		pprint_pad();
		uiDataAddr++;
	}

	// print out line
	printf("%4.4x.%4.4x | %s %s\n",uiLineAddr >> 16, uiLineAddr & 0xFFFF,szHex,szAscii);	

	// setup for next line
	pprint_start((void*)(uiLineAddr + 16));	

}

void pprint_byte(cyg_uint8 byte)
{

	// append hex value
	sprintf(szTemp,"%2.2x ",byte);
	strcat(szHex,szTemp);

	
	// append 'ascii' value, filtered for printables
	if(isprint(byte))
		sprintf(szTemp,"%c",byte);
	else
		sprintf(szTemp,".");

	strcat(szAscii,szTemp);				

	// increment data pointer
	uiDataAddr++;

	// output checker
	pprint_check();

}

void pprint_halfword(cyg_uint16 half)
{
	cyg_uint8 byte;
	int i;

	// append hex value
	sprintf(szTemp,"%4.4x ",half);
	strcat(szHex,szTemp);

	for(i = 0; i < 2; i++)
	{
		byte = half & 0xFF;

		// append 'ascii' values, filtered for printables
		if(isprint(byte))
			sprintf(szTemp,"%c",byte);
		else
			sprintf(szTemp,".");

		strcat(szAscii,szTemp);				

		// increment data pointer
		uiDataAddr++;

		// output checker
		pprint_check();
		
		half = half >> 8;
	}	

}


void pprint_word(cyg_uint32 word)
{

	cyg_uint8 byte;
	int i;

	// append hex value
	sprintf(szTemp,"%8.8x ",word);
	strcat(szHex,szTemp);

	for(i = 0; i < 4; i++)
	{
		byte = word & 0xFF;

		// append 'ascii' value, filtered for printables
		if(isprint(byte))
			sprintf(szTemp,"%c",byte);
		else
			sprintf(szTemp,".");

		strcat(szAscii,szTemp);				

		// increment data pointer
		uiDataAddr++;

		// output checker
		pprint_check();

		word = word >> 8;
	}


}


// to be implemented, checks for ctrl-c
// non-blocking io is a big pain


void set_keypress(void)
{
#if 0
    struct termios new_settings;

    tcgetattr(fileno(stdin),&stored_settings);

    new_settings = stored_settings;

    /* Disable canonical mode, and set buffer size to 1 byte */
    new_settings.c_lflag &= (~(ICANON | ECHO));
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;

    tcsetattr(fileno(stdin),TCSANOW,&new_settings);
#endif
    return;
}

void reset_keypress(void)
{
#if 0
    tcsetattr(fileno(stdin),TCSANOW,&stored_settings);
#endif
    return;
}


int isready(int fd)
{
#if 0
	int rc;
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(fd,&fds);
    tv.tv_sec = tv.tv_usec = 0;

    rc = select(fd+1, &fds, NULL, NULL, &tv);
    if (rc < 0)
      return -1;

    return FD_ISSET(fd,&fds) ? 1 : 0;
#endif
	return 0;
}


int isbreak()
{

	char c;
	char buff[10];

	
	while(read(ser1,buff,1) > 0)
	{			
		
	//	printf("%x\n",buff[0]);
		
		if(buff[0] == 3)
			return 1;
			
	}

	return 0;
}

unsigned long axtoi(char *hexStg) {
  int n = 0;         // position in string
  int m = 0;         // position in digit[] to shift
  int count;         // loop index
  unsigned long intValue = 0;  // integer value of hex string
  int digit[17];      // hold values to convert
  while (n < 16) {
     if (hexStg[n]=='\0')
        break;
     if (hexStg[n] > 0x29 && hexStg[n] < 0x40 ) //if 0 to 9
        digit[n] = hexStg[n] & 0x0f;            //convert to int
     else if (hexStg[n] >='a' && hexStg[n] <= 'f') //if a to f
        digit[n] = (hexStg[n] & 0x0f) + 9;      //convert to int
     else if (hexStg[n] >='A' && hexStg[n] <= 'F') //if A to F
        digit[n] = (hexStg[n] & 0x0f) + 9;      //convert to int
     else return 0;								// error
    n++;
  }
  count = n;
  m = n - 1;
  n = 0;
  while(n < count) {
     // digit[n] is value of hex digit at position n
     // (m << 2) is the number of positions to shift
     // OR the bits into return value
     intValue = intValue | (digit[n] << (m << 2));
     m--;   // adjust the position to set
     n++;   // next digit to process
  }
  return (intValue);
}


char line[MAX_LINE_LENGTH];

char* getline()
{
	int i = 0;
	char c = getc(stdin);

	// new magic return value
	while((c != 10) && (i < (MAX_LINE_LENGTH - 1)))
	{

		if(c == 0x8)
		{
			if(i > 0)			
			{
#if 0
				putc(stdout,c);
				putc(stdout,' ');
				putc(stdout,c);
#endif

				i--;		
			}
		}
		else
		{
			if(isprint(c))
			{
#if 0
				putc(stdout,c);
#endif
				line[i] = c;
				i++;
			}
		}
		
		c = getc(stdin);
	}

	line[i] = '\0';


	return line;

}

int
strncmpci(const char *s1, const char *s2, int len)
{
    char c1, c2;

    if (len == 0)
        return 0;
    do {
        if ((c1 = tolower(*s1++)) != (c2 = tolower(*s2++)))
            return ((unsigned char)c1 - (unsigned char)c2);
        if (c1 == 0)
            break;
    } while (--len != 0);
    return 0;
}
