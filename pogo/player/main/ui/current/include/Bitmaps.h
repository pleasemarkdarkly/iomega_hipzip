//........................................................................................
//........................................................................................
//.. File Name: IomegaBitmaps.h
//.. Date: 11/30/2000
//.. Author(s): Dan Bolstad
//.. Description of content: declarations of bitmaps used in the player
//.. Usage: These bitmaps are included in screens throughout the player
//.. Last Modified By: Dan Bolstad  danb@fullplaymedia.com
//.. Modification date: 12/13/2001
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef BITMAPS_H_
#define BITMAPS_H_

extern PegBitmap gb_Battery_Full_Bitmap;
extern PegBitmap gb_Battery_Empty_Bitmap;
extern PegBitmap gb_Volume_Full_Bitmap;
extern PegBitmap gb_Volume_Empty_Bitmap;
extern PegBitmap gb_Lock_Bitmap;
extern PegBitmap gb_Dadio_Bitmap;
extern PegBitmap gb_Menu_Selected_Dot_Bitmap;
extern PegBitmap gbPogoLogoBitmap;
extern PegBitmap gbIobjectsLogoBitmap;
extern PegBitmap gbUSBBitmap;
extern PegBitmap gbFullplayBitmap;

// control symbol icons
extern PegBitmap gb_FF_Bitmap;
extern PegBitmap gb_Rewind_Bitmap;
extern PegBitmap gb_Previous_Bitmap;
extern PegBitmap gb_Play_Bitmap;
extern PegBitmap gb_Pause_Bitmap;
extern PegBitmap gb_Next_Bitmap;
extern PegBitmap gb_Stop_Bitmap;
extern PegBitmap gb_Record_Bitmap;

// play mode icons
extern PegBitmap gb_Normal_Bitmap;
extern PegBitmap gb_Shuffle_Bitmap;
extern PegBitmap gb_Album_Bitmap;
extern PegBitmap gb_Shuffle_Album_Bitmap;
extern PegBitmap gb_Repeat_Bitmap;
extern PegBitmap gb_Repeat_Suffle_Bitmap;
extern PegBitmap gb_Repeat_Album_Bitmap;
extern PegBitmap gb_Repeat_Shuffle_Album_Bitmap;

// play mode menu icons
extern PegBitmap gb_MI_Shuffle_Album_Bitmap;
extern PegBitmap gb_MI_Shuffle_Bitmap;
extern PegBitmap gb_MI_Repeat_Suffle_Bitmap;
extern PegBitmap gb_MI_Repeat_Shuffle_Album_Bitmap;
extern PegBitmap gb_MI_Repeat_Album_Bitmap;
extern PegBitmap gb_MI_Repeat_Bitmap;
extern PegBitmap gb_MI_Normal_Bitmap;
extern PegBitmap gb_MI_Album_Bitmap;

// eq (aka tone) menu items
extern PegBitmap gb_MI_EQ_Classical_Bitmap;
extern PegBitmap gb_MI_EQ_Jazz_Bitmap;
extern PegBitmap gb_MI_EQ_Normal_Bitmap;
extern PegBitmap gb_MI_EQ_Rock_Bitmap;

// eq icons
extern PegBitmap gb_Equalizer_Normal_Bitmap;
extern PegBitmap gb_Equalizer_Jazz_Bitmap;
extern PegBitmap gb_Equalizer_Classical_Bitmap;
extern PegBitmap gb_Equalizer_Rock_Bitmap;

// menu screens bitmaps
extern PegBitmap gbUpArrowInvertedBitmap;
extern PegBitmap gbDownArrowInvertedBitmap;
extern PegBitmap gbForwardArrowBitmap;
extern PegBitmap gbBackArrowBitmap;
extern PegBitmap gbSubMenuArrowBitmap;
extern PegBitmap gbPrevMenuArrowBitmap;

// play
extern PegBitmap gbEmptyBitmap;
extern PegBitmap gbScreenBarBitmap;
extern PegBitmap gbScreenVerticalDottedBarBitmap;
extern PegBitmap gbTimeByAlbumBitmap;

// bitmap arrays icons that can display multiple bitmaps
extern PegBitmap* Play_Mode[];
extern PegBitmap* Control_Symbol[];
extern PegBitmap* EQ_Setting[];

#endif  // BITMAPS_H_


