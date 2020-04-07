//........................................................................................
//........................................................................................
//.. File Name: PlayerScreen.cpp														..
//.. Date: 09/04/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the implementation of the CPlayerScreen class		..
//.. Usage: This class is derived from the CScreen class, and							..
//..		 contains the various windows that make up the main play screen display.	..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/04/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <stdio.h>

#include <main/demos/ssi_neo/ui/PlayerScreen.h>
#include <main/demos/ssi_neo/ui/UI.h>
//#include <main/demos/ssi_neo/ui/MainMenuScreen.h>
//#include "MediaPlayer.h"
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/Fonts.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Messages.h>
#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/main/AppSettings.h>     // PLAYLIST_STRING_SIZE
#include <cyg/infra/diag.h>
//#include "DriveInterface.h"
#include <playlist/common/Playlist.h>          // for IPlaylistEntry
#include <content/common/Metadata.h>           // for IMetadata
#include <util/tchar/tchar.h>

#include <io/audio/VolumeControl.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_PLAYER_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_PLAYER_SCREEN );

extern "C"
{
//#include "battery.h"
}

//extern CMainMenuScreen*	g_pMainMenuScreen;

#define TIMER_SCROLL_TITLE				201
#define TIMER_CHECK_BATTERY				202
#define TIMER_CHECK_CHARGING			203
#define TIMER_SLEEP_AFTER_TRACK_CHANGE	204
#define METADATA_STRING_SIZE            128

const int CPlayerScreen::sc_iBatteryCheckInterval = 15;
const int CPlayerScreen::sc_iBatteryChargingCheckInterval = 15;

const int CPlayerScreen::sc_iScrollStartInterval = 100;
const int CPlayerScreen::sc_iScrollContinueInterval = 5;
const int CPlayerScreen::sc_iScrollOffset = 3;

// Amount of ticks to wait before moving up to next scan speed.
const int CPlayerScreen::sc_iScanSpeedupInterval = 11;
// Array of possible scan speeds.
const int CPlayerScreen::sc_aryScanSpeeds[4] = { 5, 60, 600, 1800 };
// Maximum index into the array of scan speeds.
const int CPlayerScreen::sc_iScanMaxIndex = 3;

// Amount of ticks to wait before sleeping the drive after a next/previous track button press.
const int CPlayerScreen::sc_iTrackChangeSleepInterval = 20;

//extern CPlayScreen* g_pMainWindow;
CPlayerScreen* CPlayerScreen::s_pPlayerScreen = 0;

// This is a singleton class.
CScreen*
CPlayerScreen::GetPlayerScreen()
{
	if (!s_pPlayerScreen) {
		s_pPlayerScreen = new CPlayerScreen(NULL);
	}
	return s_pPlayerScreen;
}


CPlayerScreen::CPlayerScreen(CScreen* pParent)
  : CScreen(pParent)
{
	// Time is initially 0.
	m_uTrackTime = m_uTrackDuration = m_iSec1s = m_iSec10s = m_iMin1s = m_iMin10s = 0;

	m_bScanningForward = m_bScanningForwardPrimed = m_bScanningBackward = m_bScanningBackwardPrimed = m_bIgnoreNextKeyup = false;

	m_iOffsetReset = 0;

	m_pOnScreenMenu = new COnScreenMenu(this);

	BuildScreen();

	SetVolume(CVolumeControl::GetInstance()->GetVolume());

	// Set the current battery level and start a timer to check the battery level every minute.
	m_iBatteryLevel = 2;  // do this so that the battery level starts full and goes down evenly
	SetBattery(m_iBatteryLevel);

	m_bScrollingTitle = false;
	m_bInitialScrollPause = false;
	m_bBuffering = false;
	m_bCharging = false;
	m_bMenuButtonPress = false;
	m_bWaitingForTrackChangeSleep = false;

    m_pPlayManager = CPlayManager::GetInstance();
    m_pContentManager = m_pPlayManager->GetContentManager();
}

CPlayerScreen::~CPlayerScreen()
{
}


void
CPlayerScreen::Draw()
{
	PegRect ProgressBarRect;
	ProgressBarRect.Set(mReal.wLeft, 61, mReal.wRight, 64);
	Invalidate(mReal);
	BeginDraw();
	CScreen::Draw();
	// Draw the progress bar
	Rectangle(ProgressBarRect, BLACK);

	// we don't want to divide by zero, do we?
	if (m_uTrackDuration > 0)
	{
		int iProgress = (int)((double)m_uTrackTime * ((double)(mReal.wRight - mReal.wLeft - 1) / (double)m_uTrackDuration));
		Line(mReal.wLeft, 62, iProgress, 62, BLACK, 2);
	}

	EndDraw();
}


