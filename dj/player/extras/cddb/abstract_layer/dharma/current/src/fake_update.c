/*
 *	fake_update.c : Performs file system operations when extracting update packages.
 */


#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/abstract_layer/dharma/test_fs.h>

char	buffer[16384];
int
fake_update_one(int* line, char* file)
{
	int				handle_4 = FS_INVALID_HANDLE, handle_5 = FS_INVALID_HANDLE, handle_6 = FS_INVALID_HANDLE, handle_7 = FS_INVALID_HANDLE;
	int				nread, nwritten;
	int				toread, towrite;
	int				error;
	int				retval = 0;
	gn_bool_t		test;

	handle_4 = gnfs_open("update01_1_4.pkg", FSMODE_ReadOnly);
	TEST_CONDITION(handle_4 != FS_INVALID_HANDLE);

	/* read header of update envelope */
	toread = 40;
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);

	toread = 28;
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);

	error = gnfs_close(handle_4); handle_4 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	/* extract first enclosure */
	handle_4 = gnfs_open("update01_1_4.pkg", FSMODE_ReadOnly);
	TEST_CONDITION(handle_4 != FS_INVALID_HANDLE);

	test = gnfs_exists("ecddb.enc");
	if (test == GN_TRUE)
	{
		error = gnfs_delete("ecddb.enc");
		TEST_CONDITION(error == SUCCESS);
	}
	handle_5 = gnfs_create("ecddb.enc", FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_5 != FS_INVALID_HANDLE);

	toread = towrite = 16384;
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_5, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);

	toread = towrite = 15828;
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_5, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);
	error = gnfs_close(handle_5); handle_5 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	test = gnfs_exists("ecddb.enc");
	TEST_CONDITION(test == GN_TRUE);

	/* "decrypt" first enclosure */
	handle_5 = gnfs_open("ecddb.enc", FSMODE_ReadWrite);
	TEST_CONDITION(handle_5 != FS_INVALID_HANDLE);
	toread = 20;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	toread = 32;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);

	test = gnfs_exists("ecddba.tmp");
	if (test == GN_TRUE)
	{
		error = gnfs_delete("ecddba.tmp");
		TEST_CONDITION(error == SUCCESS);
	}

	handle_6 = gnfs_create("ecddba.tmp", FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_6 != FS_INVALID_HANDLE);

	toread = towrite = 16384;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_6, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);

	toread = towrite = 15776;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_6, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);
	error = gnfs_close(handle_6); handle_6 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_close(handle_5); handle_5 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_delete("ecddb.enc");
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_rename_file( "ecddba.tmp", "ecddb.enc");
	TEST_CONDITION(error == SUCCESS);

	/* extract second enclosure */
	test = gnfs_exists("acddb.enc");
	if (test == GN_TRUE)
	{
		error = gnfs_delete("acddb.enc");
		TEST_CONDITION(error == SUCCESS);
	}

	handle_5 = gnfs_create("acddb.enc", FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_5 != FS_INVALID_HANDLE);

	toread = towrite = 16384;
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_5, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);

	toread = towrite = 9485;
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_5, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);
	error = gnfs_close(handle_5); handle_5 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	test = gnfs_exists("acddb.enc");
	TEST_CONDITION(test == GN_TRUE);

	/* "decrypt" second enclosure */
	handle_5 = gnfs_open("acddb.enc", FSMODE_ReadWrite);
	TEST_CONDITION(handle_5 != FS_INVALID_HANDLE);

	toread = 20;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	toread = 32;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);

	test = gnfs_exists("ecddbb.tmp");
	if (test == GN_TRUE)
	{
		error = gnfs_delete("ecddbb.tmp");
		TEST_CONDITION(error == SUCCESS);
	}

	handle_6 = gnfs_create("ecddbb.tmp", FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_6 != FS_INVALID_HANDLE);

	toread = towrite = 16384;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_6, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);

	toread = towrite = 9429;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_6, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);
	error = gnfs_close(handle_6); handle_6 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_close(handle_5); handle_5 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_delete("acddb.enc");
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_rename_file( "ecddbb.tmp", "acddb.enc");
	TEST_CONDITION(error == SUCCESS);

	/* extract third enclosure */
	test = gnfs_exists("bcddb.enc");
	if (test == GN_TRUE)
	{
		error = gnfs_delete("bcddb.enc");
		TEST_CONDITION(error == SUCCESS);
	}

	handle_5 = gnfs_create("bcddb.enc", FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_5 != FS_INVALID_HANDLE);

	toread = towrite = 16384;
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_5, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);

	toread = towrite = 12901;
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_5, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);
	error = gnfs_close(handle_5); handle_5 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	test = gnfs_exists("bcddb.enc");
	TEST_CONDITION(test == GN_TRUE);

	/* "decrypt" third enclosure */
	handle_5 = gnfs_open("bcddb.enc", FSMODE_ReadWrite);
	TEST_CONDITION(handle_5 != FS_INVALID_HANDLE);

	toread = 20;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	toread = 32;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);

	test = gnfs_exists("ecddbc.tmp");
	if (test == GN_TRUE)
	{
		error = gnfs_delete("ecddbc.tmp");
		TEST_CONDITION(error == SUCCESS);
	}


	handle_6 = gnfs_create("ecddbc.tmp", FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_6 != FS_INVALID_HANDLE);

	toread = towrite = 16384;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_6, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);

	toread = towrite = 12842;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_6, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);
	error = gnfs_close(handle_6); handle_6 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_close(handle_5); handle_5 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_delete("bcddb.enc");
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_rename_file( "ecddbc.tmp", "bcddb.enc");
	TEST_CONDITION(error == SUCCESS);

	/* extract fourth enclosure */
	gnfs_exists("ccddb.enc");
	if (test == GN_TRUE)
	{
		error = gnfs_delete("ccddb.enc");
		TEST_CONDITION(error == SUCCESS);
	}

	handle_5 = gnfs_create("ccddb.enc", FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_5 != FS_INVALID_HANDLE);

	toread = towrite = 16384;
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_5, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);

	toread = towrite = 3277;
	nread = gnfs_read( handle_4, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_5, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);
	error = gnfs_close(handle_5); handle_5 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	test = gnfs_exists("ccddb.enc");
	TEST_CONDITION(test == GN_TRUE);

	/* "decrypt" fourth enclosure */
	handle_5 = gnfs_open("ccddb.enc", FSMODE_ReadWrite);
	TEST_CONDITION(handle_5 != FS_INVALID_HANDLE);

	toread = 20;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	toread = 32;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);

	test = gnfs_exists("ecddbd.tmp");
	if (test == GN_TRUE)
	{
		error = gnfs_delete("ecddbd.tmp");
		TEST_CONDITION(error == SUCCESS);
	}


	handle_6 = gnfs_create("ecddbd.tmp", FSMODE_ReadWrite, FSATTR_ReadWrite);
	TEST_CONDITION(handle_6 != FS_INVALID_HANDLE);

	toread = towrite = 16384;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_6, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);

	toread = towrite = 3218;
	nread = gnfs_read( handle_5, buffer, toread );
	TEST_CONDITION(nread == toread);
	nwritten = gnfs_write( handle_6, buffer, towrite );
	TEST_CONDITION(nwritten == towrite);
	error = gnfs_close(handle_6); handle_6 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_close(handle_5); handle_5 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_delete("ccddb.enc");
	TEST_CONDITION(error == SUCCESS);
	error = gnfs_rename_file( "ecddbd.tmp", "ccddb.enc");
	TEST_CONDITION(error == SUCCESS);

	/* finally close package file */
	error = gnfs_close(handle_4); handle_4 = FS_INVALID_HANDLE;
	TEST_CONDITION(error == SUCCESS);

	/* compare files */
	error = compare_files("ecddb.enc", "orgecddb.enc");
	TEST_CONDITION(error == SUCCESS);

	error = compare_files("acddb.enc", "orgacddb.enc");
	TEST_CONDITION(error == SUCCESS);

	error = compare_files("bcddb.enc", "orgbcddb.enc");
	TEST_CONDITION(error == SUCCESS);

	error = compare_files("ccddb.enc", "orgccddb.enc");
	TEST_CONDITION(error == SUCCESS);

	retval = 0;
	goto CLOSE_FILES;

FAIL:
	retval = -1;

CLOSE_FILES:
	if (handle_4 != FS_INVALID_HANDLE)
		gnfs_close(handle_4);

	if (handle_5 != FS_INVALID_HANDLE)
		gnfs_close(handle_5);

	if (handle_6 != FS_INVALID_HANDLE)
		gnfs_close(handle_6);

	if (handle_7 != FS_INVALID_HANDLE)
		gnfs_close(handle_7);

	return retval;
}

