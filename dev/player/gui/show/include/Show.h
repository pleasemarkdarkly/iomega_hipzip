#ifndef _GUI_SHOW_H_
#define _GUI_SHOW_H_

#include <gui/common/UITypes.h>
#include <util/datastructures/SimpleList.h>

class CScreen;
class CDrawRegion;
// a showable is a piece of a show; a show is a collection of showables and some logic to draw
// them all.  subclasses of showables can be bitmaps, text labels, and draw regions?

// showables have a draw fn that takes no parms, since the screen object is showable but doesn't
// need an external sense of drawregion... this is the second time I've been burned b/c the screen
// is showable. er.

class CShowable {
public:
	CShowable() { }
	virtual ~CShowable() { }
	
	void SetVisible(Bool bVis) { m_bVisible = bVis; }
	Bool Visible() { return m_bVisible; }

	virtual guiRes Draw(CDrawRegion dr) = 0;

	Bool m_bVisible;
	Bool m_bDirty;
};


class CShow {
public:
	CShow();
	~CShow();

	static CDrawRegion& GetScreenDR() { return s_ScreenDR; };

	guiRes AddScreen(CScreen* s);

	guiRes Draw();
private:
	SimpleList<CScreen*> m_vScreens;
	static CDrawRegion s_ScreenDR;
};

#endif // _GUI_SHOW_H_