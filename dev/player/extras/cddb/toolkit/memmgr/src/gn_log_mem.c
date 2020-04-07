/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_log_mem.c - Implementation of memory logging utilities to help
 * track where dynamic memory is being utilized.
 */


#include	<extras/cddb/gn_platform.h>
#include	GN_STDIO_H
#include	GN_STRING_H
#include <extras/cddb/gn_fs.h>

#if defined(_GN_LOG_MEM_)

#define	_GN_MEM_NUM_PTRS_	1024

static	gn_bool_t	gLogMEM = GN_FALSE;
static	gn_char_t	gLogBuff[512];
static	gn_char_t	gLogFile[128];
static	gn_str_t	gFileName;
static  gn_handle_t	gLogHandle = FS_INVALID_HANDLE;
static	gn_int32_t	gLogCurrent;

typedef struct memloginfo
{
	void*	ptr;
	int		size;
	char	filename[24];
} memloginfo_t;

static	memloginfo_t		gLogPointers[_GN_MEM_NUM_PTRS_+1];

#define		LOG_MEM(SPRINTF_CALL)	if (gLogMEM) gnfs_write(gLogHandle, gLogBuff, SPRINTF_CALL)


/* enable memory system logging */
gn_error_t
gnmem_set_mem_logging(gn_bool_t enable, gn_cstr_t logfile)
{
	int		handle;

	if (!logfile || !*logfile)
		logfile = "mem.log";

	if (enable != GN_TRUE && gLogHandle != FS_INVALID_HANDLE)
	{
		/* check for leaks */
		int		n;

		for (n = 1; n <= _GN_MEM_NUM_PTRS_; n++)
		{
			if (gLogPointers[n].ptr != NULL)
			{
				printf("/* LEAK [%s %X %d] */\n", gLogPointers[n].filename, gLogPointers[n].ptr, gLogPointers[n].size);
			}
		}

		gnfs_close(gLogHandle);
		gLogHandle = FS_INVALID_HANDLE;
		gLogMEM = GN_FALSE;
	}
	else
	{
		gnmem_memset(&gLogPointers[0], 0, sizeof(gLogPointers));
		gLogCurrent = 0;

		if (gLogHandle != FS_INVALID_HANDLE)
		{
			gnfs_close(gLogHandle);
			gLogHandle = FS_INVALID_HANDLE;
		}
		handle = gnfs_create(logfile, FSMODE_WriteOnly|FSMODE_Create|FSMODE_Truncate, FSATTR_ReadWrite);
		if (handle != -1)
		{
			gLogHandle = handle;
			gLogMEM = GN_TRUE;
			strcpy(gLogFile, logfile);
		}
		else
			gLogMEM = GN_FALSE;
	}
	return SUCCESS;
}

static gn_int32_t
gnmem_get_pointer_index(void* ptr)
{
	int		n;

	for (n = 1; n <= _GN_MEM_NUM_PTRS_; n++)
	{
		if (gLogPointers[n].ptr == ptr)
		{
			return n;
		}
	}
	return 0;

}

static gn_int32_t
gnmem_add_pointer(void* ptr, int size, char* filename)
{
	int		n;

	for (n = 1; n <= _GN_MEM_NUM_PTRS_; n++)
	{
		if (gLogPointers[n].ptr == NULL)
		{
			gLogPointers[n].ptr = ptr;
			gLogPointers[n].size = size;
			strncpy(gLogPointers[n].filename, filename, sizeof(gLogPointers[n].filename));
			gLogPointers[n].filename[sizeof(gLogPointers[n].filename)-1] = 0;
			return n;
		}
	}
	return 0;
}

static gn_int32_t
gnmem_free_pointer(void* ptr)
{
	int		index;

	index = gnmem_get_pointer_index(ptr);
	if (index)
	{
		gLogPointers[index].ptr = NULL;
	}
	return index;
}

void
gnmem_add_mem_logging_info(gn_cstr_t info)
{
	if (gLogMEM == GN_TRUE)
	{
		LOG_MEM(sprintf(gLogBuff, "/* Application Info: %s */\r\n", info));
	}
}