SIGNED
CPlayerScreen::Message(const PegMessage &Mesg)
{
	PegRect ChildRect;
	static unsigned int s_time = 0;

	switch (Mesg.wType)
	{
		case PM_KEY:

			//DEBUG_KEYBOARD_EVENTS("PEG PM_KEY: %d\n", Mesg.iData);
			switch (Mesg.iData)
			{
				case KEY_PLAY_PAUSE:
                    if (CMediaPlayer::GetInstance()->GetPlayState() != CMediaPlayer::PLAYING)
                    {
                        CPlayManager::GetInstance()->Play();
                        SetControlSymbol(PLAY);
                    }
                    else
                    {
                        CPlayManager::GetInstance()->Pause();
                        SetControlSymbol(PAUSE);
                    }
					break;

				case KEY_STOP:
                    CPlayManager::GetInstance()->Stop();
                    m_uTrackTime = 0;
                    SetControlSymbol(STOP);
					s_time = 0;
					SetTime(0);
					break;

				case KEY_NEXT:
					if (m_bScanningForward)
					{
						if (m_iScanTime + sc_aryScanSpeeds[m_iScanIndex] < m_uTrackDuration - 5)
						{
							m_iScanTime += sc_aryScanSpeeds[m_iScanIndex];
							// If enough ticks have passed (and the maximum scan index hasn't been reached)
							// then speed up the scan rate.
							if ((m_iScanIndex < sc_iScanMaxIndex) && (++m_iScanCount > sc_iScanSpeedupInterval))
							{
								++m_iScanIndex;
								m_iScanCount = 0;
							}
						}
						else
						{
							m_iScanTime = m_uTrackDuration - 5;
							if (m_iScanTime < 0)
								m_iScanTime = 0;
						}

						SetTime(m_iScanTime);
					}
					else if (m_bScanningForwardPrimed)
					{
						m_bScanningForward = true;
						m_bScanningForwardPrimed = false;
						m_bScanningBackward = false;
						m_bScanningBackwardPrimed = false;
						m_iScanTime = m_uTrackTime;
						SetControlSymbol(FAST_FORWARD);
					}
					else
					{
						m_bScanningForwardPrimed = true;
						m_iScanCount = 0;
						// If the track is past the hour mark then start scanning in 60-second intervals.
						if (m_uTrackTime > 3600)
							m_iScanIndex = 1;
						else
							m_iScanIndex = 0;
					}
					break;

				case KEY_PREVIOUS:
					if (m_bScanningBackward)
					{
						if (m_iScanTime - sc_aryScanSpeeds[m_iScanIndex] >= 0)
						{
							m_iScanTime -= sc_aryScanSpeeds[m_iScanIndex];
							SetTime(m_iScanTime);
							// If enough ticks have passed (and the maximum scan index hasn't been reached)
							// then speed up the scan rate.
							if ((m_iScanIndex < sc_iScanMaxIndex) && (++m_iScanCount > sc_iScanSpeedupInterval))
							{
								++m_iScanIndex;
								m_iScanCount = 0;
							}
						}
						else if (m_iScanTime)
						{
							m_iScanTime = 0;
							SetTime(0);
						}
					}
					else if (m_bScanningBackwardPrimed)
					{
						m_bScanningForward = false;
						m_bScanningForwardPrimed = false;
						m_bScanningBackward = true;
						m_bScanningBackwardPrimed = false;
						m_iScanTime = m_uTrackTime;
						SetControlSymbol(REWIND);
					}
					else
					{
						m_bScanningBackwardPrimed = true;
						m_iScanCount = 0;
						// If the track is past the hour mark then start scanning in 60-second intervals.
						if (m_uTrackTime > 3600)
							m_iScanIndex = 1;
						else
							m_iScanIndex = 0;
					}
					break;

				case KEY_UP:
					SetVolume(CVolumeControl::GetInstance()->SetVolume(CVolumeControl::GetInstance()->GetVolume() + 1));
					break;

				case KEY_DOWN:
					SetVolume(CVolumeControl::GetInstance()->SetVolume(CVolumeControl::GetInstance()->GetVolume() - 1));
					break;

				case KEY_MENU:
                    /*
					Add(CMainMenuScreen::GetMainMenuScreen());
					Presentation()->MoveFocusTree(CMainMenuScreen::GetMainMenuScreen());
                    */
					break;

				case KEY_BROWSE:
					// todo : figure out how to switch between these
					m_pOnScreenMenu->ShowScreen();
					//CBrowseMenuScreen::GetBrowseMenuScreen()->SetParent(this);
					//Add(CBrowseMenuScreen::GetBrowseMenuScreen());
					//Presentation()->MoveFocusTree(CBrowseMenuScreen::GetBrowseMenuScreen());
					break;


                    /*
                case KEY_LIST_BY_ARTIST:
                    if (m_bRefreshArtists)
                    {
                        m_Artists = m_pContentManager->GetArtists();
                        m_bRefreshArtists = false;
                    }
                    if (m_Artists.Size())
                    {
                        m_iArtistIndex = (m_iArtistIndex + 1) % m_Artists.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "Next artist: Key: %d Artist: %s\n", m_Artists[m_iArtistIndex].iKey, TcharToChar(szValue, m_Artists[m_iArtistIndex].szValue));

                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecordsByArtist(mrlTracks, m_Artists[m_iArtistIndex].iKey);
                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);
                        m_pPlayManager->Play();

                       // set the playlist info string

                        TcharToChar(m_sCurrentPlaylistName, m_Artists[m_iArtistIndex].szValue);
                        m_iCurrentPlaylistCount = mrlTracks.Size();
                        m_pUserInterface->NotifyPlaying();
                    }
                    return 0;

                case PK_LIST_BY_ALBUM:
                    if (m_bRefreshAlbums)
                    {
                        m_Albums = m_pContentManager->GetAlbums();
                        m_bRefreshAlbums = false;
                    }
                    if (m_Albums.Size())
                    {
                        m_iAlbumIndex = (m_iAlbumIndex + 1) % m_Albums.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "Next artist: Key: %d Album: %s\n", m_Albums[m_iAlbumIndex].iKey, TcharToChar(szValue, m_Albums[m_iAlbumIndex].szValue));

                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecordsByAlbum(mrlTracks, m_Albums[m_iAlbumIndex].iKey);

                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);
                        m_pPlayManager->Play();
                        // set the playlist info string
                        TcharToChar(m_sCurrentPlaylistName, m_Albums[m_iAlbumIndex].szValue);
                        m_iCurrentPlaylistCount = mrlTracks.Size();
                     
                        m_pUserInterface->NotifyPlaying();
                    }
                    return 0;
                
                case PK_LIST_BY_GENRE:
                    if (m_bRefreshGenres)
                    {
                        m_Genres = m_pContentManager->GetGenres();
                        m_bRefreshGenres = false;
                    }
                    if (m_Genres.Size())
                    {
                        m_iGenreIndex = (m_iGenreIndex + 1) % m_Genres.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "Next artist: Key: %d Genre: %s\n", m_Genres[m_iGenreIndex].iKey, TcharToChar(szValue, m_Genres[m_iGenreIndex].szValue));

                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecordsByGenre(mrlTracks, m_Genres[m_iGenreIndex].iKey);

                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);
                        m_pPlayManager->Play();

                       // set the playlist info string
                        TcharToChar(m_sCurrentPlaylistName, m_Genres[m_iGenreIndex].szValue);
                        m_iCurrentPlaylistCount = mrlTracks.Size();

                        m_pUserInterface->NotifyPlaying();
                    }
                    return 0;
*/



				case KEY_RECORD:
					SetControlSymbol(RECORD);
					return 0;

                case KEY_REFRESH_CONTENT:
                    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "Refreshing playlist\n");
                    CPlayManager::GetInstance()->RefreshAllContent( );

                default:
					break;
			}
			break;

		case PM_KEY_RELEASE:
			switch (Mesg.iData)
			{
				case KEY_NEXT:
                    if (m_bScanningForward)
					{
						m_bScanningForward = false;
                        m_bScanningForwardPrimed = false;
						SynchControlSymbol();
						if (SUCCEEDED(CPlayManager::GetInstance()->Seek(m_iScanTime+1)))
							m_uTrackTime = m_iScanTime;
						else
							SetTime(m_uTrackTime);
					}
                    else
                    {
                        m_bScanningForwardPrimed = false;
                        SetControlSymbol(NEXT_TRACK);
                        if (SUCCEEDED(CPlayManager::GetInstance()->NextTrack()))
                        {
                            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "Track %d of %d\n", CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1, CPlayManager::GetInstance()->GetPlaylist()->GetSize());
                        }
                    }
					break;

				case KEY_PREVIOUS:
   					if (m_bScanningBackward)
					{
						m_bScanningBackward = false;
                        m_bScanningBackwardPrimed = false;
						SynchControlSymbol();
                        if (SUCCEEDED(CPlayManager::GetInstance()->Seek(m_iScanTime+1)))
							m_uTrackTime = m_iScanTime;
						else
							SetTime(m_uTrackTime);
					}
					else
					{
                        if (m_uTrackTime <= 5)
                        {
						    SetControlSymbol(PREVIOUS_TRACK);
                            if (SUCCEEDED(CPlayManager::GetInstance()->PreviousTrack()))
                            {
                                DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "Track %d of %d\n", CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1, CPlayManager::GetInstance()->GetPlaylist()->GetSize());
                            }
                        }
                        else
                        {
                            if (CMediaPlayer::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
                            {
                                CPlayManager::GetInstance()->Stop();
                                CPlayManager::GetInstance()->Play();
                            }
                            else
                            {
                                CPlayManager::GetInstance()->Stop();
                                SetTime(0);
                            }
                        }
                        m_bScanningBackwardPrimed = false;
                    }
					break;
			}
			break;

		case IOPM_PLAY:
			SetControlSymbol(PLAY);
			SynchControlSymbol();
			break;

        case IOPM_PAUSE:
			SetControlSymbol(PAUSE);
			SynchControlSymbol();
			break;

		case IOPM_STOP:
			SetControlSymbol(STOP);
			SynchControlSymbol();
			/*
			if (CMediaPlayer::GetInstance()->GetPlayState() == CMediaPlayer::STOPPED)
			{
				//m_uTrackTime = 0;
				SetTime(0);
			}
			*/
			break;

        case IOPM_NEW_TRACK:
        {
            set_track_message_t* pStm = (set_track_message_t*)Mesg.pData;
            SetTrack(pStm);
            delete pStm;
            break;
        }

		case IOPM_TRACK_PROGRESS:
			if (!m_bScanningBackward && !m_bScanningForward /*&& (CMediaPlayer::GetInstance()->GetPlayState() != CMediaPlayer::STOPPED)*/)
			{
    			SetTime(Mesg.lData);
				m_uTrackTime = Mesg.lData;
			}
			
			break;

		case IOPM_PLAYLIST_LOADED:
        {
            //g_pMainMenuScreen->GetPlaylistScreen()->UpdatePlaylist();
            TCHAR temp[strlen((char*)Mesg.lData)+1];
            CharToTchar(temp,(char*)Mesg.lData);
            SetText3(temp);
            //m_pText3String->DataSet((TCHAR*)Mesg.lData);

	    	break;
        }
		case IOPM_TRACK_END:
		{
            // TODO (epg,9/25/2001): this handles the eof during a scan.  I'm not sure whether it gets sent from anywhere.
			/*
			DEBUG_PLAYER_EVENTS("Peg track end message\n");
			// If the end of the track is reached then stop scanning and ignore the next key up message.
			if (m_bScanningForward || m_bScanningForwardPrimed || m_bScanningBackward ||m_bScanningBackwardPrimed)
			{
				m_bScanningForward = false;
				m_bScanningForwardPrimed = false;
				m_bScanningBackward = false;
				m_bScanningBackwardPrimed = false;
				m_bIgnoreNextKeyup = true;
			}

			SetControlSymbol(NEXT_TRACK);
			DEBUG_WAKE_DRIVE;
			WakeDrive(true, false);
			int res = CMediaPlayer::GetInstance()->NextTrack();
			if (FAILED(res))
			{
				// The end of the playlist has been reached, so stop playback.
				SetControlSymbol(STOP);
				//m_uTrackTime = 0;
				SetTime(0);

				if (FAILED(CMediaPlayer::GetInstance()->Stop()))
					SynchControlSymbol();

				if (res != CODEC_DISKERROR)
				{
					// If we come to the end of the playlist in normal or random mode,
					// the set the first track.
					IPlaylist::PlaylistMode ePM = CPlaylist::GetPlaylist()->GetPlaylistMode();
					if ((ePM == CPlaylist::NORMAL) || (ePM == CPlaylist::RANDOM))
						CMediaPlayer::GetInstance()->SetFirstTrack();
					// If we come to the end of the playlist in intro scan mode,
					// the set the first track and return to normal mode.
					else if (ePM == CPlaylist::INTRO_SCAN)
					{
						CMediaPlayer::GetInstance()->SetFirstTrack();
						SetNormalPlayMode();
					}
				}

//				if (FAILED(CMediaPlayer::GetInstance()->Stop()))
//					SynchControlSymbol();
			}
			DEBUG_SLEEP_DRIVE;
			SleepDrive();
			SleepDrive();
			*/
			break;
		}
        case IOPM_VOLUME:
            SetVolume(Mesg.lData);
            break;
        case IOPM_PLAYMODE:
            SetPlayMode(Mesg.lData);
            break;
		case PM_TIMER:
			
			switch (Mesg.iData)
			{
			case TIMER_SCROLL_TITLE:
				{
					//					PegRect ChildRect = m_pText1String->mReal;
					if (m_bInitialScrollPause)
						m_bInitialScrollPause = false;
					ChildRect = m_pText1String->mReal;
					ChildRect.wLeft -= sc_iScrollOffset;
					if (ChildRect.wLeft < m_iOffsetReset)
					{
						m_bInitialScrollPause = true;
						if (!m_bBuffering)
							SetTimer(TIMER_SCROLL_TITLE, sc_iScrollStartInterval, sc_iScrollContinueInterval);
						ChildRect.wLeft = 0;
					}
					Remove(m_pText1String);
					m_pText1String->Resize(ChildRect);
					Add(m_pText1String);
					//					Invalidate(m_pText1String->mReal);
					//					m_pText1String->Draw();
					break;
				}
#if 0				
			case TIMER_SLEEP_AFTER_TRACK_CHANGE:
				{
					DEBUG_SLEEP_DRIVE;
					SleepDrive();
					m_bWaitingForTrackChangeSleep = false;
				};
				break;
				
			case TIMER_CHECK_BATTERY:
				{
					// checking to see if this is current prevents the 
					//if(Presentation()->GetCurrentThing() == this)
					//{
					static bool s_bLowBatteryFlash;
					
					int iBatteryLevel = get_soft_charge_level();
					int pws = power_source_available();
					
					//diag_printf("Charge Level() = %d\n", iBatteryLevel);
					
					if (!pws)
					{
						// only change the battery level down, so that once it gets a low reading, it stays
						// there.  this is done so that the battery level doesn't fluxuate back and fourth
						// when it's near a boundary in charge levels.
						if (iBatteryLevel < m_iBatteryLevel)
						{
							m_iBatteryLevel = iBatteryLevel;
							SetBattery(m_iBatteryLevel);
						}
						
						if (m_iBatteryLevel == 0)
						{
							if(s_bLowBatteryFlash)
							{ // erase battery for blinking effect
								s_bLowBatteryFlash = false;
								//m_pBatteryIcon->SetIcon(&gbEmptyBitmap);
								SetBattery(1111);  // 
							}
							else
							{ // make sure the battery drawing is there
								s_bLowBatteryFlash = true;
								//m_pBatteryIcon->SetIcon(&gb_Battery_Empty_Bitmap);
								SetBattery(0);
							}
							Invalidate(m_pBatteryIcon->mReal);
							Draw();
						}
						
					}
					else
					{
						if (iBatteryLevel != MAX_CHARGE)
						{
							m_pBatteryIcon->SetIcon(&gb_Battery_Empty_Bitmap);
							Invalidate(m_pBatteryIcon->mReal);
							Draw();
							
							enable_charging();
							SetTimer(TIMER_CHECK_CHARGING, sc_iBatteryChargingCheckInterval, sc_iBatteryChargingCheckInterval);
							KillTimer(TIMER_CHECK_BATTERY);
							m_bCharging = true;
							m_iBatteryLevel = 2;  // do this so that the battery level starts full and goes down evenly
						} 
					}
					//}
					break;
				}
				
			case TIMER_CHECK_CHARGING:
				{
					static int s_iBatteryLevel = 0;
					static bool s_bFullyCharged = false;
					int iBatteryLevel = get_soft_charge_level();
					int pws = power_source_available();
					
					//diag_printf("Charging Charge Level() = %d\n", iBatteryLevel);
					
					if (!pws)
					{
						disable_charging();
						SetTimer(TIMER_CHECK_BATTERY, sc_iBatteryCheckInterval, sc_iBatteryCheckInterval);
						KillTimer(TIMER_CHECK_CHARGING);
						m_bCharging = false;
						SetBattery(m_iBatteryLevel);
						s_bFullyCharged = false;
					}
#ifdef _IOMEGA_16_
					else if (iBatteryLevel == MAX_CHARGE)
					{
						s_bFullyCharged = true;
						m_iBatteryLevel = 2;
						SetBattery(m_iBatteryLevel);
					}
#endif
					else if (!s_bFullyCharged)
					{
						// do the animation
						if(++s_iBatteryLevel <= 3)
							SetBattery(s_iBatteryLevel + 10);
						else
						{
							s_iBatteryLevel = 0;
							SetBattery(s_iBatteryLevel);
						}
					}
					break;
				}
				break;
#endif
		}
		default:    
			return CScreen::Message(Mesg);
	}
	return 0;
}

