//........................................................................................
//........................................................................................
//.. File Name: fat_test.h																..
//.. Date: 7/25/2000																	..
//.. Author(s): Todd Malsbary															..
//.. Description of content: fat file test								 				..
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 8/16/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <fs/fat/sdapi.h>

#include <stdio.h>

#define STACKSIZE (32 * 4096)

static cyg_handle_t thread;
static cyg_thread thread_obj;
static char stack[STACKSIZE];

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
show_dir(char* dir_name)
{
    DSTAT stat;
    char path[256];
    char dirstr[6];
    unsigned int i;
    char filename[15];
    filename[0] = 0;

    sprintf(path, "%s\\*.*", dir_name);
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

            if( filename[0] == 0 ) {
                strcat( filename, "A:\\" );
                strcat( filename, stat.fname );
                strcat( filename, "." );
                strcat( filename, stat.fext );
            }
            
	    if (!pc_gnext(&stat))
		break;
	}
	pc_gdone(&stat);
    }

}

bool
create_test_file(char* test_file_name)
{
    int i;
    unsigned char buf[1024];
    int fd = po_open(test_file_name, PO_WRONLY | PO_CREAT | PO_TRUNC, PS_IWRITE);
    if (fd >= 0)
    {
        diag_printf("Created file %s\n", test_file_name);
        for (i = 0; i < 1000; ++i)
        {
            po_write(fd, buf, sizeof(buf));
        }
        if (po_close((PCFD)fd) == -1)
        {
            diag_printf("Failed to close file %s: %d\n", test_file_name, pc_get_error(test_file_name[0] - 'A'));
            return false;
        }
        return true;
    }
    else
    {
        diag_printf("Unable to create file %s: %d\n", test_file_name, pc_get_error(test_file_name[0] - 'A'));
        return false;
    }
}

bool
move_test_file(char* src_file_name, char* dest_file_name)
{
    if (pc_mv(src_file_name, dest_file_name))
    {
        diag_printf("Moved file %s to %s\n", src_file_name, dest_file_name);
        return true;
    }
    else
    {
        diag_printf("Unable to move file %s to %s: %d\n", src_file_name, dest_file_name, pc_get_error(src_file_name[0] - 'A'));
        return false;
    }

}

bool
open_test_file(char* test_file_name)
{
    int fd = po_open(test_file_name, PO_RDONLY, PS_IREAD);
    if (fd >= 0)
    {
        diag_printf("Opened file %s\n", test_file_name);
        if (po_close((PCFD)fd) == -1)
        {
            diag_printf("Failed to close file %s: %d\n", test_file_name, pc_get_error(test_file_name[0] - 'A'));
            return false;
        }
        return true;
    }
    else
    {
        diag_printf("Unable to open file %s: %d\n", test_file_name, pc_get_error(test_file_name[0] - 'A'));
        return false;
    }
}

#define LONG_SRC_PATTERN "%s\\long_src_filename_%d"
#define SRC_PATTERN "%s\\src%d"

#define LONG_DST_PATTERN "%s\\long_dst_filename_%d"
#define DST_PATTERN "%s\\dst%d"

void
test_mv(char* test_dira,char* test_dirb)
{
    int i, j;
	int count = 0;
	bool bRunTest = true;
    char source[256], target[256];
    if (!pc_isdir(test_dira))
        if (!pc_mkdir(test_dira))
        {
            diag_printf("Error creating dir %s\n", test_dira);
            return;
        }
    
	if (!pc_isdir(test_dirb))
        if (!pc_mkdir(test_dirb))
        {
            diag_printf("Error creating dir %s\n", test_dirb);
            return;
        }
    show_dir(test_dira);
    
 show_dir(test_dirb);

	while(bRunTest)
	{

		// 50 of each test


		// long->short
		for(i = 0; i < 50; i++)
		{

			diag_printf("long->short %d\n",i);
			
			show_dir(test_dira);

			// sourcepath
			sprintf(source, LONG_SRC_PATTERN, test_dira, count);
			// create long file
			sprintf(target, DST_PATTERN, test_dira, count);
			
			diag_printf("mv %s to %s\n",source,target);
			create_test_file(source);
			move_test_file(source,target);


			diag_printf("check results\n");
			// try to open target file
			open_test_file(target);
			// show target dir
			show_dir(test_dira);

			count++;

		}

		// long->short to seperate dir
		for(i = 0; i < 50; i++)
		{

			diag_printf("long->short to seperate dir %d\n",i);
			
			show_dir(test_dira);

			// sourcepath
			sprintf(source, LONG_SRC_PATTERN, test_dira, count);
			// create long file
			sprintf(target, DST_PATTERN, test_dirb, count);
			
			diag_printf("mv %s to %s\n",source,target);
			create_test_file(source);
			move_test_file(source,target);


			diag_printf("check results\n");
			// try to open target file
			open_test_file(target);
			// show target dir
			show_dir(test_dirb);

			count++;

		}





		// short->long
		for(i = 0; i < 50; i++)
		{

			diag_printf("short->long %d\n",i);
			
			show_dir(test_dira);

			// sourcepath
			sprintf(source, SRC_PATTERN, test_dira, count);
			// create long file
			sprintf(target, LONG_DST_PATTERN, test_dira, count);
			
			diag_printf("mv %s to %s\n",source,target);
			create_test_file(source);
			move_test_file(source,target);


			diag_printf("check results\n");
			// try to open target file
			open_test_file(target);
			// show target dir
			show_dir(test_dira);

			count++;

		}

		// short->long to seperate dir
		for(i = 0; i < 50; i++)
		{

			diag_printf("short->long to seperate dir %d\n",i);
			
			show_dir(test_dira);

			// sourcepath
			sprintf(source, SRC_PATTERN, test_dira, count);
			// create long file
			sprintf(target, LONG_DST_PATTERN, test_dirb, count);
			
			diag_printf("mv %s to %s\n",source,target);
			create_test_file(source);
			move_test_file(source,target);


			diag_printf("check results\n");
			// try to open target file
			open_test_file(target);
			// show target dir
			show_dir(test_dirb);

			count++;

		}

		// short->short to seperate dir
		for(i = 0; i < 50; i++)
		{

			diag_printf("short->short to seperate dir %d\n",i);
			
			show_dir(test_dira);

			// sourcepath
			sprintf(source, SRC_PATTERN, test_dira, count);
			// create long file
			sprintf(target, DST_PATTERN, test_dirb, count);
			
			diag_printf("mv %s to %s\n",source,target);
			create_test_file(source);
			move_test_file(source,target);


			diag_printf("check results\n");
			// try to open target file
			open_test_file(target);
			// show target dir
			show_dir(test_dirb);

			count++;

		}

		// long->long to seperate dir
		for(i = 0; i < 50; i++)
		{

			diag_printf("long->long to seperate dir %d\n",i);
			
			show_dir(test_dira);

			// sourcepath
			sprintf(source, LONG_SRC_PATTERN, test_dira, count);
			// create long file
			sprintf(target, LONG_DST_PATTERN, test_dirb, count);
			
			diag_printf("mv %s to %s\n",source,target);
			create_test_file(source);
			move_test_file(source,target);


			diag_printf("check results\n");
			// try to open target file
			open_test_file(target);
			// show target dir
			show_dir(test_dirb);

			count++;

		}




		bRunTest = false;

	}
	
}

