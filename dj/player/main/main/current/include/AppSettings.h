#ifndef _APP_SETTINGS_INCLUDE_
#define _APP_SETTINGS_INCLUDE_

#define PLAYLIST_STRING_SIZE 128

// folders
#define ROOT_FILE_PATH          "A:"
#define ROOT_URL_PATH           "file://A:"

#define SYSTEM_FOLDER               ROOT_FILE_PATH "/system"
#define SYSTEM_URL                  ROOT_URL_PATH "/system"
#define SAVE_SETTINGS_PATH          SYSTEM_FOLDER "/settings.ddo"
#define METAKIT_HD_PERSIST_FILE     SYSTEM_FOLDER "/mkpersist.dat"
#define METAKIT_CD_PERSIST_FILE     SYSTEM_FOLDER "/mkcdpersist.dat"
#define DISK_METADATA_CACHE_PATH    SYSTEM_FOLDER "/mdcache.db"
#define CURRENT_PLAYLIST_URL        SYSTEM_URL "/current.djp"
#define CURRENT_PLAYLIST_TEMP_FILE  SYSTEM_FOLDER "/current.tmp"
#define DEBUG_LOG_FOLDER            SYSTEM_FOLDER "/debuglogs"
#define TEST_RERUN_FILE             SYSTEM_FOLDER "/TEST.CFG"
#define EVENT_LOG_FILE              SYSTEM_FOLDER "/EVENTLOG.TXT"
#define TEST_RETRY_FILE             SYSTEM_FOLDER "/TESTRTRY.CFG"
#define PRINT_SCREEN_FILE           SYSTEM_FOLDER "/prntscrn.bmp"
#define SYSTEM_PROGRESS_PATH        SYSTEM_FOLDER "/pgrs.dat"

#define CONTENT_FOLDER              ROOT_FILE_PATH "/c"

#define CD_DIR_NAME_DB_FILE         SYSTEM_FOLDER   "/cddir.db"
#define CD_DIR_NAME_BASE            CONTENT_FOLDER  "/cd"

#define CDDB_FOLDER                 ROOT_FILE_PATH "/cddb"
#define CDDB_CACHE_FILE             CDDB_FOLDER "/ecddb.cch"
#define CDDB_CACHE_BACKUP_FILE      CDDB_FOLDER "/ecddb_bk.cch"

#define CD_DEVICE_NAME              "/dev/cda/"
#define CD_MOUNT_DIR                "/"

// software update settings/defines
#define COMPANY_ID "fullplay"
#define PRODUCT_ID "dj"
#define IMAGE_NAME "app"

#define UPDATE_EXTENSION "IMG"
#define UPDATE_DIR_STRIPPED "A:\\UPDATES"
#define UPDATE_DIR "A:\\UPDATES\\"
#define UPDATE_PATH "A:\\UPDATES\\*.IMG"
#define FACTORY_PATH "A:\\UPDATES\\FACTORY.IMG"
#define CD_UPDATE_PATH "/"

// buffer size for file copies - controls UI progress intervals
#define UPDATE_COPY_SIZE  (64*1024)

// used in config file parsing
#define CDDB_GROUP_NAME "cddb"
#define CDDB_VERSION "version"
#define SYSTEM_FILES_GROUP_NAME "system"
#define FIRMWARE_GROUP_NAME "app"
#define APP_VERSION "version"
#define RESTORE_CD_CONFIG_PATH "file://restore.cfg"
#if defined(DJ_RELEASE)
#define ONLINE_CONFIG_PATH     "http://dj.fullplaymedia.com/updates/djupdates.cfg"
#else
#define ONLINE_CONFIG_PATH     "http://builder.iobjects.com/dj/current/djupdates.cfg"
#endif
#define LOCAL_CONFIG_PATH      "file://A:/UPDATES/djupdates.cfg"
// Downloaded updates get put in a seperate folder
#define DOWNLOAD_CONFIG_PATH   "file://A:/UPDATES/ONLINE/djupdates.cfg"
#define DOWNLOAD_URL_PATH      "file://A:/UPDATES/ONLINE"

// thread priorities
#define UI_THREAD_NORMAL_PRIORITY   8
#define UI_THREAD_BUSY_PRIORITY     10

// registry entry IDs
#define PLAYER_SCREEN_REGISTRY_KEY_TYPE 0x47
#define PLAYER_SCREEN_REGISTRY_KEY_NAME 0x01

#define PLAYLIST_CONSTRAINT_REGISTRY_KEY_TYPE 0x48
#define PLAYLIST_CONSTRAINT_REGISTRY_KEY_NAME 0x01

