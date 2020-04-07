//
// SimpleContentManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef SIMPLECONTENTMANAGER_H_
#define SIMPLECONTENTMANAGER_H_

#include <content/common/ContentManager.h>
#include <util/datastructures/SimpleList.h>

typedef IMetadata* FNCreateMetadata();

//////////////////////////////////////////////////////////////////////////////////////////
//	CSimpleContentManager
//////////////////////////////////////////////////////////////////////////////////////////
//
//! The CSimpleContentManager implements the IContentManager interface in a
//! simple, non-optimized manner.
//
class CSimpleContentManager : public IContentManager
{
public:
    //! pfnCreateMetadata is a pointer to the function that will be called to generate a new IMetadata record
    //! to be populated during content refreshes.
    //! If pfnCreateMetadata is 0, then no metadata record will be generated (and no
    //! metadata will be retrieved).
    CSimpleContentManager(FNCreateMetadata* pfnCreateMetadata);
    ~CSimpleContentManager();

    //! Creates an empty metadata record.
    IMetadata* CreateMetadataRecord() const;

    //! Clears the content manager and deletes its content records.
    void Clear();

    //! Called from the event handler when a data source returns a list of records.
    //! If pRecords is non-zero, then it will be filled with a list of pointers to the new records.
    void AddContentRecords(content_record_update_t* pContentUpdate, MediaRecordList* pRecords = 0);

    //! Returns the number of media records in the content manager.
    int GetMediaRecordCount();

    //! Adds an entry to the content manager.
    //! Returns a pointer to the content record.
    //! If pbAlreadyExists is not null, then its value is set to true if the content record with
    //! the same URL already exists in the content manager, false otherwise.
    IMediaContentRecord* AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists = 0);

    //! Removes a record from the content manager.
    //! Returns true if the content record was deleted, false if it wasn't found.
    bool DeleteMediaRecord(int iContentRecordID);

    //! Returns a pointer to the media content record with the specified record ID.
    //! Returns 0 if no record with a matching ID was found.
    IMediaContentRecord* GetMediaRecord(int iContentRecordID);

    //! Returns a pointer to the media content record with the specified URL.
    //! Returns 0 if no record with a matching URL was found.
    IMediaContentRecord* GetMediaRecord(const char* szURL);

    //! Returns the number of playlist records in the content manager.
    int GetPlaylistRecordCount();

    //! Adds a playlist record to the content manager.
    //! Returns a pointer to the record.
    IPlaylistContentRecord* AddPlaylistRecord(playlist_record_t& playlistRecord);

    //! Removes a playlist record from the content manager.
    //! Returns true if the content record was deleted, false if it wasn't found.
    bool DeletePlaylistRecord(int iContentRecordID);

    //! Removes all records from the given data source from the content manager.
    void DeleteRecordsFromDataSource(int iDataSourceID);

    //! Marks all records from the given data source as unverified.
    void MarkRecordsFromDataSourceUnverified(int iDataSourceID);

    //! Removes all records from the given data source that are marked as unverified.
    void DeleteUnverifiedRecordsFromDataSource(int iDataSourceID);

    void GetAllMediaRecords(MediaRecordList& records) const;
    void GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const;

    void GetAllPlaylistRecords(PlaylistRecordList& records) const;
    void GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const;

private:

    MediaRecordList  m_slMediaRecords;
    PlaylistRecordList      m_slPlaylists;

    //! Pointer to the function used to create metadata.
    FNCreateMetadata* m_pfnCreateMetadata;

    static int sm_iNextMediaRecordID;
    static int sm_iNextPlaylistRecordID;

    MediaRecordIterator FindMediaRecordByID(int iContentRecordID);
    MediaRecordIterator FindMediaRecordByURL(const char* szURL);
    void ListAllRecords(MediaRecordList& records) const;

    PlaylistRecordIterator FindPlaylistRecordByID(int iContentRecordID);
    PlaylistRecordIterator FindPlaylistRecordByURL(const char* szURL);

};


//////////////////////////////////////////////////////////////////////////////////////////
//	CSimpleMediaContentRecord
//////////////////////////////////////////////////////////////////////////////////////////

//! A demo derivation of the IMediaContentRecord interface. This object is not
//! part of the SDK proper.
class CSimpleMediaContentRecord : public IMediaContentRecord
{
public:
    CSimpleMediaContentRecord(int iID, char* szURL, int iDataSourceID, int iCodecID,
                              IMetadata* pMetadata, bool bVerified);
    ~CSimpleMediaContentRecord();

