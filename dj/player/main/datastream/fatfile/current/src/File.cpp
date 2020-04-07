//........................................................................................
//........................................................................................
//.. File Name: File.cpp																..
//.. Date: 01/25/2001																	..
//.. Author(s): Eric Gibbs																..
//.. Description of content:															..
//.. Usage:																				..
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 03/27/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#include <stdlib.h>
#include <datastream/fatfile/FileInputStream.h>
#include <util/debug/debug.h>
//#include "DriveInterface.h"
/*  Class CFile  */

CFile::CFile() : m_nCharsRead(0), m_bOpened(false), m_bOpenedSloppy(false), m_nFileSize(-1), m_hFile(-1), m_pName(0)
{
	cyg_mutex_init( &m_mutFS );
}

int CFile::Open()
{
	m_bOpened = true;
	DEBUG_FILE("FIL:Open\n");
	if(m_hFile >= 0)
	{
		DEBUG_FILE("closing old handle %d\n",m_hFile);
		po_close(m_hFile);
	}

	DEBUG_FILE("requesting new handle\n");
	m_hFile = (short)po_open((TEXT*)m_pName,PO_RDONLY | PO_NOSHAREWRITE, PS_IREAD);
	DEBUG_FILE("open %s into %d\n",m_pName,(int)m_hFile);

	if (m_hFile < 0)
	{

		DEBUG_FILE("failed once.   requesting new handle again\n");
		WakeDrive();
		m_hFile = (short)po_open((TEXT*)m_pName,PO_RDONLY | PO_NOSHAREWRITE, PS_IREAD);
		DEBUG_FILE("open %s into %d\n",m_pName,(int)m_hFile);
		if (m_hFile < 0)
		{
			m_bOpened = true;
			DEBUG_FILE("po open failed %s\n",m_pName);
			SleepDrive();
			return -1;
		}
	}

	STAT stat;
	int ret;

	ret = pc_fstat(m_hFile, &stat);
	if(ret == 0)
	{
//		diag_printf("(%s) size %d\n",m_pName,stat.st_size);
		m_nFileSize = stat.st_size;
	}
	else
	{
		diag_printf("FIL:pc_fstat unable to determine stats on input file\n");
	}

	m_bOpenedSloppy = true;
	m_nCharsRead = 0;
	return m_hFile;
}

void CFile::Close()
{
	if(m_hFile >= 0)
	{
		DEBUG_FILE("closing %s from %d\n",m_pName,(int)m_hFile);
//		cyg_mutex_lock( &m_mutFS );
		po_close(m_hFile);
//		cyg_mutex_unlock( &m_mutFS );
	}
	m_hFile = -1;
	m_bOpened = false;
	m_bOpenedSloppy = false;
}

CFile::~CFile()
{
	DEBUG_FILE("FIL:~ %s\n",m_pName);
	if (m_pName)
		delete [] m_pName;
	if (m_hFile >= 0)
		Close();
	cyg_mutex_destroy( &m_mutFS );
}

bool CFile::SameFile(const char* szname)
{
	// string compare filenames
	if (m_pName && !strcmp(szname,m_pName))
		return true;
	return false;
}

int CFile::Length()
{
	if (!m_bOpenedSloppy)
	{
		cyg_mutex_lock( &m_mutFS );
		if (!m_bOpened)
		{
			Open();
		}
		cyg_mutex_unlock( &m_mutFS );
	}
	return m_nFileSize;
}

int CFile::Open(const char* filename)
{
	DEBUG_FILE("FIL:open by name\n");

	delete [] m_pName;
	m_pName = new char[strlen(filename)];
	strcpy (m_pName, filename);
	if (!m_pName)
	{
		DEBUG_FILE("FIL:Out of Memory for PIS filename allocation\n");
	}

	return Open();
}

