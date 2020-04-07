/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_timer.h - Abstraction header file containing the declaration 
 * of timer functions.
 */


#ifndef _GN_TIMER_H_
#define _GN_TIMER_H_

 /*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>


#ifdef __cplusplus
extern "C"{
#endif 

typedef void (*pTimerRoutine)(void *buffer);

/* Get the timer count in millisec */
gn_error_t
gntm_getmscount(gn_uint32_t *tickcount);

/* Activates the specified timer routine after specied time interval */
gn_error_t
gntm_createcountdowntimer(gn_uint32_t delay,pTimerRoutine routine,void *parameter );

#ifdef __cplusplus
}
#endif 

#endif
