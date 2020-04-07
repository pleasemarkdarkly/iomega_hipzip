#include <gui/screen/Screen.h>
#include <gui/show/Show.h>
#include <util/datastructures/SimpleList.h>

CScreen::CScreen() {
}

CScreen::~CScreen() {
	m_vScreenElems.Clear();
}

guiRes CScreen::Draw(CDrawRegion dr) {
	if (m_vScreenElems.IsEmpty())
		return GUI_OK;

	dr.Clear();
    for (SimpleListIterator<CScreenElem*> itElem = m_vScreenElems.GetHead(); itElem != m_vScreenElems.GetEnd(); ++itElem)
	{
		if ((*itElem)->Visible())
			(*itElem)->Draw(dr);
	}
	dr.Draw(dr);
	return GUI_OK;
}

guiRes CScreen::AddScreenElem(CScreenElem* s) {
	m_vScreenElems.PushBack(s);
	return GUI_OK;
}

	