//
// InfoMenuScreen.h: the menu screen that shows all info available for a specific object
// danb@fullplaymedia.com 04/15/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef INFOMENUSCREEN_H_
#define INFOMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>
#include <main/main/DJPlayerState.h>
#include <util/datastructures/SimpleVector.h>
#include <content/common/ContentManager.h>

typedef struct {
    int    iAttributeID;
    TCHAR* pszCaption;
    TCHAR* pszCaptionTitle;
    TCHAR* pszCaptionValue;
    TCHAR* pszSavedValue;
    bool   bEditable;
    bool   bChanged;
    bool   bNumeric;
} CaptionItem_t;

typedef SimpleVector<CaptionItem_t> CaptionItemVector;

class CInfoMenuScreen : public CDynamicMenuScreen
{
public:

    static CInfoMenuScreen* GetInfoMenuScreen();

    static void Destroy() {
		if (s_pInfoMenuScreen)
			delete s_pInfoMenuScreen;
		s_pInfoMenuScreen = 0;
    }

    SIGNED Message(const PegMessage &Mesg);

    // Give a content record for this screen to extract the track info from
    void SetTrackInfo(IMediaContentRecord* pContentRecord);

    // Give an Album Media Record Key and extract all info for this album from
    //  the IQueryableContentManager
    void SetAlbumInfo(int iAlbumKey, CDJPlayerState::ESourceMode eSource);

    // Give an Artist Media Record Key and extract all info for this artist from
    //  the IQueryableContentManager
    void SetArtistInfo(int iArtistKey, CDJPlayerState::ESourceMode eSource);

    // Give an Genre Media Record Key and extract all info for this genre from
    //  the IQueryableContentManager
    void SetGenreInfo(int iGenreKey, CDJPlayerState::ESourceMode eSource);
    
protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex) {};

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex) {};

	// The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
	bool MenuItemHasSubMenu(int iMenuIndex);
	const TCHAR* MenuItemCaption(int iMenuIndex);

    void ClearCaptions();
    // Adds a caption to the menu list in order of being
    void AddCaptionItem(int iAttributeID, const TCHAR* pszCaptionTitle, const TCHAR* pszCaptionValue, bool bEditable, bool bNumeric=false);
    void AddCaptionItem(int iAttributeID, const char* pszCaptionTitle, const TCHAR* pszCaptionValue, bool bEditable, bool bNumeric=false);
    void AddCaptionItem(int iAttributeID, const TCHAR* pszCaptionTitle, const char* pszCaptionValue, bool bEditable, bool bNumeric=false);
    void AddCaptionItem(int iAttributeID, const char* pszCaptionTitle, const char* pszCaptionValue, bool bEditable, bool bNumeric=false);
    void AddCaptionItem(int iAttributeID, const TCHAR* pszCaptionTitle, const int iValue, bool bEditable, bool bNumeric=true);
    void AddCaptionItem(int iAttributeID, const char* pszCaptionTitle, const int iValue, bool bEditable, bool bNumeric=true);

	// edit the current item in focus
	void EditItemInFocus();

	// callback to edit the current item in focus
	void EditItemInFocusCB(bool bSave);
	static void EditItemInFocusCallback(bool bSave);

    void CommitChanges();
private:

	CInfoMenuScreen(CScreen* pParent = NULL);
	~CInfoMenuScreen();

	// A pointer to the global instance of this class.
	static CInfoMenuScreen* s_pInfoMenuScreen;	

	CaptionItemVector m_vCaptions;

    MediaRecordIterator m_itTrack;
    MediaRecordList     m_mrlTracks;

    ContentKeyValueVector m_vGenres;
    ContentKeyValueVector m_vArtists;

    bool m_bChanged;
    CDJPlayerState::ESourceMode m_eSource;
};

#endif  // INFOMENUSCREEN_H_
