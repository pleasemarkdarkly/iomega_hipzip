// DPFile.h: simple class to wrap dataplay layer interaction
// ericg@iobjects.com 08/10/01
// (c) Interactive Objects

#ifndef __DPFILE_H__ 
#define __DPFILE_H__ 

typedef int DFSHANDLE;

class CDPFile
{
public:
    enum FileSeekPos
    {
        SeekStart = 0,
        SeekCurrent,
        SeekEnd,
    };
    enum FileMode
    {
        ReadOnly,
        WriteOnly,
        ReadWrite,
        WriteCreate,
        WriteAppend,
    };
    
	CDPFile();
	~CDPFile();
	int Open(const char* szname);
	int Close();

	int Read(void* pBuffer, unsigned int nBytes);
    int Write( const void* Buffer, int Count );
    bool Flush();
    int Ioctl( int Key, void* Value );

	int Seek(FileSeekPos eOrigin, int nOffset );
    int GetOffset() const;

	int Length();
	
	void SetParent(DFSHANDLE hParentDir) { m_hParentDir = hParentDir; }	// since no paths in dfs, need to have a parent handle to open a file
private:
    int m_FD;
    int m_iPosition;
	int m_iLength;
	DFSHANDLE m_hParentDir;
};

#endif //	__DPFILE_H__ 
