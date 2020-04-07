//
// LibraryMenuScreen.h: definition of CLibraryMenuScreen class
// danb@fullplaymedia.com 02/07/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef LIBRARYMENUSCREEN_H_
#define LIBRARYMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>
#include <content/common/QueryableContentManager.h> // ContentKeyValueVector
#include <core/playmanager/PlayManager.h>
#include <main/main/DJPlayerState.h>
#include <util/datastructures/SimpleList.h>

#ifndef NO_UPNP
#include <main/iml/query/QueryResult.h>
#endif // NO_UPNP

typedef struct content_delete_info_s content_delete_info_t;

class CLibraryMenuScreen : public CDynamicMenuScreen
{
public:
    typedef enum eBrowseMode { GENRE = 0, ARTIST, ALBUM, TRACK, PLAYLIST, RADIO };

    // this is a singleton class
	static CScreen* GetLibraryMenuScreen();
    static void Destroy() {
		if (s_pLibraryMenuScreen)
			delete s_pLibraryMenuScreen;
		s_pLibraryMenuScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

	// Hides any visible screens.
	virtual void HideScreen();

    // allow other screens to configure this screens browse mode
	void SetBrowseMode(eBrowseMode eMode, bool bEntryScreen = false);
	eBrowseMode GetBrowseMode() { return m_eBrowseMode; }

    // allow other screens to configure this screens browse source
    void SetBrowseSource(CDJPlayerState::ESourceMode eSource, bool bAlertUserOfChange = false);
    CDJPlayerState::ESourceMode GetBrowseSource() { return m_eBrowseSource; }

    // allow other screens to configure this screens browse source
    void SetBrowseIML(CIML* pIML);
    CIML* GetBrowseIML() { return m_pBrowseIML; }

    void SetConstraints(int iGenreKey = CMK_ALL, int iArtistKey = CMK_ALL, int iAlbumKey = CMK_ALL);

    // Notify us that the current data source was lost
    void NotifyLostCurrentSource();

    // Returns the number of items left to query.
    int GetQueryRemainingCount() const
        { return m_iQueryItemsLeft; }

    // Cancel any pending media item append queries and drop the results on the floor
    // If bShowAlert is true then then an alert screen will pop up to tell the user what happened.
    void CancelPendingQueries(bool bShowAlert = false);

    // Stop deleting content.
    void CancelContentDeletion();

    // Tell the screen to reset and draw
    void ResetAndRefresh();

    // Tell the screen to synch with editing changes
    void ResynchWithChanges();

protected:

    // Show the InfoMenuScreen with a selected items info
    void ShowMenuItemInfo(int iMenuIndex);

    // delete the current item in focus
    void DeleteItemInFocus();

    // callback to delete the current item in focus
    void DeleteItemInFocusCB(bool bDelete);
    static void DeleteItemInFocusCallback(bool bDelete);
    void RemoveMediaRecords(MediaRecordList& mrl, int iCurrentIndex);
    void RemoveMediaRecord(IMediaContentRecord* pRecord);
    bool RemoveMediaRecords(content_delete_info_t* pDeleteInfo);
    bool StartRemoveMediaRecords(int iGenreKey = CMK_ALL, int iArtistKey = CMK_ALL, int iAlbumKey = CMK_ALL);

	// edit the current item in focus
	void EditItemInFocus();

	// callback to edit the current item in focus
	void EditItemInFocusCB(bool bSave);
	static void EditItemInFocusCallback(bool bSave);

    // callback that the alert screen calls
    // we've lost the current source, so this callback will
    // hide this screen and put the user in the source menu screen
    void LostCurrentSourceCB();
    static void LostCurrentSourceCallback();

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Notification from the scrolling list that the list has scrolled up.
	void NotifyScrollUp();

	// Notification from the scrolling list that the list has scrolled down.
	void NotifyScrollDown();

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
	const TCHAR* MenuTitleCaption();
    
    // Called when there are no items in the menu.   
    const TCHAR* EmptyMenuCaption();

private:

#ifndef NO_UPNP
    CIML*                       m_pBrowseIML;
    CIMLManager*                m_pIMLManager;
    CGeneralQueryResult*        m_pCurrentQR;
    CMediaQueryResult*          m_pMediaItemListQR;
    CMediaQueryResult*          m_pMediaItemAppendQR;
    IMLMediaInfoVector          m_miTracks;
    CRadioStationQueryResult*   m_pRadioStationQR;
    IMLRadioStationInfoVector   m_miRadioStations;

    bool                        m_bDropQueryResponses;
    bool                        m_bDropMediaItemListQueryResponses;
    bool                        m_bDropMediaItemAppendQueryResponses;
    bool                        m_bDropRadioStationQueryResponses;
    
    int                         m_nMediaItemListRetries;
    int                         m_nMediaItemAppendRetries;
    int                         m_nGeneralQueryRetries;
    int                         m_nRadioQueryRetries;

    static bool QueryFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData);
    bool QueryFunc(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount);

