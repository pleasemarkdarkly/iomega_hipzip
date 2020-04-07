#include <gui/simplegui/show/Show.h>
#include <gui/simplegui/screen/Screen.h>
#include <devs/lcd/lcd.h>
#include <gui/simplegui/screenelem/drawregion/DrawRegion.h>

CDrawRegion CShow::s_ScreenDR(LCD_HEIGHT,LCD_WIDTH,LCD_BPP);

CShow::CShow() {
    LCDEnable();
    LCDClear();
}

CShow::~CShow() {
    m_vScreens.Clear();
}

guiRes CShow::AddScreen(CScreen* s) {
    m_vScreens.PushBack(s);
    return GUI_OK;
}

guiRes CShow::Clear() 
{
    if( !m_vScreens.IsEmpty() ) {
        for( SimpleListIterator<CScreen*> itScreen = m_vScreens.GetHead();
             itScreen != m_vScreens.GetEnd();
             ++itScreen ) {
            if ((*itScreen)->Visible()) {
                (*itScreen)->Clear(s_ScreenDR);
            }
        }
    }
    return GUI_OK;
}

guiRes CShow::Draw() {
    if( !m_vScreens.IsEmpty() ) {
        for (SimpleListIterator<CScreen*> itScreen = m_vScreens.GetHead(); itScreen != m_vScreens.GetEnd(); ++itScreen)
        {
            if ((*itScreen)->Visible())
                (*itScreen)->Draw(s_ScreenDR);
        }
    }
    return GUI_OK;
}




