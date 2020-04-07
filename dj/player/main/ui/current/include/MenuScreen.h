//
// MenuScreen.h: The CMenuScreen class is an abstract base class from which more specific
//      menu screens are derived.  It provides functions for adding, displaying
//      and highlighting menu items as well as automatically handling navigation
//      through keypress messages.	
// danb@fullplaymedia.com 11/12/2000
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef MENUSCREEN_H_
#define MENUSCREEN_H_

//#include <gui/peg/peg.hpp>
#include <main/ui/ScrollingListScreen.h>

typedef struct
{
    bool                    bSelected;
    bool                    bSelectable;
    bool                    bHasSubmenu;
    WORD                    wCaptionSID;
    pGetScreenFunc          pScreen;
} MenuItemRec;

class CMenuScreen : public CScrollingListScreen
{
public:
    CMenuScreen(CScreen* pParent, WORD wScreenTitleSID, MenuItemRec* pMenuItems, int cMenuItems);
    virtual ~CMenuScreen() = 0;
    
    SIGNED Message(const PegMessage &Mesg);
    
    // Hides any visible screens.
    virtual void HideScreen();
    
    void Draw();
    
    // Sets the view mode for this screen.
    virtual void SetViewMode(CDJPlayerState::EUIViewMode eViewMode);
    
protected:
    
    // Called when the user hits the select button.
    // Acts based upon the currently highlighted menu item.
    virtual void ProcessMenuOption(int iMenuIndex) = 0;
    
    // Force a redraw of the menu.
    virtual void ForceRedraw();
    
    // Notification from the scrolling list that the list has scrolled up.
    virtual void NotifyScrollUp();
    
    // Notification from the scrolling list that the list has scrolled down.
    virtual void NotifyScrollDown();
    
    // Draws the menu, filling in the correct captions and showing the needed icons.
    void DrawMenu();
    
    // Called when the user hits the previous button.
    // Acts based upon the currently highlighted menu item.
    virtual void GotoPreviousMenu();

private:
    
    WORD _TIMER_SCROLL_TITLE();
    WORD _TIMER_SCROLL_END();
    
    static WORD s_uiInstaceCounter;
    WORD m_uiInstance;
    
    // Creates all the icons used by the menu screen.
    void BuildScreen();
    void SynchWithViewMode();

    // Scrolls the highlighted caption one letter at a time
    bool ScrollHighlightedCaption();
    
    MenuItemRec*	m_pMenuItems;
    
    PegIcon*	m_aryIcons[DISPLAY_LINES];
    PegIcon*	m_pZoomIcon;
    PegString*	m_aryCaptions[DISPLAY_LINES];
    PegString*	m_pHighlightCaptionString;
    PegString*  m_pZoomTextString;
    PegRect     m_HighlightTextRect;
    PegRect     m_ZoomTextRect;
};

#endif  // MAINMENUSCREEN_H_
