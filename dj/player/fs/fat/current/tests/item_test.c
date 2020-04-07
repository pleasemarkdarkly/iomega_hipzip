//........................................................................................
//........................................................................................
//.. File Name: item_test.c                                                                ..
//.. Date: 7/25/2000                                                                    ..
//.. Author(s): Todd Malsbary                                                            ..
//.. Description of content: fat file test                                                 ..
//.. Last Modified By: Eric Gibbs    ericg@iobjects.com                                    ..    
//.. Modification date: 8/16/2000                                                        ..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.                                      ..
//..     All rights reserved. This code may not be redistributed in source or linkable  ..
//..      object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com                                                ..
//........................................................................................
//........................................................................................

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_arch.h>
#include <fs/fat/sdapi.h>
#include <fs/utils/format_fat32/format.h>
#include <fs/utils/chkdsk_fat32/chkdsk.h>

#include <stdio.h>
#include <stdlib.h>

// Format the drive before starting
//#define PERFORM_FORMAT

// chkdsk
#define PERFORM_CHKDSK_FIRST
//#define PERFORM_CHKDSK_LAST

// List contents of the directory
//#define PERFORM_DIR_LISTING

// Generate a few files and fill them with data
//#define PERFORM_FILE_FILL
// How many files to write?
#define FILL_FILE_COUNT  5
// How many bytes per file?
#define FILL_BYTE_COUNT  ((unsigned int)2*1024*1024*1024 - 32768) // 2gb
// How many bytes per write operation?
#define FILL_BYTES_PER_WRITE (5*1024*1024)
// Perform unlink before writing?
//#define FILL_UNLINK_FIRST
// Read back the file and validate contents?
//#define FILL_VERIFY

// Create an invalid LFN across a cluster boundary.  DOES NOT WORK.
//#define PERFORM_LFN_OVER_CLUSTER

// Stress directory creation
//#define PERFORM_DIR_CREATE
// How many directories deep? Max of ~ 100
#define DIR_DEPTH_COUNT 1
// How many directories wide? No real maximum - make it > 1000 for some fireworks
#define DIR_WIDTH_COUNT 2

// Stress move code
//#define PERFORM_FILE_MOVE
// How many directories deep?
#define MOVE_DIR_DEPTH 3
// Number of files to move
#define MOVE_COUNT     50
// Size of each file (evenly divisible by 128 please)
#define MOVE_FILE_SIZE (6*1024)
// Long directory names?
#define MOVE_LONG_DIR_NAMES
// Short to long filenames?
#define MOVE_SHORT_TO_LONG

// Recording simulator
//#define PERFORM_REC_SIMULATION
//#define REC_FILE_SIZE (4*1024*1024)
#define REC_FILE_SIZE (37 + (4*1024*1024))
#define REC_FILE_COUNT 8000

#ifdef PERFORM_REC_SIMULATION
#include "mediakey_list.h"
#endif

#define STACKSIZE (512 * 4096)
static cyg_handle_t thread;
static cyg_thread thread_obj;
static char stack[STACKSIZE];

// Perform sanity checks on file reads
void test_file( const char* filename );
void multifill_test(char* base, unsigned int bytes_to_write, unsigned int count );

// Perform a fill test to validate writes
int fill_test( const char* base, unsigned int count, int idx );

bool callback(int pass, int phase, int prog, int total)
{
	return true;

}


void make_dirs(char* base,int width,int depth)
{
	int i,j;
	char *newdir;
	newdir = (char*)malloc(4000);

	for(i = 0; i < width; i++)
	{
		// create dir i
		
		sprintf(newdir,"%s/longfilename%dw%d",base,depth,i);

		// reduce output level, hopefully improving perf
		if(i % 100 == 0)
			diag_printf("creating %s\n",newdir);
		
		if(!pc_mkdir(newdir))
		{
			diag_printf("---creating %s failed\n",newdir);
		}
		else
		{
			// create some files in it
			
			multifill_test(newdir,(unsigned)FILL_BYTE_COUNT,FILL_FILE_COUNT);
#if 0
			for( j = 0; j < FILL_FILE_COUNT; j++ ) 
			{
				diag_printf( "Performing single file fill test, file %d of %d (%d bytes)\n", j, FILL_FILE_COUNT, FILL_BYTE_COUNT );
				fill_test(newdir,FILL_BYTE_COUNT, j);
			}
#endif
        

			if(depth > 0)
			{
				// reduce the depth, create another directory batch
				make_dirs(newdir,width,depth-1);
			}	
		}

		

	}

	free(newdir);


}

