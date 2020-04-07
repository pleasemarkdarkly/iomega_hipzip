//........................................................................................
//........................................................................................
//.. File Name: Screen.h																..
//.. Date: 08/22/2001																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: contains the implementation of COnScreenMenu class				..
//.. Usage:The COnScreenMenu class is an abstract base class from which more					..
//..	   specific screens are derived.  It contains info that every screen			..
//..	   needs, like screen title, menu caption, and functions to hide and show		..
//.. Last Modified By: Dan Bolstad	danb@iobjects.com									..	
//.. Modification date: 08/22/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <main/demos/ssi_neo/ui/OnScreenMenu.h>
#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/Fonts.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/UI.h>

#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/MediaPlayer.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_ONSCREEN_MENU, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_ONSCREEN_MENU );

#define TIMER_MENU_TIME_OUT				201
const int sc_iMenuTimeOutInterval = 100;

COnScreenMenu::COnScreenMenu(CScreen* pParent)
  : CScreen(pParent)
{
	BuildScreen();
}

COnScreenMenu::~COnScreenMenu()
{
//	Destroy(m_pCaption);
//	Destroy(m_pBackArrow);
//	Destroy(m_pForwardArrow);
}


// Shows screens.
void
COnScreenMenu::ShowScreen()
{
	m_pParent->Add(this);
	Presentation()->MoveFocusTree(this);

    m_iPlaylistCount = CPlayManager::GetInstance()->GetPlaylist()->GetSize();
    m_iPlaylistEntryIndex = CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry()->GetIndex();
    void* pdata;
    CPlayManager::GetInstance()->GetPlaylist()->GetEntry(m_iPlaylistEntryIndex)->GetContentRecord()->GetAttribute(MDA_TITLE,&pdata);
    m_pCaption->DataSet((TCHAR*)pdata);
        
	Draw();
	SetTimer(TIMER_MENU_TIME_OUT, sc_iMenuTimeOutInterval, 0);
}

// Shows screens.
void
COnScreenMenu::HideScreen()
{
	KillTimer(TIMER_MENU_TIME_OUT);
	CScreen::HideScreen();
}


void
COnScreenMenu::Draw()
{
	// center the caption string
	int iTextLength = Screen()->TextWidth(m_pCaption->DataGet(), &FONT_MENUSCREENENTRY);
	int iCaptionLength = m_CaptionRect.wRight - m_CaptionRect.wLeft;
	PegRect newRect;
	newRect.Set(((iCaptionLength - iTextLength) / 2) + 5, m_CaptionRect.wTop, m_CaptionRect.wRight, m_CaptionRect.wBottom);
	m_pCaption->Resize(newRect);

	BeginDraw();
	CScreen::Draw();
	EndDraw();
}

