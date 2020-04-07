#include <stdlib.h>

#include "version.h"
#include "in.h"
#include <stdio.h>
#include <io.h>
#include <sys/fcntl.h>

#include "fpmp3.h"

#if DESKTOP
#include "../../../../util/eresult/include/eresult.h"
#endif

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


/*
#if DESKTOP
ERESULT pem_do_out(unsigned char *x, int n)
{
	return(fwrite(x, 1, n, stdout));
}
#else
void pem_do_out(unsigned char *x, int n)
{
	fwrite(x, 1, n, stdout);
}
#endif
*/

int main(int argc, char **argv)
{
	int e_o_s;
	static unsigned char outbuf[2048];
	int bytes_out;
//setlinebuf(stderr);

        // dd: binary mode mojo for cygwin
        setmode( 0, O_BINARY );
        setmode( 1, O_BINARY );

fprintf(stderr, "pem" VERSION "  -- " VERSION_QUOTE "\n");
fprintf(stderr, "Copyright (C) 1998-2002  Segher Boessenkool, Arnhem, The Netherlands\n");
fprintf(stderr, "Copyright (C) 2001       Interactive Objects, Inc.\n");
    set_defaults();
    if (!parse_command(argc,argv)) { return -1; }
    in_open(file_type);
//	freopen("/c/dadio/pemanal/TonalNearSilence.wav","rb",stdin);
	memset(outbuf,0,2048);
	fpmp3_start(bitrate);

	do {
		static short buf[2304];
		int samples_read;
		int i;
		
		samples_read = fread(buf, 4, 1152, stdin);
		
		e_o_s = samples_read < 1152;
		bytes_out = 0;
		fpmp3_encode(buf,outbuf,&bytes_out);
		fwrite(outbuf,1,bytes_out,stdout);
		fprintf(stderr,"%d\n",bytes_out);

	} while (!e_o_s);
	
	fpmp3_finish(outbuf,&bytes_out);
	fwrite(outbuf,1,bytes_out,stdout);
	
	
	
    in_close();
	
    return 0;
} 

