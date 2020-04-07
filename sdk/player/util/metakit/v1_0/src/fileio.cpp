// fileio.cpp --
// $Id: fileio.cpp,v 1.14 2001/12/11 01:59:51 wcvs Exp $
// This is part of MetaKit, see http://www.equi4.com/metakit/

/** @file
 * Implementation of c4_FileStrategy and c4_FileStream
 */

#include "header.h"
#include <util/metakit/mk4io.h>

// start address of sram
#ifdef __EDB7312
#include <pkgconf/mlt_arm_edb7312_ram.h>
#endif



#if q4_WIN32
#if q4_MSVC && !q4_STRICT
#pragma warning(disable: 4201) // nonstandard extension used : ...
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

#if q4_UNIX && HAVE_MMAP
#include <sys/types.h>
#include <sys/mman.h>
#endif

#if q4_UNIX
#include <unistd.h>
#include <fcntl.h>
#endif

#include <time.h>
#include <string.h>

#include <util/debug/debug.h>
DEBUG_MODULE(DEBUG_MK_FILEIO);
DEBUG_USE_MODULE(DEBUG_MK_FILEIO);

/////////////////////////////////////////////////////////////////////////////

#if q4_CHECK
#include <stdlib.h>

void f4_AssertionFailed(const char* cond_, const char* file_, int line_)
{
    fprintf(stderr, "Assertion failed: %s (file %s, line %d)\n",
                        cond_, file_, line_);
    abort();
}
#endif

/////////////////////////////////////////////////////////////////////////////
// c4_FileStream

c4_FileStream::c4_FileStream (CFatFile* stream_, bool owned_)
    : _stream (stream_), _owned (owned_)
{
}

c4_FileStream::~c4_FileStream ()
{
    if (_owned)
        _stream->Close();
}

int c4_FileStream::Read(void* buffer_, int length_)
{
    d4_assert(_stream != 0);

    return (int) _stream->Read(buffer_, length_);
}

bool c4_FileStream::Write(const void* buffer_, int length_)
{
    d4_assert(_stream != 0);

    return (int) _stream->Write(buffer_, length_) == length_;
}

/////////////////////////////////////////////////////////////////////////////
// c4_FileStrategy

c4_FileStrategy::c4_FileStrategy (CFatFile* file_)
    : _file (file_), _cleanup (0)
{
    ResetFileMapping();
}

c4_FileStrategy::~c4_FileStrategy ()
{
    _file = 0;
    ResetFileMapping();

    if (_cleanup)
    {
        _cleanup->Close();
        delete _cleanup;
    }

    d4_assert(_mapStart == 0);
}

bool c4_FileStrategy::IsValid() const
{ 
    return _file != 0; 
}

t4_i32 c4_FileStrategy::FileSize()
{
    d4_assert(_file != 0);

    long size = _file->Length();

    return size;
}

t4_i32 c4_FileStrategy::FreshGeneration()
{
    return time(0);
}

void c4_FileStrategy::ResetFileMapping()
{
  #if q4_WIN32
    if (_mapStart != 0)
    {
	_mapStart -= _baseOffset;
        d4_dbgdef(BOOL g =)
          ::UnmapViewOfFile((char*) _mapStart);
        d4_assert(g);
        _mapStart = 0;
	_dataSize = 0;
    }

    if (_file != 0)
    {
        t4_i32 len = FileSize();

        if (len > 0)
        {
            FlushFileBuffers((HANDLE) _get_osfhandle(_fileno(_file)));
            HANDLE h = ::CreateFileMapping((HANDLE) _get_osfhandle(_fileno(_file)),
                                                0, PAGE_READONLY, 0, len, 0);
            d4_assert(h);   // check for errors, but can continue without mapping

            if (h)
            {
                _mapStart = (t4_byte*) ::MapViewOfFile(h, FILE_MAP_READ, 0, 0, len);
                d4_assert(_mapStart != 0);

                if (_mapStart != 0)
		{
		    _mapStart += _baseOffset;
		    _dataSize = len - _baseOffset;
		}

        d4_dbgdef(BOOL f =)
	  ::CloseHandle(h);
                d4_assert(f);
            }
        }
    }
  #elif q4_UNIX && HAVE_MMAP
    if (_mapStart != 0)
    {
	_mapStart -= _baseOffset;
        munmap((char*) _mapStart, _baseOffset + _dataSize); // also loses const
        _mapStart = 0;
	_dataSize = 0;
    }

    if (_file != 0)
    {
        t4_i32 len = FileSize();

        if (len > 0)
        {
            _mapStart = (const t4_byte*) mmap(0, len, PROT_READ, MAP_SHARED,
                                                        fileno(_file), 0);
            if (_mapStart != (void*) -1L)
	    {
		_mapStart += _baseOffset;
		_dataSize = len - _baseOffset;
	    }
	    else
		_mapStart = 0;
        }
    }
  #endif
}