// Hides any visible menu screens.
void
CPlayerScreen::HideMenu()
{
//	CMainMenuScreen::GetMainMenuScreen()->HideScreen();
//	Invalidate(mReal);
//	Draw();
}

// Tells the interface to pause any cycle-chomping activity while the buffers are read.
void
CPlayerScreen::NotifyBufferReadBegin()
{
	m_bBuffering = true;
	KillTimer(TIMER_SCROLL_TITLE);
	KillTimer(TIMER_CHECK_CHARGING);
	KillTimer(TIMER_CHECK_BATTERY);
}

// Tells the interface that the buffer read is over, so let the chomping begin!
void
CPlayerScreen::NotifyBufferReadEnd()
{
	m_bBuffering = false;
	if (m_bScrollingTitle)
		if (m_bInitialScrollPause)
			SetTimer(TIMER_SCROLL_TITLE, sc_iScrollStartInterval, sc_iScrollContinueInterval);
		else
			SetTimer(TIMER_SCROLL_TITLE, sc_iScrollContinueInterval, sc_iScrollContinueInterval);

	if (m_bCharging)
		SetTimer(TIMER_CHECK_CHARGING, sc_iBatteryChargingCheckInterval, sc_iBatteryChargingCheckInterval);
	else
		SetTimer(TIMER_CHECK_BATTERY, sc_iBatteryCheckInterval, sc_iBatteryCheckInterval);
}

