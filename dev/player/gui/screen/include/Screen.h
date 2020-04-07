#include <gui/common/UITypes.h>
#include <gui/screenelem/drawregion/DrawRegion.h>

class CShowable;

class CScreen : public CShowable {
public:
	CScreen();
	~CScreen();

	guiRes Draw(CDrawRegion dr);
	guiRes AddScreenElem(CScreenElem* s);
private:
	SimpleList<CScreenElem*> m_vScreenElems;
};
