#ifndef DEMOS_H_DEFINED
#define DEMOS_H_DEFINED

#define DEMO_COUNT 3
#define DEMO_DESC_LEN 64
#define DEMO_ALIAS_LEN 32

#define CODEC_ROM_START 0xe0100000
#define CODEC_ROM_LEN 0x100000	// warning, this value experimental!!

#define DEMO0_TITLE "Hard Drive Player"
#define DEMO0_ALIAS "hddplayer"
#define DEMO0_FLASH 0xe0020000	// starting at 128k
#define DEMO0_LEN	0xA0000		// extending over 640k

#define DEMO1_TITLE "Streaming Player"
#define DEMO1_ALIAS "streamer"
#define DEMO1_FLASH 0xe0200000
#define DEMO1_LEN	0x100000

#define DEMO2_TITLE "MP3 Encoder Demo"
#define DEMO2_ALIAS "mp3enc"
#define DEMO2_FLASH 0xe0300000
#define DEMO2_LEN	0xC0000

#if DEMO_COUNT > 3

#define DEMO3_TITLE "Compact Flash Player"
#define DEMO3_ALIAS "cfplayer"
#define DEMO3_FLASH 0xe0500000
#define DEMO3_LEN	0x100000

#define DEMO4_TITLE "Flash ROM Player"
#define DEMO4_ALIAS "flashplayer"
#define DEMO4_FLASH 0xe0600000
#define DEMO4_LEN	0x100000

#define DEMO5_TITLE "Microdrive Player"
#define DEMO5_ALIAS "mdplayer"
#define DEMO5_FLASH 0xe0400000
#define DEMO5_LEN	0x100000

#define DEMO6_TITLE "DAR Player"
#define DEMO6_ALIAS "dar"
#define DEMO6_FLASH 0xe0100000
#define DEMO6_LEN	0x100000

#endif

extern char demo_names[DEMO_COUNT][DEMO_DESC_LEN];
extern char demo_aliases[DEMO_COUNT][DEMO_ALIAS_LEN];
extern unsigned long demo_lens[DEMO_COUNT];
extern unsigned long demo_flash_bases[DEMO_COUNT];

// this fn is a little hackish, since you can't just iterate through the #define names based on indexes.  it must be updated in demojumper.c to reflect
// the defines above.
void setup_demos(void);

#endif // DEMOS_H_DEFINED