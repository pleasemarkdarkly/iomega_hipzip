//
// NetDataSourceImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef NETDATASOURCEIMP_H_
#define NETDATASOURCEIMP_H_

#include <datasource/netdatasource/NetDataSource.h>

#include <util/timer/Timer.h> // for timer_handle_t

// fdecl
class IInputStream;
class IContentManager;
typedef struct media_record_info_s media_record_info_t;
//struct eth_drv_sc;
//struct ether_drv_stats;

class CNetDataSourceImp : public IDataSource
{
public:
    
    CNetDataSourceImp( int iInterface, bool iForceInit );
    ~CNetDataSourceImp();

    int GetClassID() const
        { return NET_DATA_SOURCE_CLASS_ID; }
    void SetInstanceID( int iDataSourceID )
        { m_iDataSourceID = iDataSourceID; SynchConnectedEvent(); }
    int GetInstanceID() const
        { return m_iDataSourceID; }

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
    
    // Not implemented.  Entries are added manually by calling GenerateEntry.
    ERESULT ListAllEntries(unsigned short usScanID, RefreshMode mode, int iUpdateChunkSize)
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
    int GetSerialNumberLength() const;

    //! Get the serial number from the media and copy it into the buffer.
    //! Returns the number of bytes copied, or -1 if an error was occurred.
    int GetSerialNumber( char* pBuffer, int iBufferLen ) const;

    //! Indicates whether or not the datasource supports prebuffering.
    bool QueryCanPrebuffer( const IContentRecord* pRecord ) const
        { return false; }

    // Add an entry to the content manager by parsing the given url
    IMediaContentRecord* GenerateEntry( IContentManager* pContentManager, const char* pURL, int iCodecID ) const;

    //! Returns true if the network is initialized, false otherwise.
    bool IsInitialized();
    
private:
    
    int ParseURL( const char* pURL, media_record_info_t& mediaContent, int iCodecID ) const;

    void SynchConnectedEvent();
    
    int m_iDataSourceID;

	//! incremented by the timer routine when the network is not available
	int m_iNetworkDownCount;

    //! incremented by the timer routine when the network is available but not initializing
    //! (i.e. connected but no DHCP server available)
    int m_iNetworkInitFailCount;
    
    void TryStartLink( bool iForceInit = false );
    static void CheckMediaStatusCB(void*);

    void SettingsChanged();
    static void SettingsChangeCB(const char*, void*);
    
    const char* m_pName;
    int m_iInterface;
    timer_handle_t m_TimerHandle;
    bool m_bNetworkInitialized;
	bool m_bNetworkConnected;
    
};


#endif  // NETDATASOURCEIMP_H_
