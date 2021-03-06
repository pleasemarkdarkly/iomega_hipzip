//........................................................................................
//........................................................................................
//.. File Name: IomegaBitmaps.cpp
//.. Date: 7/21/2000
//.. Author(s): Ed Miller
//.. Description of content: declarations of bitmaps used in the player
//.. Usage: These bitmaps are included in screens throughout the player
//.. Last Modified By: Dan Bolstad   danb@fullplaymedia.com
//.. Modification date: 12/13/2001
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.
//..	 All rights reserved. This code may not be redistributed in source or linkable
//.. 	 object form without the express written consent of Fullplay Media Systems.
//.. Contact Information: www.fullplaymedia.com
//........................................................................................
//........................................................................................
#include <gui/peg/peg.hpp>


// ************  Menu Bitmaps ************

ROMDATA UCHAR ucEmptyBitmap[1] = {0x00};
PegBitmap gbEmptyBitmap = { 0x00, 1, 0, 0, 0x000000ff, (UCHAR *) ucEmptyBitmap};

ROMDATA UCHAR ucScreenBarBitmap[230] = {
0xc0,0x18,0x80,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x80,0x08,0xc0,0x18,};
PegBitmap gbScreenBarBitmap = { 0x04, 1, 115, 13, 0x000000ff, (UCHAR *) ucScreenBarBitmap};

ROMDATA UCHAR ucSolidUpArrowInvertedBitmap[7] = {
0x80,0xc0,0xe0,0xf0,0xe0,0xc0,0x80,};
PegBitmap gbSolidUpArrowInvertedBitmap = { 0x04, 1, 7, 4, 0x000000ff, (UCHAR *) ucSolidUpArrowInvertedBitmap};

ROMDATA UCHAR ucSolidDownArrowInvertedBitmap[7] = {
0x10,0x30,0x70,0xf0,0x70,0x30,0x10,};
PegBitmap gbSolidDownArrowInvertedBitmap = { 0x04, 1, 7, 4, 0x000000ff, (UCHAR *) ucSolidDownArrowInvertedBitmap};

ROMDATA UCHAR ucForwardArrowBitmap[10] = {
0x00,0x00,0x80,0x80,0xc1,0x80,0xe3,0x80,0xf7,0x80,};
PegBitmap gbForwardArrowBitmap = { 0x04, 1, 5, 9, 0x000000ff, (UCHAR *) ucForwardArrowBitmap};

ROMDATA UCHAR ucBackArrowBitmap[4] = {
0xee,0xc6,0x82,0x00,};
PegBitmap gbBackArrowBitmap = { 0x04, 1, 4, 7, 0x000000ff, (UCHAR *) ucBackArrowBitmap};

ROMDATA UCHAR uc_Menu_Selected_Dot_Bitmap[10] = {
0xe1,0x80,0xc0,0x80,0xc0,0x80,0xc0,0x80,0xe1,0x80,};
PegBitmap gb_Menu_Selected_Dot_Bitmap = { 0x04, 1, 5, 9, 0x000000ff, (UCHAR *) uc_Menu_Selected_Dot_Bitmap};

// ************  Other Bitmaps ************

ROMDATA UCHAR uc_Battery_Full_Bitmap[13] = {
0xc3,0x18,0x7e,0x42,0x42,0x7e,0x42,0x42,0x7e,0x42,0x42,0x7e,0x00,};
PegBitmap gb_Battery_Full_Bitmap = { 0x04, 1, 13, 8, 0x000000ff, (UCHAR *) uc_Battery_Full_Bitmap};

ROMDATA UCHAR uc_Battery_Empty_Bitmap[13] = {
0xc3,0x18,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x00,};
PegBitmap gb_Battery_Empty_Bitmap = { 0x04, 1, 13, 8, 0x000000ff, (UCHAR *) uc_Battery_Empty_Bitmap};

ROMDATA UCHAR uc_Volume_Full_Bitmap[21] = {
0x3f,0x3f,0x3f,0x1f,0x1f,0x1f,0x0f,0x0f,0x0f,0x07,0x07,0x07,0x03,0x03,0x03,0x01,0x01,0x01,0x00,0x00,0x00,};
PegBitmap gb_Volume_Full_Bitmap = { 0x04, 1, 21, 8, 0x000000ff, (UCHAR *) uc_Volume_Full_Bitmap};

ROMDATA UCHAR uc_Volume_Empty_Bitmap[21] = {
0x3f,0x3f,0x3f,0x5f,0x5f,0x5f,0x6f,0x6f,0x6f,0x77,0x77,0x77,0x7b,0x7b,0x7b,0x7d,0x7d,0x7d,0x7e,0x7e,0x00,};
PegBitmap gb_Volume_Empty_Bitmap = { 0x04, 1, 21, 8, 0x000000ff, (UCHAR *) uc_Volume_Empty_Bitmap};

ROMDATA UCHAR uc_Lock_Bitmap[14] = {
0x83,0x80,0x00,0x80,0x03,0x00,0x03,0x00,0x03,0x00,0x00,0x80,0x83,0x80,};
PegBitmap gb_Lock_Bitmap = { 0x04, 1, 7, 9, 0x000000ff, (UCHAR *) uc_Lock_Bitmap};

ROMDATA UCHAR uc_FF_Bitmap[30] = {
0xff,0x80,0xfe,0x80,0x7f,0x00,0x3a,0x00,0x1c,0x00,0x08,0x00,0xff,0x80,0xfe,0x80,0x7f,0x00,0x3a,0x00,0x1c,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,};
PegBitmap gb_FF_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_FF_Bitmap};

