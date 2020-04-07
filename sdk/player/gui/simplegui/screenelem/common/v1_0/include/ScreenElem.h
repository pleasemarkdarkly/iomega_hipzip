#ifndef _GUI_SCREEN_ELEMENT_H_
#define _GUI_SCREEN_ELEMENT_H_

#include <gui/simplegui/common/UITypes.h>
#include <gui/simplegui/show/Show.h>

class CDrawRegion;

// screen elements are showable, but in addition they have a screen location associated with them
class CScreenElem : public CShowable {
  public:
    virtual guiRes Draw (CDrawRegion& region) = 0;
    guiPoint m_start;
    guiRect m_clip;
};

#endif // _GUI_SCREEN_ELEMENT_H_