char test_lfndir[] = "A:\\lfntest";
#define LFN_PATTERNA "%s\\t%d"
#define LFN_PATTERNB "%s\\[foo]lon.3.s rc_%d.mp2. 1"
#define LFN_PATTERNC "%s\\fsfjgksjdlfkg%d.e.f.g"
#define LFN_PATTERND "%s\\test%d.mp3"
#define LFN_PATTERNE "%s\\test%d.mp"

static 
size_t strtrunk(const char *s, size_t maxlen)
{
	size_t x;
	for(x=0; (s[x]) && (s[x] != ' ') && (x < maxlen); x++)
		;
	return x;
}

static
char *MakePath(char *target, const char *path, const char *name, const char *ext)
{
	int pathlen = strtrunk(path,EMAXPATH);
	int namelen = strtrunk(name,8);
	int extlen = strtrunk(ext,3);
	char* str = target;
//	memcpy(str, "file://",7);
//	str += 7;
	memcpy(str, path, pathlen);
	str += pathlen;
	memcpy(str,"\\",1);
	str += 1;
	memcpy(str, name, namelen);
	str += namelen;
	*str++ = '.';
	memcpy(str, ext, extlen);
	str += extlen;
	*str = '\0';
	return target;
}

void
lfn_test()
{

	char tempPath[EMAXPATH];
	int i;
  DSTAT ds;

	char file[256];

	if(!pc_isdir(test_lfndir))
	{
        if (!pc_mkdir(test_lfndir))
		{
			diag_printf("error creating test dir\n");
			return;
		}
	}


	for(i = 0; i < 20; i++)
	{
		sprintf(file, LFN_PATTERNA, test_lfndir, i);
		create_test_file(file);
		open_test_file(file);

	}

	show_dir(test_lfndir);


	for(i = 0; i < 20; i++)
	{
		sprintf(file, LFN_PATTERNB, test_lfndir, i);
		create_test_file(file);
		open_test_file(file);

	}

	show_dir(test_lfndir);

	for(i = 0; i < 20; i++)
	{
		sprintf(file, LFN_PATTERNC, test_lfndir, i);
		create_test_file(file);
		open_test_file(file);

	}

	show_dir(test_lfndir);
	for(i = 0; i < 20; i++)
	{
		sprintf(file, LFN_PATTERND, test_lfndir, i);
		create_test_file(file);
		open_test_file(file);

	}

	show_dir(test_lfndir);
	
		for(i = 0; i < 20; i++)
	{
		sprintf(file, LFN_PATTERNE, test_lfndir, i);
		create_test_file(file);
		open_test_file(file);

	}

	show_dir(test_lfndir);
	  if(pc_gfirst(&ds,"A:\\TEST\\*.*"))
	  {
			  do
			  {
					  if(!(ds.fattribute & (ADIRENT | AVOLUME)))
					  {
                          
							  MakePath(tempPath,ds.path,ds.fname,ds.fext);

							  diag_printf("deleting %s\n",tempPath);
							  pc_unlink(tempPath);
					  }

			  } while(pc_gnext(&ds));
	  }

	  pc_gdone(&ds);

}

void
fat_thread(cyg_addrword_t data)
{
    DSTAT stat;
    char * path = "A:\\*.*";
    char dirstr[6];
    unsigned int status;
    unsigned int i;

    char filename[15];
    filename[0] = 0;

	diag_printf("Sandisk fat move test\n");
    
    status = pc_system_init(0);
    if (!status) {
	diag_printf("Could not initialize drive 0\n");
	return;
    }
    diag_printf("Drive 0 successfully initialized\n");

	lfn_test();

    // test_mv("A:\\movetesta","A:\\movetestb");    // Uncomment to test the pc_mv bug.

	diag_printf("Move test complete\n");

	while(1);
}


void
cyg_user_start(void)
{
    cyg_thread_create(10, fat_thread, (cyg_addrword_t)0, "fat_thread",
		      (void *)stack, STACKSIZE, &thread, &thread_obj);
    cyg_thread_resume(thread);
}