ROMDATA UCHAR uc_Rewind_Bitmap[30] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x1c,0x00,0x3a,0x00,0x7f,0x00,0xfe,0x80,0xff,0x80,0x08,0x00,0x1c,0x00,0x3a,0x00,0x7f,0x00,0xfe,0x80,0xff,0x80,};
PegBitmap gb_Rewind_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Rewind_Bitmap};

ROMDATA UCHAR uc_Previous_Bitmap[30] = {
0x7f,0x00,0xaa,0x80,0x7f,0x00,0x08,0x00,0x1c,0x00,0x3a,0x00,0x7f,0x00,0xfe,0x80,0xff,0x80,0x08,0x00,0x1c,0x00,0x3a,0x00,0x7f,0x00,0xfe,0x80,0xff,0x80,};
PegBitmap gb_Previous_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Previous_Bitmap};

ROMDATA UCHAR uc_Play_Bitmap[30] = {
0x00,0x00,0x7f,0x00,0xaa,0x80,0xfd,0x80,0x7f,0x00,0x7d,0x00,0x7b,0x00,0x3e,0x00,0x3a,0x00,0x36,0x00,0x1c,0x00,0x14,0x00,0x08,0x00,0x00,0x00,0x00,0x00,};
PegBitmap gb_Play_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Play_Bitmap};

ROMDATA UCHAR uc_Pause_Bitmap[30] = {
0x00,0x00,0x00,0x00,0x7f,0x00,0xaa,0x80,0xff,0x80,0x7f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x00,0xaa,0x80,0xff,0x80,0x7f,0x00,0x00,0x00,0x00,0x00,};
PegBitmap gb_Pause_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Pause_Bitmap};

ROMDATA UCHAR uc_Next_Bitmap[30] = {
0xff,0x80,0xfe,0x80,0x7f,0x00,0x3a,0x00,0x1c,0x00,0x08,0x00,0xff,0x80,0xfe,0x80,0x7f,0x00,0x3a,0x00,0x1c,0x00,0x08,0x00,0x7f,0x00,0xaa,0x80,0x7f,0x00,};
PegBitmap gb_Next_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Next_Bitmap};

ROMDATA UCHAR uc_Stop_Bitmap[30] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x00,0xaa,0x80,0xff,0x80,0xfe,0x80,0xff,0x80,0xfe,0x80,0xff,0x80,0xfe,0x80,0x7f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,};
PegBitmap gb_Stop_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Stop_Bitmap};

ROMDATA UCHAR uc_Record_Bitmap[30] = {
0x00,0x00,0x00,0x00,0x1c,0x00,0x77,0x00,0x7d,0x00,0xff,0x80,0xfe,0x80,0xff,0x80,0xfe,0x80,0xff,0x80,0x7f,0x00,0x7f,0x00,0x1c,0x00,0x00,0x00,0x00,0x00,};
PegBitmap gb_Record_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Record_Bitmap};

ROMDATA UCHAR ucScreenVerticalDottedBarBitmap[32] = {
0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,};
PegBitmap gbScreenVerticalDottedBarBitmap = { 0x04, 1, 4, 64, 0x000000ff, (UCHAR *) ucScreenVerticalDottedBarBitmap};

ROMDATA UCHAR ucTimeByAlbumBitmap[7] = {
0x38,0x44,0xa2,0x92,0x8a,0x44,0x38,};
PegBitmap gbTimeByAlbumBitmap = { 0x04, 1, 7, 7, 0x000000ff, (UCHAR *) ucTimeByAlbumBitmap};

ROMDATA UCHAR ucUSBBitmap[80] = {
0x80,0x80,0xba,0x80,0xbe,0x80,0xba,0x80,0xbe,0x80,0xba,0x80,0xbe,0x80,0xba,0x80,0xbe,0x80,0xba,0x80,0x00,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x3e,0x00,0x9c,0x80,0xdd,0x80,0xeb,0x80,0xdd,0x80,0xeb,0x80,
0xdd,0x80,0xeb,0x80,0xdd,0x80,0xeb,0x80,0xeb,0x80,0xeb,0x80,0xeb,0x80,0xeb,0x80,};
PegBitmap gbUSBBitmap = { 0x04, 1, 40, 9, 0x000000ff, (UCHAR *) ucUSBBitmap};

ROMDATA UCHAR uc_Shuffle_Album_Bitmap[12] = {
0xfe,0xfe,0xc6,0x92,0x18,0x1c,0x70,0x30,0x92,0xc6,0xfe,0xfe,};
PegBitmap gb_Shuffle_Album_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_Shuffle_Album_Bitmap};

ROMDATA UCHAR uc_Shuffle_Bitmap[12] = {
0xfe,0xfe,0xee,0xe6,0xe2,0xe0,0x0e,0x8e,0xce,0xee,0xfe,0xfe,};
PegBitmap gb_Shuffle_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_Shuffle_Bitmap};

ROMDATA UCHAR uc_Repeat_Suffle_Bitmap[12] = {
0x00,0x7c,0xee,0xe6,0xe2,0xe0,0x0e,0x8e,0xce,0xee,0x7c,0x00,};
PegBitmap gb_Repeat_Suffle_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_Repeat_Suffle_Bitmap};

ROMDATA UCHAR uc_Repeat_Shuffle_Album_Bitmap[12] = {
0x00,0x7c,0xc6,0x92,0x18,0x1c,0x70,0x30,0x92,0xc6,0x7c,0x00,};
PegBitmap gb_Repeat_Shuffle_Album_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_Repeat_Shuffle_Album_Bitmap};

