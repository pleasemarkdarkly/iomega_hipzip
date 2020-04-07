//........................................................................................
//........................................................................................
//.. File Name: gumi.h																	..
//.. Date: 7/21/2000																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: Graphics structure declarations   							..
//.. Usage: demojumper GUI																..
//.. Last Modified By: Malsbary	toddm@iobjects.com										..	
//.. Modification date: 9/5/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef GUMI_H_
#define GUMI_H_

//#include "demojumper.h"

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// Select either clockwise or counter-clockwise screen rotation:

#define ROTATE_CCW          // counter clockwise
//#define ROTATE_CW         // clockwise
/*--------------------------------------------------------------------------*/

typedef short int SIGNED;       // 16 bit signed
typedef unsigned char UCHAR;    // 8 bit unsigned
typedef unsigned short WORD;    // 16 bit unsigned

typedef struct tagGumiRect {
  SIGNED wTop;
  SIGNED wBottom;
  SIGNED wLeft;
  SIGNED wRight;
} GumiRect;

typedef struct Point{
  SIGNED x;
  SIGNED y;
} GumiPoint;

typedef struct Font{
  UCHAR uType;
  UCHAR uAscent;
  UCHAR uDescent;
  UCHAR uHeight;
  WORD  wBytesPerLine;
  WORD  wFirstChar;
  WORD  wLastChar;
  WORD  *pOffsets;
  UCHAR *pWidths;
  UCHAR *pData;
} GumiFont;

typedef struct Color{
  UCHAR uForeground;
  UCHAR uBackground;
  UCHAR uFlags;
} GumiColor;

#endif  //  GUMI_H_
