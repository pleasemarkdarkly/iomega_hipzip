/*
* SanDisk Host Developer's Toolkit
*
* Copyright EBS Inc. 1996
* Copyright (c) 1996 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
*
*/
/*
*****************************************************************************
    REGRESS.C   - RTFS Excerciser

     This program performs two functions. It calls virtually all of the API 
     routines plus it stress tests the system for driver bugs and memory leaks.
     It works by repeatedly opening a disk and then entering an inner loop
     which creates a directory and then creates N subdirectories below that.
     Finally the inner loop creates NUSERFILES files, writes to them, reads from
     them, seeks, truncates, closes, renames and deletes them. Along the way
     it check set current working directory and get working directory. Finally
     the inner loop deletes all of the subdirectories it created and compares 
     the current disk free space to the free space before it started. These
     should be the same. After the inner loop completes the outer loop closes
     the drive and then reopens it to continue the test.

     There are a few functions that do not get tested, they are:
        pc_gfirst
        pc_gnext
        pc_gdone
        pc_abort

     Not all modes of po_open and po_lseek are tested either. Nor does it 
     test your port in multitasking mode. You may modify this program and
     run it in multiple threads if you wish.

     The following parameters may be changed:

        USEPRINTF -  Set this to zero to run completely quietly. If this
                     is done you should set a break point in regress_error()
                     to catch errors.
        test_disk[] - The drive where the test will occur.
        test_dir[]   - The directory where the test will occur
        INNERLOOP   - The Number of times we run the inner loop
        OUTERLOOP   - The Number of times we run the inner loop
        SUBDIRDEPTH  - The depth of the tested subdirectories.
        NSUBDIRS     - The number of subdirectories below test_dir. Must
                       be less then 26. Each of these directories will 
                       have SUBDIRDEPTH subdirectories below it.


To run the program type REGRESS.

*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fs/fat/sdapi.h>

#include <errno.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>

#define RGE_FLTDRIVE       3
#define RGE_FREEERROR      4
#define RGE_LEAKERROR      5
#define RGE_MKDIR          6
#define RGE_SCWD           7
#define RGE_MKDIRERR       9
#define RGE_PWD           10
#define RGE_RMDIR         11
#define RGE_DSKCLOSE      12
#define RGE_OPEN          13
#define RGE_SEEK          14
#define RGE_WRITE         15
#define RGE_READ          16
#define RGE_TRUNC         17
#define RGE_FLUSH         18
#define RGE_CLOSE         19
#define RGE_UNLINK        20
#define RGE_MV            21

/* Porting issues */
/* Set this to zero if you don't have printf */
#define USEPRINTF 1
/* Set this to 1 if you have printf and want to see more */
#define VERBOSE   1

#define DONT_UNLINK

TEXT test_disk[128];      /* The drive where the test will occur */
#define NLONGS 512        /* Longs to write in file write test */
#define SUBDIRDEPTH  4    /* Depth of subdirectories */
#define NSUBDIRS     8    /* <= 26 Number of subdirs at below RTFSTEST */
#define NTESTFILES      NUSERFILES

ULONG buf[NLONGS]; /* Used in the file write test */

TEXT test_dir[] = "RTFSTE"; /* Test will occur in this Directory + number appended*/
#define INNERLOOP  100          /* Number of times we run the test suite 
                                    between open and close. */
#define OUTERLOOP   1          /* Number of times we open the drive
                                    run the inner loop and close the drive */
#define THREADCOUNT  1          /* Number of threads accessing the filesystem */

#if (VERBOSE)
#define DIAG_OUT(X,Y) diag_printf("%s %s\n", X, Y);
#else
#define DIAG_OUT(X,Y)
#endif

#define STACKSIZE (8192 * 6)
typedef struct thread_data_s
{
    cyg_handle_t threadh;
    cyg_thread thread;
    char tstack[STACKSIZE];
    void* arg;
    cyg_sem_t completed;
} thread_data_t;

thread_data_t threads[THREADCOUNT];

SDVOID do_test(unsigned int);
SDVOID do_file_test(SDVOID);
SDVOID do_rm(char *buffer, int level);
#define regress_error(a) _regress_error( __FUNCTION__, __LINE__, a )
SDVOID _regress_error(const char* func, int line, int error);