// Tells the play screen that the track is about to change.
void
CPlayerScreen::NotifyTrackAboutToChange(bool bShowResult)
{
}


// Asks the play screen to update the current track's track number, in case it changed.
// This is called when the user leaves the play list screen or play list manager screen.
void
CPlayerScreen::UpdateTrackNumber()
{
/*
	const CPlaylistEntry* pEntry = CPlaylist::GetPlaylist()->GetCurrentEntry();
	if (pEntry)
		SetTrackNumber(CPlaylist::GetPlaylist()->GetPlaylistEntryIndex(pEntry) + 1);
	else
		SetTrackNumber(0);
*/
}




void

CPlayerScreen::SetTrack(set_track_message_t* pStm)
{
	if (pStm->metadata)
	{
		m_uTrackDuration = pStm->duration;
		TCHAR *szScratch;
        TCHAR szTemp[METADATA_STRING_SIZE];
        void *pData;
        
        if (SUCCEEDED(pStm->metadata->GetAttribute(MDA_TITLE, &pData)))
            szScratch = (TCHAR*) pData;
            //TcharToCharN(szScratch, (TCHAR*)pData, METADATA_STRING_SIZE);
        else
            szScratch = 0;

//#define TEST_FONT
#ifdef TEST_FONT
		TCHAR szUnicodeText[] = {0xb000, 0xb001, 0xb002, 0xb003, 0xb004, 0xb005, 0xb006, 0xb007, 0xb008, 0xb009, 0x0000};
		strcpy((TCHAR*)szScratch, szUnicodeText);
#endif // TEST_FONT

#if 0
		// make sure the title we just got can be represented correctly by our font
		// if we can't display it correctly, just show "Track #"
        if (!Screen()->TextTest(szScratch, &FONT_PLAYSCREEN))
		{
			diag_printf("Can't Print the Track\n");
			char szTempTrackNum[10];
			strcpy(szScratch, LS(SID_TRACK));
			//sprintf(szTempTrackNum, " %d", (CPlaylist::GetPlaylist()->GetPlaylistEntryIndex(pContentRecord) + 1));
			//diag_printf("szTempTrackNum [%s]\n", szTempTrackNum);
			//strcat(szScratch, szTempTrackNum);
		}
#endif
		// See if the title needs to be scrolled.
		int iTitleWidth = Screen()->TextWidth(szScratch, &FONT_PLAYSCREEN);
		if (iTitleWidth > mReal.wRight)
		{
			//sprintf(szTemp, "%s -- ", szScratch);
			tstrcpy(szTemp, szScratch);
            TCHAR tsz[5];
            CharToTchar(tsz," -- ");
			tstrcat(szTemp, tsz);
			m_iOffsetReset = -Screen()->TextWidth(szTemp, &FONT_PLAYSCREEN);
			tstrcat(szTemp, szScratch);
			m_bScrollingTitle = true;
			m_bInitialScrollPause = true;
			if (!m_bBuffering)
			{
				SetTimer(TIMER_SCROLL_TITLE, sc_iScrollStartInterval, sc_iScrollContinueInterval);
			}
            m_pText1String->DataSet(szTemp);
		}
		else
		{
			m_bScrollingTitle = false;
            m_pText1String->DataSet(szScratch);
			KillTimer(TIMER_SCROLL_TITLE);
		}

        if (SUCCEEDED(pStm->metadata->GetAttribute(MDA_ARTIST, &pData)))
            //TcharToCharN(szScratch, (TCHAR*)pData, METADATA_STRING_SIZE);
            szScratch = (TCHAR*)pData;
        else
            szScratch = 0;
        m_pText2String->DataSet(szScratch);
	}
	else
	{
		TCHAR szTempString[] = {'s','t','2','0','\n', 0};
		m_pText1String->DataSet(szTempString);
		SetTrackNumber(0);
		m_uTrackDuration = 0;
		m_bScrollingTitle = false;
		KillTimer(TIMER_SCROLL_TITLE);
	}
    if (pStm->metadata_copy)
        delete pStm->metadata;
	SynchControlSymbol();
	SetTime(0);
}

