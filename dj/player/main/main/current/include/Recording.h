// Recording.h: helper functions for recording content
// edwardm@iobjects.com 11/27/01
// (c) Interactive Objects

#ifndef RECORDING_H_
#define RECORDING_H_

#include <content/common/ContentManager.h>  // media_record_info_t
#include <core/mediaplayer/PlayStream.h>    // playstream_settings_t
#include <main/main/AppSettings.h>          // PLAYLIST_STRING_SIZE
#include <playlist/common/Playlist.h>       // PlaylistMode
#include <util/eresult/eresult.h>

#define RECORDING_ERROR_ZONE   0x81

const int RECORDING_NO_ERROR        = MAKE_ERESULT( SEVERITY_SUCCESS, RECORDING_ERROR_ZONE, 0x00 );
//! Unspecified error.
const int RECORDING_ERROR           = MAKE_ERESULT( SEVERITY_SUCCESS, RECORDING_ERROR_ZONE, 0x01 );
//! InitializeRecording() failed or hasn't been called yet.
const int RECORDING_NOT_INITIALIZED = MAKE_ERESULT( SEVERITY_FAILED,  RECORDING_ERROR_ZONE, 0x02 );
//! No disk in the CD drive to rip.
const int RECORDING_NO_DISK         = MAKE_ERESULT( SEVERITY_FAILED,  RECORDING_ERROR_ZONE, 0x03 );
//! No tracks on the CD drive to rip.
const int RECORDING_NO_TRACKS       = MAKE_ERESULT( SEVERITY_FAILED,  RECORDING_ERROR_ZONE, 0x04 );
//! Not enough space on the hard drive.
const int RECORDING_NO_SPACE        = MAKE_ERESULT( SEVERITY_FAILED,  RECORDING_ERROR_ZONE, 0x05 );
//! The maximum number of hard drive tracks has been reached.
const int RECORDING_TRACK_LIMIT_REACHED = MAKE_ERESULT( SEVERITY_FAILED,  RECORDING_ERROR_ZONE, 0x06 );
//! Track has already been recorded.
const int RECORDING_DUPLICATE       = MAKE_ERESULT( SEVERITY_FAILED,  RECORDING_ERROR_ZONE, 0x07 );
//! Track is a radio stream.
const int RECORDING_RADIO_STREAM    = MAKE_ERESULT( SEVERITY_FAILED,  RECORDING_ERROR_ZONE, 0x08 );


// fdecl
class CCDDataSource;
class CCDDirGen;
class CFatDataSource;
class IMediaContentRecord;
class CNetDataSource;
class IUserInterface;
typedef struct set_stream_event_data_s set_stream_event_data_t;
typedef struct change_stream_event_data_s change_stream_event_data_t;
typedef struct file_copier_finish_s file_copier_finish_t;
typedef struct cdda_toc_s cdda_toc_t;

class CRecordingManager
{
public:

    //! Returns a pointer to the global recording manager.
    static CRecordingManager* GetInstance();

    //! Destroy the singleton global recording manager.
    static void Destroy();

    //! Give the recording manager a 
    void SetUserInterface( IUserInterface* pUI );

    //! Tell the recording manager that a CD is in the drive.
    //! If recording is possible then LED will turn on.
    void NotifyCDInserted(const cdda_toc_t* pTOC);

    //! Tell the recording manager that the CD has been ejected.
    //! Stops recording if the current data source is the CD.
    void NotifyCDRemoved();

    //! Called from the event after when the idle coder finishes encoding a file.
    //! The old entry is removed from the content manager and replaced with the new version.
    void NotifyEncodingFinished(const char* szInURL, const char* szOutURL);

    //! Called from the event loop after the file copier finishes copying a file.
    //! A new entry is added to the content manager.
    void NotifyCopyingFinished(file_copier_finish_t* pFileCopyInfo);


    //! Tells the recording manager to record the current track and continue
    //! recording after that track is done.
    ERESULT StartRecording();
    //! Tells the recording manager to record all tracks on the CD.
    //! This will create a playlist of all CD tracks and set the first one.
    //! On EVENT_STREAM_SET playback will begin.
    ERESULT StartRecordFull();
    //! Starts recording the given track.
    //! IMPORTANT!  This function assumes that the caller has already checked to
    //! make sure that the current track is a CD track.
    ERESULT StartRecordSingle(IPlaylistEntry* pEntry);
    //! Stops recording.
    ERESULT StopRecording(bool bPrintMessage = true);
    //! Returns true if in full CD record or single track record mode.
    bool IsRecording() const;
    //! Returns true if in full CD record mode.
    bool IsRecordingFull() const;
    //! Returns true if in single track record mode.
    bool IsRecordingSingle() const;