ROMDATA UCHAR uc_Repeat_Album_Bitmap[12] = {
0x00,0x7c,0xc6,0x82,0x00,0x7c,0x38,0x10,0x82,0xc6,0x7c,0x00,};
PegBitmap gb_Repeat_Album_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_Repeat_Album_Bitmap};

ROMDATA UCHAR uc_Repeat_Bitmap[12] = {
0x00,0x7c,0xfe,0xfe,0x00,0x82,0xc6,0xee,0xfe,0xfe,0x7c,0x00,};
PegBitmap gb_Repeat_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_Repeat_Bitmap};

ROMDATA UCHAR uc_Normal_Bitmap[12] = {
0xfe,0xfe,0xfe,0xfe,0x00,0x82,0xc6,0xee,0xfe,0xfe,0xfe,0xfe,};
PegBitmap gb_Normal_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_Normal_Bitmap};

ROMDATA UCHAR uc_Album_Bitmap[12] = {
0xfe,0xfe,0xc6,0x82,0x00,0x7c,0x38,0x10,0x82,0xc6,0xfe,0xfe,};
PegBitmap gb_Album_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_Album_Bitmap};

ROMDATA UCHAR uc_Equalizer_Normal_Bitmap[30] = {
0xbf,0x80,0x5f,0x80,0x40,0x00,0x5f,0x00,0x40,0x00,0x5f,0x80,0xbf,0x80,0xff,0x80,0xbf,0x80,0x5f,0x80,0x40,0x00,0x5f,0x00,0x40,0x00,0x5f,0x80,0xbf,0x80,};
PegBitmap gb_Equalizer_Normal_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Equalizer_Normal_Bitmap};

ROMDATA UCHAR uc_Equalizer_Jazz_Bitmap[30] = {
0xdf,0x80,0xaf,0x80,0x20,0x00,0x2f,0x00,0x20,0x00,0xaf,0x80,0xdf,0x80,0xff,0x80,0xf7,0x80,0xeb,0x80,0x08,0x00,0x0b,0x00,0x08,0x00,0xeb,0x80,0xf7,0x80,};
PegBitmap gb_Equalizer_Jazz_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Equalizer_Jazz_Bitmap};

ROMDATA UCHAR uc_Equalizer_Classical_Bitmap[30] = {
0xef,0x80,0xd7,0x80,0x10,0x00,0x17,0x00,0x10,0x00,0xd7,0x80,0xef,0x80,0xff,0x80,0xdf,0x80,0xaf,0x80,0x20,0x00,0x2f,0x00,0x20,0x00,0xaf,0x80,0xdf,0x80,};
PegBitmap gb_Equalizer_Classical_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Equalizer_Classical_Bitmap};

ROMDATA UCHAR uc_Equalizer_Rock_Bitmap[30] = {
0xfd,0x80,0xfa,0x80,0x02,0x00,0x02,0x00,0x02,0x00,0xfa,0x80,0xfd,0x80,0xff,0x80,0xfe,0x80,0xfd,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0xfd,0x00,0xfe,0x80,};
PegBitmap gb_Equalizer_Rock_Bitmap = { 0x04, 1, 15, 9, 0x000000ff, (UCHAR *) uc_Equalizer_Rock_Bitmap};

// Menu Icons
ROMDATA UCHAR uc_MI_Shuffle_Album_Bitmap[12] = {
0x00,0x00,0x38,0x6c,0xe6,0xe2,0x8e,0xce,0x6c,0x38,0x00,0x00,};
PegBitmap gb_MI_Shuffle_Album_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_MI_Shuffle_Album_Bitmap};

ROMDATA UCHAR uc_MI_Shuffle_Bitmap[12] = {
0x00,0x00,0x10,0x18,0x1c,0x1e,0xf0,0x70,0x30,0x10,0x00,0x00,};
PegBitmap gb_MI_Shuffle_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_MI_Shuffle_Bitmap};

ROMDATA UCHAR uc_MI_Repeat_Suffle_Bitmap[12] = {
0xfe,0x82,0x10,0x18,0x1c,0x1e,0xf0,0x70,0x30,0x10,0x82,0xfe,};
PegBitmap gb_MI_Repeat_Suffle_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_MI_Repeat_Suffle_Bitmap};

ROMDATA UCHAR uc_MI_Repeat_Shuffle_Album_Bitmap[12] = {
0xfe,0x82,0x38,0x6c,0xe6,0xe2,0x8e,0xce,0x6c,0x38,0x82,0xfe,};
PegBitmap gb_MI_Repeat_Shuffle_Album_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_MI_Repeat_Shuffle_Album_Bitmap};

ROMDATA UCHAR uc_MI_Repeat_Album_Bitmap[12] = {
0xfe,0x82,0x38,0x7c,0xfe,0x82,0xc6,0xee,0x7c,0x38,0x82,0xfe,};
PegBitmap gb_MI_Repeat_Album_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_MI_Repeat_Album_Bitmap};

ROMDATA UCHAR uc_MI_Repeat_Bitmap[12] = {
0xfe,0x82,0x00,0x00,0xfe,0x7c,0x38,0x10,0x00,0x00,0x82,0xfe,};
PegBitmap gb_MI_Repeat_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_MI_Repeat_Bitmap};

ROMDATA UCHAR uc_MI_Normal_Bitmap[12] = {
0x00,0x00,0x00,0x00,0xfe,0x7c,0x38,0x10,0x00,0x00,0x00,0x00,};
PegBitmap gb_MI_Normal_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_MI_Normal_Bitmap};

