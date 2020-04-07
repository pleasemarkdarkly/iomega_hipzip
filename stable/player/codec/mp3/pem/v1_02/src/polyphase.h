#ifndef __POLYPHASE_H__
#define __POLYPHASE_H__

#include "types.h"

void init_polyphase(void);
void polyphase(const short *x, fixed16 *sa, fixed16 *sb);

#endif//__POLYPHASE_H__
