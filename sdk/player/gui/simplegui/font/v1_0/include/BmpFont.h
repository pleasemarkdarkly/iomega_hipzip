#ifndef BMPFONT_H_
#define BMPFONT_H_

#include <gui/simplegui/common/UITypes.h>

class CDrawRegion;

class CBmpFont {
public:
    CBmpFont(short h, unsigned char* data, short* offsets, unsigned char* widths, short bytes_per_line);
    ~CBmpFont() {}
    short DrawChar(char c, CDrawRegion& dr, guiRect& clip, guiPoint& start, guiColor& color);

private:
    short m_nHeight;
    unsigned char* m_aData;
    short* m_aOffsets;
    unsigned char* m_aWidths;
    short m_cBytesPerLine;
};

#endif  // BMPFONT_H_