void exit(int);
void test_drive( char* );


int main(int argc, TEXT **argv) /*__fn__*/
{
#ifdef DONT_UNLINK
    // grab an arbitrary value
    srand( 0x54313780 + *(unsigned int*)0x00102234);
#endif
    
#if TOTAL_DRIVES >= 1
    test_drive( "A:" );
#endif
#if TOTAL_DRIVES >= 2
    test_drive( "B:" );
#endif
    diag_printf("Finished\n");
    return 0;
}
void test_drive( char* name )
{
    COUNT inner_loop;
    COUNT outer_loop;
    int threadi;
    long long nfree;
    long long nfree_too;
    INT16 i;
  
    strcpy(test_disk, name );

    i= (INT16)(test_disk[0]-'A');
    if (!pc_system_init(i))
    {
          diag_printf("Failed to initialize drive %d.\n", i);
          exit(1);
    }

    for (outer_loop = 0; outer_loop < OUTERLOOP; outer_loop++)
    {
#if USEPRINTF
#if (!VERBOSE)
        diag_printf("\n %d:", outer_loop);
#endif
#endif
        if (!pc_set_default_drive((INT16)(test_disk[0]-'A')))
            regress_error(RGE_FLTDRIVE);
        for (inner_loop = 0; inner_loop < INNERLOOP; inner_loop++)
        {
#if USEPRINTF
#if (!VERBOSE)
            diag_printf("+");
#endif
#endif
            /* Check freespace */
            nfree = pc_free64((INT16)(test_disk[0]-'A'));
            if (!nfree)
                regress_error(RGE_FREEERROR);

            // Launch all the threads
            for( threadi = 0; threadi < THREADCOUNT; threadi++ )
            {
                threads[threadi].arg = (void*) threadi;
                cyg_semaphore_init( &( threads[threadi].completed ), 0 );
                cyg_thread_create( 11, do_test, (cyg_uint32) &(threads[threadi]), "test thread",
                    (void*)&(threads[threadi].tstack[0]), STACKSIZE,
                    &(threads[threadi].threadh), &(threads[threadi].thread) );
                cyg_thread_resume( threads[threadi].threadh );
            }

            // Wait for all threads to exit
            for( threadi = 0; threadi < THREADCOUNT; threadi++ )
            {
                cyg_semaphore_wait( &( threads[threadi].completed ) );
                while( !cyg_thread_delete( threads[threadi].threadh ) ) {
                    cyg_thread_delay(1);
                }
            }

#ifndef DONT_UNLINK
            /* Check freespace again. They should match */
            nfree_too = pc_free64((INT16)(test_disk[0]-'A'));
            if (!nfree_too)
                regress_error(RGE_FREEERROR);
            if (nfree_too != nfree) {
                diag_printf("nfree = %x, nfree_too = %x\n", nfree, nfree_too);
                regress_error(RGE_LEAKERROR);
            }
#endif
        }
    }
    /* Release all resources used by the disk the disk */
    //    pc_system_close((INT16)(test_disk[0]-'A'));
    //    return(0);
}

/*
    Make the test directory
    loop
        Step into the test directory
        Make another subdiretory
            loop
                Make N deep subdirectories
                    Change into each
                    compare what we know is the directory with
                    what pc_pwd returns.
            End loop
            In the lowest level directory
                loop 
                    create a file
                    open it with multiple file descriptors
                    loop
                        write to it on multiple FDs
                    loop
                        seek and read  on multiple FDs. Testing values
                    flush
                    truncate
                    close                   
                end loop
                loop
                    rename
                    delete                  
                end loop
            now delete all of the subdirectories

*/      

