/*
 *	test_fs.h : Declarations for the validation application.
 */

#include	GN_STRING_H

#define		MAX_FILE_LEN		128

#define		TEST_CONDITION(x)	if (!(x)) { *line = __LINE__ - 1; strncpy(file, __FILE__, MAX_FILE_LEN); goto FAIL; }


extern int compare_files(char* file1, char* file2);