void dir_test(int depth, int width)
{
	pc_mkdir("A:/TESTDIRE");

	if(depth < 100)
		make_dirs("A:/TESTDIRE",width,depth);
	else
		diag_printf("depth probably too deep - play with buffers\n");

}

void
test_file( const char* filename ) 
{
    PCFD fd = po_open( (char*)filename, PO_RDONLY, PS_IREAD );
    char buf1[20], buf2[20];
    short err;
    
    if( fd < 0 ) {
        diag_printf("Unable to open %s\n", filename );
        return ;
    }

    po_read( fd, buf1, 20 );
    
    po_close( fd );

    fd = po_open( (char*)filename, PO_RDONLY, PS_IREAD );

#if 0
    po_read( fd, buf2,    5 );
    po_read( fd, buf2+5,  5 );
    po_read( fd, buf2+10, 5 );
    po_read( fd, buf2+15, 5 );
#else
    po_lseek( fd, 15, PSEEK_SET, &err );
    po_read( fd, buf2+15, 5 );
    po_lseek( fd, 10, PSEEK_SET, &err );
    po_read( fd, buf2+10, 5 );
    po_lseek( fd, 5, PSEEK_SET, &err );
    po_read( fd, buf2+5, 5 );
    po_lseek( fd, 0, PSEEK_SET, &err );
    po_read( fd, buf2, 5 );
#endif
    po_close( fd );
    
    if( memcmp( buf1, buf2, 20 ) != 0 ) {
        diag_printf("File cmp failed\n");
    }
    else {
        diag_printf("File cmp passed\n");
    }
}


void
multifill_test(char* base, unsigned int bytes_to_write, unsigned int count )
{

	PCFD fd[64];

    static char buf[FILL_BYTES_PER_WRITE];
    static int once = 0;

    char *filename;

	filename = (char*)malloc(4096);
    unsigned int i,bytes_written = 0;

	for(i = 0; i < count; i++)
	{

	    sprintf( filename, "%s/TESTNameLong%02d.BIN", base, i );
		diag_printf("opening %s\n",filename);


#ifdef FILL_UNLINK_FIRST
		pc_unlink( filename );
#endif

		fd[i] = po_open( filename, PO_WRONLY | PO_CREAT, PS_IWRITE );
		if( fd[i] < 0 ) {
			diag_printf( "failed to open file %s for writing\n", filename );
			return ;
		}
	}

    if( !once ) {
        for( i = 0; i < FILL_BYTES_PER_WRITE; i++ ) {
            buf[i] = i & 0xff;
        }
        once = 1;
    }
    while( bytes_written < bytes_to_write ) {
		
		for(i = 0; i < count; i++)
		{
			unsigned int count = FILL_BYTES_PER_WRITE;
			if( count > (bytes_to_write-bytes_written) ) {
				count = (bytes_to_write-bytes_written);
			}
			if( count > po_write( fd[i], buf, count ) )
			{
				diag_printf( "write operation returned less than %d bytes\n", count );
			}
			bytes_written += count;
		}
    }

	for(i = 0; i < count; i++)
	{
		po_close( fd[i] );
	}

	free(filename);
    
}