SDVOID do_test( cyg_uint32 data ) /* __fn__ */
{
    INT16 i;
    INT16 j;
    TEXT buffer[132];
    TEXT home[32];
    TEXT buffer3[132];
    TEXT c;
    TEXT *p;
    TEXT *p2;
    TEXT my_test_dir[32];

    thread_data_t* pData = (thread_data_t*) data;
    sprintf(my_test_dir, "%s%02d", test_dir, (int)pData->arg);

    /* Make the top level subdirs and subdirs down to layer SUBDIRDEPTH */  
    home[0] = '\0';

    pc_strcat(home, test_disk);
    pc_strcat(home, "\\");
    pc_strcat(home, my_test_dir);

    DIAG_OUT("Creating Subdirectory", home)
        if (!pc_mkdir(home))
            regress_error(RGE_MKDIR);
    if (!pc_set_cwd(home))
        regress_error(RGE_SCWD);

    for (i = 0; i < NSUBDIRS; i++)
    {
        if (!pc_set_cwd(home))
            regress_error(RGE_SCWD);
        /* Make the top level subdirs */    
        buffer[0] = '\0'; /* These two lines are strcpy() */
#ifdef DONT_UNLINK
        {
#if THREADCOUNT==1 && 0
            // single threaded apps can use an incrementing value
            // dc- DONT_UNLINK with the below code causes the app to fail on subsequent tries
            static unsigned int r = 0;
            r++;
#else
            // yes, this is thread unsafe, but should yield semi-random results
            unsigned int r = rand() % 9999;
#endif
            pc_strcat(buffer, "SUB_????");
            buffer[4] =  (r / 1000) + '0';
            buffer[5] = ((r / 100) % 10) + '0';
            buffer[6] = ((r / 10)  % 10) + '0';
            buffer[7] =  (r % 10) + '0';
        }
#else
        pc_strcat(buffer, "SUB_?");
        c = (TEXT)i;
        c += 'A';
        buffer[4] = c;
#endif
        DIAG_OUT("      Creating Subdirectory", buffer);

        if (!pc_mkdir(buffer))
            regress_error(RGE_MKDIR);
        for (j = 0; j < SUBDIRDEPTH; j++)
        {
            if (!pc_set_cwd(home))
                regress_error(RGE_SCWD);
            pc_strcat(buffer,"\\SUBDIR");
            DIAG_OUT("          Creating Subdirectory", buffer);

            if (!pc_mkdir(buffer))
                regress_error(RGE_MKDIR);
            /* Create a dir. We know this will fail. Force error recovery */
            if (pc_mkdir(buffer))
                regress_error(RGE_MKDIRERR);
            // yield
            cyg_thread_delay(1);
            /* Go into the new directory */
            // set the cwd again since we yielded, allowing another thread
            //  to modify the global current directory
            if (!pc_set_cwd(home))
                regress_error(RGE_SCWD);
            if (!pc_set_cwd(buffer))
                regress_error(RGE_SCWD);
            /* Get the dir string */
            if (!pc_pwd(test_disk, buffer3))
                regress_error(RGE_PWD);
            DIAG_OUT("PWD Returns", buffer3);
                /* Funky. skip D:\RTFSTEST\ and then compare pwd with what we 
                   set. Should match */
            p = buffer3;
            p2 = home;
            p2 += 2;
            while (*p2) { p++; p2++; };
            p++;
            if (!strcmp(p, buffer))
                regress_error(RGE_PWD);
        }
    }       

    /* Do the file test */
    do_file_test();

    /* DELETE the subdirs */
#ifndef DONT_UNLINK
    if (!pc_set_cwd(home))
        regress_error(RGE_SCWD);
    for (i = 0; i < NSUBDIRS; i++)
    {
        /* Delete sub directories SUB_?\SUBDIR\SUBDIR ... */
        for (j = SUBDIRDEPTH; j > 0; j--)
        {
            buffer[0] = '\0'; /* These two lines are strcpy() */
            pc_strcat(buffer, "SUB_?");
            c = (TEXT)i;
            c += 'A';
            buffer[4] = c;

            do_rm(buffer, j);
        }
        /* Delete sub directories SUB_? */
        buffer[0] = '\0'; /* These two lines are strcpy() */
        pc_strcat(buffer, "SUB_?");
        c = (TEXT)i;
        c += 'A';
        buffer[4] = c;
        if (!pc_rmdir(buffer))
            regress_error(RGE_RMDIR);
    }       

    /* Delete the test dir */
    if (!pc_set_cwd("..\\"))
        regress_error(RGE_SCWD);
    if (!pc_rmdir(my_test_dir))
        regress_error(RGE_RMDIR);
#endif
    cyg_semaphore_post( &( pData->completed ) );
}

