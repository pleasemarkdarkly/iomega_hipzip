#ifndef SHOW_H_
#define SHOW_H_

#include <gui/simplegui/common/UITypes.h>
#include <util/datastructures/SimpleList.h>

class CScreen;
class CDrawRegion;
// A showable is a piece of a show; a show is a collection of showables and some logic to draw
// them all.  Subclasses of showables can be bitmaps, text labels, and draw regions.

// Showables have a draw fn that takes no parms, since the screen object is showable but doesn't
// need an external sense of drawregion.

class CShowable {
public:
    CShowable() { }
    virtual ~CShowable() { }
	
    void SetVisible(bool bVis) { m_bVisible = bVis; }
    bool Visible() { return m_bVisible; }
    
    void SetDirty(bool bDirty) { m_bDirty = bDirty; }
    bool Dirty()   { return m_bDirty;   }

    virtual guiRes Draw(CDrawRegion& dr) = 0;

    bool m_bVisible;
    bool m_bDirty;
};


class CShow {
public:
    CShow();
    ~CShow();

    static CDrawRegion& GetScreenDR() { return s_ScreenDR; };

    guiRes AddScreen(CScreen* s);

    guiRes Clear();
    guiRes Draw();
private:
    SimpleList<CScreen*> m_vScreens;
    static CDrawRegion s_ScreenDR;
};

#endif // SHOW_H_