void
CPlayerScreen::SetTrackNumber(unsigned int uTrack)
{
	int ones = uTrack % 10;
	int tens = (uTrack % 100) / 10;

	m_pTrackNumberTensIcon->SetIcon(Small_Number[tens]);
	m_pTrackNumberOnesIcon->SetIcon(Small_Number[ones]);
	Invalidate(m_pTrackNumberTensIcon->mReal);
	Invalidate(m_pTrackNumberOnesIcon->mReal);
	Draw();
}



void
CPlayerScreen::SetVolume(unsigned int uVolume)
{
	int ones = uVolume % 10;
	int tens = (uVolume % 100) / 10;

	m_pVolumeNumberTensIcon->SetIcon(Small_Number_Inverted[tens]);
	m_pVolumeNumberOnesIcon->SetIcon(Small_Number_Inverted[ones]);
	Invalidate(m_pVolumeNumberTensIcon->mReal);
	Invalidate(m_pVolumeNumberOnesIcon->mReal);

	Draw();
}


void
CPlayerScreen::SetTime(unsigned int uTime)
{
	if (uTime == m_uTrackTime)
		return;

	m_uTrackTime = uTime;

	int seconds = m_uTrackTime % 60;
	int minutes = (m_uTrackTime % 3600) / 60;
	int hours = m_uTrackTime / 3600;

	int s_ones = seconds % 10;
	int s_tens = (seconds % 100) / 10;
	int m_ones = minutes % 10;
	int m_tens = (minutes % 100) / 10;
	int h_ones = hours % 10;
	int h_tens = (hours % 100) / 10;

	if (s_ones != m_iSec1s)
	{
		m_pTimeNumberSecondsOnesIcon->SetIcon(Time_Number[s_ones]);
		Invalidate(m_pTimeNumberSecondsOnesIcon->mReal);
		m_iSec1s = s_ones;
	}
	if (s_tens != m_iSec10s)
	{
		m_pTimeNumberSecondsTensIcon->SetIcon(Time_Number[s_tens]);
		Invalidate(m_pTimeNumberSecondsTensIcon->mReal);
		m_iSec10s = s_tens;
	}
	if (m_ones != m_iMin1s)
	{
		m_pTimeNumberMinutesOnesIcon->SetIcon(Time_Number[m_ones]);
		Invalidate(m_pTimeNumberMinutesOnesIcon->mReal);
		m_iMin1s = m_ones;
	}
	if (m_tens != m_iMin10s)
	{
		m_pTimeNumberMinutesTensIcon->SetIcon(Time_Number[m_tens]);
		Invalidate(m_pTimeNumberMinutesTensIcon->mReal);
		m_iMin10s = m_tens;
	}
	if (hours > 0)
	{
		if (h_ones != m_iHr1s)
		{
			m_pTimeNumberHoursOnesIcon->SetIcon(Time_Number[h_ones]);
			Invalidate(m_pTimeNumberHoursOnesIcon->mReal);
			m_iHr1s = h_ones;
		}
		if (h_tens != m_iHr10s)
		{
			m_pTimeNumberHoursTensIcon->SetIcon(Time_Number[h_tens]);
			Invalidate(m_pTimeNumberHoursTensIcon->mReal);
			m_iHr10s = h_tens;
		}
		m_pTimeColonHoursIcon->SetIcon(&gb_Time_Colon_Bitmap);
		Invalidate(m_pTimeColonHoursIcon->mReal);
	}
	else
	{
		// don't show the hours numbers at all
		m_iHr1s = 0;
		m_iHr10s = 0;
		m_pTimeNumberHoursOnesIcon->SetIcon(&gbEmptyBitmap);
		Invalidate(m_pTimeNumberHoursOnesIcon->mReal);
		m_pTimeNumberHoursTensIcon->SetIcon(&gbEmptyBitmap);
		Invalidate(m_pTimeNumberHoursTensIcon->mReal);
		m_pTimeColonHoursIcon->SetIcon(&gbEmptyBitmap);
		Invalidate(m_pTimeColonHoursIcon->mReal);

	}

	Draw();

}


