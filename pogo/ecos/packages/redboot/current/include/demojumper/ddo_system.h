#ifndef DDO_SYSTEM_H
#define DDO_SYSTEM_H

/* Thread priorites */

// pthread.00000800                  1 (exits)
#define PLAYER_ENTRY_THREAD_PRIORITY 2 // (exits)
// Network Alarm Support             6
// Network Support                   7
// DHCP Lease Management             8
#define KEYBOARD_THREAD_PRIORITY     9
#define DECODER_THREAD_PRIORITY      10
#define BUFFER_THREAD_PRIORITY       11
#define TITLE_STREAM_THREAD_PRIORITY 12
#define UI_THREAD_PRIORITY           13
#define LCD_THREAD_PRIORITY          14

/* Stack sizes */

//#define KEYBOARD_THREAD_STACK_SIZE ((1 * 1024) < CYGNUM_HAL_STACK_SIZE_MINIMUM ? CYGNUM_HAL_STACK_SIZE_MINIMUM : (1 * 1024))
#define KEYBOARD_THREAD_STACK_SIZE (128 * 1024)
//#define DECODER_THREAD_STACK_SIZE ((1 * 1024) < CYGNUM_HAL_STACK_SIZE_MINIMUM ? CYGNUM_HAL_STACK_SIZE_MINIMUM : (1 * 1024))
#define DECODER_THREAD_STACK_SIZE (128 * 1024)
//#define BUFFER_THREAD_STACK_SIZE (4 * 1024)
#define BUFFER_THREAD_STACK_SIZE (128 * 1024)
//#define TITLE_STREAM_THREAD_STACK_SIZE (4 * 1024)
#define TITLE_STREAM_THREAD_STACK_SIZE (128 * 1024)
//#define UI_THREAD_STACK_SIZE (3 * 1024)
#define UI_THREAD_STACK_SIZE (128 * 1024)

/* Supported codecs */

#define SUPPORT_MP3				// Support the MP3 Codec (requires mp3_codec.*)
//#define SUPPORT_HIPP_MP3			// Support the Hipp MP3 Codec (requires mp3_codec_hipp.*)
//#define SUPPORT_WMA				// Support the WMA Codec (requires wma_codec.*)
//#define SUPPORT_AAC				// Support the AAC Codec (requires aac_codec.*)
//#define SUPPORT_ACELP				// Support the ACELP Codec (requires acelp_codec.*)
//#define SUPPORT_VORBIS			// Support the OggVorbis codec (requires vorbis_codec.*)

#endif /* DDO_SYSTEM_H */