ROMDATA UCHAR uc_MI_Album_Bitmap[12] = {
0x00,0x00,0x38,0x7c,0xfe,0x82,0xc6,0xee,0x7c,0x38,0x00,0x00,};
PegBitmap gb_MI_Album_Bitmap = { 0x04, 1, 12, 7, 0x000000ff, (UCHAR *) uc_MI_Album_Bitmap};

ROMDATA UCHAR uc_MI_EQ_Classical_Bitmap[15] = {
0x10,0x28,0xee,0xea,0xee,0x28,0x10,0x00,0x20,0x50,0xde,0xd2,0xde,0x50,0x20,};
PegBitmap gb_MI_EQ_Classical_Bitmap = { 0x04, 1, 15, 7, 0x000000ff, (UCHAR *) uc_MI_EQ_Classical_Bitmap};

ROMDATA UCHAR uc_MI_EQ_Jazz_Bitmap[15] = {
0x20,0x50,0xde,0xd2,0xde,0x50,0x20,0x00,0x10,0x28,0xee,0xea,0xee,0x28,0x10,};
PegBitmap gb_MI_EQ_Jazz_Bitmap = { 0x04, 1, 15, 7, 0x000000ff, (UCHAR *) uc_MI_EQ_Jazz_Bitmap};

ROMDATA UCHAR uc_MI_EQ_Normal_Bitmap[15] = {
0x40,0xa0,0xbe,0xa2,0xbe,0xa0,0x40,0x00,0x40,0xa0,0xbe,0xa2,0xbe,0xa0,0x40,};
PegBitmap gb_MI_EQ_Normal_Bitmap = { 0x04, 1, 15, 7, 0x000000ff, (UCHAR *) uc_MI_EQ_Normal_Bitmap};

ROMDATA UCHAR uc_MI_EQ_Rock_Bitmap[15] = {
0x08,0x14,0xf6,0xf6,0xf6,0x14,0x08,0x00,0x04,0x0a,0xfa,0xfa,0xfa,0x0a,0x04,};
PegBitmap gb_MI_EQ_Rock_Bitmap = { 0x04, 1, 15, 7, 0x000000ff, (UCHAR *) uc_MI_EQ_Rock_Bitmap};

ROMDATA UCHAR ucSubMenuArrowBitmap[4] = {
0x82,0x44,0x28,0x10,};
PegBitmap gbSubMenuArrowBitmap = { 0x04, 1, 4, 8, 0x000000ff, (UCHAR *) ucSubMenuArrowBitmap};

ROMDATA UCHAR ucPrevMenuArrowBitmap[4] = {
0x10,0x28,0x44,0x82,};
PegBitmap gbPrevMenuArrowBitmap = { 0x04, 1, 4, 8, 0x000000ff, (UCHAR *) ucPrevMenuArrowBitmap};

ROMDATA UCHAR ucUpArrowInvertedBitmap[7] = {
0x80,0x40,0x20,0x10,0x20,0x40,0x80,};
PegBitmap gbUpArrowInvertedBitmap = { 0x04, 1, 7, 4, 0x000000ff, (UCHAR *) ucUpArrowInvertedBitmap};

ROMDATA UCHAR ucDownArrowInvertedBitmap[7] = {
0x10,0x20,0x40,0x80,0x40,0x20,0x10,};
PegBitmap gbDownArrowInvertedBitmap = { 0x04, 1, 7, 4, 0x000000ff, (UCHAR *) ucDownArrowInvertedBitmap};

PegBitmap* Control_Symbol[] = {
  &gb_FF_Bitmap,
  &gb_Rewind_Bitmap,
  &gb_Previous_Bitmap,
  &gb_Play_Bitmap,
  &gb_Pause_Bitmap,
  &gb_Next_Bitmap,
  &gb_Stop_Bitmap,
  &gb_Record_Bitmap
};

PegBitmap* Play_Mode[] = {
  &gb_Normal_Bitmap,
  &gb_Shuffle_Bitmap,
  &gb_Album_Bitmap,
  &gb_Shuffle_Album_Bitmap,
  &gb_Repeat_Bitmap,
  &gb_Repeat_Suffle_Bitmap,
  &gb_Repeat_Album_Bitmap,
  &gb_Repeat_Shuffle_Album_Bitmap
};

PegBitmap* EQ_Setting[] = {
  &gb_Equalizer_Normal_Bitmap,
  &gb_Equalizer_Classical_Bitmap,
  &gb_Equalizer_Jazz_Bitmap,
  &gb_Equalizer_Rock_Bitmap,
};

