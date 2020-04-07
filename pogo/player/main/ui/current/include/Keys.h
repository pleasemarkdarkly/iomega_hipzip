//........................................................................................
//........................................................................................
//.. File Name: Keys.h															..
//.. Date: 09/24/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definitions of button values for peg		 				..
//.. Usage: Used by peg to identify events												..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/24/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef KEYS_H_
#define KEYS_H_

#include <gui/peg/peg.hpp>

#ifdef PEGWIN32

#define	IOPK_PLAYPAUSE		'5'//' '
#define	IOPK_NEXT			'6'//PK_RIGHT
#define	IOPK_PREV			'4'//PK_LEFT
#define	IOPK_MENU			'9'//PK_CR
#define	IOPK_UP				'8'//PK_LNUP
#define	IOPK_DOWN			'2'//PK_LNDN
#define	IOPK_RECORD			'7'//PK_LNDN
#define	IOPK_DIAL_IN		PK_LEFT
#define	IOPK_DIAL_UP		PK_LNUP
#define	IOPK_DIAL_DOWN		PK_LNDN

#else

#define KEY_PLAY_PAUSE          14
#define KEY_STOP                13
#define KEY_PREVIOUS            12
#define KEY_NEXT                11
#define KEY_UP                  10
#define KEY_DOWN                9
#define KEY_TEST_STIMULATE      8
#define KEY_REFRESH_CONTENT     7
#define KEY_MENU                6
#define KEY_RECORD              5
#define KEY_DIAL_IN             4
#define KEY_DIAL_UP             3
#define KEY_DIAL_DOWN           2
#define KEY_BREAK_POINT         1

#endif // PEGWIN32

#endif	// KEYS_H_