/* Delete a subdir at level */
SDVOID do_rm(char *buffer, int level) /*__fn__*/
{
    int i;
    for (i = 0; i < level; i++)
        pc_strcat(buffer,"\\SUBDIR");
    DIAG_OUT("          Removing Directory", buffer);
    if (!pc_rmdir(buffer))
        regress_error(RGE_RMDIR);
}


/* Test file manipulation routines */
SDVOID do_file_test() /*__fn__*/
{
    PCFD fdarray[NTESTFILES];
    int i;
    int j;
    ULONG index;
    INT16 err_flag;

#if USEPRINTF
#if (!VERBOSE)
    diag_printf("-");
#endif
#endif
    DIAG_OUT("        Performing File io test", " ");

    fdarray[0] = po_open("FILE", PO_RDWR|PO_CREAT|PO_EXCL, PS_IWRITE|PS_IREAD);
    if (fdarray[0] < 0)
        regress_error(RGE_OPEN);

    for (i = 1; i < NTESTFILES;i++)
    {
        /* This should fail */
        fdarray[i] = po_open("FILE", PO_RDWR|PO_CREAT|PO_EXCL, PS_IWRITE|PS_IREAD);
        if (fdarray[i] >= 0)
            regress_error(RGE_OPEN);
        /* This should work */
        fdarray[i] = po_open("FILE", PO_RDWR, PS_IWRITE|PS_IREAD);
        if (fdarray[i] < 0)
            regress_error(RGE_OPEN);
    }
    /* Write into the file using all file descriptors */
    index = 0;
    for (i = 0; i < NTESTFILES;i++)
    {
      err_flag=0;
        po_lseek(fdarray[i], 0L, PSEEK_END, &err_flag);
        if (err_flag) {
            diag_printf( "Write to file: " );
            regress_error(RGE_SEEK);
        }
        for (j = 0; j < NLONGS; j++)
            buf[j] = index++;
        if (po_write(fdarray[i], (UTINY *) buf, (NLONGS*4)) != (NLONGS*4))
            regress_error(RGE_WRITE);
    }

    /* Read file using all fds */
    index = 0;
    for (i = 0; i < NTESTFILES;i++)
    {
      err_flag=0;
        if (po_lseek(fdarray[i], (ULONG) (index*4), PSEEK_SET, &err_flag) != (ULONG) (index*4)){
            diag_printf("Read file: ");
            regress_error(RGE_SEEK);
        }
        if (po_read(fdarray[i], (UTINY *) buf, (NLONGS*4)) != (NLONGS*4))
            regress_error(RGE_READ);
        for (j = 0; j < NLONGS; j++)
        {
            if (buf[j] != index++)
                regress_error(RGE_READ);
        }
    }

    /* This should fail */  
    if (po_truncate(fdarray[0], 256))
        regress_error(RGE_TRUNC);

    if (!po_flush(fdarray[0]))
        regress_error(RGE_FLUSH);

    /* Close all secondary files */
    for (i = 1; i < NTESTFILES;i++)
    {
        if (po_close(fdarray[i]) != 0)
            regress_error(RGE_CLOSE);
    }
    /* This should work */  
    if (!po_truncate(fdarray[0], 256))
        regress_error(RGE_TRUNC);

    /* This should fail */  
    if (pc_unlink("FILE"))
        regress_error(RGE_UNLINK);

    if (po_close(fdarray[0]) != 0)
        regress_error(RGE_CLOSE);

    if (!pc_mv("FILE", "NEWFILE"))
        regress_error(RGE_MV);

    /* This should work */
#ifndef DONT_UNLINK
    if (!pc_unlink("NEWFILE"))
        regress_error(RGE_UNLINK);
#endif
}


SDVOID _regress_error(const char* func, int line, int error) /*__fn__*/
{
    // ignore mkdir errors - these typically happen when regress
    //	 is run on unclean media
    if( error == RGE_MKDIR ) {
      return ;
    }
#if USEPRINTF
    diag_printf("Function %s Line %d error number %d errno %d\n", func, line, error, errno);
#endif
  
    /* Release all resources used by the disk the disk */
    pc_system_close((INT16)(test_disk[0]-'A'));
    exit(0);
}