char* buf = NULL;
int
fill_test( const char* base, unsigned int bytes_to_write, int idx )
{
    //    char buf[FILL_BYTES_PER_WRITE];
    static int once = 0;
    char filename[256];
    PCFD fd;
    unsigned int i,bytes_written = 0;

    if( !buf ) {
        buf = (char*) malloc(FILL_BYTES_PER_WRITE);
        if( !buf ) diag_printf(" failed alloc for %d bytes\n", FILL_BYTES_PER_WRITE );
    }
    sprintf( filename, "%s/t%d.BIN", base, idx );

#ifdef FILL_UNLINK_FIRST
    pc_unlink( filename );
#endif

    fd = po_open( filename, PO_WRONLY | PO_CREAT, PS_IWRITE );
    if( fd < 0 ) {
        diag_printf( "failed to open file %s for writing\n", filename );
        return -1;
    }
    diag_printf(" opened file %s for writing\n", filename );

    if( !once ) {
        for( i = 0; i < FILL_BYTES_PER_WRITE; i++ ) {
            buf[i] = i & 0xff;
        }
        once = 1;
    }
    while( bytes_written < bytes_to_write ) {
		
        unsigned int res, count = FILL_BYTES_PER_WRITE;
        if( count > (bytes_to_write-bytes_written) ) {
            count = (bytes_to_write-bytes_written);
        }
        res = po_write( fd, buf, count );
        if( count > res )
        {
            diag_printf( "write operation returned less than %d bytes (%d)\n", count, res );
            break;
        }
        bytes_written += res;
        
    }
    po_close( fd );
    
#ifdef FILL_VERIFY
    fd = po_open( filename, PO_RDONLY, PS_IREAD );
    if( fd < 0 ) {
        diag_printf( "failed to open file %s for verify\n", filename );
        return -1;
    }

    {
        char tmp[FILL_BYTES_PER_WRITE];
        unsigned int bytes_read = 0;
        while( bytes_read < bytes_to_write ) {
            int i,count = FILL_BYTES_PER_WRITE;
            if( count > (bytes_to_write-bytes_read) ) {
                count = (bytes_to_write-bytes_read);
            }

            if( count > po_read( fd, tmp, count ) ) {
                diag_printf( "read operation returned less than %d bytes\n", count );
            }
            for( i = 0; i < count; i++ ) {
                if( tmp[i] != (i & 0xff) ) {
                    diag_printf( "verify mismatch at index %d\n", bytes_read + i );
                    return -1;
                }
            }
        }
    }
#endif // FILL_VERIFY
    return 0;
}

int
sfn_fill_test( char* base, int bytes_to_write, int idx )
{
    static char buf[FILL_BYTES_PER_WRITE];
    static int once = 0;
    char filename[64];
    PCFD fd;
    int i,bytes_written = 0;

    sprintf( filename, "%s/TEST%02d.BIN", base, idx );

#ifdef FILL_UNLINK_FIRST
    pc_unlink( filename );
#endif

    fd = po_open( filename, PO_WRONLY | PO_CREAT, PS_IWRITE );
    if( fd < 0 ) {
        diag_printf( "failed to open file %s for writing\n", filename );
        return -1;
    }
    diag_printf(" opened file %s for writing\n", filename );

    if( !once ) {
        for( i = 0; i < FILL_BYTES_PER_WRITE; i++ ) {
            buf[i] = i & 0xff;
        }
        once = 1;
    }
    while( bytes_written < bytes_to_write ) {
		
        int count = FILL_BYTES_PER_WRITE;
        if( count > (bytes_to_write-bytes_written) ) {
            count = (bytes_to_write-bytes_written);
        }
        if( count > po_write( fd, buf, count ) )
        {
            diag_printf( "write operation returned less than %d bytes\n", count );
        }
        bytes_written += count;
    }
    po_close( fd );
    
#ifdef FILL_VERIFY
    fd = po_open( filename, PO_RDONLY, PS_IREAD );
    if( fd < 0 ) {
        diag_printf( "failed to open file %s for verify\n", filename );
        return -1;
    }

    {
        char tmp[FILL_BYTES_PER_WRITE];
        int bytes_read = 0;
        while( bytes_read < bytes_to_write ) {
            int i,count = FILL_BYTES_PER_WRITE;
            if( count > (bytes_to_write-bytes_read) ) {
                count = (bytes_to_write-bytes_read);
            }

            if( count > po_read( fd, tmp, count ) ) {
                diag_printf( "read operation returned less than %d bytes\n", count );
            }
            for( i = 0; i < count; i++ ) {
                if( tmp[i] != (i & 0xff) ) {
                    diag_printf( "verify mismatch at index %d\n", bytes_read + i );
                    return -1;
                }
            }
        }
    }
#endif // FILL_VERIFY
    return 0;
}
#ifdef PERFORM_REC_SIMULATION

