//==========================================================================
//
//      fileio1.c
//
//      Test fileio system
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// The contents of this file are subject to the Red Hat eCos Public License 
// Version 1.1 (the "License"); you may not use this file except in         
// compliance with the License.  You may obtain a copy of the License at    
// http://www.redhat.com/                                                   
//                                                                          
// Software distributed under the License is distributed on an "AS IS"      
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the 
// License for the specific language governing rights and limitations under 
// the License.                                                             
//                                                                          
// The Original Code is eCos - Embedded Configurable Operating System,      
// released September 30, 1998.                                             
//                                                                          
// The Initial Developer of the Original Code is Red Hat.                   
// Portions created by Red Hat are                                          
// Copyright (C) 1998, 1999, 2000 Red Hat, Inc.                             
// All Rights Reserved.                                                     
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):           nickg
// Contributors:        nickg
// Date:                2000-05-25
// Purpose:             Test fileio system
// Description:         This test uses the testfs to check out the initialization
//                      and basic operation of the fileio system
//                      
//                      
//                      
//                      
//                      
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/hal.h>
#include <pkgconf/kernel.h>
#include <pkgconf/io_fileio.h>

#include <cyg/kernel/kapi.h>
#include <cyg/kernel/ktypes.h>         // base kernel types
#include <cyg/infra/cyg_trac.h>        // tracing macros
#include <cyg/infra/cyg_ass.h>         // assertion macros

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

#include <cyg/fileio/fileio.h>

#include <cyg/infra/testcase.h>
#include <cyg/infra/diag.h>            // HAL polled output

#include <io/storage/blk_dev.h>

//==========================================================================

#if 0
MTAB_ENTRY( ramfs_mte1,
                   "/",
                   "ramfs",
                   "",
                   0);
#endif

//==========================================================================

