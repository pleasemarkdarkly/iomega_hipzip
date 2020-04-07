//........................................................................................
//........................................................................................
//.. File Name: Bitmaps.h
//.. Date: 10/03/2001
//.. Author(s): Dan Bolstad
//.. Description of content: declarations of bitmaps used in the player
//.. Usage: These bitmaps are included in screens throughout the player
//.. Last Modified By: Dan Bolstad  danb@iobjects.com
//.. Modification date: 10/10/2001
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef BITMAPS_H_
#define BITMAPS_H_

typedef unsigned char UCHAR;


typedef struct SGUIBitmap {
  int bpp;
  int w;
  int h;
  UCHAR* buff;
} SimpleBitmap;



extern SimpleBitmap gbEmptyBitmap;
extern SimpleBitmap gbSolidUpArrowInvertedBitmap;
extern SimpleBitmap gbSolidDownArrowInvertedBitmap;
extern SimpleBitmap gbWarningBitmap;
extern SimpleBitmap gbInstructionsBitmap;
extern SimpleBitmap gbIobjectsLogoBitmap;
extern SimpleBitmap gbFullplayLogoBitmap;

#endif  // BITMAPS_H_


