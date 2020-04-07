/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * branding.h - Example fulfillment of Gracenote Branding Requirements
 */

#ifndef	_BRANDING_H_
#define _BRANDING_H_


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Prototypes
 */

void
brand_display_on_powerup(void);

void
brand_display_on_lookup(void);

void
brand_display_on_update(void);

void
brand_set_duration_on_powerup(int delay);

void
brand_set_duration_on_lookup(int delay);

void
brand_set_duration_on_update(int delay);


#ifdef __cplusplus
}
#endif 


#endif /* _BRANDING_H_ */
