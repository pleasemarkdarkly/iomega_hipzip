#include <gui/simplegui/common/UITypes.h>
#include <gui/simplegui/screenelem/drawregion/DrawRegion.h>
#include <util/debug/debug.h>
#include <devs/lcd/lcd.h>

#include <string.h>	// memset

DEBUG_USE_MODULE(DEBUG_GUI);

CDrawRegion::CDrawRegion(int h, int w, int bpp) : m_nHeight(h), m_nWidth(w), m_nBpp(bpp)
{
    m_cPixels = h*w*bpp/8;
    m_aPixels = new unsigned char [m_cPixels + 1];
    m_nByteWidth = w*bpp/8;
    if (w*bpp%8) ++m_nByteWidth;

    m_rScreen.ul.x = 0;
    m_rScreen.ul.y = 0;
    m_rScreen.lr.x = w;
    m_rScreen.lr.y = h;
}

void CDrawRegion::PutPixel(int x, int y, int val)
{
    unsigned char* p = m_aPixels;
	
    p += y * m_nByteWidth;
    p += x * m_nBpp / 8;
    if (m_nBpp < 8)
    {
        short oddbits = (x * m_nBpp) % 8;
        unsigned char mask = 0xFF << (8-m_nBpp);	

        mask >>= oddbits;						
        *p &= mask ^ 0xFF;
        *p |= val << (m_nBpp-oddbits);
    }
}

void CDrawRegion::PutBytes(int x, int y, unsigned char* src, int count) {
    unsigned char* p = m_aPixels;

    p += y * m_nByteWidth;
    p += x * m_nBpp / 8;
    memcpy ((void*) p, (void*) src, count);
}

int CDrawRegion::CheckPixel(int x, int y)
{
    //dc- is this routine supposed to return the pixel at that location?
    return 0;
}

guiRes CDrawRegion::Draw(CDrawRegion& dr) {
    return LCDWriteRaw(m_aPixels, m_cPixels );
}

guiRes CDrawRegion::Clear( guiRect& region )
{
    // calculate the area to clear
    // the first part of these calculations dodge inverted rects and could
    // probably be ditched
    int startx = (region.lr.x < region.ul.x ? region.lr.x : region.ul.x );
    int starty = (region.lr.y < region.ul.y ? region.lr.y : region.ul.y );
    
    int x = region.lr.x - region.ul.x;
    int y = region.lr.y - region.ul.y;

    if( x < 0 ) x *= -1;
    if( y < 0 ) y *= -1;

    int endx = startx + x;
    int endy = starty + y;

    int mask = (8 / m_nBpp) - 1;
    
    if( startx & mask ) {
        // take care of the odd pixels
        int z;
        for( z = startx; z & mask; z++ ) {
            for( int i = starty; i < endy; i++ ) {
                this->PutPixel( z, i, 0 );
            }
        }
        
        startx = z;
    }
    if( endx & mask ) {
        // take care of the other odd pixels
        int z;
        for( z = endx; z & mask; z-- ) {
            for( int i = starty; i < endy; i++ ) {
                this->PutPixel( z, i, 0 );
            }
        }
        
        endx = z;
    }

    // memset the rest
    int line_width = ((endx - startx) * m_nBpp ) / 8;

    int inc = (m_rScreen.lr.x * m_nBpp) / 8;
    unsigned char* scr = m_aPixels + (inc * starty) + ((startx * m_nBpp) / 8);
        
    if( line_width ) {
        for( int i = starty; i <= endy; i++ ) {
            memset( (void*)scr, 0, line_width );
            scr += inc;
        }
    }
    return GUI_OK;
}

guiRes CDrawRegion::Clear()
{
    // this is faster than calling this->Clear( m_rScreen )
    memset ((void*) m_aPixels, 0, m_cPixels);
    return GUI_OK;
}