// iobjects dadio logo
ROMDATA UCHAR uc_Dadio_Bitmap[475] = {
0xff,0xfe,0x0f,0xff,0xfc,0xff,0xf8,0xc3,0xff,0xfc,0xff,0xe3,0xf0,0xff,0xfc,0xff,0xc7,0xfc,0x3f,0xfc,
0xff,0x8f,0x00,0x0f,0xfc,0xff,0x1e,0x00,0x01,0xfc,0xff,0x3f,0x00,0x01,0xfc,0xfe,0x3f,0xc0,0x00,0xfc,
0xfc,0x3f,0x39,0x00,0x7c,0xfc,0x1e,0xed,0xc0,0x1c,0xf8,0x1d,0xe5,0xc0,0x1c,0xf0,0x1d,0x89,0x78,0x50,
0xf0,0x1c,0xfc,0x28,0x20,0xe0,0x41,0xfd,0x4c,0x2c,0xe0,0xc3,0xfd,0xf6,0x04,0xc0,0x87,0xfd,0xf2,0x04,
0xc0,0x0f,0xf8,0xdb,0x04,0x80,0x1e,0xf0,0x6b,0x04,0x80,0x1f,0x61,0x29,0x04,0x80,0x5c,0xe0,0xad,0x04,
0x00,0xea,0xe0,0x29,0x04,0x01,0x6a,0xa0,0x2b,0x04,0x00,0xaa,0xb0,0x69,0x00,0x00,0x4a,0xfc,0xdb,0x08,
0x00,0x1c,0xbb,0xd9,0x08,0x00,0x0f,0x80,0xf9,0x00,0x00,0x07,0xf0,0x7b,0x80,0x00,0x03,0xe1,0x33,0xe0,
0x80,0x3d,0xe0,0xa0,0xc8,0x80,0x3e,0x20,0x1e,0x10,0x80,0x3f,0xc0,0x3f,0xf4,0xc0,0x3f,0xf0,0x7f,0xe4,
0xc0,0x7f,0xf8,0xff,0xcc,0xe0,0x7f,0xff,0xff,0x8c,0xe0,0xff,0xc0,0xff,0x1c,0xf3,0xff,0x3f,0x3e,0x3c,
0xf9,0xfc,0x7f,0x9c,0x7c,0xfc,0xf0,0xff,0xc0,0xfc,0xfe,0x03,0xff,0xe1,0xfc,0xff,0x0f,0xff,0xff,0xfc,
0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xc1,0xf0,0x00,0x00,0x00,
0xdd,0xf0,0x00,0x00,0x00,0xc0,0x70,0x00,0x00,0x00,0xff,0xf0,0x00,0x00,0x00,0xc1,0x70,0x00,0x00,0x04,
0xff,0xf0,0x7f,0xff,0x04,0x41,0xf0,0xff,0xff,0x04,0x5d,0xf0,0xff,0xff,0x0c,0x01,0xf8,0x7f,0xff,0x0c,
0xff,0xfc,0x0f,0xfc,0x1c,0xc1,0x7f,0x00,0x00,0x7c,0xff,0xff,0xf0,0x03,0xfc,0xfd,0xff,0xff,0xff,0xfc,
0xc0,0x7c,0x01,0xff,0xfc,0xfd,0xf8,0x00,0x78,0xfc,0xff,0xf8,0xfe,0x38,0x7c,0xc5,0xf9,0xff,0x3c,0x7c,
0xd5,0xf9,0xff,0x3c,0x7c,0xc1,0xf0,0x00,0x00,0x7c,0xff,0xf0,0x00,0x00,0x7c,0xc0,0x70,0x00,0x00,0xfc,
0xff,0xf3,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xfe,0x00,0x7f,0xfc,0xc5,0xfc,0x00,0x3f,0xfc,
0xd5,0xfc,0x3f,0x1f,0xfc,0xc1,0xfc,0x7f,0x9f,0xfc,0xff,0xfc,0x7f,0x9f,0xfc,0xc1,0xfc,0x00,0x1f,0xfc,
0xdf,0xf8,0x00,0x00,0x0c,0xc1,0xf8,0x00,0x00,0x0c,0xff,0xf8,0x00,0x00,0x0c,0xc1,0xf8,0xff,0xff,0xfc,
0xdd,0xff,0xff,0xf0,0xfc,0xc0,0x60,0x00,0x60,0xfc,0xff,0xe0,0x00,0x60,0xfc,0xc1,0x60,0x00,0x7f,0xfc,
0xff,0xe3,0xff,0xff,0xfc,0xc1,0xff,0xc0,0x0f,0xfc,0xdd,0xfc,0x00,0x01,0xfc,0xc1,0xf8,0x00,0x00,0xfc,
0xff,0xf8,0x00,0x00,0x7c,0xff,0xf0,0x00,0x00,0x3c,0xff,0xf0,0x03,0xe0,0x1c,0xc0,0x70,0x07,0xf8,0x1c,
0xdf,0x70,0x07,0xf8,0x1c,0xdf,0x70,0x07,0xf8,0x1c,0xc0,0x78,0x03,0xf0,0x1c,0xff,0xf8,0x01,0xe0,0x3c,
0xd8,0x7c,0x00,0x00,0x7c,0xdb,0x7e,0x00,0x00,0xfc,0xc3,0x7f,0x80,0x03,0xfc,};
PegBitmap gb_Dadio_Bitmap = { 0x04, 1, 95, 38, 0x000000ff, (UCHAR *) uc_Dadio_Bitmap};