void
simulate_recording( const char* szPath, int count, int size ) 
{
    char filename[256];
    char dirname[256];
    int i, key=0, x;
    PCFD fd, metakit, progress;
    int* list = mediakey_list;

#if 0
    // generate a dummy cddb install
    pc_mkdir("a:\\cddb");

    // make cddb\ecddb.idx, 77529088 bytes
    x = 77529088;
    fd = po_open( "a:\\cddb\\ecddb.idx", PO_WRONLY | PO_CREAT, PS_IWRITE );
    if( fd < 0 ) {
        diag_printf(" failed to open ecddb.idx\n");
        return ;
    }
    while( x> 0 ) {
        po_write( fd, 4, 32768 );
        x -= 32768;
    }
    po_close( fd );

    // make cddb\ecddb.mdt, 182876160 bytes
    x = 182876160;
    fd = po_open( "a:\\cddb\\ecddb.mdt", PO_WRONLY | PO_CREAT, PS_IWRITE );
    if( fd < 0 ) {
        diag_printf(" failed to open ecddb.mdt\n");
        return ;
    }
    while( x > 0 ) {
        po_write( fd, 4, 32768 );
        x -= 32768;
    }
    po_close( fd );
#endif

    pc_mkdir( "a:\\system" );
    metakit = po_open( "a:\\system\\metakit.dat", PO_WRONLY | PO_CREAT, PS_IWRITE );
    x = 0;
    
    progress = po_open( "a:\\system\\prgrs.dat", PO_WRONLY | PO_CREAT, PS_IWRITE );
    po_truncate( progress, 0 );
    po_write( progress, 4, 32 );
    po_close( progress );
    
    for( i = 0; i < count && list[i]; i++ ) {
        int dirkey = list[i] / 100;
        int filekey = list[i] - (dirkey * 100);  // cheaper than mod
        STAT stat;

        // build target filename
        sprintf( filename, "%s\\d%d\\%d.mp3", szPath, dirkey, filekey );
        if( pc_stat( filename, &stat ) == 0 ) {
            diag_printf(" file %s exists\n", filename );
            continue;
        }
        
        // count of written out files
        x++;

        if( x && ((x % 200) == 0) ) {
            progress = po_open( "a:\\system\\prgrs.dat", PO_WRONLY, PS_IWRITE );
            po_truncate( progress, 0 );
            po_write( progress, 4, 32 );
            po_close( progress );

            diag_printf("***** WROTE %d metakit bytes*****************\n", po_write( metakit, 4, 97000 ));

            progress = po_open( "a:\\system\\prgrs.dat", PO_WRONLY, PS_IWRITE );
            po_truncate( progress, 0 );
            po_write( progress, 4, 32 );
            po_close( progress );
        }
        
        
        sprintf( dirname, "%s\\d%d", szPath, dirkey );
        pc_mkdir( dirname ); // will return an error if already exists, so ignore

        fill_test( dirname, count, filekey );

        sprintf( dirname, "%s\\d%d\\t%d.bin", szPath, dirkey, filekey );
        if( !pc_mv( dirname, filename ) ) {
            diag_printf(" failed to move %s to %s\n", dirname, filename );
        }
        // simulate the reopen done in MoveAndVerify() (debug version)
        fd = po_open( filename, PO_RDONLY, PS_IREAD );
        po_close( fd );

        {
            char buf[2048];
            unsigned short flag;
            // now we feed the file into a codec, which reads up some of the data;
            fd = po_open( filename, PO_RDWR, PS_IREAD | PS_IWRITE );

            // wma does some funky read/seek operations here that will be difficult to simulate, since they vary
            //  depending on the file
            po_read( fd, buf, 374 );
            po_lseek( fd, SEEK_SET, 370, &flag );
            po_read( fd, buf, 1024 );

            // mp3 seeks to the end, reads off the last 32 bytes, then seeks back to the start
            // po_lseek( fd, SEEK_END, -32, &flag );
            // po_read( fd, buf, 32 );
            // po_lseek( fd, SEEK_SET, 0, &flag );
        
            // findandparsetag does the following:
            //  seek( end, - x )
            //  read( x )
            //  seek( start, 0 )
            //  seek( start, 0 )

            po_lseek( fd, SEEK_END, -13, &flag );
            if( po_read( fd, buf, 13 ) < 13 ) {
                printf(" read came up short\n");
            }
            po_lseek( fd, SEEK_SET, 0, &flag );
            po_lseek( fd, SEEK_SET, 0, &flag );
            po_close( fd );
        }
    }

    po_close( metakit );
    
    // needs to be done since we use fill_test
    if( count && buf ) free( buf );
}
#endif // PERFORM_REC_SIMULATION