    // IContentRecord functions
    int GetID() const
        { return m_iID; }
    const char* GetURL() const
        { return m_szURL; }
    int GetDataSourceID() const
        { return m_iDataSourceID; }
    bool IsVerified() const
        { return m_bVerified; }
    void SetVerified(bool bVerified)
        { m_bVerified = bVerified; }
    ContentRecordStatus GetStatus() const
        { return m_eStatus; }
    void SetStatus(ContentRecordStatus status)
        { m_eStatus = status; }

    // IMediaContentRecord functions
    int GetCodecID() const
        { return m_iCodecID; }
    const TCHAR* GetTitle() const;
    const TCHAR* GetAlbum() const;
    const TCHAR* GetGenre() const;
    const TCHAR* GetArtist() const;

    void SetDataSourceID(int iDataSourceID);
    void SetCodecID(int iCodecID);
    void SetTitle(const TCHAR* szTitle);
    void SetAlbum(const TCHAR* szAlbum);
    void SetGenre(const TCHAR* szGenre);
    void SetArtist(const TCHAR* szArtist);

    // IMetadata functions
    IMetadata* Copy() const
        {
            return m_pMetadata ? m_pMetadata->Copy() : 0;
        }
    bool UsesAttribute(int iAttributeID) const
        {
            return m_pMetadata ? m_pMetadata->UsesAttribute(iAttributeID) : METADATA_NOT_USED;
        }
    ERESULT SetAttribute(int iAttributeID, void* pAttributeValue)
        {
            return m_pMetadata ? m_pMetadata->SetAttribute(iAttributeID, pAttributeValue) : METADATA_NOT_USED;
        }
    ERESULT UnsetAttribute(int iAttributeID)
        {
            return m_pMetadata ? m_pMetadata->UnsetAttribute(iAttributeID) : METADATA_NOT_USED;
        }
    ERESULT GetAttribute(int iAttributeID, void** ppAttributeValue) const
        {
            return m_pMetadata ? m_pMetadata->GetAttribute(iAttributeID, ppAttributeValue) : METADATA_NOT_USED;
        }
    void MergeAttributes(const IMetadata* pMetadata, bool bOverwrite = false)
        {
            if (m_pMetadata)
                m_pMetadata->MergeAttributes(pMetadata, bOverwrite);
        }
    void ClearAttributes()
        {
            if (m_pMetadata)
                m_pMetadata->ClearAttributes();
        }


protected:

    int         m_iID;  // Unique content record ID
    char*       m_szURL;
    int         m_iDataSourceID;
    int         m_iCodecID;
    IMetadata*  m_pMetadata;
    bool        m_bVerified;
    ContentRecordStatus m_eStatus;
};

//////////////////////////////////////////////////////////////////////////////////////////
//	CSimplePlaylistRecord
//////////////////////////////////////////////////////////////////////////////////////////

//! A demo derivation of the IPlaylistContentRecord interface. This
//! class is not part of the sdk proper
class CSimplePlaylistRecord : public IPlaylistContentRecord
{
public:

    CSimplePlaylistRecord(int iID, char* szURL, int iDataSourceID, int iPlaylistFormatID, bool bVerified);
    ~CSimplePlaylistRecord();

    // IContentRecord functions
    int GetID() const
        { return m_iID; }
    const char* GetURL() const
        { return m_szURL; }
    void SetDataSourceID(int iDataSourceID)
        { m_iDataSourceID = iDataSourceID; }
    int GetDataSourceID() const
        { return m_iDataSourceID; }
    bool IsVerified() const
        { return m_bVerified; }
    void SetVerified(bool bVerified)
        { m_bVerified = bVerified; }
    ContentRecordStatus GetStatus() const
        { return m_eStatus; }
    void SetStatus(ContentRecordStatus status)
        { m_eStatus = status; }

    // IPlaylistContentRecord functions
    void SetPlaylistFormatID(int iPlaylistFormatID)
        { m_iPlaylistFormatID = iPlaylistFormatID; }
    int GetPlaylistFormatID() const
        { return m_iPlaylistFormatID; }

private:

    int         m_iID;  // Unique content record ID
    char*       m_szURL;
    int         m_iDataSourceID;
    int         m_iPlaylistFormatID;
    bool        m_bVerified;
    ContentRecordStatus m_eStatus;
};

//@}

#endif	// SIMPLECONTENTMANAGER_H_
