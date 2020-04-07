//
// FatFile.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef __FATFILE_H__
#define __FATFILE_H__

#define FATFILE_IOCTL_TRUNCATE 0x01 // allowable for files opened for writing

// CFatFile class
//! Since FAT filesystems have both read and write ability, we provide
//! a common class to implement FAT file i/o operations, and wrap this in
//! other useful classes (such as CFatFileInputStream and CFatFileOutputStream).
class CFatFile
{
public:
    //! An enumeration for controlling seek origins
    enum FileSeekPos
    {
        SeekStart = 0,
        SeekCurrent,
        SeekEnd,
    };

    //! An enumeration for determining file open modes
    enum FileMode
    {
        ReadOnly,
        WriteOnly,
        ReadWrite,
        WriteCreate,
        WriteAppend,
    };
    
    CFatFile();
    ~CFatFile();

    // returns true on success, false on failure
    //! Attempt to open the given file in the specified file mode.
    bool Open( const char* Filename, FileMode Mode = ReadWrite );

    //! Close the open file
    bool Close();

    //! Read the specified number of bytes from the file to the buffer
    int Read( void* Buffer, int Count );
    
    //! Write the specified number of bytes from the buffer to the
    //! file
    int Write( const void* Buffer, int Count );

    //! Flush any buffered data to the disk
    bool Flush();

    //! A generic interface for disk commands, not currently supported
    int Ioctl( int Key, void* Value );

    //! Seek from the specified origin to the offset
    int Seek( FileSeekPos Origin, int Offset );

    //! Returns the current offset (position) within the file
    int GetOffset() const;

    //! Return the length of the current file
    int Length() const;

    //! Return the current position
    int Position() const;

    //! Static routine to delete a file from the filesystem
    static bool Delete( const char* Filename );

    //! Return the new length, after shortening to the given size.
    int Truncate( int nSize );

    //! Guard read access to the true end of the file.
    void SetSafeLen( int nLen );
    int GetSafeLen();
    
private:
    char m_Filename[256];
    int m_FD;
    int m_iPosition;
    int m_iLength;
    int m_iFlags;

    int m_nSafeLen;
};

//@}

#endif //  __FATFILE_H__
