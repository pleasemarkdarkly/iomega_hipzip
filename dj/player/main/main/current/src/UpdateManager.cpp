// UpdateManager.cpp: single codebase for cddb/fw update process
// danc@fullplaymedia.com
// (c) fullplay: we put the full in "full of crap"

#include <main/main/UpdateManager.h>
#include <main/main/AppSettings.h>
#include <main/main/FatHelper.h>
#include <main/main/DJPlayerState.h>  // GetVersion()

#include <main/util/update/ParseConfig.h>
#include <main/util/update/UpdateApp.h>

#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_upd.h>

#include <datasource/datasourcemanager/DataSourceManager.h>  // be source independent
#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>

#include <fs/flash/flashmanager.h>

#include <util/debug/debug.h>

#include <string.h>
#include <stdlib.h>  // atoi

DEBUG_MODULE_S(UM, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(UM);

CUpdateManager* CUpdateManager::s_pInstance = NULL;

CUpdateManager*
CUpdateManager::GetInstance()
{
    if( !s_pInstance )
        s_pInstance = new CUpdateManager;
    return s_pInstance;
}

void
CUpdateManager::Destroy()
{
    delete s_pInstance;
    s_pInstance = NULL;
}

CUpdateManager::CUpdateManager()
{
    m_FileCB = DefaultFileStatusCB;
    m_FlashCB = DefaultFlashStatusCB;
    m_iLastConfigFileLen = 0;
    m_pLastConfigFile = NULL;
    m_pLastConfigFileSource = NULL;
}

CUpdateManager::~CUpdateManager()
{
    Reset();
}

void
CUpdateManager::Reset()
{
    m_LastUpdates = UM_UPDATE_UNAVAIL;

    m_iLastConfigFileLen = 0;
    delete [] m_pLastConfigFile;
    delete [] m_pLastConfigFileSource;
    m_pLastConfigFileSource = m_pLastConfigFile = NULL;
}

void
CUpdateManager::Configure( FileCopyStatusCB fileCB, FlashUpdateStatusCB flashCB )
{
    m_FileCB  = fileCB;
    m_FlashCB = flashCB;
}

// Scan a config file for updates
update_t
CUpdateManager::CheckForUpdates( const char* szSourceURL ) 
{
    update_t ret = UM_UPDATE_UNAVAIL;

    Reset();
    
    char* pConfigFile = GetConfig( szSourceURL );

    if( pConfigFile == NULL ) {
        return UM_ERROR;
    }

    CParseConfig parser;

    int len = strlen( pConfigFile );
    if( !parser.ParseBuffer( pConfigFile, len ) ) {
        DEBUG( UM, DBGLEV_ERROR, "Unable to parse config file\n");
        
        delete [] pConfigFile;
        return UM_ERROR;
    }
    
    const char* ver;
    if( (ver = parser.FindVariable(FIRMWARE_GROUP_NAME, APP_VERSION)) != NULL )
    {
        // In release mode, only update to versions other than the current one
#ifdef DJ_RELEASE
        if( atoi(ver) != CDJPlayerState::GetInstance()->GetPlayerVersion() )
#endif
        // firmware update was found
        ret |= UM_UPDATE_FIRMWARE;
    }
    if( parser.FindGroup(SYSTEM_FILES_GROUP_NAME) != NULL )
    {
        // In release mode, only update to versions other than the current one
#ifdef DJ_RELEASE
        if( atoi(ver) != CDJPlayerState::GetInstance()->GetPlayerVersion() )
#endif
        // system files update was found
        ret |= UM_UPDATE_SYSTEM;
    }
    if( parser.FindVariable(CDDB_GROUP_NAME, CDDB_VERSION) != NULL )
    {
        // full version of cddb was found
        ret |= UM_UPDATE_CDDBFULL;
    }

    // check for cddb updates, which have varying tags
    if ( GetCDDBUpdateCount(parser) > 0 )
    {
        ret |= UM_UPDATE_CDDBUPDA;
    }

    m_LastUpdates           = ret;
    m_pLastConfigFile       = pConfigFile;
    m_iLastConfigFileLen    = len;
    m_pLastConfigFileSource = new char[ strlen(szSourceURL) + 1 ];
    strcpy( m_pLastConfigFileSource, szSourceURL );
    
    return ret;
}

// Copy updates (including the config file) from a given source to a (presumably local) URL
update_t
CUpdateManager::CopyUpdatesFromSource( const char* szSourceURL, const char* szDestinationURL, update_t updates ) 
{
    DEBUGP( UM, DBGLEV_TRACE, "CopyUpdatesFromSource [%s]\n                   to [%s]\n", szSourceURL, szDestinationURL );

    update_t ret = UM_UPDATE_UNAVAIL;

    // If someone just called CheckForUpdates on this file, use the cached copy; otherwise
    // call CheckForUpdates() to validate the file, and use the cached buffer
    if( !m_pLastConfigFileSource || strcmp( szSourceURL, m_pLastConfigFileSource ) != 0 ) {
        update_t avail = CheckForUpdates( szSourceURL );
        if( avail == UM_UPDATE_UNAVAIL ) {
            DEBUGP( UM, DBGLEV_TRACE, "Update is Unavailable\n" );
            return ret;
        }
    }

    // copy the config file
    if( CopyFile( szSourceURL, DOWNLOAD_CONFIG_PATH ) < 0 ) {
        DEBUGP( UM, DBGLEV_TRACE, "Failed to copy the cfg file\n" );
        return UM_ERROR;
    }

    // the following will never fail
    CParseConfig parser;
    parser.ParseBuffer( m_pLastConfigFile, m_iLastConfigFileLen );

    // determine the base directory for the source items
    char* szSourceDir = new char[ 512 ];     // eek
    int iEndSourceDir;
    strcpy( szSourceDir, m_pLastConfigFileSource );
    for( iEndSourceDir = strlen(m_pLastConfigFileSource); iEndSourceDir > 0 && szSourceDir[iEndSourceDir] != '/'; iEndSourceDir-- );
    szSourceDir[iEndSourceDir] = '\0';
    
    // determine the base directory for the destination items
    char* szDestDir = new char[ 512 ];       // eek
    int iEndDestDir;
    strcpy( szDestDir, szDestinationURL );
    iEndDestDir = strlen(szDestinationURL);

    //
    // Check for a full CDDB copyrequest
    //
    if( (updates & UM_UPDATE_CDDBFULL) ) {
        if( !(m_LastUpdates & UM_UPDATE_CDDBFULL) ) {
            DEBUG( UM, DBGLEV_ERROR, "Full CDDB copy requested but not available\n");
        }
        else {
            const char* tmp = parser.FindVariable(CDDB_GROUP_NAME, CDDB_VERSION);
            if( atoi(tmp) == 0 ) {
                DEBUG( UM, DBGLEV_ERROR, "Full CDDB copy requested but version is 0\n");
            }
            else {
                SimpleList<FileInfo> cddbfiles;
                if( parser.GetFileList(CDDB_GROUP_NAME, cddbfiles) )
                {
                    if( CopyFiles( szSourceDir, szDestDir, cddbfiles ) >= 0 ) {
                        ret |= UM_UPDATE_CDDBFULL;
                    }
                }
            }
        }
    }

    //
    // Check for the update system files request
    //
    if( (updates & UM_UPDATE_SYSTEM) ) {
        if( !(m_LastUpdates & UM_UPDATE_SYSTEM) ) {
            DEBUG( UM, DBGLEV_ERROR, "System update requested but not available\n");
        }
        else {
            // Get a list of files to copy over
            SimpleList<FileInfo> files;
            if( parser.GetFileList(SYSTEM_FILES_GROUP_NAME, files) )
            {
                if( CopyFiles( szSourceDir, szDestDir, files ) >= 0 ) {
                    ret |= UM_UPDATE_SYSTEM;
                }
            }
        }
    }

    //
    // Check for the firmware update request
    //
    if( (updates & UM_UPDATE_FIRMWARE) ) {
        if( !(m_LastUpdates & UM_UPDATE_FIRMWARE) ) {
            DEBUG( UM, DBGLEV_ERROR, "Firmware update requested but not available\n");
        }
        else {
            // Find the firmware image and copy it
            SimpleList<FileInfo> appfile;
            if( parser.FindVariable(FIRMWARE_GROUP_NAME, APP_VERSION) &&
                parser.GetFileList(FIRMWARE_GROUP_NAME, appfile)      &&
                appfile.Size() == 1 )
            {
                // there should only be one image file
                if( CopyFiles( szSourceDir, szDestDir, appfile ) >= 0 ) {
                    ret |= UM_UPDATE_FIRMWARE;
                }
            } else {
                DEBUG( UM, DBGLEV_ERROR, "Firmware update config file corrupt\n");
                diag_printf("%s\n", m_pLastConfigFile );
            }
        }
    }

    delete [] szSourceDir; delete [] szDestDir;

    DEBUGP( UM, DBGLEV_TRACE, "CopyUpdatesFromSource returning 0x%x\n", ret );
    return ret;
}

// Perform the requested updates from the given source (a config file url)
// This does not handle CDDB update requests
int
CUpdateManager::UpdateFromSource( const char* szSourceURL, update_t updates ) 
{
    DEBUGP( UM, DBGLEV_TRACE, "UpdateFromSource [%s] 0x%x\n", szSourceURL, updates );

    // If someone just called CheckForUpdates on this file, use the cached copy; otherwise
    // call CheckForUpdates() to validate the file, and use the cached buffer
    if( !m_pLastConfigFileSource || strcmp( szSourceURL, m_pLastConfigFileSource ) != 0 ) {
        update_t avail = CheckForUpdates( szSourceURL );
        if( avail == UM_UPDATE_UNAVAIL )
            return -1;
    }

    // the following will never fail
    CParseConfig parser;
    parser.ParseBuffer( m_pLastConfigFile, m_iLastConfigFileLen );

    // determine the base directory for the source items
    char* szSourceDir = new char[ 512 ];     // eek
    int iEndSourceDir;
    strcpy( szSourceDir, m_pLastConfigFileSource );
    for( iEndSourceDir = strlen(m_pLastConfigFileSource); iEndSourceDir > 0 && szSourceDir[iEndSourceDir] != '/'; iEndSourceDir-- );
    szSourceDir[iEndSourceDir] = '\0';
    
    // determine the base directory for the destination items
    char* szDestDir = new char[ 512 ];       // eek
    int iEndDestDir;
    strcpy( szDestDir, ROOT_URL_PATH );
    iEndDestDir = strlen(ROOT_URL_PATH);

    //
    // Check for a full CDDB copyrequest
    //
    if( (updates & UM_UPDATE_CDDBFULL) ) {
        if( !(m_LastUpdates & UM_UPDATE_CDDBFULL) ) {
            DEBUG( UM, DBGLEV_ERROR, "Full CDDB copy requested but not available\n");
            delete [] szSourceDir; delete [] szDestDir;
            return -1;
        }

        const char* tmp = parser.FindVariable(CDDB_GROUP_NAME, CDDB_VERSION);
        if( atoi(tmp) == 0 ) {
            DEBUG( UM, DBGLEV_ERROR, "Full CDDB copy requested but version is 0\n");
            delete [] szSourceDir; delete [] szDestDir;
            return -1;
        }

        SimpleList<FileInfo> cddbfiles;
        if( parser.GetFileList(CDDB_GROUP_NAME, cddbfiles) )
        {
            if( CopyFiles( szSourceDir, szDestDir, cddbfiles ) < 0 ) {
                delete [] szSourceDir; delete [] szDestDir;
                return -1;
            }
        }
    }

    //
    // Check for the update system files request
    //
    if( (updates & UM_UPDATE_SYSTEM) ) {
        if( !(m_LastUpdates & UM_UPDATE_SYSTEM) ) {
            DEBUG( UM, DBGLEV_ERROR, "System update requested but not available\n");
            delete [] szSourceDir; delete [] szDestDir;
            return -1;
        }

        // Get a list of files to copy over
        SimpleList<FileInfo> files;
        if( parser.GetFileList(SYSTEM_FILES_GROUP_NAME, files) )
        {
            if( CopyFiles( szSourceDir, szDestDir, files ) < 0 ) {
                delete [] szSourceDir; delete [] szDestDir;
                return -1;
            }
        }
    }

    //
    // Check for the firmware update request
    //
    if( (updates & UM_UPDATE_FIRMWARE) ) {
        if( !(m_LastUpdates & UM_UPDATE_FIRMWARE) ) {
            DEBUG( UM, DBGLEV_ERROR, "Firmware update requested but not available\n");
            delete [] szSourceDir; delete [] szDestDir;
            return -1;
        }

        // Find the firmware image and load it into ram
        SimpleList<FileInfo> appfile;
        if( parser.FindVariable(FIRMWARE_GROUP_NAME, APP_VERSION) &&
            parser.GetFileList(FIRMWARE_GROUP_NAME, appfile)      &&
            appfile.Size() == 1 )
        {
            strcat( &(szSourceDir[iEndSourceDir]), appfile.PopFront().path );

            if( UpdateFirmware( szSourceDir ) < 0 ) {
                delete [] szSourceDir; delete [] szDestDir;
                return -1;
            }
            
            szSourceDir[iEndSourceDir] = '\0';
        } else {
            DEBUG( UM, DBGLEV_ERROR, "Firmware update config file corrupt\n");
            diag_printf("%s\n", m_pLastConfigFile );
            delete [] szSourceDir; delete [] szDestDir;
            return -1;
        }
    }

    delete [] szSourceDir; delete [] szDestDir;
    return 0;
}

int
CUpdateManager::GetCDDBUpdateCount( CParseConfig& parser )
{
    SimpleList<cddb_update_info_t> cddbfiles;
    return GetCDDBUpdateListHelper( parser, cddbfiles, false );
}

int
CUpdateManager::GetCDDBUpdateListHelper( CParseConfig& parser, SimpleList<cddb_update_info_t>& cddbfiles, bool bGetList )
{
    // First, open a cddb session and get a list of the current CDDB update levels
    gn_upd_error_t           error   = UPDERR_NoError;
    gn_upd_install_profile_t profile = {0};
    
    error = gn_upd_init();
    if (error != UPDERR_NoError)
        DEBUG( UM, DBGLEV_ERROR, "Error intializing update: 0x%x: %s\n", error, gnerr_get_error_message(error) );
    else
    {
        error = gn_upd_load_install_profile(&profile);
        if (error != UPDERR_NoError)
            DEBUG( UM, DBGLEV_ERROR, "Error getting update profile: 0x%x: %s\n", error, gnerr_get_error_message(error) );
        gn_upd_shutdown();
    }

    int i = 0;
    int count = 0;
    const char* tmp = NULL;
    do
    {
        char szCDDBUpdateGroupName[32];
        sprintf(szCDDBUpdateGroupName, "cddb_update_%d", i);
        if((tmp = parser.FindVariable(szCDDBUpdateGroupName, "version")) != NULL)
        {
            int ver = atoi(tmp);
            
            // make sure we dont already have this
            int j = 0;
            for( ; j < profile.update_level_element_count; j++ ) {
                if( profile.update_level_list[j] == ver ) {
                    DEBUG( UM, DBGLEV_INFO, "Found CDDB update %d but already added\n", ver );
                    break;
                }
            }
            if (j != profile.update_level_element_count)
            {
                // important: since we do a continue, this increment is getting skipped
                i++;
                continue;
            }
            
            // new update, so let's process it
            DEBUG( UM, DBGLEV_INFO, "Found CDDB update %d\n", ver );
            SimpleList<FileInfo> appfiles;
            
            if (parser.GetFileList(szCDDBUpdateGroupName, appfiles))
            {
                while (!appfiles.IsEmpty())
                {
                    cddb_update_info_t update_info;
                    
                    // for now, pass off the raw path to the caller
                    memcpy((void*)update_info.path, (void*)appfiles.PopFront().path, EMAXPATH);
                    update_info.update_level = ver;
                    if (bGetList)
                        cddbfiles.PushBack(update_info);
                    count++;
                }
            }
        }
        ++i;
    } while (tmp != NULL);
    
    gn_upd_cleanup_profile(&profile);

    return count;
}


int
CUpdateManager::GetCDDBUpdateList( const char* szSourceURL, SimpleList<cddb_update_info_t>& cddbfiles )
{
    // If someone just called CheckForUpdates on this file, use the cached copy; otherwise
    // call CheckForUpdates() to validate the file, and use the cached buffer
    if( !m_pLastConfigFileSource || strcmp( szSourceURL, m_pLastConfigFileSource ) != 0 ) {
        update_t avail = CheckForUpdates( szSourceURL );
        if( avail == UM_UPDATE_UNAVAIL )
            return -1;
    }
    
    if( !(m_LastUpdates & UM_UPDATE_CDDBUPDA) ) {
        DEBUG( UM, DBGLEV_WARNING, "CDDB Update requested but not available\n");
        return 0;
    }

    // the following will never fail
    CParseConfig parser;
    parser.ParseBuffer( m_pLastConfigFile, m_iLastConfigFileLen );

    return GetCDDBUpdateListHelper( parser, cddbfiles, true );
}

void
CUpdateManager::FlashCallbackProxy()
{
    s_pInstance->FlashCBProxy();
}
void
CUpdateManager::FlashCBProxy()
{
    m_FlashCB( m_iCurrentFlash++, m_iTotalFlash );
}

void
CUpdateManager::DefaultFileStatusCB( const char* szName, int iCurrent, int iTotal )
{
    if( iCurrent == 0 ) {
        DEBUG( UM, DBGLEV_INFO, "Now copying file %s\n" );
    }
    else {
        DEBUG( UM, DBGLEV_INFO, "." );
    }
}

void
CUpdateManager::DefaultFlashStatusCB( int iCurrent, int iTotal )
{
    DEBUG( UM, DBGLEV_INFO, "." );
}


// Update the device firmware
int
CUpdateManager::UpdateFirmware( const char* szSourceURL )
{
    IInputStream* pFirmware;
    
    if( !( pFirmware = CDataSourceManager::GetInstance()->OpenInputStream( szSourceURL ) ) ) {
        DEBUG( UM, DBGLEV_ERROR, "Can't open %s\n", szSourceURL );
        return -1;
    }

    int len = pFirmware->Length();
    char* buff = new char[len];
    if( !buff || (pFirmware->Read( buff, len ) < len) ) {
        DEBUG( UM, DBGLEV_ERROR, "Can't read %s\n", szSourceURL );
        delete pFirmware;
        if( buff ) delete [] buff;
        return -1;
    }

    delete pFirmware;

    // Now program the image
    CUpdateApp* pUpdate = CUpdateApp::GetInstance();
    if( !pUpdate->VerifyImage( buff, len ) ) {
        DEBUG( UM, DBGLEV_ERROR, "Failed to verify %s\n", szSourceURL );
        delete [] buff;
        return -1;
    }
    
    // Progress fun
    m_iCurrentFlash = 0;
    m_iTotalFlash = (2*len) / CFlashManager::GetInstance()->GetBlockSize();
    
    if( !pUpdate->UpdateImage( buff, len, FlashCallbackProxy ) ) {
        DEBUG( UM, DBGLEV_ERROR, "Failed to update firmware (!!!!!!)\n");
        delete [] buff;
        return -1;
    }
    
    delete [] buff;
    return 0;
}

// Copy a file from one location to another
int
CUpdateManager::CopyFile( const char* szSourceURL, const char* szDestinationURL ) 
{
    DEBUGP( UM, DBGLEV_TRACE, "CopyFile [%s]\n      to [%s]\n", szSourceURL, szDestinationURL );

    // give a quick update so we know asap what we're trying to copy
    m_FileCB( szSourceURL, 0, 0 );

    IInputStream* pInputStream = CDataSourceManager::GetInstance()->OpenInputStream( szSourceURL );
    if( !pInputStream ) {
        DEBUG( UM, DBGLEV_ERROR, "Cannot open input file %s for copy\n", szSourceURL );
        return -1;
    }

    // from fathelper, get the path from the destination URL and make sure the output directory is available
    char* szDestPath = new char[strlen(szDestinationURL)];
    strcpy( szDestPath, FullFilenameFromURLInPlace(szDestinationURL) );

    // trim off the filename
    for( int i = strlen(szDestPath); i; i-- ) {
        if( szDestPath[i] == '/' ) {
            szDestPath[i] = '\0';
            break;
        }
    }
    
    if( !VerifyOrCreateDirectory( szDestPath ) ) {
        DEBUG( UM, DBGLEV_ERROR, "Cannot handle output URL %s (path creation)\n", szDestinationURL );

        delete [] szDestPath;
        delete pInputStream;
        return -1;
    }
    
    delete [] szDestPath;
    
    IOutputStream* pOutputStream = CDataSourceManager::GetInstance()->OpenOutputStream( szDestinationURL );
    if( !pOutputStream ) {
        DEBUG( UM, DBGLEV_ERROR, "Cannot open output file %s for copy\n", szDestinationURL );
        delete pInputStream;
        return -1;
    }

    char* buff = new char[ UPDATE_COPY_SIZE ];
    if( !buff ) {
        DEBUG( UM, DBGLEV_ERROR, "Cannot allocate copy buffer\n");
        delete pInputStream;
        delete pOutputStream;
        return -1;
    }

    int res = 0;
    
    int copied = 0, len = pInputStream->Length();
    while( copied < len ) {
        // Issue this at the top so the callback routine can detect a file change by looking at 0 copied
        m_FileCB( szSourceURL, copied, len );

        int count;
        if( (len - copied) < UPDATE_COPY_SIZE ) {
            count = len-copied;
        }
        else {
            count = UPDATE_COPY_SIZE;
        }

        if( pInputStream->Read( buff, count ) < count ) {
            DEBUG( UM, DBGLEV_ERROR, "Error copying %s to %s, failed to read\n", szSourceURL, szDestinationURL );
            res = -1;
            break;
        }
        if( pOutputStream->Write( buff, count ) < count ) {
            DEBUG( UM, DBGLEV_ERROR, "Error copying %s to %s, failed to write\n", szSourceURL, szDestinationURL );
            res = -1;
            break;
        }

        copied += count;
    }

    // Issue this here so a 'full' bar can be displayed
    m_FileCB( szSourceURL, copied, len );

    delete pInputStream;
    delete pOutputStream;
    delete [] buff;

    if( res < 0 ) {
        // delete target file
        if( !pc_unlink((TEXT*) FilenameFromURLInPlace(szDestinationURL)) )
        {
            DEBUG( UM, DBGLEV_WARNING, "Unable to Delete Partial File %s\n", FilenameFromURLInPlace(szDestinationURL) );
        }
    }
    
    DEBUGP( UM, DBGLEV_TRACE, "CopyFile - %d\n", res );
    return res;
}

// Delete all update files at a given source URL (currently doesn't delete directories)
// This is intended to be used after copying update files from a source to the local HD
// and is intended to only work for URLs pointing to the local HD
// Returns: -1 on error, otherwise number of files deleted
int
CUpdateManager::DeleteUpdatesAtSource( const char* szSourceURL )
{
    DEBUGP( UM, DBGLEV_TRACE, "DeleteUpdatesAtSource [%s]\n", szSourceURL );

    int iFilesDeleted = 0;

    // the following will never fail
    CParseConfig parser;
    parser.ParseBuffer( m_pLastConfigFile, m_iLastConfigFileLen );

    // determine the base directory for the source items
    char* szSourceDir = new char[ 512 ];     // eek
    int iEndSourceDir;
    strcpy( szSourceDir, m_pLastConfigFileSource );
    for( iEndSourceDir = strlen(m_pLastConfigFileSource); iEndSourceDir > 0 && szSourceDir[iEndSourceDir] != '/'; iEndSourceDir-- );
    szSourceDir[iEndSourceDir] = '\0';

    SimpleListIterator<FileInfo> itFiles;

    // cddb files
    SimpleList<FileInfo> cddbfiles;
    if( parser.GetFileList(CDDB_GROUP_NAME, cddbfiles) ) {
        for( itFiles = cddbfiles.GetHead(); itFiles != cddbfiles.GetEnd(); ++itFiles ) {
            strcat( &(szSourceDir[iEndSourceDir]), (*itFiles).path );
            if( !pc_unlink( (TEXT*)FullFilenameFromURLInPlace(szSourceDir) ) ) {
                DEBUG( UM, DBGLEV_WARNING, "Unable to Delete File [%s]\n", FullFilenameFromURLInPlace(szSourceDir) );
                delete [] szSourceDir;
                return -1;
            }
            DEBUGP( UM, DBGLEV_TRACE, "    Deleted File [%s]\n", FullFilenameFromURLInPlace(szSourceDir) );
            iFilesDeleted++;
            szSourceDir[iEndSourceDir] = '\0';
        }
    }

    // system files
    SimpleList<FileInfo> sysfiles;
    if( parser.GetFileList(SYSTEM_FILES_GROUP_NAME, sysfiles) ) {
        for( itFiles = sysfiles.GetHead(); itFiles != sysfiles.GetEnd(); ++itFiles ) {
            strcat( &(szSourceDir[iEndSourceDir]), (*itFiles).path );
            if( !pc_unlink((TEXT*) FullFilenameFromURLInPlace(szSourceDir) ) ) {
                DEBUG( UM, DBGLEV_WARNING, "Unable to Delete File [%s]\n", FullFilenameFromURLInPlace(szSourceDir) );
                delete [] szSourceDir;
                return -1;
            }
            DEBUGP( UM, DBGLEV_TRACE, "    Deleted File [%s]\n", FullFilenameFromURLInPlace(szSourceDir) );
            iFilesDeleted++;
            szSourceDir[iEndSourceDir] = '\0';
        }
    }

    // firmware file(s)
    SimpleList<FileInfo> firmwarefile;
    if( parser.GetFileList(FIRMWARE_GROUP_NAME, firmwarefile) ) {
        for( itFiles = firmwarefile.GetHead(); itFiles != firmwarefile.GetEnd(); ++itFiles ) {
            if( !pc_unlink((TEXT*) strcat( &(szSourceDir[iEndSourceDir]), (*itFiles).path )) ) {
                DEBUG( UM, DBGLEV_WARNING, "Unable to Delete File [%s]\n", szSourceDir );
                delete [] szSourceDir;
                return -1;
            }
            DEBUGP( UM, DBGLEV_TRACE, "    Deleted File [%s]\n", szSourceDir );
            iFilesDeleted++;
            szSourceDir[iEndSourceDir] = '\0';
        }
    }

    // delete the config file
    if( !pc_unlink((TEXT*) FilenameFromURLInPlace(szSourceURL)) ) {
        DEBUGP( UM, DBGLEV_TRACE, "Failed to delete the cfg file\n" );
        delete [] szSourceDir;
        return -1;
    }
    else {
        DEBUGP( UM, DBGLEV_TRACE, "    Deleted File [%s]\n", FilenameFromURLInPlace(szSourceURL) );
        iFilesDeleted++;
    }

    DEBUGP( UM, DBGLEV_TRACE, "DeleteUpdatesAtSource -\n" );
    delete [] szSourceDir;
    return iFilesDeleted;
}
    
void
CUpdateManager::DeleteUpdates()
{
    DEBUGP( UM, DBGLEV_TRACE, "DeleteUpdates\n" );
    CUpdateApp::GetInstance()->DeleteUpdates();
}

// Note: this routine modifies the passed in directory paths...
int
CUpdateManager::CopyFiles( char* szSourceDir, char* szDestDir, SimpleList<FileInfo>& files ) 
{
    DEBUGP( UM, DBGLEV_TRACE, "CopyFiles\n" );

    int iEndSourceDir = strlen( szSourceDir );
    int iEndDestDir   = strlen( szDestDir );
    
    for( SimpleListIterator<FileInfo> itFiles = files.GetHead();
         itFiles != files.GetEnd(); ++itFiles )
    {
        // So ghetto.
        strcat( &(szSourceDir[iEndSourceDir]), (*itFiles).path );
        strcat( &(szDestDir[iEndDestDir]), (*itFiles).path );
        if( CopyFile( szSourceDir, szDestDir ) < 0 ) {
            DEBUG( UM, DBGLEV_ERROR, "Failed to copy\n" );
            return -1;
        }
        szSourceDir[iEndSourceDir] = '\0';
        szDestDir[iEndDestDir] = '\0';
    }
    return 0;
}

// Open a config file and read it into a buffer
char*
CUpdateManager::GetConfig( const char* szSourceURL )
{
    IInputStream* pInputStream = CDataSourceManager::GetInstance()->OpenInputStream( szSourceURL );

    if( !pInputStream ) {
        DEBUG( UM, DBGLEV_ERROR, "Can't open source %s\n", szSourceURL );
        return NULL;
    }

    if( !pInputStream->Length() ) {
        // TODO handle this gracefully. it comes up with some http input streams off some servers
        // that dont provide the Content-Length field
        DEBUG( UM, DBGLEV_ERROR, "Input stream with no length\n");
        
        delete pInputStream;
        return NULL;
    }

    // read up the whole config file
    int len = pInputStream->Length();
    char* pConfigFile = new char[ len + 1 ];

    int r = pInputStream->Read( pConfigFile, len );
    if( r < len ) {
        DEBUG( UM, DBGLEV_ERROR, "Failed to read in config file (%d of %d read)\n", r, len);

        delete pInputStream;
        delete pConfigFile;
        return NULL;
    }
    // Some streams present us with non-null terminated text files, which makes the parser unhappy
    pConfigFile[len] = 0;

    delete pInputStream;
    return pConfigFile;
}