void
CPlayerScreen::SetBattery(unsigned int uLevel)
{
	m_pBatteryFullIcon->SetIcon(&gb_Battery_Full_Bitmap);
	PegRect newRect = m_pBatteryIcon->mReal;
	switch(uLevel)
	{
		case 1:  // 1/3
			newRect.wRight = newRect.wLeft + 8;
			break;
		case 2:  // 2/3
			newRect.wRight = newRect.wLeft + 5;
			break;
		case 3:  // full
			newRect.wRight = newRect.wLeft + 0;
			break;
		case 11:
			newRect.wRight = newRect.wLeft + 8;
			break;
		case 12:
			newRect.wRight = newRect.wLeft + 5;
			break;
		case 13:
			newRect.wRight = newRect.wLeft + 0;
			break;
		case 1111:  // everything blank
			m_pBatteryFullIcon->SetIcon(&gbEmptyBitmap);
			newRect.wRight = newRect.wLeft - 1;
			break;
		default: // empty
			newRect.wRight = newRect.wRight + 13;
			break;
	}

	m_pBatteryIcon->Resize(newRect);
	Invalidate(m_pBatteryFullIcon->mReal);
	Draw();
}


void
//CPlayerScreen::SetPlayMode(IPlaylist::PlaylistMode ePlaylistMode)
CPlayerScreen::SetPlayMode(int iPlayMode)
{
	//m_pPlayModeIcon->SetIcon(Play_Mode[ePlaylistMode]);
	m_pPlayModeIcon->SetIcon(Play_Mode_Inverted[iPlayMode]);
	m_pPlayModeIcon->Invalidate(m_pPlayModeIcon->mReal);
	m_pPlayModeIcon->Draw();
}

