//
// LineInDataSource.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef LINEINDATASOURCE_H_
#define LINEINDATASOURCE_H_

#include <datasource/common/DataSource.h>

#define LINE_IN_DATA_SOURCE_CLASS_ID  5

// fdecl
class IContentManager;
class IInputStream;
class CLineInDataSourceImp;
class IMediaContentRecord;
typedef struct media_record_info_s media_record_info_t;

//! The line in data source allows an application to connect
//! to the line in device (the ADC on the board).
class CLineInDataSource : public IDataSource
{
public:
    
    CLineInDataSource( );
    ~CLineInDataSource();

    int GetClassID() const
        { return LINE_IN_DATA_SOURCE_CLASS_ID; }
    int GetInstanceID() const;

    //! Not implemented.  Entries are added manually by calling GenerateEntry.
    void SetDefaultRefreshMode(RefreshMode mode) { }
    //! Not implemented.  Entries are added manually by calling GenerateEntry.
    RefreshMode GetDefaultRefreshMode() const
        { return DSR_DEFAULT; }

    //! Not implemented.  Entries are added manually by calling GenerateEntry.
    void SetDefaultUpdateChunkSize(int iUpdateChunkSize) { }
    //! Not implemented.  Entries are added manually by calling GenerateEntry.
    int GetDefaultUpdateChunkSize() const { return 0; }

    //! Copies the string the data source uses to prefix its URLs into the given string provided.
    bool GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const;
    
    //! Not implemented.  Entries are added manually by calling GenerateEntry.
    ERESULT ListAllEntries(unsigned short usScanID, RefreshMode mode, int iUpdateChunkSize)
        { return MAKE_ERESULT(SEVERITY_FAILED, 0, 0); }

    //! Not implemented.  Entries are added manually by calling GenerateEntry.
    void GetContentMetadata(content_record_update_t* pContentUpdate)
        { }

    //! Asks the source to open this URL for reading.
    //! Returns 0 if the URL was unable to be opened, otherwise
    //! it returns the proper subclass of IInputStream for this file type.
    IInputStream* OpenInputStream(const char* szURL);

    //! Asks the source to open this URL for writing.
    //! Returns 0 if the URL was unable to be opened, otherwise
    //! it returns the proper subclass of IOutputStream for this file type.
    IOutputStream* OpenOutputStream(const char* szURL)
        { return 0; }   // Not supported

    //! Asks the source the length of the media serial number, if available.
    //! Returns 0 if no serial number is available, or the length in bytes.
    int GetSerialNumberLength() const
        { return 0; }   // Not supported

    //! Get the serial number from the media and copy it into the buffer.
    //! Returns the number of bytes copied, or -1 if an error was occurred.
    int GetSerialNumber( char* pBuffer, int iBufferLen ) const
        { return 0; }   // Not supported

    //! Indicates whether or not the datasource supports prebuffering
    bool QueryCanPrebuffer( const IContentRecord* pRecord ) const
        { return false; }

    //! Add an entry to the content manager by parsing the given URL
    IMediaContentRecord* GenerateEntry(IContentManager* pContentManager, const char* pURL);
    
private:
    
    void SetInstanceID(int iDataSourceID);

    CLineInDataSourceImp*   m_pImp;
};

//@}

#endif  // LINEINDATASOURCE_H_