    static bool MediaItemListQueryFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData);
    bool MediaItemListQueryFunc(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount);

    static bool MediaItemAppendQueryFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData);
    static bool MediaItemSelectQueryFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData);
    bool MediaItemAppendQueryFunc(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, bool bAppend);
    bool ProcessMediaItemResults(int iStartIndex, int iItemCount, bool bAppend);

    static bool RadioStationQueryFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData);
    bool RadioStationQueryFunc(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount);

    int  m_iHoleStart;
    int  m_iHoleEnd;
    void SetHoleSize(CGeneralQueryResult* pQR);
    void SetHoleSize(CMediaQueryResult* pQR);
    void SetHoleSize(CRadioStationQueryResult* pQR);
    int GetHoleSize()
        { return m_iHoleEnd - m_iHoleStart; }
    const TCHAR* BuildQueryingString(const TCHAR* tcPrefix);

    int  m_iCurrentQueryID;

    int GetStartOfNextQueryBlock(CGeneralQueryResult* pQR, int iBlockSize);
    int GetStartOfNextQueryBlock(CMediaQueryResult* pQR, int iBlockSize);
    int GetStartOfNextQueryBlock(CRadioStationQueryResult* pQR, int iBlockSize);

    int AdjustIndex(int iIndex);
    
    int m_iPreviousHoleSize;
    int m_iPreviousHoleStart;
    
#endif // NO_UPNP

    int     m_iQueryItemsLeft;

    bool    m_bLoadingLocalPlaylist;
    bool    m_bCancelLocalPlaylist;
    bool    m_bAppendLocalPlaylist;

    bool    m_bCancelContentDeletion;

    // return true if the screen data needs to be updated to match constraints.
    bool ConstraintsHaveChanged(int iGenreKey,int iArtistKey,int iAlbumKey, CDJPlayerState::ESourceMode source, eBrowseMode mode);

    // A helper function to save a string locally.
    // Used for saving the menu titles when drilling into submenus
    // If dst isn't null, it deletes the data it's pointing to 
    TCHAR* SaveString(TCHAR* dst, const TCHAR* src);

    // navigation accelerators that jump down/up to the next/previous starting letter
    void JumpToNextLetter();
    void JumpToPreviousLetter();
    
    // return the first letter of the menu item at index nIndex, taking query-holes into account
    TCHAR FirstLetterOfMenuItemWithHoles(int nIndex);
    // return the first letter of the track title at index nIndex, taking query-holes into account
    TCHAR FirstLetterOfTrackTitleWithHoles(int nIndex);
    // return the first letter of the radio station name at index nIndex, taking query-holes into account
    TCHAR FirstLetterOfRadioNameWithHoles(int nIndex);

    int m_iGenreKey;
    int m_iArtistKey;
    int m_iAlbumKey;

	TCHAR* m_pszGenre;
    TCHAR* m_pszArtist;
    TCHAR* m_pszAlbum;

	bool m_bAppend;
    bool m_bInvalidLibrary;

    ContentKeyValueVector m_MenuItems;
	PlaylistRecordList m_prlPlaylistMenuItems;
    int m_iIterIndex;
    MediaRecordIterator m_itTrack;
    MediaRecordList m_mrlTracks;
    MediaRecordIterator m_itRadioStation;
    MediaRecordList m_mrlRadioStations;

    // Used to cache top-level content queries
    ContentKeyValueVector m_vGenreCache;
    ContentKeyValueVector m_vArtistCache;
    ContentKeyValueVector m_vAlbumCache;
    MediaRecordList m_mrlTrackCache;
    MediaRecordList m_mrlRadioStationCache;

    int     m_iCurrentSourceIndex;

  	int	m_iGenreTopIndex;	    // The index of the item at the top of the screen.
  	int	m_iArtistTopIndex;	    // The index of the item at the top of the screen.
	int m_iAlbumTopIndex;	    // The index of the item at the top of the screen.
    int m_iTrackTopIndex;       // The index of the item at the top of the screen.
    int m_iPlaylistTopIndex;    // The index of the item at the top of the screen.
    int m_iRadioTopIndex;       // The index of the item at the top of the screen.
    int m_iDesiredIndex;        // The index of the highlighted item from before.
    
    CLibraryMenuScreen();
	~CLibraryMenuScreen();

    eBrowseMode m_eEntryScreen;         // what mode did we enter this screen in?  (so we can exit correctly)
	eBrowseMode m_eBrowseMode;
    CDJPlayerState::ESourceMode m_eBrowseSource;
    CDJPlayerState* m_pDJPlayerState;  // what is the current source mode?

    static CLibraryMenuScreen* s_pLibraryMenuScreen;

};

#endif  // LIBRARYMENUSCREEN_H_