void
_gnmem_log_malloc(void* block, gn_size_t size, gn_uint32_t line, gn_str_t file)
{
	if (gLogMEM)
	{
#if defined(PLATFORM_WINDOWS)
		gFileName = strrchr(file, '\\');
#else
		gFileName = strrchr(file, '/');
#endif
		if (!gFileName)
			gFileName = file;
		else
			gFileName++;

		if (block != NULL)
			gLogCurrent = gnmem_add_pointer(block, size, gFileName);
		else
			gLogCurrent = -1;

		LOG_MEM(sprintf(gLogBuff, "/* ALLOC (%s: %d) [%s %d] (%d): size = %d, block = %X  */ \r\n", gFileName, line, gFileName, size, gLogCurrent, size, block));
		if (block != NULL)
			LOG_MEM(sprintf(gLogBuff, "pointer_%d = gnmem_malloc(%d);\r\n", gLogCurrent, size));
		else
			LOG_MEM(sprintf(gLogBuff, "pointer_err = gnmem_malloc(%d);\r\n", size));

	}
}

void
_gnmem_log_realloc(void* block, void* newblock, gn_size_t size, gn_uint32_t line, gn_str_t file)
{
	if (gLogMEM)
	{
		gn_int32_t		oldBlock = 0;
		gn_int32_t		newBlock = 0;
		gn_size_t		oldSize = 0;
		gn_str_t		oldFile = "";

		oldBlock = gnmem_get_pointer_index(block);
		if (oldBlock)
		{
			oldSize = gLogPointers[oldBlock].size;
			oldFile = gLogPointers[oldBlock].filename;
		}

#if defined(PLATFORM_WINDOWS)
		gFileName = strrchr(file, '\\');
#else
		gFileName = strrchr(file, '/');
#endif
		if (!gFileName)
			gFileName = file;
		else
			gFileName++;

		if (newblock != NULL)
		{
			gnmem_free_pointer(block);
			newBlock = gnmem_add_pointer(newblock, size, oldFile);
		}
		else
			newBlock = -1;

		LOG_MEM(sprintf(gLogBuff, "/* REALLOC (%s: %d) [%s %d %d] (old: %d, new: %d): size = %d, oldblock = %X, block = %X  */ \r\n", gFileName, line, oldFile, oldSize, size, oldBlock, newBlock, size, block, newblock));
		if (block != NULL)
			LOG_MEM(sprintf(gLogBuff, "pointer_%d = gnmem_realloc(0x%X, %d);\r\n", newBlock, block, size));
		else
			LOG_MEM(sprintf(gLogBuff, "pointer_err = gnmem_realloc(0x%X, %d);\r\n", block, size));

	}
}


void 
_gnmem_log_free(void *block, gn_uint32_t line, gn_str_t file)
{
	if (gLogMEM)
	{
		gn_size_t	size = 0;
		gn_str_t	filename = "";

		gLogCurrent = gnmem_free_pointer(block);
		if (gLogCurrent)
		{
			size = gLogPointers[gLogCurrent].size;
			filename = gLogPointers[gLogCurrent].filename;
		}

#if defined(PLATFORM_WINDOWS)
		gFileName = strrchr(file, '\\');
#else
		gFileName = strrchr(file, '/');
#endif
		if (!gFileName)
			gFileName = file;
		else
			gFileName++;

		LOG_MEM(sprintf(gLogBuff, "/* FREE (%s: %d) [%s %d] (%d): block = %X  */ \r\n", gFileName, line, filename, size, gLogCurrent, block));
		LOG_MEM(sprintf(gLogBuff, "pointer_%d = gnmem_free(0x%X);\r\n", gLogCurrent, block));

	}
	free(memblock);
	return SUCCESS;
}


#else

gn_error_t
gnmem_set_mem_logging(gn_bool_t enable, gn_cstr_t logfile)
{
	enable = GN_TRUE;
	logfile = NULL;
	return SUCCESS;
}

void
gnmem_add_mem_logging_info(gn_cstr_t info)
{
	info = NULL;
}

#endif	/* #if defined(_GN_LOG_MEM_) */


