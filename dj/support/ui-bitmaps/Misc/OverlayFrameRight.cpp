
#include "peg.hpp"
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

ROMDATA UCHAR ucOverlayFrameRightBitmap[20] = {
0x01,0xff,0xff,0xe0,0x00,0x80,0x07,0xf8,0x00,0x40,0xfc,0x00,0x00,0x0f,0xc0,0xff,0xf0,0x03,0xff,0xc0,};
PegBitmap gbOverlayFrameRightBitmap = { 0x04, 1, 4, 34, 0x000000ff, (UCHAR *) ucOverlayFrameRightBitmap};

