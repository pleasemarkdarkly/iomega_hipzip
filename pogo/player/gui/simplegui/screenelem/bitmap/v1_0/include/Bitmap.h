#ifndef BITMAP_H_
#define BITMAP_H_

#include <gui/simplegui/common/UITypes.h>
#include <gui/simplegui/screenelem/common/ScreenElem.h>

class CBitmap : public CScreenElem {
public:
    CBitmap(short w, short h, short bpp, unsigned char* data);
    ~CBitmap();
    guiRes Draw(CDrawRegion& dr);
private:
    short m_nBpp;
    unsigned char* m_aData;
    short m_nHeight;
    short m_nWidth;
};

#endif  // BITMAP_H_
