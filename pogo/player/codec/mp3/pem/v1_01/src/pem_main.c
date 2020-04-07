#include <stdlib.h>

#include "version.h"
#include "param.h"
#include "types.h"
#include "in.h"
#include "layer3.h"
#include <stdio.h>

struct mpeg mpeg;

static void set_defaults(void)
{
    mpeg.bitrate = 128;
}

static int parse_command(int argc, char** argv)
{
    int i = 0;

    while (++i < argc && argv[i][0] == '-')
        switch (argv[i][1]) {
            case 'b' : mpeg.bitrate = atoi(argv[++i]);
                       break;
            default  : return 0;
       }

	if (argc != i)
		return 0;
    return 1;
}

static int find_bitrate_index(int bitrate)
{
    static long rates[15] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320};
    int i;

	for (i = 0; i < 15; i++)
		if (bitrate == rates[i])
			return i;

    return -1;
}

static void check_config(void)
{
    mpeg.bitrate_index = find_bitrate_index(mpeg.bitrate);
}

bool pem_init(int bitrate)
{
//	set_defaults();
    mpeg.bitrate = bitrate;
    check_config();
	init_param();
	L3_init();
}

void pem_finish()
{
	L3_finish();
}


#if 0
int main(int argc, char **argv)
{
//setlinebuf(stderr);

fprintf(stderr, "pem" VERSION "  -- " VERSION_QUOTE "\n(");
#ifdef DEBUGGING
fprintf(stderr, "debugging");
#else
#ifdef PROFILING
fprintf(stderr, "profiling");
#else
fprintf(stderr, "production");
#endif
#endif
fprintf(stderr, " version)\nCopyright (C) 1998-2001  Segher Boessenkool, Arnhem, The Netherlands\n");
    set_defaults();
    if (!parse_command(argc,argv)) { return -1; }
    in_open();
    check_config();
	init_param();

    L3_compress();
    in_close();

if(0){
	int i;
	extern long long qqqt[];
	long long s = 0;

	for (i = 0; i < 10000; i++)
		fprintf(stderr, "%d: %lld %lld\n", i, qqqt[i], (s += qqqt[i]));

}
    return 0;
} 
#endif

