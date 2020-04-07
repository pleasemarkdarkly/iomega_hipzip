#include <gui/simplegui/screenelem/textlabel/TextLabel.h>
#include <gui/simplegui/screenelem/drawregion/DrawRegion.h>
#include <gui/simplegui/font/BmpFont.h>

#include <string.h>  // strcpy, strlen

#ifndef NULL
#define NULL 0
#endif

CTextLabel::CTextLabel() : m_pText(0), m_pFont(0)
{
    m_color = 0xFF >> (8-LCD_BPP);
    m_pText = NULL;
}
 
CTextLabel::~CTextLabel()
{
    if( m_pText ) {
        delete [] m_pText;
    }
}

guiRes CTextLabel::Draw(CDrawRegion& dr) {
    char* p = m_pText;
    short x = m_start.x;

    dr.Clear( m_clip );
    
    while (*p && x < m_clip.lr.x) {
        m_start.x += m_pFont->DrawChar(*p, dr, m_clip, m_start,m_color);
        ++p;
    }

    m_start.x = x;

    this->SetDirty( false );
    
    return GUI_OK;
}

guiRes CTextLabel::SetFont(CBmpFont* pFont)
{
    if( m_pFont != pFont ) {
        m_pFont = pFont;
        if( m_pText ) {
            this->SetDirty( true );
        }
    }
    return GUI_OK;
}

guiRes CTextLabel::SetText(const char* s)
{
    delete m_pText;
    m_pText = NULL;
    
    m_pText = new char[strlen(s) + 1];
    if (!m_pText)
        return GUI_MEMORY;
    strcpy (m_pText,s);

    this->SetDirty( true );
    
    return GUI_OK;
}
