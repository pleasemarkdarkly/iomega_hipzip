// UpdateManager.h: single codebase for cddb/fw update process
// danc@fullplaymedia.com
// (c) fullplay: we put the play in "foreplay"

#ifndef __UPDATEMANAGER_H__
#define __UPDATEMANAGER_H__

#include <util/datastructures/SimpleList.h> // SimpleList

#include <main/util/update/ParseConfig.h>   // FileInfo

// Since we have to bitfield this, it's not a prime candidate for ERESULTS. this is unfortunate, since
// it limits our ability to get decent error values

#define UM_UPDATE_UNAVAIL  0x00    // no update available
#define UM_UPDATE_FIRMWARE 0x01    // new firmware available
#define UM_UPDATE_SYSTEM   0x02    // new system files available
#define UM_UPDATE_CDDBFULL 0x04    // full version of cddb available
#define UM_UPDATE_CDDBUPDA 0x08    // cddb update available
#define UM_ERROR           0x10    // generic error code

typedef unsigned int update_t;

typedef void (*FileCopyStatusCB)(const char* szName, int iCurrent, int iTotal);
typedef void (*FlashUpdateStatusCB)(int iCurrent, int iTotal);


// Used to hold a list of CDDB update files and
// their update levels found on the CD.
typedef struct cddb_update_info_s
{
	char    path[512];    // how long can a path be? URL?
    int     update_level;
} cddb_update_info_t;


class CUpdateManager
{
public:
    // Singleton interface
    static CUpdateManager* GetInstance();
    static void Destroy();

    // Construct/destruct
    CUpdateManager();
    ~CUpdateManager();

    // Clear the last saved config file - on CD eject maybe? Doesn't change callbacks
    void Reset();
    
    // Configure the update manager to make status callbacks
    void Configure( FileCopyStatusCB fileCB, FlashUpdateStatusCB flashCB );
    
    // Look for updates from a given source
    // Returns: the appropriate UM_UPDATE_* codes (bitwise field)
    update_t CheckForUpdates( const char* szSourceURL );

    // Copy updates (including the config file) from a given source to a (presumably local) URL
    // Returns: the appropriate UM_UPDATE_* codes (bitwise field) that specifies 
    //          what files were copied and ready to be updated.  Use this returned structure
    //          to see if it copied all of expected files correctly
    update_t CopyUpdatesFromSource( const char* szSourceURL, const char* szDestinationURL, update_t updates );

    // Perform an update of the given items from the specified source
    // Returns: 0 on success, -1 on failure
    int UpdateFromSource( const char* szSourceURL, update_t updates );

    // Fetch a list of CDDB update files from the specified source
    // This does version comparison with CDDB
    // Returns: -1 on error, otherwise number of new files found
    int GetCDDBUpdateList( const char* szSourceURL, SimpleList<cddb_update_info_t>& cddbfiles );

    // Copy a file from one location to another
    int CopyFile( const char* szSourceURL, const char* szDestinationURL );

    // Delete all update files at a given source URL (currently doesn't delete directories)
    // This is intended to be used after copying update files from a source to the local HD
    // and is intended to only work for URLs pointing to the local HD
    // Returns: -1 on error, otherwise number of files deleted
    int DeleteUpdatesAtSource( const char* szSourceURL );
    
    // Delete all (hd) updates - proxied call to UpdateApp for now
    void DeleteUpdates();
    
private:
    static CUpdateManager* s_pInstance;

    // Proxy support for the (incapable) flash callback routine
    static void FlashCallbackProxy();
    void FlashCBProxy();
    int m_iCurrentFlash, m_iTotalFlash;
    
    static void DefaultFileStatusCB( const char* szName, int iCurrent, int iTotal );
    static void DefaultFlashStatusCB( int iCurrent, int iTotal );
    
    FileCopyStatusCB m_FileCB;
    FlashUpdateStatusCB m_FlashCB;

    int UpdateFirmware( const char* szSourceURL );
    int UpdateCDDB( SimpleList<cddb_update_info_t>& cddbfiles );

    int CopyFiles( char* szSourceDir, char* szDestDir, SimpleList<FileInfo>& files );
    
    char* GetConfig( const char* szURL );

    int GetCDDBUpdateCount( CParseConfig& parser );
    int GetCDDBUpdateListHelper( CParseConfig& parser, SimpleList<cddb_update_info_t>& cddbfiles, bool bGetList );
    
    // Config caching
    char* m_pLastConfigFileSource;
    char* m_pLastConfigFile;
    int m_iLastConfigFileLen;
    update_t m_LastUpdates;
};

#endif // __UPDATEMANAGER_H__
