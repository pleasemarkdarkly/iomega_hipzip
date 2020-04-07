/*
 *	check_api.c : Exercises required APIs as found in gn_fs.h
 */


#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/abstract_layer/dharma/test_fs.h>

char	buffer[16384];
char	iobuff1[10] = "0123456789";
char	iobuff2[10] = "1234567890";

int
check_fs_api(int* line, char* file)
{
	gn_file_t		handle_1, handle_2, handle_3, handle_4;
	gn_foffset_t	offset_1, offset_2;
	gn_foffset_t	eof_1, eof_2;
	gnfs_attr_t		attr_1, attr_2;
	int				nread, nwritten;
	int				toread, towrite;
	int				error;
	int				retval;
	gn_bool_t		test;
	char			*testfile1 = "testfil1";
	char			*testfile2 = "testfil2";
	char			*testfile3 = "testfil3";
	char			*testfile4 = "testfil4";

	handle_1 = handle_2 = handle_3 = handle_4 = FS_INVALID_HANDLE;

	/* remove test files (if they exist) */
	if (gnfs_exists(testfile1))
	{
#if !defined(GN_NO_FILE_ATTR)
		error = gnfs_set_attr(testfile1, FSATTR_ReadWrite);
		TEST_CONDITION(error == SUCCESS);
#endif
		error = gnfs_delete(testfile1);
		TEST_CONDITION(error == SUCCESS);
	}

	if (gnfs_exists(testfile2))
	{
#if !defined(GN_NO_FILE_ATTR)
		error = gnfs_set_attr(testfile2, FSATTR_ReadWrite);
		TEST_CONDITION(error == SUCCESS);
#endif
		error = gnfs_delete(testfile2);
		TEST_CONDITION(error == SUCCESS);
	}

	if (gnfs_exists(testfile3))
	{
#if !defined(GN_NO_FILE_ATTR)
		error = gnfs_set_attr(testfile3, FSATTR_ReadWrite);
		TEST_CONDITION(error == SUCCESS);
#endif
		error = gnfs_delete(testfile3);
		TEST_CONDITION(error == SUCCESS);
	}

	if (gnfs_exists(testfile4))
	{
#if !defined(GN_NO_FILE_ATTR)
		error = gnfs_set_attr(testfile1, FSATTR_ReadWrite);
		TEST_CONDITION(error == SUCCESS);
#endif
		error = gnfs_delete(testfile4);
		TEST_CONDITION(error == SUCCESS);
	}

	/*
	 * basic named file operations
	 */
	handle_1 = gnfs_create(testfile1, FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_1 != FS_INVALID_HANDLE);

	error = gnfs_close(handle_1); handle_1 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	handle_1 = gnfs_open(testfile1, FSMODE_ReadWrite);
	TEST_CONDITION(handle_1 != FS_INVALID_HANDLE);

	error = gnfs_close(handle_1); handle_1 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	/* make sure file exists */
	test = gnfs_exists(testfile1);
	TEST_CONDITION(test == GN_TRUE);

	/* rename existing file */
	error = gnfs_rename_file(testfile1, testfile2);
	TEST_CONDITION(error == SUCCESS);

	/* verify file exists with new name */
	test = gnfs_exists(testfile2);
	TEST_CONDITION(test == GN_TRUE);

	/* verify renamed file doesn't exist */
	test = gnfs_exists(testfile1);
	TEST_CONDITION(test == GN_FALSE);

	error = gnfs_delete(testfile2);
	TEST_CONDITION(error == SUCCESS);

	/* verify attempt to rename non-existent file fails */
	error = gnfs_rename_file(testfile3, testfile4);
	TEST_CONDITION(error != SUCCESS);

	/* verify non-existent renamed files doesn't exist */
	test = gnfs_exists(testfile3);
	TEST_CONDITION(test == GN_FALSE);

	test = gnfs_exists(testfile4);
	TEST_CONDITION(test == GN_FALSE);


	/*
	 * reading writing at specific file location
	 */
	handle_1 = gnfs_create(testfile1, FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_1 != FS_INVALID_HANDLE);

	offset_1 = 0;
	offset_2 = gnfs_seek(handle_1, offset_1, FS_SEEK_START);
	TEST_CONDITION(offset_1 == offset_2);

	towrite = sizeof(iobuff1);
	nwritten = gnfs_write(handle_1, iobuff1, towrite);
	TEST_CONDITION(nwritten == towrite);

	towrite = sizeof(iobuff1);
	nwritten = gnfs_write(handle_1, iobuff1, towrite);
	TEST_CONDITION(nwritten == towrite);

	offset_2 = gnfs_seek(handle_1, offset_1, FS_SEEK_START);
	TEST_CONDITION(offset_1 == offset_2);

	toread = towrite;
	nread = gnfs_read(handle_1, buffer, toread);
	TEST_CONDITION(nread == toread);

	/* make sure data is identical */
	TEST_CONDITION(gnmem_memcmp(iobuff1, buffer, nread) == 0);

	toread = towrite;
	nread = gnfs_read(handle_1, buffer, toread);
	TEST_CONDITION(nread == toread);

	/* make sure data is identical */
	TEST_CONDITION(gnmem_memcmp(iobuff1, buffer, nread) == 0);


	/*
	 * extending end of file, writing at end of file
	 */
	eof_1 = 2048;
	eof_2 = gnfs_set_eof(handle_1, eof_1);
	TEST_CONDITION(eof_1 == eof_2);

	towrite = sizeof(iobuff1);
	nwritten = gnfs_write(handle_1, iobuff1, towrite);
	TEST_CONDITION(nwritten == towrite);

	offset_1 = 1024;
	towrite = sizeof(iobuff1);
	nwritten = gnfs_write_at(handle_1, offset_1, iobuff1, towrite);
	TEST_CONDITION(nwritten == towrite);

	offset_2 = gnfs_seek(handle_1, offset_1, FS_SEEK_START);
	TEST_CONDITION(offset_1 == offset_2);

	toread = towrite;
	nread = gnfs_read(handle_1, buffer, toread);
	TEST_CONDITION(nread == toread);

	/* make sure data is identical */
	TEST_CONDITION(gnmem_memcmp(iobuff1, buffer, nread) == 0);

	eof_2 = gnfs_seek(handle_1, eof_1, FS_SEEK_START);
	TEST_CONDITION(eof_1 == eof_2);

	nread = gnfs_read(handle_1, buffer, toread);
	TEST_CONDITION(nread == toread);

	/* make sure data is identical */
	TEST_CONDITION(gnmem_memcmp(iobuff1, buffer, nread) == 0);

	error = gnfs_close(handle_1); handle_1 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	test = gnfs_exists(testfile1);
	TEST_CONDITION(test == GN_TRUE);

	error = gnfs_delete(testfile1);
	TEST_CONDITION(error == SUCCESS);




#if !defined(GN_NO_FILE_ATTR)
	/*
	 * file attribute support
	 */
	handle_2 = gnfs_create(testfile2, FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_2 != FS_INVALID_HANDLE);

	error = gnfs_close(handle_2); handle_2 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	test = gnfs_exists(testfile2);
	TEST_CONDITION(test == GN_TRUE);

	attr_1 = FSATTR_Read;
	error = gnfs_set_attr(testfile2, attr_1);
	TEST_CONDITION(error == SUCCESS);

	attr_2 = gnfs_get_attr(testfile2);
	TEST_CONDITION(attr_1 == attr_2);

	/* try to open readonly file read/write */
	handle_2 = gnfs_open(testfile2, FSMODE_ReadWrite);
	TEST_CONDITION(handle_2 == FS_INVALID_HANDLE);

	/* try to delete readonly file */
	error = gnfs_delete(testfile2);
	TEST_CONDITION(error != SUCCESS);

	/* reset to read/write and try to open again */
	attr_1 = FSATTR_ReadWrite;
	error = gnfs_set_attr(testfile2, attr_1);
	TEST_CONDITION(error == SUCCESS);

	attr_2 = gnfs_get_attr(testfile2);
	TEST_CONDITION(attr_1 == attr_2);

	/* try to open readonly file read/write */
	handle_2 = gnfs_open(testfile2, FSMODE_ReadWrite);
	TEST_CONDITION(handle_2 != FS_INVALID_HANDLE);

	error = gnfs_close(handle_2); handle_2 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	error = gnfs_delete(testfile2);
	TEST_CONDITION(error == SUCCESS);
#endif	/* #if !defined(GN_NO_FILE_ATTR) */

	retval = 0;
	goto NOFAIL;

FAIL:
	retval = -1;

NOFAIL:
	if (handle_1 != FS_INVALID_HANDLE)
		gnfs_close(handle_1);

	if (handle_2 != FS_INVALID_HANDLE)
		gnfs_close(handle_2);

	if (handle_3 != FS_INVALID_HANDLE)
		gnfs_close(handle_3);

	if (handle_4 != FS_INVALID_HANDLE)
		gnfs_close(handle_4);

	return retval;
}

