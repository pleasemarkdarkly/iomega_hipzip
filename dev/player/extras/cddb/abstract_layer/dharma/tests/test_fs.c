/*
 *	test_fs.c : Defines the entry point for the validation application.
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/abstract_layer/dharma/test_fs.h>

#include <cyg/kernel/kapi.h>
#include <util/debug/debug.h>

#define STACKSIZE (32 * 4096)

static cyg_handle_t thread;
static cyg_thread thread_obj;
static char stack[STACKSIZE];

extern int check_fs_api(int* line, char* file);
extern int fake_update_one(int* line, char* file);

void
main_thread(cyg_addrword_t data)
{
	int		ret;
	char	file[MAX_FILE_LEN];
	int		line = 0;


    gnfs_set_root_directory("a:/cddb/");

	ret = gnfs_initialize();
	printf("gnfs_initialize() returned %d\n", ret);
	if (ret)
	{
		printf("gnfs_initialize() failed\n");
		return;
	}

	ret = check_fs_api(&line, file);
	printf("check_fs_api() returned %d\n", ret);
	if (ret)
		printf("check_fs_api() failed at '%s', line %d\n", file, line);

	ret = fake_update_one(&line, file);
	printf("fake_update_one() returned %d\n", ret);
	if (ret)
		printf("fake_update_one() failed at '%s', line %d\n", file, line);

	ret = gnfs_shutdown();
	printf("gnfs_shutdown() returned %d\n", ret);
	if (ret)
	{
		printf("gnfs_shutdown() failed\n");
		return;
	}

	return;
}


int compare_files(char* file1, char* file2)
{
	int				retval = 0;
	int				nread1, nread2;
	int				toread1, toread2;
	gn_foffset_t	size1, size2;
	char			iobuff1[1024];
	char			iobuff2[1024];
	gn_file_t		handle1, handle2;

	handle1 = gnfs_open(file1, FSMODE_ReadOnly);
	handle2 = gnfs_open(file2, FSMODE_ReadOnly);

	if (handle1 == FS_INVALID_HANDLE || handle2 == FS_INVALID_HANDLE)
	{
		retval = -1;
	}
	else
	{
		size1 = gnfs_seek(handle1, 0L, FS_SEEK_END);
		size2 = gnfs_seek(handle2, 0L, FS_SEEK_END);
		gnfs_seek(handle1, 0L, FS_SEEK_START);
		gnfs_seek(handle2, 0L, FS_SEEK_START);
		if (size1 == FS_INVALID_OFFSET || size2 == FS_INVALID_OFFSET)
			retval = -1;
		else if (size1 != size2)
			retval = -1;
		else
		{
			while (size1)
			{
				if (size1 > sizeof(iobuff1))
					toread1 = sizeof(iobuff1);
				else
					toread1 = size1;

				toread2 = toread1;
				nread1 = gnfs_read(handle1, iobuff1, toread1);
				nread2 = gnfs_read(handle2, iobuff2, toread2);
				if (nread1 == toread1 && nread2 == toread2)
				{
					if (gnmem_memcmp(iobuff1, iobuff2, nread1))
					{
						retval = -1;
						break;
					}
				}
				else
				{
					retval = -1;
					break;
				}
				size1 -= toread1;

			}
		}
	}



	if (handle1 != FS_INVALID_HANDLE)
		gnfs_close(handle1);

	if (handle2 != FS_INVALID_HANDLE)
		gnfs_close(handle2);

	return retval;
}


void
cyg_user_start(void)
{
    cyg_thread_create(10, main_thread, (cyg_addrword_t)0, "main_thread",
		      (void *)stack, STACKSIZE, &thread, &thread_obj);
    cyg_thread_resume(thread);
}