// fullplay text logo
ROMDATA UCHAR ucFullplayBitmap[475] = {
0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xfe,0xff,0xfc,0xff,0xff,0xfe,0xff,0xfc,
0xff,0xf8,0x00,0x1f,0xfc,0xff,0xf8,0x00,0x0f,0xfc,0xff,0xff,0xfe,0xe7,0xfc,0xff,0xff,0xfe,0xf7,0xfc,
0xff,0xff,0xfe,0xf7,0xfc,0xff,0xff,0xfe,0xf7,0xfc,0xff,0xff,0xfe,0xf7,0xfc,0xff,0xff,0xff,0xff,0xfc,
0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xfe,0x00,0xff,0xfc,0xff,0xfc,0x00,0xff,0xfc,
0xff,0xf9,0xff,0xff,0xfc,0xff,0xfb,0xff,0xff,0xfc,0xff,0xfb,0xff,0xff,0xfc,0xff,0xfb,0xff,0xff,0xfc,
0xff,0xfb,0xff,0xff,0xfc,0xff,0xfb,0xff,0xff,0xfc,0xff,0xfb,0xff,0xff,0xfc,0xff,0xfb,0xff,0xff,0xfc,
0xff,0xfd,0xff,0xff,0xfc,0xff,0xf8,0x00,0xff,0xfc,0xff,0xf8,0x00,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,
0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xf8,0x00,0x07,0xfc,0xff,0xf8,0x00,0x07,0xfc,
0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,
0xff,0xf8,0x00,0x07,0xfc,0xff,0xf8,0x00,0x07,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,
0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0x80,0x00,0xff,0xfc,0xff,0x80,0x00,0xff,0xfc,
0xff,0xfd,0xfd,0xff,0xfc,0xff,0xfb,0xfe,0xff,0xfc,0xff,0xfb,0xfe,0xff,0xfc,0xff,0xfb,0xfe,0xff,0xfc,
0xff,0xfb,0xfe,0xff,0xfc,0xff,0xfb,0xfe,0xff,0xfc,0xff,0xfb,0xfe,0xff,0xfc,0xff,0xfb,0xfe,0xff,0xfc,
0xff,0xfb,0xfe,0xff,0xfc,0xff,0xf9,0xfc,0xff,0xfc,0xff,0xfc,0x01,0xff,0xfc,0xff,0xfe,0x03,0xff,0xfc,
0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xf8,0x00,0x07,0xfc,
0xff,0xf8,0x00,0x07,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,
0xff,0xfc,0x39,0xff,0xfc,0xff,0xf8,0x18,0xff,0xfc,0xff,0xfb,0xde,0xff,0xfc,0xff,0xfb,0xde,0xff,0xfc,
0xff,0xfb,0xde,0xff,0xfc,0xff,0xfb,0xde,0xff,0xfc,0xff,0xfb,0xde,0xff,0xfc,0xff,0xfb,0xde,0xff,0xfc,
0xff,0xfb,0xde,0xff,0xfc,0xff,0xfb,0xde,0xff,0xfc,0xff,0xfb,0xde,0xff,0xfc,0xff,0xfd,0xbc,0xff,0xfc,
0xff,0xf8,0x01,0xff,0xfc,0xff,0xf8,0x03,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xfe,0xff,0xfc,
0xff,0xff,0xf8,0xff,0xfc,0xff,0xbf,0xf1,0xff,0xfc,0xff,0xbf,0xc7,0xff,0xfc,0xff,0xbf,0x0f,0xff,0xfc,
0xff,0x9c,0x3f,0xff,0xfc,0xff,0xc0,0xff,0xff,0xfc,0xff,0xe1,0xff,0xff,0xfc,0xff,0xf8,0x7f,0xff,0xfc,
0xff,0xfe,0x1f,0xff,0xfc,0xff,0xff,0x87,0xff,0xfc,0xff,0xff,0xe1,0xff,0xfc,0xff,0xff,0xf8,0xff,0xfc,
0xff,0xff,0xfe,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,0xff,0xff,0xff,0xff,0xfc,};
PegBitmap gbFullplayBitmap = { 0x04, 1, 95, 38, 0x000000ff, (UCHAR *) ucFullplayBitmap};