    //! Tells the recording manager to start ripping.
    ERESULT StartRipping();
    //! Tells the recording manager to rip all tracks in the current playlist.
    //! recording after that track is done.
    ERESULT StartRipPlaylist();
    //! Tells the recording manager to rip all tracks on the CD.
    //! This will create a playlist of all CD tracks and set the first one.
    ERESULT StartRipFull();
    //! Starts ripping the given track.
    //! IMPORTANT!  This function assumes that the caller has already checked to
    //! make sure that the current track is a CD track.
    ERESULT StartRipSingle(IPlaylistEntry* pEntry);
    //! Stops ripping.
    ERESULT StopRipping(bool bPrintMessage = true);
    //! Returns true if in full CD rip or single track rip mode.
    bool IsRipping() const;
    //! Returns true if in full CD rip mode.
    bool IsRippingFull() const;
    //! Returns true if in single track rip mode.
    bool IsRippingSingle() const;

    //! Returns true if the current track will be kept after recording/ripping.
    bool IsKeepingCurrentTrack() const;

    //! Turns off recording for the current track.
    //! \param usStringID The ID of the string in Strings.hpp that explains why recording is disabled.
    void DisableRecording();
    bool IsRecordingEnabled() const;

    //! Restores the play mode set before ripping started.
    void RestorePlayMode();

    //! Called from the event loop whenever a track is set.
    //! Preps a media content record for recording.
    //! Returns true if the current track should be skipped (i.e., we're ripping and
    //! this track has already been ripped), false otherwise.
    bool NotifyStreamSet(set_stream_event_data_t* pSSED, IMediaContentRecord* pContentRecord);

    //! Called from the event loop whenever a track starts playing or from the
    //! player screen when the user presses pause.
    //! Starts/continues copying a data CD.
    void NotifyStreamPlaying();

    //! Called from the event loop whenever a track starts playing or from the
    //! player screen when the user presses pause.
    //! Pauses copying a data CD.
    void NotifyStreamPaused();

    //! Returns true if ripping/recording is paused.
    bool IsPaused() const
        { return m_bPaused; }

    //! Called from the event loop whenever a track ends.
    //! Performs cleanup if ripping or recording.
    //! Returns true if playback should be stopped, false otherwise.
    bool NotifyStreamEnd(change_stream_event_data_t* pCSED);

    //! Called from the event loop whenever a track stops playing before completion
    //! (e.g., next track pressed, stop pressed).
    //! Performs cleanup if ripping or recording.
    void NotifyStreamAbort(change_stream_event_data_t* pCSED);

    //! Called when a partial file is to be deleted.
    void DeletePartialRecording(char* szFileName);

    // Given a URL for a track on the CD, this function returns a pointer to a recorded
    // version of that track on the HD.
    // If this track hasn't been recorded, then 0 is returned.
    IMediaContentRecord* FindCDTrackOnHD(const char* szURL);

    // Given a media content record for a track on CD or FML, this function returns a pointer to a recorded
    // version of that track on the HD.
    // If this track hasn't been recorded, then 0 is returned.
    IMediaContentRecord* FindTrackOnHD(IMediaContentRecord* pContentRecord);

    // Checks space and track limits on the hard drive.
    // If the space limit is reached, then RECORDING_NO_SPACE is returned.
    // If the track limit is reached, then RECORDING_TRACK_LIMIT_REACHED is returned.
    // In both cases a message is printed in the player screen's system text.
    // Otherwise, RECORDING_NO_ERROR is returned.
    ERESULT CheckSpaceAndTrackLimits(bool bPrintMessage = true);

    // Given a media content record this function returns the index in STRING_IDS specifying why
    // the track can't be recorded, or SID_EMPTY_STRING if it can be recorded.
    unsigned short CannotRecord(IMediaContentRecord* pContentRecord);

    // Checks to see if the given content record is a radio stream.
    // A record is judged as a radio stream if it comes from the net
    // and doesn't match the URL pattern of an fml stream.
    bool IsRadioStream(IMediaContentRecord* pContentRecord);

