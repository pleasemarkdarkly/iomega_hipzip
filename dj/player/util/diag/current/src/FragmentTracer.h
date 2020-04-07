// FragmentTracer.h: tool to track memory fragmentation
// danc@fullplaymedia.com
// 10/31/02

#ifndef __FRAGMENTTRACKER_H__
#define __FRAGMENTTRACKER_H__

#ifdef __cplusplus
extern "C" {
#endif

void frag_dump_current(void);
void frag_reset(void);

#ifdef __cplusplus
};
#endif
    
#endif // __FRAGMENTTRACKER_H__
