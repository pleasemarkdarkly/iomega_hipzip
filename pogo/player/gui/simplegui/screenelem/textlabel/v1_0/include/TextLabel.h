#include <gui/simplegui/common/UITypes.h>
#include <gui/simplegui/screenelem/common/ScreenElem.h>

class CBmpFont;
class CDrawRegion;

class CTextLabel : public CScreenElem {
  public:
    CTextLabel();
    ~CTextLabel();

    guiRes SetText(const char* s);
    const char* GetText() { return m_pText; }

    guiRes SetFont(CBmpFont* pFont);

    guiRes Draw(CDrawRegion& dr);
  private:
    char* m_pText;
    CBmpFont* m_pFont;
    guiColor m_color;
};