bool c4_FileStrategy::DataOpen(const char* fname_, int mode_)
{
    d4_assert(!_file);

    char* filename = 0;
    if (fname_[1] != ':' && fname_[2] != '/')
    {
        filename = new char[strlen(fname_) + 4];
        strcpy (filename,"a:/");
        strcat(filename,fname_);
    }
    else
        filename = (char*)fname_;

    _cleanup = 0;
    _file = new CFatFile;
    if (_file->Open(filename, mode_ > 0 ? CFatFile::ReadWrite : CFatFile::ReadOnly))
    {
        //setbuf(_file, 0); // 30-11-2001
        _cleanup = _file;
        ResetFileMapping();
        return true;
    }

//   d4_assert(_file != 0);
    return false;
}

int c4_FileStrategy::DataRead(t4_i32 pos_, void* buf_, int len_)
{
    d4_assert(_baseOffset + pos_ >= 0);
    d4_assert(_file != 0);

    _file->Seek(CFatFile::SeekStart, _baseOffset + pos_);
    return _file->Read(buf_, len_);
    
//    return fseek(_file, _baseOffset + pos_, 0) != 0 ? -1 :
//	    (int) fread(buf_, 1, len_, _file);
}

void c4_FileStrategy::DataWrite(t4_i32 pos_, const void* buf_, int len_)
{
    d4_assert(_baseOffset + pos_ >= 0);
    d4_assert(_file != 0);
    
    int filelen = _file->Length();
    if (_baseOffset + pos_ > filelen ) {
        long diff = _baseOffset + pos_ - filelen;
        const char* fast_random_buffer = (char*)CYGMEM_REGION_sram;

        _file->Seek(CFatFile::SeekStart, filelen);
        if (diff < 16*1024)
            _file->Write(fast_random_buffer,diff);
        else {
            while (diff > 0) {
                _file->Write(fast_random_buffer,diff > 16*1024 ? 16*1024 : diff);
                diff -= 16*1024;
            }
        }
    }
    int ret = _file->Seek(CFatFile::SeekStart, _baseOffset + pos_);
    
    if (ret != _baseOffset + pos_) {
        DEBUGP(DEBUG_MK_FILEIO, DBGLEV_WARNING, "seek in write doesn't match request (%d, %d)\n",ret,_baseOffset + pos_);
        // Things never get better from this point on.  Reset and let the recovery process handle this.
        DBASSERT(DEBUG_MK_FILEIO, ret == _baseOffset + pos_, "seek in write doesn't match request (%d, %d)\n",ret,_baseOffset + pos_);
    }
    
    ret = _file->Write(buf_, len_);
    
    if (ret != len_) {
        DEBUGP(DEBUG_MK_FILEIO, DBGLEV_WARNING, "write in datawrite doesn't match request (%d, %d)\n",ret,len_);
        // Things never get better from this point on.  Reset and let the recovery process handle this.
        DBASSERT(DEBUG_MK_FILEIO, ret == len_, "write in datawrite doesn't match request (%d, %d)\n",ret,len_);
    }

/*
    if (fseek(_file, _baseOffset + pos_, 0) != 0 ||
	    (int) fwrite(buf_, 1, len_, _file) != len_)
    {
        _failure = ferror(_file);
        d4_assert(_failure != 0);
        d4_assert(true); // always force an assertion failure in debug mode
    }
*/
}

void c4_FileStrategy::DataCommit(t4_i32 limit_)
{
    d4_assert(_file != 0);

/*
    if (fflush(_file) < 0)
    {
        _failure = ferror(_file);
        d4_assert(_failure != 0);
        d4_assert(true); // always force an assertion failure in debug mode
	return;
    }
*/
    _file->Flush();

    if (limit_ > 0)
    {
#if 0 // can't truncate file in a portable way!
            // unmap the file first, WinNT is more picky about this than Win95
        FILE* save = _file;

        _file = 0;
        ResetFileMapping();
        _file = save;

        _file->SetLength(limit_); // now we can resize the file
#endif
        ResetFileMapping(); // remap, since file length may have changed
    }
}

/////////////////////////////////////////////////////////////////////////////
