//
// Timers.h: defines for PEG timers used in the ui
// danb@fullplaymedia.com 03/12/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//

#ifndef TIMERS_H_
#define TIMERS_H_

// Alert Screen
#define AS_TIMER_TIMEOUT                                100

// CD Triage Screen
#define CDTS_TIMER_SCROLL_TITLE                         300
#define CDTS_TIMER_SCROLL_END                           301
#define CDTS_TIMER_INSTRUCTIONS                         302

// Edit Screen
#define ES_TIMER_TIMEOUT                                400
#define ES_TIMER_CURSOR_ADVANCE                         401

// MultiString
#define TIMER_MULTISTRING                               500

// PlayerScreen
#define PS_TIMER_SCROLL_TEXT                            600
#define PS_TIMER_SCROLL_END                             601
#define PS_TIMER_SLEEP_AFTER_TRACK_CHANGE               602
#define PS_TIMER_IR                                     603
#define PS_TIMER_DO_TRACK_CHANGE                        604

// Splash Screen
#define SS_TIMER_SCREEN_CHANGE                          700

// Yes No Screen
#define YNS_TIMER_TIMEOUT                               800

// Letter Select Screen
#define LSS_TIMER_TIMEOUT                               900

// Quick Browse Screen
#define QBMS_TIMER_SCROLL_TITLE                        1000
#define QBMS_TIMER_SCROLL_END                          1001

// Menu Screen
#define MS_TIMER_SCROLL_TITLE                          2000
#define MS_TIMER_SCROLL_END                            2001

// Dynamic Menu Screen
#define DMS_TIMER_SCROLL_TITLE                         2200
#define DMS_TIMER_SCROLL_END                           2201

// Line In Screen
#define LIS_TIMER_CLIP_CHECK                           2400

// Note:  Start the next timer at 2500 at the least



//
// timer values for scrolling that are shared among many ui screens
//

#define SCROLL_FAST_START_INTERVAL                      500
#define SCROLL_FAST_MENU_ITEM_START_INTERVAL            175
#define SCROLL_FAST_END_INTERVAL                        175
#define SCROLL_FAST_CONTINUE_INTERVAL                    15

#define SCROLL_SLOW_START_INTERVAL                      750
#define SCROLL_SLOW_MENU_ITEM_START_INTERVAL            250
#define SCROLL_SLOW_END_INTERVAL                        250
#define SCROLL_SLOW_CONTINUE_INTERVAL                    30


#endif  // TIMERS_H_

