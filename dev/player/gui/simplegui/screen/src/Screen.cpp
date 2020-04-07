#include <gui/simplegui/screen/Screen.h>
#include <gui/simplegui/show/Show.h>
#include <util/datastructures/SimpleList.h>

CScreen::CScreen()
{
}

CScreen::~CScreen()
{
    m_vScreenElems.Clear();
}

guiRes CScreen::Clear(CDrawRegion& dr) 
{
    dr.Clear();
    return GUI_OK;
}

guiRes CScreen::Draw(CDrawRegion& dr) {
    if (m_vScreenElems.IsEmpty())
        return GUI_OK;

    //dr.Clear();
    
    for (SimpleListIterator<CScreenElem*> itElem = m_vScreenElems.GetHead(); itElem != m_vScreenElems.GetEnd(); ++itElem)
    {
        // dc- items are responsible for clearing their dirty flag
        //     if they elect not to they will be redrawn every time
        if ((*itElem)->Visible() && (*itElem)->Dirty() )
            (*itElem)->Draw(dr);
    }
    
    dr.Draw(dr);
    
    return GUI_OK;
}

guiRes CScreen::AddScreenElem(CScreenElem* s) {
    m_vScreenElems.PushBack(s);
    return GUI_OK;
}

