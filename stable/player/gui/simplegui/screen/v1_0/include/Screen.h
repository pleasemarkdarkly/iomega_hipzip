#include <gui/simplegui/common/UITypes.h>
#include <gui/simplegui/screenelem/drawregion/DrawRegion.h>

class CShowable;

class CScreen : public CShowable {
  public:
    CScreen();
    ~CScreen();

    guiRes Clear(CDrawRegion& dr);
    guiRes Draw(CDrawRegion& dr);
    guiRes AddScreenElem(CScreenElem* s);
  private:
    SimpleList<CScreenElem*> m_vScreenElems;
};