#define DJ_PLAYER_STATE_REGISTRY_KEY_TYPE               0x49
#define DJ_PLAYER_STATE_CD_SESSION_INDEX_KEY_NAME       0x01
#define DJ_PLAYER_STATE_PLAYLIST_COUNTER_KEY_NAME       0x02
#define DJ_PLAYER_STATE_ENCODE_BITRATE_KEY_NAME         0x03
#define DJ_PLAYER_STATE_UI_VIEW_MODE_KEY_NAME           0x04
#define DJ_PLAYER_STATE_RANDOM_NUMBER_SEED_KEY_NAME     0x05
#define DJ_PLAYER_STATE_CURRENT_DATA_SOURCE_KEY_NAME    0x06
#define DJ_PLAYER_STATE_PLAYLIST_INDEX_KEY_NAME         0x07
#define DJ_PLAYER_STATE_PLAYLIST_TITLE_KEY_NAME         0x08
#define DJ_PLAYER_STATE_EJECT_CD_AFTER_RIP_NAME         0x09
#define DJ_PLAYER_STATE_ENABLE_EXT_CHARS_NAME           0x10
#define DJ_PLAYER_STATE_ENABLE_WEB_CONTROL_NAME         0x11
#define DJ_PLAYER_STATE_SHOW_TRACK_NUM_IN_TITLE_NAME    0x12
#define DJ_PLAYER_STATE_SHOW_ALBUM_WITH_ARTIST_NAME     0x13
#define DJ_PLAYER_STATE_TEXT_SCROLL_SPEED_NAME          0x14
#define DJ_PLAYER_STATE_LED_BRIGHTNESS_NAME             0x15
#define DJ_PLAYER_STATE_LCD_BRIGHTNESS_NAME             0x16
#define DJ_PLAYER_STATE_PLAY_CD_WHEN_INSERTED           0x17

#define LINE_RECORDER_REGISTRY_KEY_TYPE 0x4C
#define LINE_RECORDER_REGISTRY_KEY_NAME 0x01

#define DJ_UI_VIEW_MODE_REGISTRY_KEY_TYPE 0x4D
#define DJ_UI_VIEW_MODE_REGISTRY_KEY_NAME 0x01

#define DJ_ENCODING_UPDATE_LIST_KEY_TYPE        0x4E
#define DJ_ENCODING_UPDATE_LIST_KEY_NAME        0x01

// sram / sram codec settings
#define SRAM_START (0x60000000)
#define SRAM_SIZE (47*1024+512)
#define CODEC_SRAM_POOL_ADDRESS (SRAM_START + 1024)
#define CODEC_SRAM_POOL_SIZE (27136) // (leaving 10k for pem)

// src blending threshold
#define SRC_BLENDING_THRESHOLD 22050

// idle state - how many ms before the device is considered idle, currently 60sec
#define IDLE_TIME_LIMIT 60000

// how many ms do we wait to post a system shutdown message after a HARD_POWER_OFF, currently 2sec
#define SHUTDOWN_WAIT_TIME 2000

// recording settings
#define RECORDINGS_PATH CONTENT_FOLDER "/r"
#define RECORDING_SESSION_DIR_NAME_STEM ("s")
#define RECORDING_FILE_NAME_STEM ("r")
#define RECORDING_SESSION_NAME_STEM ("Session ")
#define RECORDING_TRACK_NAME_STEM ("Recording ")
#define RECORDING_SESSION_ID_DIGITS 3
#define RECORDING_FILE_ID_DIGITS 2
#define RECORDING_GENRE_V1 (37)    // the character value for 'Sound Clip'
#define RECORDING_GENRE_TEXT ("Recordings")    // the character value for 'Sound Clip'
#define RECORDING_YEAR ("")
#define RECORDING_COMMENT ("")
#define NO_SESSIONS_MESSAGE ("Session Dirs Full!")

// drive space threshholds (spacemgr.h and spacemgr.cpp)
// warn the user at this many mbytes (or less) of available space; should be greater than a single CD
//#define SPACEMGR_WARN_MBYTES 1024
#define SPACEMGR_WARN_MBYTES 64
// stop recording/ripping at this many mbytes (or less) of available space
#define SPACEMGR_LOW_MBYTES  64

// Make sure recording and HD info are on the same page when reporting space/time limits.
#define DJ_BYTES_PER_KILOBYTE       1000
#define DJ_KILOBYTES_PER_MEGABYTE   1000
#define DJ_MEGABYTES_PER_GIGABYTE   1000

// MAX_HD_TRACKS is the maximum number of HD tracks that the system will see.
// Once MAX_HD_TRACKS is reached then no more tracks will be recorded to HD.
// If MAX_HD_TRACKS is undefined then there's no limit to the number of tracks that can be on the HD.
#define MAX_HD_TRACKS       10000

// MAX_CD_TRACKS is the maximum number of CD tracks that the system will see on a CD.
// Once MAX_CD_TRACKS is reached then no more tracks will be playable from a CD.
// If MAX_CD_TRACKS is undefined then there's no limit to the number of tracks that can be on a CD.
#define MAX_CD_TRACKS        1000

// MAX_CD_DIRECTORIES is the maximum number of directories on a CD that the system will scan on a CD.
// Once MAX_CD_DIRECTORIES is reached then no more tracks will be playable from a CD.
// If MAX_CD_DIRECTORIES is undefined then there's no limit to the number of directories that can be on a CD.
#define MAX_CD_DIRECTORIES   1000

// MAX_PLAYLIST_TRACKS is the maximum number of tracks that can be added to a playlist.
// If MAX_PLAYLIST_TRACKS is undefined then there's no limit to the number of tracks that can be in a playlist.
#define MAX_PLAYLIST_TRACKS 10000

// what string language is in use, 0=english
#define CURRENT_LANGUAGE (0)

#endif // _APP_SETTINGS_INCLUDE_
