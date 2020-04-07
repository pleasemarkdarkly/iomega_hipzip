// THIS FILE IS NOT CURRENTLY USED BY EMBEDDED PEM

//noise gate filtering:
//#define GATE 60

#include <stdio.h>
#include <math.h>

#include "types.h"
#include "in.h"


void swap2(unsigned short *x)
{
#ifdef BIGENDIAN
	*x = ((*x & 0xff) << 8) | ((*x & 0xff00) >> 8);
#else
	(void)x;
#endif
}

void swap4(unsigned long *x)
{
#ifdef BIGENDIAN
	*x = ((*x & 0xff) << 24) | ((*x & 0xff00) << 8) | ((*x & 0xff0000) >> 8) | ((*x & 0xff000000) >> 24);
#else
	(void)x;
#endif
}

static void ERROR(char *s)
{
	fprintf(stderr, "ERROR: %s\n", s);
	exit(1);
}


static int checkString(char *string)
{
    char temp[1024];
    int  length = strlen(string);
    if (fread(temp, 1, length, stdin) < (unsigned)length)
	ERROR("Premature EOF in input !?!");
    temp[length] = (char)0;
    return !strcmp(temp,string);
}

void in_close(void)
{
    fclose(stdin);
}

static int wave_open(void)
{
    unsigned long filesize;
    unsigned long header_size;
    unsigned short wFormatTag;
    unsigned short channels;
    unsigned long samplerate;
    unsigned long dAvgBytesPerSec;
    unsigned short wBlockAlign;
    unsigned short bits;
    unsigned long dummy;

    if(!checkString("RIFF")) ERROR("Input not a MS-RIFF file");
    fread(&filesize, 4, 1, stdin);

    if(!checkString("WAVE")) ERROR("Input not WAVE audio");

    if(!checkString("fmt "))  ERROR("Can't find format chunk");
    fread(&header_size, 4, 1, stdin);

    fread(&wFormatTag, 2, 1, stdin);
swap2(&wFormatTag);
    if(wFormatTag!=0x0001) ERROR("Unknown WAVE format");

    fread(&channels, 2, 1, stdin);
swap2(&channels);
    if(channels!=2) ERROR("only stereo supported\n");

    fread(&samplerate, 4, 1, stdin);
swap4(&samplerate);
    if (samplerate!=44100) ERROR("only 44100 Hz supported");

    fread(&dAvgBytesPerSec, 4, 1, stdin);
    fread(&wBlockAlign, 2, 1, stdin);

    fread(&bits, 2, 1, stdin);
swap2(&bits);
    if(bits != 16) ERROR("NOT 16 Bit !!!\n");

    if(!checkString("data")) ERROR("Can't find data chunk");

	fread(&dummy, 4, 1, stdin);

    return 1;
}


int in_open(char type)
{
	switch(type) {
		case 'r':
			return 1;
		case 'w':
			return wave_open();
		default:
			ERROR("input type not supported");
	}
	return 0;
}


void in_do(short *x, int n)
{
	int j;

//mpeg.ms_stereo = 1;


#ifdef GATE
	for (j = 0; j < 2 * n; j++)
		if (x[j] <= GATE && x[j] >= -GATE)
			x[j] = 0;
#endif


#if 0
	if (mpeg.ms_stereo) {
		short *t = x;
		short *b = buffer;
		for (j = 0; j < n; j++) {
			int t0 = ((int)*t++);
			int t1 = ((int)*t++);
			*b++ = (t0 + t1) >> 1; // losing the low bit here!
			*b++ = (t0 - t1) >> 1;
		}
	} else {
		for (j = 0; j < n; j++) {
			buffer[2*j]   = ((int)x[2*j]); // << 16;
			buffer[2*j+1] = ((int)x[2*j+1]); // << 16;
		}
	}

	for ( ; j < 1152; j++) {
		buffer[2*j]   = 0;
		buffer[2*j+1] = 0;
	}
#endif
}

