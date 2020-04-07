#include <gui/common/UITypes.h>
#include <gui/screenelem/common/ScreenElem.h>

class CBitmap : public CScreenElem {
public:
	guiRes Draw(guiScreenBuffer buf);
private:
	DrawRegion* m_pDrawRegion;
};
