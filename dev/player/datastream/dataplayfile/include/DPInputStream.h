#ifndef __PIS_INCLUDE_FILE_DEFINED__ 
#define __PIS_INCLUDE_FILE_DEFINED__

#include <datastream/input/InputStream.h>

// TODO: move this to FileInputStreamKeys.h
#define DPFILE_INPUT_ID  0x05

class CDPFile;

class CDPInputStream : public IInputStream
{
public:
    DEFINE_INPUTSTREAM( "DataPlay Input", DPFILE_INPUT_ID );

	CDPInputStream();
	~CDPInputStream();
	
	int Open(const char* szname);
	int Close();

    int Read( void* Buffer, int Count );
    int Ioctl( int Key, void* Value );

	// (epg,8/10/2001): todo fixme, total guess
    int GetInputUnitSize() { return 256; }

	int Length();

	bool CanSeek() const { return true; } 
    int Seek( InputSeekPos Origin, int Offset );
	
    int Length() const;
    int Position() const;

private:
    CDPFile* m_pFile;
};

#endif // __PIS_INCLUDE_FILE_DEFINED__
