#include <gui/screenelem/textlabel/TextLabel.h>
#include <gui/screenelem/drawregion/DrawRegion.h>
#include <gui/font/BmpFont.h>

CTextLabel::CTextLabel() : m_pText(0), m_pFont(0) {
	m_color = 0xFF >> (8-LCD_BPP);
}
 
CTextLabel::~CTextLabel() {

}

guiRes CTextLabel::Draw(CDrawRegion dr) {
	char* p = m_pText;
	short x = m_start.x;
	while (*p && x < m_clip.lr.x) {
		m_start.x += m_pFont->DrawChar(*p, dr, m_clip, m_start,m_color);
		++p;
	}
	m_start.x = x;
	return GUI_OK;
}

guiRes CTextLabel::SetFont(CBmpFont* pFont)
{
	m_pFont = pFont;
	return GUI_OK;
}

guiRes CTextLabel::SetText(const char* s) {
	delete m_pText;
	m_pText = new char[strlen(s) + 1];
	if (!m_pText)
		return GUI_MEMORY;
	strcpy (m_pText,s);
	return GUI_OK;
}
