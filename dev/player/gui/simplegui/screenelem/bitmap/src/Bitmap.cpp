#include <gui/simplegui/screenelem/bitmap/Bitmap.h>
#include <gui/simplegui/screenelem/drawregion/DrawRegion.h>

CBitmap::CBitmap(short w, short h, short bpp, unsigned char* data) : m_nBpp(bpp), m_aData(data), m_nHeight(h), m_nWidth(w)
{
    m_clip.ul.x = m_clip.ul.y = 0;
    m_clip.lr.x = LCD_WIDTH - 1;
    m_clip.lr.y = LCD_HEIGHT - 1;
    m_start.x = m_start.y = 0;
    this->SetDirty( true );
}

CBitmap::~CBitmap() 
{
}

guiRes CBitmap::Draw(CDrawRegion& dr)
{
    if (m_nBpp > 8)
        return GUI_BITDEPTH;

    dr.Clear( m_clip );
    
    // copy we can molest
    guiRect clip = m_clip;  
    // if the clip is before the m_start point, shrink the clip to it.
    if (clip.ul.x < m_start.x)
        clip.ul.x = m_start.x;
    if (clip.ul.y < m_start.y)
        clip.ul.y = m_start.y;
    // if the clip is beyond the lr edge of the image, shrink the clip to the bounds.
    if (clip.lr.y - m_start.y > m_nHeight - 1)
        clip.lr.y = m_start.y + m_nHeight - 1;
    if (clip.lr.x - m_start.x > m_nWidth - 1)
        clip.lr.x = m_start.x + m_nWidth - 1;
    // now the clip is guaranteed to be inside the image.  we can therefore scan based on only clip.
    short bytesPerLine = m_nWidth * m_nBpp / 8;
    if (m_nWidth * m_nBpp % 8)
        ++bytesPerLine;
    short width = clip.lr.x - clip.ul.x + 1;
    short pixPerByte = 8/m_nBpp;
    short oddPix = 0;
    unsigned char mask = 0;

    unsigned char *bitdatastart = m_aData;
    bitdatastart += (clip.ul.x - m_start.x) / pixPerByte;   // skip horizontally into img
    bitdatastart += (clip.ul.y - m_start.y) * bytesPerLine;	// skip vertically into img

    if(m_nBpp == 1) {
      // column draw

      bytesPerLine = m_nHeight * m_nBpp / 8;
      if (m_nHeight * m_nBpp % 8)
	++bytesPerLine;
   
      for( short xpos = clip.ul.x; xpos <= clip.lr.x; ++xpos) {
	   unsigned char *bitdata = bitdatastart;

	// assume padding is on the end of a column
	short ypos = clip.lr.y;

	for(int i = 0; i < m_nHeight; i++) {
	  
	  if(i > 0 && ((i % 8) == 0))
	    bitdata++;
	  
	  if((*bitdata >> (7 - (i % 8))) & 0x1)
	    dr.PutPixel(xpos,ypos,0x0);
	  else
	    dr.PutPixel(xpos,ypos,0xF);
	  ypos--;
		  
	}
	
	bitdatastart += bytesPerLine;
      }
      
      return GUI_OK;
    }


    if (m_start.x % pixPerByte)
    {   // not byte aligned drawing
        for (short ypos = clip.ul.y; ypos <= clip.lr.y; ++ypos)
        {
            short nFrom;
            short xpos = clip.ul.x;
            short pixels = width;
            unsigned char *bitdata = bitdatastart;
            nFrom = ((clip.ul.x - m_start.x) % pixPerByte);
            mask = 0xFF << (8-m_nBpp);
            mask >>= nFrom * m_nBpp;
            
            if (clip.ul.x % pixPerByte)
            {   // draw the odd left pixels
                oddPix = pixPerByte - clip.ul.x % pixPerByte;
                while (oddPix)
                {   
                    if (!mask)
                    {
                        mask = 0xFF << (8-m_nBpp);
                        ++bitdata;
                        nFrom = 0;
                    }
                    dr.PutPixel(xpos,ypos,*bitdata & mask);
                    --pixels;
                    --oddPix;
                    ++xpos;
                    mask >>= m_nBpp;
                    ++nFrom;
                }
            }
            while (pixels >= pixPerByte) // while enough src to fill a byte
            {   // draw pixels a byte at a time
                unsigned char dest = 0;
                short pix = pixPerByte;
                short nTo = 0;
                while (pix)
                {
                    if (!mask)
                    {
                        mask = 0xFF << (8-m_nBpp);
                        ++bitdata;
                        nFrom = 0;
                    }
                    if (nFrom < nTo)    // the dest pixel is further right in the byte
                        dest |= (*bitdata & mask) >> (nTo - nFrom) * m_nBpp;
                    else
                        dest |= (*bitdata & mask) << (nFrom - nTo) * m_nBpp;
                    --pix;
                    ++nTo;
                    ++nFrom;
                    mask >>= m_nBpp;
                }
                dr.PutBytes(xpos,ypos,&dest,1);
                xpos += pixPerByte;
                pixels -= pixPerByte;
            }
            if (pixels)
            {   // draw the odd right pixels
                while (pixels)
                {
                    if (!mask)
                    {
                        mask = 0xFF << (8-m_nBpp);
                        ++bitdata;
                        nFrom = 0;
                    }
                    dr.PutPixel(xpos,ypos,*bitdata & mask);
                    --pixels;
                    --oddPix;
                    ++xpos;
                    mask >>= m_nBpp;
                }
            }
            bitdatastart += bytesPerLine;
        }
    }
    else {
  
	  
      // for every row
      for (short ypos = clip.ul.y; ypos <= clip.lr.y; ++ypos)	{
	// byte aligned
	short pixels = width; // pixels in row
	short xpos = clip.ul.x; // start x position on screen
	unsigned char *bitdata = bitdatastart; // pointer to data

	  if ((oddPix = (clip.ul.x - m_start.x) % pixPerByte))
	    {  
		
	      // draw the odd left pixels
	      mask = 0xFF << (8-m_nBpp);
	      mask >>= (pixPerByte-oddPix)*m_nBpp;
	      while (oddPix)
		{   
		  dr.PutPixel(xpos,ypos,*bitdata & mask);
		  --pixels;
		  --oddPix;
		  ++xpos;
		  mask >>= m_nBpp;
		}
	      ++bitdata;
	    }
	  if (pixels > pixPerByte)
	    {
	      short bytes = pixels/pixPerByte;
	      dr.PutBytes(xpos,ypos,bitdata,bytes);
	      xpos += bytes*pixPerByte;
	      pixels -= bytes*pixPerByte;
	      bitdata += bytes;
	    }
	  if (pixels)
	    {   // draw the odd right pixels
	      mask = 0xFF << (8-m_nBpp);
	      while (pixels)
		{
		  dr.PutPixel(xpos,ypos,*bitdata & mask);
		  --pixels;
		  ++xpos;
		  mask >>= m_nBpp;
		}
	    }
	  bitdatastart += bytesPerLine;
	}

    }
    
    
    return GUI_OK;
}

