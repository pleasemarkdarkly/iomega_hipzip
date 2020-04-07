#include <gui/common/UITypes.h>
#include <gui/screenelem/drawregion/DrawRegion.h>
#include <util/debug/debug.h>
#include <devs/lcd/lcd.h>

#include <string.h>	// memset

DEBUG_USE_MODULE(DEBUG_GUI);

CDrawRegion::CDrawRegion(int h, int w, int bpp) : m_height(h), m_width(w), m_bpp(bpp) {
	m_cPixels = h*w*bpp/8;
	m_pixels = new unsigned char [m_cPixels + 1];
}

void CDrawRegion::PutPixel(int x, int y, int val) {
	unsigned char* p = m_pixels;

	p += y * m_width * m_bpp / 8;
	p += x * m_bpp / 8;
	if (m_bpp < 8)
	{
		short oddbits = (x * m_bpp) % 8;
		unsigned char mask = 0xFF << (8-m_bpp);	

		mask >>= oddbits;						
		*p &= mask ^ 0xFF;
		*p |= val << (m_bpp-oddbits);
	}
}

int CDrawRegion::CheckPixel(int x, int y) {

}

guiRes CDrawRegion::Draw(CDrawRegion dr) {
	return LCDWriteRaw(m_pixels, m_cPixels );
}

guiRes CDrawRegion::Clear() {
	memset ((void*) m_pixels, 0, m_cPixels);
	return GUI_OK;
}
