#ifndef __FPMP3_H__
#define __FPMP3_H__

#ifdef __cplusplus
extern "C" {
#endif

long fpmp3_start(int bitrate);

long fpmp3_encode(short *pcm, unsigned char *mp3out, unsigned int *bytesout);

long fpmp3_finish(unsigned char *mp3out, unsigned int *bytesout);

#ifdef __cplusplus
}
#endif

#endif//__FPMP3_H__