int CFile::Read(unsigned char* pBuffer, unsigned int nBytes)
{
	DEBUG_FILE("FIL:Read\n");
	if (!m_bOpenedSloppy)
	{
	  	cyg_mutex_lock( &m_mutFS );
		if (!m_bOpened)
		{
			Open();
		}
		cyg_mutex_unlock( &m_mutFS );
	}
	if(m_hFile < 0)
	{
		DEBUG_FILE("FIL:read on null handle for %s\n",m_pName);
		return -1;
	}

	//DEBUG_FILE("FIL:reading from h=%d\n",m_hFile);
//	cyg_mutex_lock( &m_mutFS );
	UCOUNT result = po_read(m_hFile, (UTINY*)pBuffer,(UCOUNT)nBytes);
//	cyg_mutex_unlock( &m_mutFS );
	//DEBUG_FILE("FIL:result %d\n",result);
#if 0
	if (nBytes == 128)
	{
		diag_printf("FILE\n");
		diag_printf("reference TAG= %d %d %d\n",'T','A','G');
		diag_printf("\n<");
		for (int i = 0; i < 128; i++)
			diag_printf("%d ",pBuffer[i]);
		diag_printf(">\n");
	}		
#endif

	if(result == 0xFFFF)
	{
		return -1;
	}
	else
	{
		m_nCharsRead += result;
		return (int)result;
	}
}

int CFile::Seek(int nOffset, InputSeekPos eOrigin)
{
	if (!m_bOpenedSloppy)
	{
		cyg_mutex_lock( &m_mutFS );
		if (!m_bOpened)
		{
			Open();
		}
		cyg_mutex_unlock( &m_mutFS );
	}
	if(m_hFile < 0)
		return -1;

	switch(eOrigin)
	{
	case SeekStart:
		if(nOffset >= 0)
		{
			m_nCharsRead = (int)po_lseek(m_hFile,(ULONG)nOffset,PSEEK_SET,NULL);
			return m_nCharsRead;
		}
		else
		{
			DEBUG_FILE("FIL:Invalid negative offset from bof requested of seek\n");
			return -1;
		}
	case SeekCurrent:
		if(nOffset < 0)
		{
			ULONG current_offset = po_lseek(m_hFile,0,PSEEK_CUR,NULL);
			int res;
//			cyg_mutex_lock( &m_mutFS );
			res = (int)po_lseek(m_hFile,current_offset + (ULONG)nOffset,PSEEK_SET,NULL);
//			cyg_mutex_unlock( &m_mutFS );
			return res;
		}
		else
		{
			int res;
//			cyg_mutex_lock( &m_mutFS );
			res = (int)po_lseek(m_hFile,(ULONG)nOffset,PSEEK_CUR,NULL);
//			cyg_mutex_unlock( &m_mutFS );
			return res;
		}
	case SeekEnd:
		// (epg,6/7/2000): ok, dammit.  po_lseek adds the offset to the file size and seeks from the beginning of the file.
		// so the sense of the offset from eof is that we want it to always be negative.  however, po_lseek wants a ulong
		// offset, so that is never possible.  what we therefore have to do is figure out in advance what the offset is from
		// the bof and send it in that way.  
		if(nOffset <= 0)
		{
			nOffset += m_nFileSize;

			int res;
//			cyg_mutex_lock( &m_mutFS );
			res = (int)po_lseek(m_hFile,(ULONG)nOffset,PSEEK_SET,NULL);
//			cyg_mutex_unlock( &m_mutFS );
			return res;
			/* // was: (seeks to end, then seeks to end+offset from start of file)
			ULONG current_offset = po_lseek(m_hFile,0,PSEEK_END,NULL);
			return (int)po_lseek(m_hFile,current_offset + (ULONG)nOffset,PSEEK_SET,NULL);
			*/
		}
		else
		{
			DEBUG_FILE("FIL:Invalid positive offset from eof requested of seek\n");
			return -1;
		}
	}
	return -1;
}

void CFile::SetFile(const char* szname)
{
	DEBUG_FILE("FIL:SetFile\n");
	// copy name to member
	if (m_pName)
	{
		DEBUG_FILE("delete old string\n");
		delete [] m_pName;
	}
	m_pName = new char[strlen(szname)];
	DEBUG_FILE("got new chars\n");
	strcpy (m_pName, szname);
	DEBUG_FILE("copied new name %s\n",m_pName);
	if (!m_pName)
	{	
		DEBUG_FILE("FIL:Out of Memory for PIS filename allocation\n");
	}
	DEBUG_FILE("calling into open\n");
	m_bOpened = false;
}

void CFile::InvalidateHandle()
{
	DEBUG_FILE("fil:invalidate file handle\n");
	m_hFile = -1;
	m_bOpened = false;
	m_bOpenedSloppy = false;
}
