#ifndef __IMGUTIL_H__
#define __IMGUTIL_H__


#ifdef __cplusplus
extern "C" {
#endif

// routine to perform crc32 on a buffer, consistent between generator and processor (host and device)
unsigned int img_crc32( const unsigned char* buff, unsigned int len );

#ifdef __cplusplus
};
#endif

#endif // __IMGUTIL_H__
