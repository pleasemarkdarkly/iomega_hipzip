#ifndef _APP_SETTINGS_INCLUDE_
#define _APP_SETTINGS_INCLUDE_

#define PLAYLIST_STRING_SIZE 128
#define SYSTEM_DIRECTORY_PATH ("a:/system")
#define SAVE_SETTINGS_PATH ("a:/system/settings.ddo")
#define METAKIT_PERSIST_PATH ("a:/system/mkpersist.dat")
#define SYSTEM_PROGRESS_PATH ("a:/system/tasks.dat")

#define ROOT_FILE_PATH ("A:")
#define URL_STEM ("file://")
#define ROOT_URL_PATH ("file://A:")
#define CONTENT_UPDATE_INITIAL_SYSMSGSCRN_TEXT ("Restoring State")

// registry entry IDs
#define PLAYER_SCREEN_REGISTRY_KEY_TYPE 0x47
#define PLAYER_SCREEN_REGISTRY_KEY_NAME 0x01

#define PLAYLIST_CONSTRAINT_REGISTRY_KEY_TYPE 0x48
#define PLAYLIST_CONSTRAINT_REGISTRY_KEY_NAME 0x01

#define RECORDER_REGISTRY_KEY_TYPE 0x49
#define RECORDER_REGISTRY_KEY_NAME 0x01

// sram
#define SRAM_START (0x60000000)
#define SRAM_SIZE (47*1024+512)
// codec sram usage location + size
#define CODEC_SRAM_POOL_ADDRESS (SRAM_START + 1024*4)
#define CODEC_SRAM_POOL_SIZE (32*1024)
// src blending threshold
#define SRC_BLENDING_THRESHOLD 22050

// failure recourse settings
#define BOOT_FAILURE_SAFEMODE_THRESHOLD 4

// recording settings
#define RECORDINGS_PATH ("A:/Recordings")
#define RECORDING_SESSION_DIR_NAME_STEM ("Session")
#define RECORDING_FILE_NAME_STEM ("Recording")
#define RECORDING_SESSION_ID_DIGITS 3
#define RECORDING_FILE_ID_DIGITS 2
#define RECORDING_GENRE (37)    // the character value for 'Sound Clip'
#define RECORDING_ARTIST ("Recordings")
#define RECORDING_YEAR ("")
#define RECORDING_COMMENT ("")
#define NO_SESSIONS_MESSAGE ("Session Dirs Full!")

// content update chunk size, basically unused.
#define CONTENT_UPDATE_CHUNK_SIZE 200

// idle player shutdown timer settings
#define PLAYER_IDLE_SHUTDOWN_SECONDS 120
#define PLAYER_IDLE_SHUTDOWN_TICKS (PLAYER_IDLE_SHUTDOWN_SECONDS * 100)
// backlight timeout
#define BACKLIGHT_TIMEOUT_SECONDS 3
#define BACKLIGHT_TIMEOUT_TICKS (BACKLIGHT_TIMEOUT_SECONDS * 100)

#endif // _APP_SETTINGS_INCLUDE_