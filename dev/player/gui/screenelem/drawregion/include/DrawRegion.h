#ifndef _DRAW_REGION_H_
#define _DRAW_REGION_H_

#include <gui/common/UITypes.h>
#include <gui/screenelem/common/ScreenElem.h>

class CDrawRegion : public CShowable {
public:
	CDrawRegion(int h, int w, int bpp);
	inline void PutPixel(int x, int y, int val);
	inline int CheckPixel(int x, int y);
	guiRes Draw(CDrawRegion dr);
	guiRes Clear();

	int m_height;
	int m_width;
	int m_bpp;
	int m_cPixels;
	unsigned char* m_pixels;
};

#endif // _DRAW_REGION_H_