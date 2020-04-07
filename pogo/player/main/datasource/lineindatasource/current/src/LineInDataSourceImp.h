//
// LineInDataSourceImp.h
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef LINEINDATASOURCEIMP_H_
#define LINEINDATASOURCEIMP_H_

#include <main/datasource/lineindatasource/LineInDataSource.h>

class CLineInDataSourceImp : public IDataSource
{
public:
    
    CLineInDataSourceImp( );
    ~CLineInDataSourceImp();

    int GetClassID() const
        { return LINE_IN_DATA_SOURCE_CLASS_ID; }
    int GetInstanceID() const
        { return m_iDataSourceID; }
    void SetInstanceID( int iDataSourceID )
        { m_iDataSourceID = iDataSourceID; }

    //! Not implemented.  Entries are added manually by calling GenerateEntry.
    void SetDefaultRefreshMode(RefreshMode mode) { }
    //! Not implemented.  Entries are added manually by calling GenerateEntry.
    RefreshMode GetDefaultRefreshMode() const
        { return DSR_DEFAULT; }

    //! Not implemented.  Entries are added manually by calling GenerateEntry.
    void SetDefaultUpdateChunkSize(int iUpdateChunkSize) { }
    //! Not implemented.  Entries are added manually by calling GenerateEntry.
    int GetDefaultUpdateChunkSize() const { return 0; }

    // Copies the string the data source uses to prefix its URLs into the given string provided.
    bool GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const;
    // Given a URL from this data source, this function creates a URL prefix for the local path.
    bool GetLocalURLPrefix(const char* szURL, char* szLocalURLPrefix, int iMaxLength) const
        { return false; }
    
    // Not implemented.  Entries are added manually by calling GenerateEntry.
    ERESULT ListAllEntries(RefreshMode mode, int iUpdateChunkSize)
        { return MAKE_ERESULT(SEVERITY_FAILED, 0, 0); }

    // Not implemented.  Entries are added manually by calling GenerateEntry.
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

    // Add an entry to the content manager by parsing the given url
//    IMediaContentRecord* GenerateEntry( IContentManager* pContentManager, const char* pURL );
    
private:
    
    int m_iDataSourceID;
    
//    int ParseURL( const char* pURL, media_record_info_t& mediaContent );
};


#endif  // LINEINDATASOURCEIMP_H_
