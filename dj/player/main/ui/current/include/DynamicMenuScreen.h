//
// DynamicMenuScreen.h: The CDynamicMenuScreen class is an abstract base class from which more specific
//      menu screens are derived.  It provides functions for adding, displaying
//      and highlighting menu items as well as automatically handling navigation
//      through keypress messages.	
// danb@fullplaymedia.com 09/11/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef DYNAMICMENUSCREEN_H_
#define DYNAMICMENUSCREEN_H_

#include <main/ui/ScrollingListScreen.h>

class CDynamicMenuScreen : public CScrollingListScreen
{
public:
    CDynamicMenuScreen(pGetScreenFunc pGetChildFunc, WORD wScreenTitleSID);
    virtual ~CDynamicMenuScreen() = 0;
    
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
    
    // Called when the user hits the next button.
    // Acts based upon the currently highlighted menu item.
    virtual void GotoSubMenu(int iMenuIndex);
    
    // Called when the user hits the previous button.
    // Acts based upon the currently highlighted menu item.
    virtual void GotoPreviousMenu();
    
    // The functions help get info about the menu items so we can draw them correctly
    // These functions must be defined by the derived class
    virtual bool MenuItemHasSubMenu(int iMenuIndex);
    virtual bool MenuItemSelected(int iMenuIndex);
    virtual bool MenuItemSelectable(int iMenuIndex);
    virtual const TCHAR* MenuItemCaption(int iMenuIndex);
    virtual const TCHAR* MenuTitleCaption();
    virtual const TCHAR* EmptyMenuCaption();
    
    // Force a redraw of the menu.
    virtual void ForceRedraw();
    
    // Notification from the scrolling list that the list has scrolled up.
    virtual void NotifyScrollUp();
    
    // Notification from the scrolling list that the list has scrolled down.
    virtual void NotifyScrollDown();
    
    // Draws the menu, filling in the correct captions and showing the needed icons.
    void DrawMenu();
    
    // A pointer to a child screen's singlton function
    pGetScreenFunc m_pGetChildScreenFunc;
    
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
    
    PegIcon*	m_aryIcons[DISPLAY_LINES];
    PegIcon*	m_pZoomIcon;
    PegString*	m_aryCaptions[DISPLAY_LINES];
    PegString*	m_pHighlightCaptionString;
    PegString*  m_pZoomTextString;
    PegRect     m_HighlightTextRect;
    PegRect     m_ZoomTextRect;
    
    TCHAR*	m_pszMenuTitle;
};

#endif  // MAINDYNAMICMENUSCREEN_H_