void
CPlayerScreen::SetControlSymbol(ControlSymbol eControlSymbol)
{
	m_eControlSymbol = eControlSymbol;

	m_pControlSymbolIcon->SetIcon(Control_Symbol[eControlSymbol]);
	m_pControlSymbolIcon->Invalidate(m_pControlSymbolIcon->mReal);
	m_pControlSymbolIcon->Draw();
}


void
CPlayerScreen::SetLockIcon(bool on)
{
	if (on)
		m_pLockIcon->SetIcon(&gb_Lock_Bitmap);
	else
		m_pLockIcon->SetIcon(&gbEmptyBitmap);

	Invalidate(m_pLockIcon->mReal);
	Draw();
}

void
CPlayerScreen::SetText1(const TCHAR* szText)
{
	m_pText1String->DataSet(szText);
	Screen()->Invalidate(m_pText1String->mReal);
	Draw();
}

void
CPlayerScreen::SetText2(const TCHAR* szText)
{
	m_pText2String->DataSet(szText);
	Screen()->Invalidate(m_pText2String->mReal);
	Draw();
}

void
CPlayerScreen::SetText3(const TCHAR* szText)
{
	m_pText3String->DataSet(szText);
	Screen()->Invalidate(m_pText3String->mReal);
	Draw();
}

