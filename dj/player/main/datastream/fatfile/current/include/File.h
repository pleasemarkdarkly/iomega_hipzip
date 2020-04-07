#ifndef __PIS_INCLUDE_FILE_DEFINED__ 
#define __PIS_INCLUDE_FILE_DEFINED__

#include "sdapi.h"
#include <datastream/input/InputStream.h>
#include <cyg/kernel/kapi.h>
#include <cyg/kernel/thread.hxx>													

class CFile : public IInputStream
{
public:
	CFile();
	~CFile();
	void SetFile(const char* szname);
	bool SameFile(const char* szname);
	int Read(unsigned char* pBuffer, unsigned int nBytes);
	int Seek(int nOffset, InputSeekPos eOrigin);
	void Close();
	int Length();
	void InvalidateHandle();
	int Open(const char* szname);
	int m_nCharsRead;	
private:
	int Open();
	bool m_bOpened;		
	bool m_bOpenedSloppy;
	int	m_nFileSize;
	cyg_mutex_t m_mutFS;
	PCFD m_hFile;
	char* m_pName;
};

#endif // __PIS_INCLUDE_FILE_DEFINED__
