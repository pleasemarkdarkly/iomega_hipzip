//........................................................................................
//........................................................................................
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 8/16/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/*****************************************************************************
* Filename: SDTYPES.H - Type Definition for HDTK
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997 - 1999 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description: 
*       Data Type Definitions  
*
*****************************************************************************/

#ifndef _SDTYPES_H_


#ifdef __cplusplus
extern "C" {
#endif




#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0    
#endif



/* Pseudo types to eliminate confusion about the sizes of native types */

typedef char            CHAR;   /*  8-bit signed */
typedef char            INT08;  /*  8-bit signed */
typedef short           INT16;  /* 16-bit signed */
typedef short           COUNT;  /* 16 bit signed */
typedef long            INT32;  /* 32-bit signed */
typedef long            LONG;   /* 32-bit signed */
typedef long long	INT64;	/* 64-bit signed */
typedef unsigned char   UCHAR;  /*  8-bit unsigned */
typedef unsigned char   UINT08; /*  8-bit unsigned */
typedef unsigned short  UINT16; /* 16-bit unsigned */
typedef unsigned short  UCOUNT; /* 16-bit unsigned */
typedef unsigned long   UINT32; /* 32-bit unsigned */
typedef unsigned long   ULONG;  /* 32-bit unsigned */
typedef unsigned long long UINT64;	/* 64-bit unsigned */

typedef unsigned short  SDWCHAR; /* Unicode character */


typedef void *          HANDLE;

typedef UINT16          WINHANDLE;
typedef UINT16          CLIENTHANDLE;


typedef unsigned char   UTINY;  /* unsigned 8 bit */
typedef unsigned char   UTEXT;  /* unsigned 8 bit for string */
typedef char            TEXT;   /* char for string */
typedef unsigned long   BLOCKT; /* 32-bit unsigned */
typedef short           PCFD;   /* file desc */


#define SDVOID          void      /* void  */
#define SDIMPORT        extern    /* For external data */
#define SDLOCAL         static    /* For internal data and functions */ 
#define SDGLOBAL        extern    /* For external functions */


/* Null pointer */
#define SDNULL    ((void *)0)



typedef enum SD_YES_NO
{
	NO  = FALSE,
	YES = TRUE
} SDBOOL;



#define BLOCKEQ0        0L



#ifdef __cplusplus
}
#endif


#define _SDTYPES_H_

#endif