#define SHOW_RESULT( _fn, _res ) \
diag_printf("<FAIL>: " #_fn "() returned %d %s\n", _res, _res<0?strerror(errno):"");

//==========================================================================

#define IOSIZE  100

#define LONGNAME1       "long_file_name_that_should_take_up_more_than_one_directory_entry_1"
#define LONGNAME2       "long_file_name_that_should_take_up_more_than_one_directory_entry_2"


//==========================================================================

#ifndef CYGPKG_LIBC_STRING

char *strcat( char *s1, const char *s2 )
{
    char *s = s1;
    while( *s1 ) s1++;
    while( (*s1++ = *s2++) != 0);
    return s;
}

#endif

//==========================================================================

static void
mem_stats(void)
{
	struct mallinfo mem_info;		    
	mem_info = mallinfo();
	diag_printf("Memory system: Total=0x%08x Used = 0x%08x Free=0x%08x Max=0x%08x\n",
		    mem_info.arena, mem_info.arena - mem_info.fordblks, mem_info.fordblks,
		    mem_info.maxfree);
}

static void
listalldir(char * name, int depth)
{
	int err;
	DIR * dirp;
	int i;
	
	dirp = opendir(name);
	if (dirp == NULL) SHOW_RESULT(opendir, -1);

	for (;;) {
		struct dirent * entry = readdir(dirp);
		if (entry == 0) {
			break;
		}
		
		for (i = 0; i < depth; ++i) {
			diag_printf("   ");
		}
		
		diag_printf("%s", entry->d_name);
		{
			char fullname[PATH_MAX];
			struct stat sbuf;

			if (name[0]) {
				strcpy(fullname, name);
				if (!(name[0] == '/' && name[1] == 0)) {
					strcat(fullname, "/");
				}
			}
			else {
				fullname[0] = 0;
			}

			strcat(fullname, entry->d_name);

			err = stat(fullname, &sbuf);
			if (err < 0) {
				if (errno == ENOSYS) {
					diag_printf(" <no status available>");
				}
				else {
					SHOW_RESULT(stat, err);
				}
			}
			else {
				diag_printf(" [mode %08x ino %08x nlink %d size %d]",
					    sbuf.st_mode,sbuf.st_ino,sbuf.st_nlink,sbuf.st_size);
			}

			diag_printf("\n");

			if (S_ISDIR(sbuf.st_mode)) {
				if (entry->d_name[0] != '.') {
					listalldir(fullname, depth + 1);
				}
			}
		}
	}

	err = closedir(dirp);
	if (err < 0) {
		SHOW_RESULT(stat, err);
	}
}

static void listdir( char *name, int statp )
{
    int err;
    DIR *dirp;
    
    diag_printf("<INFO>: reading directory %s\n",name);
    mem_stats();

    diag_printf("<INFO>: opening dir\n");
    dirp = opendir( name );
    mem_stats();
    if( dirp == NULL ) SHOW_RESULT( opendir, -1 );

    for(;;)
    {
	    diag_printf("<INFO>: reading dir\n");
	    mem_stats();
	    struct dirent *entry = readdir( dirp );
	    mem_stats();    
        
        if( entry == NULL )
            break;

        diag_printf("<INFO>: entry %s",entry->d_name);
        if( statp )
        {
            char fullname[PATH_MAX];
            struct stat sbuf;

            if( name[0] )
            {
                strcpy(fullname, name );
                if( !(name[0] == '/' && name[1] == 0 ) )
                    strcat(fullname, "/" );
            }
            else fullname[0] = 0;
            
            strcat(fullname, entry->d_name );
            
            err = stat( fullname, &sbuf );
            if( err < 0 )
            {
                if( errno == ENOSYS )
                    diag_printf(" <no status available>");
                else SHOW_RESULT( stat, err );
            }
            else
            {
                diag_printf(" [mode %08x ino %08x nlink %d size %d]",
                            sbuf.st_mode,sbuf.st_ino,sbuf.st_nlink,sbuf.st_size);
            }
        }

        diag_printf("\n");
    }

    err = closedir( dirp );
    if( err < 0 ) SHOW_RESULT( stat, err );
}

//==========================================================================

static void createfile( char *name, size_t size )
{
    char buf[IOSIZE];
    int fd;
    ssize_t wrote;
    int i;
    int err;

    diag_printf("<INFO>: create file %s size %d\n",name,size);

    err = access( name, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );
    
    for( i = 0; i < IOSIZE; i++ ) buf[i] = i%256;
 
    fd = open( name, O_WRONLY|O_CREAT );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    while( size > 0 )
    {
        ssize_t len = size;
        if ( len > IOSIZE ) len = IOSIZE;
        
        wrote = write( fd, buf, len );
        if( wrote != len ) SHOW_RESULT( write, wrote );        

        size -= wrote;
    }

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}

//==========================================================================

#if 0
static void maxfile( char *name )
{
    char buf[IOSIZE];
    int fd;
    ssize_t wrote;
    int i;
    int err;
    size_t size = 0;
    
    diag_printf("<INFO>: create maximal file %s\n",name);

    err = access( name, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );
    
    for( i = 0; i < IOSIZE; i++ ) buf[i] = i%256;
 
    fd = open( name, O_WRONLY|O_CREAT );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    do
    {
        wrote = write( fd, buf, IOSIZE );
        if( wrote < 0 ) SHOW_RESULT( write, wrote );        

        size += wrote;
        
    } while( wrote == IOSIZE );

    diag_printf("<INFO>: file size == %d\n",size);

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}
#endif

//==========================================================================

static void checkfile( char *name )
{
    char buf[IOSIZE];
    int fd;
    ssize_t done;
    int i;
    int err;
    off_t pos = 0;

    diag_printf("<INFO>: check file %s\n",name);
    
    err = access( name, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );

    fd = open( name, O_RDONLY );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    for(;;)
    {
        done = read( fd, buf, IOSIZE );
        if( done < 0 ) SHOW_RESULT( read, done );

        if( done == 0 ) break;

        for( i = 0; i < done; i++ )
            if( buf[i] != i%256 )
            {
                diag_printf("buf[%d+%d](%02x) != %02x\n",pos,i,buf[i],i%256);
                CYG_TEST_FAIL("Data read not equal to data written\n");
            }
        
        pos += done;
    }

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}

//==========================================================================

static void copyfile( char *name2, char *name1 )
{

    int err;
    char buf[IOSIZE];
    int fd1, fd2;
    ssize_t done, wrote;

    diag_printf("<INFO>: copy file %s -> %s\n",name2,name1);

    err = access( name1, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );

    err = access( name2, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );
    
    fd1 = open( name1, O_WRONLY|O_CREAT );
    if( fd1 < 0 ) SHOW_RESULT( open, fd1 );

    fd2 = open( name2, O_RDONLY );
    if( fd2 < 0 ) SHOW_RESULT( open, fd2 );
    
    for(;;)
    {
        done = read( fd2, buf, IOSIZE );
        if( done < 0 ) SHOW_RESULT( read, done );

        if( done == 0 ) break;

        wrote = write( fd1, buf, done );
        if( wrote != done ) SHOW_RESULT( write, wrote );

        if( wrote != done ) break;
    }

    err = close( fd1 );
    if( err < 0 ) SHOW_RESULT( close, err );

    err = close( fd2 );
    if( err < 0 ) SHOW_RESULT( close, err );
    
}

//==========================================================================

static void comparefiles( char *name2, char *name1 )
{
    int err;
    char buf1[IOSIZE];
    char buf2[IOSIZE];
    int fd1, fd2;
    ssize_t done1, done2;
    int i;

    diag_printf("<INFO>: compare files %s == %s\n",name2,name1);

    err = access( name1, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );

    err = access( name1, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );
    
    fd1 = open( name1, O_RDONLY );
    if( fd1 < 0 ) SHOW_RESULT( open, fd1 );

    fd2 = open( name2, O_RDONLY );
    if( fd2 < 0 ) SHOW_RESULT( open, fd2 );
    
    for(;;)
    {
        done1 = read( fd1, buf1, IOSIZE );
        if( done1 < 0 ) SHOW_RESULT( read, done1 );

        done2 = read( fd2, buf2, IOSIZE );
        if( done2 < 0 ) SHOW_RESULT( read, done2 );

        if( done1 != done2 )
            diag_printf("Files different sizes\n");
        
        if( done1 == 0 ) break;

        for( i = 0; i < done1; i++ )
            if( buf1[i] != buf2[i] )
            {
                diag_printf("buf1[%d](%02x) != buf1[%d](%02x)\n",i,buf1[i],i,buf2[i]);
                CYG_TEST_FAIL("Data in files not equal\n");
            }
    }

    err = close( fd1 );
    if( err < 0 ) SHOW_RESULT( close, err );

    err = close( fd2 );
    if( err < 0 ) SHOW_RESULT( close, err );
    
}

//==========================================================================
// main

void
dumpbuf(char * buf, int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		diag_printf("%c", buf[i]);
	}
}

