//........................................................................................
//........................................................................................
//.. File Name: BrowseMenuScreen.h														..
//.. Date: 09/20/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CBrowseMenuScreen class		 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/20/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef BROWSEMENUSCREEN_H_
#define BROWSEMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#include <main/ui/DynamicMenuScreen.h>
#include <content/common/QueryableContentManager.h> // ContentKeyValueVector
#include <core/playmanager/PlayManager.h>
#include <main/util/datastructures/SortList.h>

struct tMediaRecordMetadataMapping;

class CBrowseMenuScreen : public CDynamicMenuScreen
{
public:
	typedef enum eBrowseMode { ARTIST = 0, ALBUM, TRACK };
	// this is a singleton class
	static CScreen* GetBrowseMenuScreen();
    static void Destroy() {
		if (s_pBrowseMenuScreen)
			delete s_pBrowseMenuScreen;
		s_pBrowseMenuScreen = 0;
    }
	// allow other screens to configure this screens browse mode
	void SetBrowseMode(eBrowseMode eMode, bool bMenuEnterMode = true);

    void SetConstraints(int iGenreKey = CMK_ALL, int iArtistKey = CMK_ALL, int iAlbumKey = CMK_ALL);

    void Minimize();
protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex);

	// Called when the user hits the previous button.
	// Acts based upon the currently highlighted menu item.
	void GotoPreviousMenu();

    // The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
	bool MenuItemHasSubMenu(int iMenuIndex);
	const TCHAR* MenuItemCaption(int iMenuIndex);
	PegBitmap* MenuItemBitmap(int iMenuIndex);
    

private:
    typedef SortList<IMediaContentRecord*> ContentRecordSortList;
    typedef SimpleListIterator<IMediaContentRecord*> ContentRecordIterator;

    void SortMediaRecordListByTitle(ContentRecordSortList* mrlTracks);
    void InitMappingToListOrder(tMediaRecordMetadataMapping* map, ContentRecordSortList* mrlTracks);
    void PopulateMapWithTitleMetadata(tMediaRecordMetadataMapping* map, int nRecords);
    void SortMap(tMediaRecordMetadataMapping* map, int nRecords);
    void ReorderListByMap(tMediaRecordMetadataMapping* map, ContentRecordSortList* mrlTracks);

    int m_iArtistKey;
    int m_iAlbumKey;
    int m_iGenreKey;

    ContentKeyValueVector m_MenuItems;
    MediaRecordIterator m_itTrack;
    int m_iIterIndex;
    ContentRecordSortList m_mrlTracks;

  	int		m_iArtistTopIndex;	// The index of the item at the top of the screen.
	int		m_iArtistLineIndex;	// The line number of the item currently highlighted.

	int		m_iAlbumTopIndex;	// The index of the item at the top of the screen.
	int		m_iAlbumLineIndex;	// The line number of the item currently highlighted.

    CBrowseMenuScreen();
	~CBrowseMenuScreen();

	eBrowseMode m_eBrowseMode;
	eBrowseMode m_eMenuEnterMode;	// this tells us what mode we originally entered this browse screen

    static CBrowseMenuScreen* s_pBrowseMenuScreen;
};

#endif  // BROWSEMENUSCREEN_H_
