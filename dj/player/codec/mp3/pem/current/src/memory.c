#include "memory.h"

#ifndef PEM_USE_SRAM
struct sram the_fake_sram;
#endif
struct dram dram;

struct sram *sram_p;