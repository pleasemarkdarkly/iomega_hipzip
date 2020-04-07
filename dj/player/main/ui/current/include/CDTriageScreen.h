//
// CDTriageScreen.h: contains the definition of the CCDTriageScreen class
// danb@fullplaymedia.com 12/04/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef CDTRIAGESCREEN_H_
#define CDTRIAGESCREEN_H_

#include <gui/peg/peg.hpp>
#include <main/ui/ScrollingListScreen.h>
#include <main/main/DJPlayerState.h>
#include <core/playmanager/PlayManager.h>
#include <main/ui/PlayerScreen.h>

typedef struct cd_multiple_hit_event_s cd_multiple_hit_event_t;
class IContentManager;
class IDiskInfo;

class CCDTriageScreen : public CScrollingListScreen
{
public:
	static CCDTriageScreen* GetInstance();
    static void Destroy() {
		if (s_pCDTriageScreen)
			delete s_pCDTriageScreen;
		s_pCDTriageScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);
    void Draw();
    void HideScreen();

    // Called when a CD is inserted.
    // Resets the state of the selection screen.
    void NotifyCDInserted();

    // Called when a CD is ejected.
    // Makes sure any multiple metadata messages that arrive asynchronously are not displayed.
    void NotifyCDRemoved();

    void DeleteMetadataList();
    bool ProcessMetadataList(cd_multiple_hit_event_t* pList = NULL);

private:
	CCDTriageScreen(CScreen* pParent);
	virtual ~CCDTriageScreen();

	void BuildScreen();

    void DeleteMultipleHitEvent(cd_multiple_hit_event_t* pList);

	void SetTitleText(const TCHAR* szText);
	void SetArtistText(const TCHAR* szText);

	void ProcessMenuOption(int iMenuIndex);
    void GotoSubMenu(int iMenuIndex) {};
    void GotoPreviousMenu() {};
	void ForceRedraw();

	// Checks to see if any of the strings need scrolling and configures the peg alarms to do so.
	void SynchTextScrolling();

	// This function will scroll all text fields in unison one letter.
	// Returns true if some scrolling happend.
	bool ScrollTextFields();

	// Scrolls from menu items
	void ScrollMenu(int iMenuIndex);

    // Called when the user chooses a disk.
    // Adds that disk's metadata to the system.
    void ProcessSelection(IDiskInfo* pDiskInfo);

	PegString*  m_pTitleTextString;
	PegString*  m_pArtistTextString;
	PegString*  m_pInstructionsTextString;
    PegRect     m_TitleTextRect;
    PegRect     m_ArtistTextRect;
    PegIcon*    m_pBottomScreenHorizontalDottedBarIcon;

    cd_multiple_hit_event_t* m_pHitList;

    CDJPlayerState*         m_pDJPlayerState;
    IContentManager*        m_pContentManager;
    CPlayManager*           m_pPlayManager;
    CPlayerScreen*          m_pPlayerScreen;

    bool m_bSelectionMade;  // True if the user has already chosen metadata for the current CD.
    bool m_bCDInserted;

	static CCDTriageScreen* s_pCDTriageScreen;
};

#endif  // CDTRIAGESCREEN_H_