ROMDATA UCHAR ucIobjectsLogoBitmap[1024] = {
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xfe,0x0f,0xff,0xff,0xff,0xff,0xff,0xfc,0x7c,0x07,0xff,0xff,
0xff,0xff,0xff,0xc0,0x78,0x01,0xff,0xff,0xff,0xff,0xfe,0x00,0x70,0x00,0xff,0xff,
0xff,0xff,0xfc,0x00,0x60,0xf0,0x7f,0xff,0xff,0xff,0xfc,0x3d,0x41,0x80,0x0f,0xff,
0xff,0xff,0xfc,0x20,0x43,0x80,0x01,0xff,0xff,0xff,0xfc,0x20,0x07,0xc0,0x00,0xff,
0xff,0xff,0xfc,0x02,0x0f,0xe0,0x0c,0x7f,0xff,0xff,0xfc,0x39,0x1f,0xc0,0x08,0x3f,
0xff,0xff,0xfc,0x65,0x1f,0x3c,0xf6,0x1f,0xff,0xff,0xfc,0x4d,0x1f,0x44,0xd8,0x0f,
0xff,0xff,0xfc,0x61,0x0e,0xec,0xa6,0x0d,0xff,0xff,0xfc,0x3e,0x2e,0xda,0x5a,0x11,
0xff,0xff,0xfc,0x00,0x62,0x7e,0xfb,0x17,0xff,0xff,0xfe,0x3e,0x41,0xfe,0xfd,0x0b,
0xff,0xff,0xfe,0x20,0x43,0xfe,0xfe,0x83,0xff,0xff,0xfe,0x38,0x03,0xbe,0xfd,0x83,
0xff,0xff,0xfe,0x08,0x03,0x9c,0x3e,0x83,0xff,0xff,0xfe,0x08,0x0f,0x38,0x9d,0x83,
0xff,0xff,0xfe,0x80,0x3a,0xb8,0xde,0x87,0xff,0xff,0xfe,0x7a,0x7e,0xb8,0x9d,0x87,
0xff,0xff,0xfe,0x20,0x7a,0xb9,0x9e,0x87,0xff,0xff,0xfe,0x20,0x3e,0xac,0x3d,0x8f,
0xff,0xff,0xfe,0x20,0x03,0x3f,0xfd,0x8f,0xff,0xff,0xfe,0x30,0x03,0xdc,0x7e,0x87,
0xff,0xff,0xfe,0x28,0x01,0xe0,0x3d,0xa7,0xff,0xff,0xfe,0x38,0x06,0xf8,0x99,0x63,
0xff,0xff,0xfe,0x28,0x07,0x78,0xd2,0x73,0xff,0xff,0xfe,0x20,0x07,0xb9,0x8f,0xb1,
0xff,0xff,0xfe,0x20,0x1f,0xc4,0x1f,0xf1,0xff,0xff,0xfe,0x38,0x1f,0xfe,0x3f,0xe3,
0xff,0xff,0xfe,0x28,0x1f,0xff,0xff,0xe3,0xff,0xff,0xfe,0x28,0x3f,0xf0,0xff,0xc7,
0xff,0xff,0xfe,0x20,0xff,0xe0,0x3f,0xc7,0xff,0xff,0xfe,0x28,0xff,0xc7,0x07,0x0f,
0xff,0xff,0xfe,0x3c,0xff,0x9f,0xc0,0x1f,0xff,0xff,0xfe,0x28,0xfe,0x3f,0xf8,0x3f,
0xff,0xff,0xfe,0x20,0x38,0x7f,0xfc,0x7f,0xff,0xff,0xfe,0x20,0x00,0xff,0xff,0xff,
0xff,0xff,0xfe,0x18,0x03,0xff,0xff,0xff,0xff,0xff,0xff,0x30,0x0f,0xff,0xff,0xff,
0xff,0xff,0xff,0x20,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xe0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x80,0x00,0x00,0x0f,
0xff,0xff,0xff,0xff,0x80,0x00,0x00,0x0f,0xff,0xff,0xff,0x8f,0x80,0x00,0x00,0x0f,
0xff,0xff,0xff,0x77,0x80,0x00,0x00,0x1f,0xff,0xff,0xff,0x01,0x80,0x00,0x00,0x1f,
0xff,0xff,0xff,0xff,0x83,0xff,0xfc,0x1f,0xff,0xff,0xff,0x05,0x87,0xff,0xf8,0x3f,
0xff,0xff,0xff,0xff,0x83,0xff,0xf8,0x3f,0xff,0xff,0xfd,0x8f,0x81,0xff,0xf0,0x7f,
0xff,0xff,0xfd,0x77,0xc0,0x7f,0xc0,0xff,0xff,0xff,0xfe,0x07,0xf0,0x00,0x01,0xff,
0xff,0xff,0xff,0xff,0xfe,0x00,0x0f,0xff,0xff,0xff,0xff,0x05,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xc0,0x03,0xe7,0xff,0xff,0xff,0xff,0xf7,0x80,0x01,0xc3,0xff,
0xff,0xff,0xff,0x01,0x80,0x00,0xc1,0xff,0xff,0xff,0xff,0xf7,0x87,0xf8,0xe1,0xff,
0xff,0xff,0xff,0xff,0x8f,0xfc,0xf1,0xff,0xff,0xff,0xff,0xb7,0xc0,0x00,0xc1,0xff,
0xff,0xff,0xff,0x57,0x80,0x00,0x01,0xff,0xff,0xff,0xff,0x0f,0x80,0x00,0x03,0xff,
0xff,0xff,0xff,0xff,0x80,0x00,0x07,0xff,0xff,0xff,0xff,0x81,0xbf,0xff,0xff,0xff,
0xff,0xff,0xff,0x7f,0xf0,0x07,0xff,0xff,0xff,0xff,0xff,0xff,0xc0,0x01,0xff,0xff,
0xff,0xff,0xff,0xff,0x80,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x83,0xfc,0x7f,0xff,
0xff,0xff,0xff,0xb7,0xc3,0xfe,0x7f,0xff,0xff,0xff,0xff,0x57,0xe0,0x00,0x3f,0xff,
0xff,0xff,0xff,0x0f,0x80,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0x80,0x00,0x00,0x3f,
0xff,0xff,0xff,0x87,0x80,0x00,0x00,0x1f,0xff,0xff,0xff,0x7f,0x9f,0xff,0xfe,0x1f,
0xff,0xff,0xff,0x07,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xe1,0x83,0xff,
0xff,0xff,0xff,0x8f,0x00,0x01,0x03,0xff,0xff,0xff,0xff,0x77,0x00,0x01,0x07,0xff,
0xff,0xff,0xff,0x01,0x00,0x01,0xff,0xff,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,
0xff,0xff,0xff,0x05,0xf8,0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0xe0,0x00,0x07,0xff,
0xff,0xff,0xff,0x8f,0xc0,0x00,0x03,0xff,0xff,0xff,0xff,0x77,0xc0,0x00,0x01,0xff,
0xff,0xff,0xff,0x8f,0x80,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0x80,0x06,0x00,0x7f,
0xff,0xff,0xff,0xff,0x80,0x1f,0x80,0x7f,0xff,0xff,0xff,0xff,0x80,0x3f,0xe0,0x7f,
0xff,0xff,0xff,0x83,0x80,0x3f,0xe0,0x7f,0xff,0xff,0xff,0x7d,0x80,0x0f,0x80,0xff,
0xff,0xff,0xff,0x7d,0xc0,0x07,0x00,0xff,0xff,0xff,0xff,0x83,0xe0,0x00,0x01,0xff,
0xff,0xff,0xff,0xff,0xf0,0x00,0x03,0xff,0xff,0xff,0xff,0x73,0xfc,0x00,0x0f,0xff,
0xff,0xff,0xff,0x6d,0xdf,0x80,0x3f,0xff,0xff,0xff,0xff,0x8d,0x1f,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xdf,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xbf,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xbf,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,};
PegBitmap gbIobjectsLogoBitmap = { 0x04, 1, 128, 64, 0x000000ff, (UCHAR *) ucIobjectsLogoBitmap};