void
CPlayerScreen::BuildScreen()
{
	PegRect ChildRect;
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	// text area
	ChildRect.Set(mReal.wLeft, 17, mReal.wRight, 28);
	//TCHAR szTitle[PLAYLIST_STRING_SIZE * 2 + 4] = {0};
	//const CPlaylistEntry* pEntry = CPlaylist::GetPlaylist()->GetCurrentEntry();
	//if (pEntry)
	//	pEntry->GetFormattedTitle(szTitle);
	TCHAR szTitle[256 * 2 + 4] = {0};
	CharToTchar(szTitle, "Song Title");
	m_pText1String = new PegString(ChildRect, szTitle, 0, FF_NONE | TT_COPY);
	m_pText1String->SetFont(&FONT_PLAYSCREEN);
	m_pText1String->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pText1String);

	// 2nd text area
	ChildRect.Set(mReal.wLeft, 30, mReal.wRight, 41);
	//TCHAR szTitle[PLAYLIST_STRING_SIZE * 2 + 4] = {0};
	//const CPlaylistEntry* pEntry = CPlaylist::GetPlaylist()->GetCurrentEntry();
	//if (pEntry)
	//	pEntry->GetFormattedTitle(szTitle);
	//TCHAR szTitle[256 * 2 + 4] = {0};
	CharToTchar(szTitle, "Artist");
	m_pText2String = new PegString(ChildRect, szTitle, 0, FF_NONE | TT_COPY);
	m_pText2String->SetFont(&FONT_PLAYSCREEN);
	m_pText2String->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pText2String);

	// 3rd text area
	ChildRect.Set(mReal.wLeft, mReal.wBottom - 12, mReal.wRight, mReal.wBottom);
	//TCHAR szTitle[PLAYLIST_STRING_SIZE * 2 + 4] = {0};
	//const CPlaylistEntry* pEntry = CPlaylist::GetPlaylist()->GetCurrentEntry();
	//if (pEntry)
	//	pEntry->GetFormattedTitle(szTitle);
	//TCHAR szTitle[256 * 2 + 4] = {0};
	CharToTchar(szTitle, "3/41 : My Playlist");
	m_pText3String = new PegString(ChildRect, szTitle, 0, FF_NONE | TT_COPY);
	m_pText3String->SetFont(&FONT_PLAYSCREEN);
	m_pText3String->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pText3String);

	// the black part of the top screen
	ChildRect.Set(mReal.wLeft, mReal.wTop, mReal.wRight, 15);
	m_pScrennTopIcon = new PegIcon(ChildRect, &gbBlackTitleBarBitmap);
	m_pScrennTopIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pScrennTopIcon);

	// speaker
	ChildRect.Set(72, 3, 81, 11);
	m_pSpeakerIcon = new PegIcon(ChildRect, &gb_Volume_Symbol_Bitmap);
	m_pSpeakerIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pSpeakerIcon);

	// volume number display
	ChildRect.Set(84, 3, 88, 11);
	m_pVolumeNumberTensIcon = new PegIcon(ChildRect, Small_Number_Inverted[8]);
	m_pVolumeNumberTensIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pVolumeNumberTensIcon);

	ChildRect.Set(90, 3, 94, 11);
	m_pVolumeNumberOnesIcon = new PegIcon(ChildRect, Small_Number_Inverted[8]);
	m_pVolumeNumberOnesIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pVolumeNumberOnesIcon);

	// battery
	ChildRect.Set(mReal.wRight - 16, mReal.wTop + 4, mReal.wRight - 1, mReal.wTop + 12);
	m_pBatteryFullIcon = new PegIcon(ChildRect, &gb_Battery_Full_Bitmap);
	m_pBatteryFullIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pBatteryFullIcon);

	ChildRect.Set(mReal.wRight - 16, mReal.wTop + 4, mReal.wRight - 1, mReal.wTop + 12);
	m_pBatteryIcon = new PegIcon(ChildRect, &gb_Battery_Empty_Bitmap);
	m_pBatteryIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pBatteryIcon);

	// track number display
	int iTrack10s = 0, iTrack1s = 0;
	if (1) // if (pEntry)
	{
		int iTrackNo = 12; //CPlaylist::GetPlaylist()->GetPlaylistEntryIndex(pEntry) + 1;
		iTrack10s = iTrackNo / 10;
		iTrack1s = iTrackNo % 10;
	}

	ChildRect.Set(28, 36, 32, 44);
	m_pTrackNumberTensIcon = new PegIcon(ChildRect, Small_Number[iTrack10s]);
	m_pTrackNumberTensIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	//this->Add(m_pTrackNumberTensIcon);

	ChildRect.Set(34, 36, 38, 44);
	m_pTrackNumberOnesIcon = new PegIcon(ChildRect, Small_Number[iTrack1s]);
	m_pTrackNumberOnesIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	//this->Add(m_pTrackNumberOnesIcon);


	// time display
	ChildRect.Set(45, 43, 53, 58);
	m_pTimeNumberHoursTensIcon = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pTimeNumberHoursTensIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTimeNumberHoursTensIcon);

	ChildRect.Set(56, 43, 64, 58);
	m_pTimeNumberHoursOnesIcon = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pTimeNumberHoursOnesIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTimeNumberHoursOnesIcon);

	ChildRect.Set(67, 46, 69, 56);
	m_pTimeColonHoursIcon = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pTimeColonHoursIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTimeColonHoursIcon);

	ChildRect.Set(72, 43, 80, 58);
	m_pTimeNumberMinutesTensIcon = new PegIcon(ChildRect, Time_Number[0]);
	m_pTimeNumberMinutesTensIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTimeNumberMinutesTensIcon);

	ChildRect.Set(83, 43, 91, 58);
	m_pTimeNumberMinutesOnesIcon = new PegIcon(ChildRect, Time_Number[0]);
	m_pTimeNumberMinutesOnesIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTimeNumberMinutesOnesIcon);

	ChildRect.Set(94, 46, 96, 56);
	m_pTimeColonIcon = new PegIcon(ChildRect, &gb_Time_Colon_Bitmap);
	m_pTimeColonIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTimeColonIcon);

	ChildRect.Set(99, 43, 107, 58);
	m_pTimeNumberSecondsTensIcon = new PegIcon(ChildRect, Time_Number[0]);
	m_pTimeNumberSecondsTensIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTimeNumberSecondsTensIcon);

	ChildRect.Set(110, 43, 118, 58);
	m_pTimeNumberSecondsOnesIcon = new PegIcon(ChildRect, Time_Number[0]);
	m_pTimeNumberSecondsOnesIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTimeNumberSecondsOnesIcon);

	// control symbol
	ChildRect.Set(4, 45, 21, 57);
	m_pControlSymbolIcon = new PegIcon(ChildRect, Control_Symbol[STOP]);
	m_pControlSymbolIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pControlSymbolIcon);

	// screen icon  (player, voice recorder, fm, etc)
	ChildRect.Set(mReal.wLeft + 1, mReal.wTop + 1, mReal.wLeft + 13, mReal.wTop + 13);
	m_pScreenIcon = new PegIcon(ChildRect, &gbPlayerScreenBitmap);
	m_pScreenIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pScreenIcon);

	// play mode
	ChildRect.Set(mReal.wLeft + 20, 2, mReal.wLeft + 36, 14);
//	m_pPlayModeIcon = new PegIcon(ChildRect, Play_Mode[CPlaylist::GetPlaylist()->GetPlaylistMode()]);
	m_pPlayModeIcon = new PegIcon(ChildRect, Play_Mode_Inverted[0]);
	m_pPlayModeIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pPlayModeIcon);

	// lock 
	ChildRect.Set(91, 51, 97, 62);
	m_pLockIcon = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pLockIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pLockIcon);
}


// Query the media player for its current play state
// and display the appropriate control symbol.
void
CPlayerScreen::SynchControlSymbol()
{
	if (m_bScanningForward)
	{
		SetControlSymbol(FAST_FORWARD);
		return;
	}
	if (m_bScanningBackward)
	{
		SetControlSymbol(REWIND);
		return;
	}
	switch (CMediaPlayer::GetInstance()->GetPlayState())
	{
		case CMediaPlayer::PLAYING:
			SetControlSymbol(PLAY);
			break;

		case CMediaPlayer::PAUSED:
			SetControlSymbol(PAUSE);
			break;

		case CMediaPlayer::STOPPED:
        case CMediaPlayer::NOT_CONFIGURED:
			SetControlSymbol(STOP);
			break;
	}
	//SetControlSymbol(m_eControlSymbol);
}

// Sets normal play mode in the playlist, play screen, and play mode menu.
void
CPlayerScreen::SetNormalPlayMode()
{
/*
	CPlaylist::GetPlaylist()->SetPlaylistMode(CPlaylist::NORMAL);
	SetPlayMode(CPlaylist::NORMAL);
	g_pMainMenuScreen->GetPlayModeMenuScreen()->SetPlayMode(CPlaylist::NORMAL);
*/
}

