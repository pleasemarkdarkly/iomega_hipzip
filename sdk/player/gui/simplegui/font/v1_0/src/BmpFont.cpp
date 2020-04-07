#include <gui/simplegui/font/BmpFont.h>
#include <gui/simplegui/common/UITypes.h>
#include <gui/simplegui/screenelem/drawregion/DrawRegion.h>
#include <util/debug/debug.h>

DEBUG_MODULE(DEBUG_GUI);
DEBUG_USE_MODULE(DEBUG_GUI);

CBmpFont::CBmpFont(short h, unsigned char* data, short* offsets, unsigned char* widths, short bytes_per_line)
    : m_nHeight (h),
      m_aData (data),
      m_aOffsets (offsets),
      m_aWidths (widths),
      m_cBytesPerLine (bytes_per_line) 
{
}

short CBmpFont::DrawChar(char c, CDrawRegion& dr, guiRect& clip, guiPoint& start, guiColor& color)
{
    unsigned char *bitdata;
    unsigned char *bitdatastart;
    short xpos = start.x;
    short width = 0;
    if (m_aWidths)
        width = m_aWidths[c];
    else
        width = m_aOffsets[c+1] - m_aOffsets[c];
    short offset = m_aOffsets[c];	// the pixel offset on the first line of the bitmap, where the character starts.

    if (clip.lr.y - clip.ul.y > m_nHeight - 1)
        clip.lr.y = clip.ul.y + m_nHeight - 1;
    if (xpos + width > clip.lr.x)
        width = clip.lr.x - xpos + 1;
	
    short byteoff = offset / 8;
    bitdatastart = m_aData + byteoff;
    bitdatastart += (clip.ul.y - start.y) * m_cBytesPerLine;	// start on a lower line, if the char is to be drawn starting above clip.
	
    for (short ypos = clip.ul.y; ypos <= clip.lr.y; ypos++)
    {
        bitdata = bitdatastart;
        unsigned char mask = 0x80 >> (offset & 7);
        short bits = 0;
        unsigned char byte = *bitdata++;

        while(bits < width)
        {
            if (!mask)
            {
                mask = 0x80;
                byte = *bitdata++;
            }
            if (xpos >= clip.ul.x)
                if (byte & mask)
                    dr.PutPixel(xpos, ypos, color);
            mask >>= 1;
            xpos++;
            bits++;
            if (xpos > clip.lr.x)
                break;
        }
        bitdatastart += m_cBytesPerLine;
        xpos -= width;
    }
	
    return width;
}
