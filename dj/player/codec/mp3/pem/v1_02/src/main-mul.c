#include <stdlib.h>

#include "version.h"
#include "in.h"
#include "codec.h"
#include <stdio.h>


static char file_type = 'w';
int bitrate;


static void set_defaults(void)
{
    bitrate = 128;
}

static int parse_command(int argc, char** argv)
{
    int i = 0;

    while (++i < argc && argv[i][0] == '-')
        switch (argv[i][1]) {
            case 'b' : bitrate = atoi(argv[++i]);
                       break;
            case 'r' : file_type = 'r';
                       break;
            default  : return 0;
       }

	if (argc != i)
		return 0;
    return 1;
}


void pem_do_out(unsigned char *x, int n)
{
	fwrite(x, 1, n, stdout);
}


int main(int argc, char **argv)
{
FILE *fp;
	int e_o_s;
int loop;
for (loop = 0; loop < 2; loop++) {
fp = fopen("/data/wav/stil.wav", "rb");

setlinebuf(stderr);

fprintf(stderr, "pem" VERSION "  -- " VERSION_QUOTE "\n");
fprintf(stderr, "Copyright (C) 1998-2001  Segher Boessenkool, Arnhem, The Netherlands\n");
fprintf(stderr, "Copyright (C) 2001       Interactive Objects, Inc.\n");
    set_defaults();
    if (!parse_command(argc,argv)) { return -1; }
    in_open(file_type);


	if (pem_init(bitrate) == 0) {

		do {
			static short buf[2304];
			int samples_read;
			int i;
			int samples_processed;

			samples_read = fread(buf, 4, 1152, fp);
	    
			e_o_s = samples_read < 1152;

			for (i = 0; i < 2 * samples_read; i++)	// this just fixes endianness
				swap2(buf + i);

			samples_processed = pem_work(buf, samples_read, e_o_s);
			if (samples_processed < samples_read)
				fprintf(stderr, "? ? ?\n");	// either the MPEG spec changed
								// (not bloody likely) or we have
								// an error condition (like, not feeding
								// pem enough data (which should not happen)
		} while (!e_o_s);

		pem_fini();

	}


    in_close();
}

    return 0;
} 