void
fat_thread(cyg_addrword_t data)
{
	unsigned int status;


#ifdef PERFORM_CHKDSK_FIRST

    chkdsk_fat32("/dev/hda/",callback);

#endif // PERFORM_CHKDSK_FIRST

#ifdef PERFORM_FORMAT
    diag_printf("Formatting drive /dev/hda/\n");
    format_drive( "/dev/hda/", 0 );
    
    diag_printf("Press any key to continue...\n");
    //    fgetc(stdin);
#endif // PERFORM_FORMAT
    
    status = pc_system_init(0);
    if (!status) {
        diag_printf("Could not initialize drive 0\n");
        return;
    }
    diag_printf("Drive 0 successfully initialized\n");

#ifdef PERFORM_DIR_LISTING
    {
        DSTAT stat;
        char * path = "A:\\*.*";
        char dirstr[6];
        unsigned int i;
     
        diag_printf("Printing directory contents\n");
        if (pc_gfirst(&stat, path)) {
            for (i = 0;;) {
                ++i;
                if (stat.fattribute & AVOLUME) {
                    strcpy(dirstr, "<VOL>");
                }
                else if (stat.fattribute & ADIRENT) {
                    strcpy(dirstr, "<DIR>");
                }
                else {
                    strcpy(dirstr, "     ");
                }
                printf("%-8s.%-3s %7ld %5s %02d-%02d-%02d %02d:%02d\n",
                    &(stat.fname[0]),
                    &(stat.fext[0]),
                    stat.fsize,
                    dirstr,
                    (stat.fdate >> 5) & 0xf,
                    (stat.fdate & 0x1f),
                    (80 + (stat.fdate >> 9)) & 0xff,
                    (stat.ftime >> 11) & 0x1f,
                    (stat.ftime >> 5) & 0x3f);

                if (!pc_gnext(&stat))
                    break;
            }
            pc_gdone(&stat);
        }
        
        diag_printf("Press any key to continue...\n");
        //        fgetc(stdin);
    }
#endif // PERFORM_DIR_LISTING
    
#ifdef PERFORM_FILE_FILL
    {
        int i;
        const char* dirname = "A:/C/Recordings";
        if( !pc_mkdir( "A:/C" ) ) {
            diag_printf(" could not make directory A:/C\n");
        }
        if( !pc_mkdir( (TEXT*)dirname ) ) {
            diag_printf(" could not make directory dirname\n");
        }

        for( i = 0; i < FILL_FILE_COUNT; i++ ) 
        {
            diag_printf( "Performing single file fill test, file %d of %d (%u bytes)\n", i+1, FILL_FILE_COUNT, (unsigned)FILL_BYTE_COUNT );
            if( fill_test( dirname , (unsigned)FILL_BYTE_COUNT, i) < 0 ) {
                break;
            }
        }
        if( buf ) free( buf );
        diag_printf("Press any key to continue...\n");
        //        fgetc(stdin);
    }
#endif // PERFORM_FILE_FILL

#ifdef PERFORM_LFN_OVER_CLUSTER
    {
        /* This test is designed to create a bad LFN entry.
           It currently looks like FAT is forgetting to write out a SFN for the LFN
           entries when the LFN entries belong in the previous cluster. */
        
        int i;
        const char* dirname = "A:/C/Recordings";
        pc_mkdir( "A:/C" );
        pc_mkdir( dirname );

        /* 1365 LFN entries cause the problem. */
        for( i = 0; i < 1366; ++i )
        {
            diag_printf( "Performing LFN single file fill test, file %d of %d (%d bytes)\n", i+1, FILL_FILE_COUNT, FILL_BYTE_COUNT );
            if( fill_test( dirname, FILL_BYTE_COUNT, i ) < 0 )
            {
                break;
            }
        }
        diag_printf("Press any key to continue...\n");
    }
#endif // PERFORM_LFN_OVER_CLUSTER

    
#ifdef PERFORM_DIR_CREATE
    {
		diag_printf("Directory create test\n");
        dir_test( DIR_DEPTH_COUNT, DIR_WIDTH_COUNT );
        diag_printf("\nPress any key to continue...\n");
        //        fgetc(stdin);
    }
#endif // PERFORM_DIR_CREATE

#ifdef PERFORM_FILE_MOVE
    {
        int i;
        char path[256], file1[256], file2[256], fname[256], dest[256];
        char* subdir_name;

        diag_printf( "Performing file move test, %d files of %d bytes each\n", MOVE_COUNT, MOVE_FILE_SIZE );
#ifdef MOVE_LONG_DIR_NAMES
        sprintf( path, "A:/Move test dir" );
        subdir_name = "Move test subdir";
#else
        sprintf( path, "A:/MVTEST" );
        subdir_name = "MVSUB";
#endif // MOVE_LONG_DIR_NAMES

        // Build the directories out
        if( !pc_mkdir( path ) ) {
            diag_printf("failed to make directory %s\n", path );
            return ;
        }
        for( i = 1; i < MOVE_DIR_DEPTH; i++ ) {
            strcat( path, "/" );
            strcat( path, subdir_name );
            if( !pc_mkdir( path ) ) {
                diag_printf("failed to make directory %s\n", path );
                return ;
            }
        }

        strcat( path, "/" );
        strcpy( file1, path );
        strcpy( file2, path );

#ifdef MOVE_SHORT_TO_LONG
        strcat( file1, "TEST" );
        strcat( file2, "Test File " );
#else
        strcat( file1, "TEST" );
        strcat( file2, "TEST" );
#endif // MOVE_SHORT_TO_LONG

        for( i = 0; i < MOVE_COUNT; i++ ) {
            // Generate a file
            PCFD fd;
            char buf[128];
            int z;

            sprintf( fname, "%s%d.BIN", file1, i );
            sprintf( dest, "%s%d.BIN", file2, i );
            
            fd = po_open( fname, PO_WRONLY | PO_CREAT, PS_IWRITE );
            if( fd < 0 ) {
                diag_printf("error opening file %s\n", fname );
                continue;
            }

            // Fill
            for( z = 0; z < MOVE_FILE_SIZE; z + 128 ) {
                if( po_write( fd, buf, 128 ) < 128 ) {
                    diag_printf("failed to write 128 bytes to file %d\n", i );
                }
            }

            po_close( fd );

            // Move
            if( !pc_mv( fname, dest ) ) {
                diag_printf("failed to move %s to %s\n", fname, dest );
            }
            
        }
        diag_printf("Press any key to continue...\n");
        //        fgetc(stdin);
    }
#endif // PERFORM_FILE_MOVE
    
#ifdef PERFORM_REC_SIMULATION
    // Simulate the recording system in the app
    // Build out a content directory and a simulated UDN
    {
        char* dirname = "A:\\c\\f488";
        pc_mkdir("A:\\c");
        pc_mkdir( dirname );
    
        simulate_recording( dirname, REC_FILE_COUNT, REC_FILE_SIZE );
    }

#endif // PERFORM_REC_SIMULATION
    
    pc_system_close(0);

#ifdef PERFORM_CHKDSK_LAST

	chkdsk_fat32("/dev/hda/",callback);

#endif // PERFORM_CHKDSK_LAST

    diag_printf("Test finished\n");

}


void
cyg_user_start(void)
{
    cyg_thread_create(10, fat_thread, (cyg_addrword_t)0, "fat_thread",
        (void *)stack, STACKSIZE, &thread, &thread_obj);
    cyg_thread_resume(thread);
}