ROMDATA UCHAR ucPogoLogoBitmap[1024] = {
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf0,0x3f,0xff,
0xff,0xff,0xff,0xff,0xe0,0x00,0x3f,0xff,0xff,0xff,0xff,0x80,0x00,0x00,0x3f,0xff,
0x00,0x0e,0x00,0x00,0x00,0x00,0x3f,0xff,0x00,0x0e,0x00,0x00,0x00,0x0e,0x3f,0xff,
0xf3,0xce,0x00,0x00,0x3f,0xfc,0x3f,0xff,0xf1,0x8e,0x00,0xf0,0xff,0xfc,0x3f,0xff,
0xf0,0x0f,0xff,0xf8,0xff,0xf8,0x7f,0xff,0xf8,0x1f,0xff,0xf8,0x7f,0xc0,0x7f,0xff,
0xff,0xff,0xff,0xf8,0x00,0x00,0xff,0xff,0x00,0x0f,0xff,0xfc,0x00,0x01,0xff,0xff,
0x00,0x0f,0xff,0xfe,0x00,0x07,0xff,0xff,0xf9,0xcf,0xff,0xff,0xc0,0x3f,0xff,0xff,
0xe1,0xcf,0xff,0xff,0xff,0xff,0xff,0xff,0x80,0x8f,0xe0,0x00,0x1f,0xff,0xff,0xff,
0x08,0x1f,0x00,0x00,0x07,0xff,0xff,0xff,0x3c,0x3c,0x00,0x00,0x01,0xff,0xff,0xff,
0xff,0xf8,0x00,0x00,0x00,0xff,0xff,0xff,0xc0,0x38,0x0f,0xff,0xf0,0xff,0xff,0xff,
0x80,0x18,0x7f,0xff,0xf8,0x7f,0xff,0xff,0x1f,0x88,0x7f,0xff,0xf0,0x7f,0xff,0xff,
0x3f,0xc8,0x1f,0xfc,0x00,0x7f,0xff,0xff,0x3f,0xcc,0x00,0x00,0x00,0xff,0xff,0xff,
0x1f,0x8e,0x00,0x00,0x01,0xff,0xff,0xff,0x80,0x1f,0xc0,0x00,0x1f,0xff,0xff,0xff,
0xe0,0x3f,0xfe,0x07,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0x80,0x1f,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0xf0,0x00,0x00,0x3f,0xff,0xff,
0x3f,0xcf,0xc0,0x00,0x00,0x07,0xff,0xff,0x3f,0xcf,0x00,0x00,0x00,0x00,0xff,0xff,
0x1f,0x8e,0x00,0x00,0x00,0x00,0x7f,0xff,0x87,0x1e,0x03,0xff,0xfe,0x00,0x3f,0xff,
0xc0,0x3c,0x1f,0xff,0xff,0xf0,0x1f,0xff,0xe0,0x7c,0x3f,0xff,0xf1,0xfc,0x1f,0xff,
0xff,0xfc,0x3f,0xff,0xe1,0xfe,0x0f,0xff,0xc0,0x0c,0x00,0x1f,0xe1,0xff,0x0f,0xff,
0x80,0x0e,0x00,0x00,0x01,0xff,0x0f,0xff,0x1f,0xff,0x00,0x00,0x01,0xff,0x8f,0xff,
0x3f,0xff,0xf0,0x00,0x01,0xff,0xcf,0xff,0x3f,0xff,0xff,0xf8,0x01,0xff,0xff,0xff,
0x00,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0x80,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0x00,0x01,0xff,0xff,0xff,0xff,0xc0,0x3c,0x00,0x00,0x1f,0xff,0xff,0xff,
0x80,0x18,0x00,0x00,0x07,0xff,0xff,0xff,0x1f,0x88,0x00,0x00,0x01,0xff,0xff,0xff,
0x3f,0xc8,0x7f,0xff,0x00,0xff,0xff,0xff,0x3f,0xc8,0x7f,0xff,0xe0,0xff,0xff,0xff,
0x1f,0x8c,0x1f,0xff,0xf8,0x7f,0xff,0xff,0x9f,0x9e,0x00,0x7f,0xf0,0x7f,0xff,0xff,
0xff,0xff,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xcf,0xc0,0x00,0x00,0xff,0xff,0xff,
0xff,0xcf,0xf8,0x00,0x01,0xff,0xff,0xff,0x00,0x0f,0xff,0xc0,0x0f,0xff,0xff,0xff,
0x00,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xcf,0xc0,0xff,0xff,0xff,0xff,0xff,
0xff,0xcf,0x00,0x1f,0xff,0xff,0xff,0xff,0x9e,0x3e,0x3f,0x87,0xff,0xff,0xff,0xff,
0x1c,0x1e,0x60,0x63,0xff,0xff,0xff,0xff,0x38,0x0c,0xcf,0x33,0x0f,0xff,0xff,0xff,
0x38,0xcc,0x46,0x72,0x00,0x01,0xff,0xff,0x31,0xcc,0x63,0xc6,0x00,0x00,0x01,0xff,
0x03,0x8e,0x30,0x0f,0x00,0x00,0x00,0x07,0x87,0x9f,0x1f,0xff,0xf8,0x00,0x00,0x00,
0xff,0xff,0x81,0xff,0xff,0xe0,0x00,0x00,0xff,0xff,0xe1,0xff,0xff,0xff,0x80,0x00,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x01,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,};
PegBitmap gbPogoLogoBitmap = { 0x04, 1, 128, 64, 0x000000ff, (UCHAR *) ucPogoLogoBitmap};


