#ifndef __PSY_H__
#define __PSY_H__

void start_scalefacs(struct granule_t *gi);
void calc_scalefacs(int ch, struct granule_t *gi);
void compute_scalefacs(int ch, struct granule_t *gi);
void apply_scalefacs(int ch, struct granule_t *gi);

extern fixed64_32 eeb[2];
extern fixed64_32 eb[21];
extern fixed64_32 heb[2][21];

extern fixed16 sf[21];
extern fixed16 old_sf[2][21];

extern fixed16 leb[21];
extern fixed16 lebs[21];
extern fixed16 lthr[21];

extern fixed16 enm[144];

#endif// __PSY_H__
