#ifndef __RESERVOIR_H__
#define __RESERVOIR_H__

void reservoir_start(int frame_bits);
int reservoir_get(int mean_bits, int wanted_bits);
void reservoir_add(int bits);
void reservoir_put2(struct granule_t *gi);
void reservoir_put3(struct granule_t *gi);
int reservoir_end(void);
void reservoir_init(void);

#endif
