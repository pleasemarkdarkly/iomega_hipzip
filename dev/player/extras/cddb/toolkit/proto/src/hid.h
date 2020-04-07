/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * hid.h - Simple hashing API for Embedded Cddb Prototype.
 */

#ifndef _HID_H_
#define _HID_H_

#define HIDSZ		4	/* Size of a hash ID. */
#define HIDASCSZ	8	/* Size of a hash ID in ascii hex form. */

#ifdef __cplusplus
extern "C"{
#endif 

typedef gn_uchar_t hid_t[HIDSZ];

void gen_hid(gn_uint32_t *, gn_uint32_t, hid_t *);
void hid2asc(hid_t *, gn_str_t);
void asc2hid(gn_str_t asc, hid_t *hid);

#ifdef __cplusplus
}
#endif 

#endif /* _HID_H_ */