SIGNED
COnScreenMenu::Message(const PegMessage &Mesg)
{
	static WORD s_TestStringID = SID_NORMAL;

	switch (Mesg.wType)
	{
		case PM_KEY:

			KillTimer(TIMER_MENU_TIME_OUT);
			SetTimer(TIMER_MENU_TIME_OUT, sc_iMenuTimeOutInterval, 0);

			switch (Mesg.iData)
			{
				case KEY_NEXT:
                {
                    // go to the next item in the list
                    m_iPlaylistEntryIndex = (m_iPlaylistEntryIndex + 1) % m_iPlaylistCount;
                    void* pdata;
                    CPlayManager::GetInstance()->GetPlaylist()->GetEntry(m_iPlaylistEntryIndex)->GetContentRecord()->GetAttribute(MDA_TITLE,&pdata);
                    m_pCaption->DataSet((TCHAR*)pdata);
                    //m_pCaption->DataSet(CPlayManager::GetInstance()->GetPlaylist()->GetEntry(m_iPlaylistEntryIndex)->GetContentRecord()->GetTitle());
                    /*
                    if(s_TestStringID < SID_TRACKS)
						m_pCaption->DataSet(LS(s_TestStringID++));
                    */
					Draw();
					return 0;
                }
				case KEY_PREVIOUS:
					// go to the previous item in the list
                    if (m_iPlaylistEntryIndex > 0)
                        --m_iPlaylistEntryIndex;
                    else
                        m_iPlaylistEntryIndex = m_iPlaylistCount - 1;
                    void* pdata;
                    CPlayManager::GetInstance()->GetPlaylist()->GetEntry(m_iPlaylistEntryIndex)->GetContentRecord()->GetAttribute(MDA_TITLE,&pdata);
                    m_pCaption->DataSet((TCHAR*)pdata);
                    //m_pCaption->DataSet(CPlayManager::GetInstance()->GetPlaylist()->GetEntry(m_iPlaylistEntryIndex)->GetContentRecord()->GetTitle());
                    /*
					if(s_TestStringID > SID_ENGLISH)
						m_pCaption->DataSet(LS(s_TestStringID--));
                    */
					Draw();
					return 0;

				case KEY_PLAY_PAUSE:
                {
					// this should play the current track
                    IPlaylist* pl = CPlayManager::GetInstance()->GetPlaylist();
                    IPlaylistEntry* current = pl->GetCurrentEntry();
                    IPlaylistEntry* target = pl->GetEntry(m_iPlaylistEntryIndex);
                    CMediaPlayer* mp = CMediaPlayer::GetInstance();
                    ERESULT res = PM_ERROR;
                    CMediaPlayer::PlayState ps = mp->GetPlayState();
			        if (pl->SetCurrentEntry(target))
			        {
                        if (SUCCEEDED(res = mp->SetSong(target)))
				        {
                            if ((ps == CMediaPlayer::PLAYING) && (mp->GetPlayState() != CMediaPlayer::PLAYING))
						        mp->Play();
                            return res;
				        }

                  		if (FAILED(res))
                        {
			                DEBUG(DBG_ONSCREEN_MENU, DBGLEV_WARNING, "No good target track found, returning to current good track\n");
			                if (pl->SetCurrentEntry(current))
			                {
				                if (SUCCEEDED(res = mp->SetSong(current)))
				                {
                                    if ((ps == CMediaPlayer::PLAYING) && (mp->GetPlayState() != CMediaPlayer::PLAYING))
						                mp->Play();
                                    return res;
				                }

				                // Couldn't even set the current song.  This is bad.
				                DEBUG(DBG_ONSCREEN_MENU, DBGLEV_WARNING, "Couldn't set original song\n");
                                // TODO: mark as bad

                                // Try to go to the previous track.
                                if (FAILED(res = CPlayManager::GetInstance()->PreviousTrack()))
				                {
					                DEBUG(DBG_ONSCREEN_MENU, DBGLEV_WARNING, "No good files found\n");
                                    // TODO: this playlist is invalid.  Tell someone.
				                }
			                }
                        }
			        }
					return 0;
                }
				case KEY_BROWSE:
					HideScreen();
                    CBrowseMenuScreen::GetBrowseMenuScreen()->SetParent(CPlayerScreen::GetPlayerScreen());
					Presentation()->Add(CBrowseMenuScreen::GetBrowseMenuScreen());
                    Presentation()->MoveFocusTree(CBrowseMenuScreen::GetBrowseMenuScreen());
					//Presentation()->MoveFocusTree(m_pParent);
					return 0;

				case KEY_MENU:
				case KEY_UP:
				case KEY_DOWN:
					return 0;
				default:
					return CScreen::Message(Mesg);
			}
			break;

		case PM_KEY_RELEASE:

			switch (Mesg.iData)
			{
				case KEY_NEXT:
					return 0;
				case KEY_PREVIOUS:
					return 0;

				//case KEY_BROWSE:
				case KEY_MENU:
				case KEY_UP:
				case KEY_DOWN:
					HideScreen();
					Presentation()->MoveFocusTree(m_pParent);
					return 0;

				default:
					return CScreen::Message(Mesg);
			}
			break;

		case PM_TIMER:
			
			switch (Mesg.iData)
			{
				case TIMER_MENU_TIME_OUT:
					HideScreen();
					Presentation()->MoveFocusTree(m_pParent);
					return 0;

				default:
					return CScreen::Message(Mesg);
			}
			break;

		default:
			return CScreen::Message(Mesg);
	}
}

// Creates all the icons used by the menu screen.
void
COnScreenMenu::BuildScreen()
{
	PegRect ChildRect;
	mReal.Set(0, UI_Y_SIZE-15, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	m_CaptionRect.Set(mReal.wLeft + 6, mReal.wTop + 2, mReal.wRight - 6, mReal.wBottom - 1);
	m_pCaption = new PegString(m_CaptionRect, LS(SID_MENU), 0, FF_NONE | TT_COPY);
	m_pCaption->SetFont(&FONT_MENUSCREENENTRY);
	m_pCaption->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pCaption);

	ChildRect.Set(mReal.wLeft + 1, mReal.wTop + 4, mReal.wLeft + 5, mReal.wTop + 11);
	m_pBackArrow = new PegIcon(ChildRect, &gbBackArrowBitmap);
	Add(m_pBackArrow);

	ChildRect.Set(mReal.wRight - 5, mReal.wTop + 4, mReal.wRight - 1, mReal.wTop + 11);
	m_pForwardArrow = new PegIcon(ChildRect, &gbForwardArrowBitmap);
	Add(m_pForwardArrow);
}
