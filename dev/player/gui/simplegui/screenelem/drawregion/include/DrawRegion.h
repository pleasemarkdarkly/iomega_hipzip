#ifndef _DRAW_REGION_H_
#define _DRAW_REGION_H_

#include <gui/simplegui/common/UITypes.h>
#include <gui/simplegui/screenelem/common/ScreenElem.h>

class CDrawRegion : public CShowable {
  public:
    CDrawRegion(int h, int w, int bpp);
    void PutPixel(int x, int y, int val);
    void PutBytes(int x, int y, unsigned char* src, int count);
    int CheckPixel(int x, int y);
    guiRes Draw( CDrawRegion& dr );
    guiRes Clear( guiRect& region );
    guiRes Clear();

    guiRect m_rScreen;
    int m_nHeight;
    int m_nWidth;
    int m_nByteWidth;
    int m_nBpp;
    int m_cPixels;
    unsigned char* m_aPixels;
};

#endif // _DRAW_REGION_H_