#define NUM_MOUNTS 4
#define NUM_OPENS  8

int main( int argc, char **argv )
{
    int err;
    int fd;
    char buf[512];
    int n;
    int i;
    
    CYG_TEST_INIT();

    // --------------------------------------------------------------

#if 0
    diag_printf("<INFO>: Doing multiple mount, unmounts\n");
    for (i = 0; i < NUM_MOUNTS; ++i) {
	    diag_printf("<INFO>: Mounting\n");
	    err = mount("/dev/hdb/", "/", "cd9660");
	    if (err < 0) SHOW_RESULT(mount, err);
	    mem_stats();

	    diag_printf("<INFO>: Unmounting\n");
	    err = umount("/");
	    if (err < 0) SHOW_RESULT(unmount, err);
	    mem_stats();
    }
#endif
    
    // --------------------------------------------------------------

    do {
	err = mount("/dev/cda/", "/", "cd9660");
	if (err < 0) SHOW_RESULT(mount, err);
    } while (err < 0);

    // --------------------------------------------------------------

#if 0
    diag_printf("<INFO>: Doing multiple open, close\n");
    for (i = 0; i < NUM_OPENS; ++i) {
	    diag_printf("<INFO>: Opening\n");
	    err = open("/READ1ST.TXT", O_RDONLY);
	    if (err < 0) SHOW_RESULT(open, err);
	    mem_stats();

	    diag_printf("<INFO>: Closing\n");
	    err = close(err);
	    if (err < 0) SHOW_RESULT(close, err);
	    mem_stats();
    }
#endif
    
    // --------------------------------------------------------------

#if 0
    err = chdir( "/" );
    if( err < 0 ) SHOW_RESULT( chdir, err );
#endif

    // --------------------------------------------------------------
    
#if 1
    listdir( "/" , true);
#endif
    
    // --------------------------------------------------------------

#if 0
    fd = open("/READ1ST.TXT", O_RDONLY);
    if (fd < 0) {
	    SHOW_RESULT(open, fd);
    }
    else {
	    do {
		    n = read(fd, buf, 512);
		    dumpbuf(buf, n);
	    } while (n == 512);
    }
    close(fd);
#endif

    // --------------------------------------------------------------

#if 1
    listalldir("/", 0);
#endif

    // --------------------------------------------------------------
    
#if 0
    // File and directory miscellaneous

    // Open for writing 
    fd = open("/READ1ST.TXT", O_WRONLY);
    if (fd < 0) {
	    SHOW_RESULT(open, fd);
    }
    else {
	    close(fd);
    }

    // Open for reading and try to write
    fd = open("/READ1ST.TXT", O_RDONLY);
    if (fd < 0) {
	    SHOW_RESULT(open, fd);
    }

    n = write(fd, buf, sizeof(buf));
    if (n < 0) {
	    SHOW_RESULT(open, n);
    }

    close(fd);

    // Open and seek around
    fd = open("/READ1ST.TXT", O_RDONLY);
    if (fd < 0) SHOW_RESULT(open, fd);

    n = read(fd, buf, 128);
    SHOW_RESULT(read, n);
    dumpbuf(buf, n);
    diag_printf("\n\n");
    
    off_t pos = lseek(fd, 0, SEEK_END);
    diag_printf("<INFO>: Seek to end, position is %d\n", (unsigned int)pos);

    n = read(fd, buf, 128);
    SHOW_RESULT(read, n);
    dumpbuf(buf, n);
    diag_printf("\n\n");

#if 0
    pos = lseek(fd, 0, SEEK_SET);
    diag_printf("<INFO>: Seek to beginning, position is %d\n", (unsigned int)pos);

    n = read(fd, buf, 128);
    SHOW_RESULT(read, n);
    dumpbuf(buf, n);
    diag_printf("\n\n");

    pos = lseek(fd, 128, SEEK_SET);
    diag_printf("<INFO>: Seek, position is %d\n", (unsigned int)pos);

    n = read(fd, buf, 128);
    SHOW_RESULT(read, n);
    dumpbuf(buf, n);
    diag_printf("\n\n");
    
    pos = lseek(fd, 256, SEEK_SET);
    diag_printf("<INFO>: Seek, position is %d\n", (unsigned int)pos);

    n = read(fd, buf, 128);
    SHOW_RESULT(read, n);
    dumpbuf(buf, n);
    diag_printf("\n\n");
#else
    for (i = 0; i < 4; ++i) {
	    pos = lseek(fd, i * 128, SEEK_SET);

	    n = read(fd, buf, 128);
	    dumpbuf(buf, n);
    }
    diag_printf("\n\n");
#endif
    //ioctl();

    // Fsync the file
    err = fsync(fd);
    if (err < 0) SHOW_RESULT(fsync, 0);
    
    struct stat statbuf;
    err = fstat(fd, &statbuf);

    close(fd);

    //getinfo();
    //setinfo();
#endif
    
    // --------------------------------------------------------------

    DIR * dirp;
    
#if 1
    diag_printf("<INFO>: Doing multiple opendir, closedir\n");
    for (i = 0; i < NUM_OPENS; ++i) {
	    diag_printf("<INFO>: Opening\n");
	    dirp = opendir("/");
	    if (dirp < 0) SHOW_RESULT(open, dirp);
	    mem_stats();

	    diag_printf("<INFO>: Closing\n");
	    err = closedir(dirp);
	    if (err < 0) SHOW_RESULT(close, err);
	    mem_stats();
    }

    diag_printf("<INFO>: opening dir\n");
    dirp = opendir("/");
    mem_stats();

    diag_printf("<INFO>: reading dir\n");
    struct dirent * entry = readdir(dirp);
    mem_stats();
    diag_printf("<INFO>: %s\n", entry->d_name);

    diag_printf("<INFO>: rewind dir\n");
    rewinddir(dirp);
    mem_stats();

    diag_printf("<INFO>: reading dir\n");
    entry = readdir(dirp);
    mem_stats();
    diag_printf("<INFO>: %s\n", entry->d_name);

    diag_printf("<INFO>: closing dir\n");
    closedir(dirp);
    mem_stats();
#endif
    
    // --------------------------------------------------------------

    CYG_TEST_PASS_FINISH("fileio1");
}

// -------------------------------------------------------------------------
// EOF fileio1.c