    // Saves/loads the list of encoded URLs waiting to take the place of raw files.
    // This info is stored in the registry.
    void SaveEncodingUpdateList();
    void LoadEncodingUpdateList();

    // Traverses the list of encoding updates and tries to replace raw files with encoded ones.
    void ProcessEncodingUpdates();

    // Called to attempt to replace a raw file with its encoded equivalent.
    // If the raw file is in the current playlist then nothing is done and false is returned.
    // If the raw file's content record isn't in the content manager then assume the file has
    // been deleted.  Delete the encoded file and return true.
    // Otherwise, remove the raw content record from the content manager and replace it with
    // a record for the encoded file.  Delete the raw file and return true.
    bool ProcessEncodingUpdate(const char* szRawURL, const char* szEncodedURL);

    // Called when encoding finishes yet the raw file can't be deleted yet.
    // Adds the URLs of the raw file and the encoded file to the update list for later processing.
    void AddEncodingUpdate(const char* szInURL, const char* szOutURL);

    // Clears the list of pending encoded file updates.
    void ClearEncodingUpdates();

private:

    CRecordingManager();
    ~CRecordingManager();

    static CRecordingManager* s_pSingleton;   // The global singleton recording manager.

    bool m_bCDInitialized;
    bool m_bRecordingEnabled;       // True if the current track can be recorded.
    bool m_bRecording;              // True if the user has chosen to record the track.
    bool m_bRipping;                // True if we're ripping a CD or a single track.
    bool m_bSingle;                 // True if a single track is being ripped/recorded.
    bool m_bPaused;                 // True if ripping/recording is paused.
    bool m_bKeepCurrent;            // True if the current track should be kept after recording finishes.
    bool m_bCopyReady;              // True if a data CD track was copied and is ready for addition to
                                    // the content manager.

    IPlaylist::PlaylistMode m_eMode;    // The play mode the system was in before starting a full rip.

    media_record_info_t     m_mri;  // Content record for the track currently being ripped.
    file_copier_finish_t*   m_pFileCopyInfo;    // Cache file copy info when recording.
    int m_iTempIndex;               // Index used to create temp files for ripping.

    char m_szRipToFileBase[PLAYLIST_STRING_SIZE];   // Base string for constructing filenames to rip CD content to.
    char m_szSaveToURLBase[PLAYLIST_STRING_SIZE];   // Base string for constructing URLs to rip CD content to.
    char m_szCopyToURLBase[PLAYLIST_STRING_SIZE];   // Base string for constructing URLs to copy ISO CD content to.
    char m_szIsoRoot[PLAYLIST_STRING_SIZE];         // Root of the URL for ISO files.

    int m_iPCMCodecID;              // Cached ID of the PCM codec.
    int m_iMP3CodecID;              // Cached ID of the MP3 codec.
    int m_iWMACodecID;              // Cached ID of the WMA codec.
    int m_iWAVCodecID;              // Cached ID of the WAV codec.

    CCDDirGen*      m_pCDDirGen;    // Generates directory names for CD ripping.

    CCDDataSource*  m_pCDDS;
    CFatDataSource* m_pFatDS;
    CNetDataSource* m_pNetDS;
    IUserInterface* m_pUserInterface;

    void InitializeCDRip(const cdda_toc_t* pTOC);
    bool PrepIsoCopy(IMediaContentRecord* pContentRecord, unsigned long ulTrackTime);
    void CleanupCurrentIsoCopy();
    void CleanupPreparedIsoCopy(file_copier_finish_t* pFileCopyInfo);
    bool AddCopyToContentManager(file_copier_finish_t* pFileCopyInfo);

    bool PrepNetCopy(IMediaContentRecord* pContentRecord, unsigned long ulTrackTime);
    bool GetNetCopyURL(IMediaContentRecord* pContentRecord, char* szBuffer, int iBufferSize);

    static bool CreatePlayStreamCB( IMediaContentRecord* pContentRecord, CPlayStreamSettings* pSettings );
    bool CreatePlayStream( IMediaContentRecord* pContenRecord, CPlayStreamSettings* pSettings );

    // Used to store URLs of files that have been encoded but whose raw files are in
    // the playlist (and hence dangerous to copy over).
    typedef struct encode_update_s
    {
        char*   szRawURL;
        char*   szEncodedURL;
    } encode_update_t;

    SimpleList<encode_update_t> m_slEncodeUpdates;

};

#endif  // RECORDING_H_
