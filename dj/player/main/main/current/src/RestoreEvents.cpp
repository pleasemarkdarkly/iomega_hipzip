// RestoreEvents.cpp: how we get events for the restore system
// danb@iobjects.com 05/01/02
// (c) Fullplay Media

#include <main/main/RestoreEvents.h>

#include <core/events/SystemEvents.h>

#include <main/main/MiniCDMgr.h>
// #include <datasource/cddatasource/CDDataSource.h>

#include <gui/peg/peg.hpp>

#include <main/main/AppSettings.h>

#include <main/main/EventTypes.h>
#include <main/ui/common/UserInterface.h>
#include <main/ui/Keys.h>
#include <main/ui/RestoreMenuScreen.h>
#include <main/ui/RestoreScreen.h>
#include <main/ui/Strings.hpp>

#include <fs/fat/sdapi.h>
#include <cyg/kernel/kapi.h>

#include <stdio.h>
#include <stdlib.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( REV, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( REV );


#ifndef NULL
#define NULL 0
#endif

CRestoreEvents::CRestoreEvents() :
    m_bPowerHeld(false)
{
 
	m_pMiniCDMgr = CMiniCDMgr::GetInstance();
    m_pUserInterface = NULL;
}

CRestoreEvents::~CRestoreEvents() 
{}

void
CRestoreEvents::SetUserInterface( IUserInterface* pUI ) 
{
    DEBUGP( REV, DBGLEV_TRACE, "CRestoreEvents::SetUserInterface()\n");
    m_pUserInterface = pUI;
}

void
CRestoreEvents::RefreshInterface()
{
    if (m_pUserInterface)
    {
        DEBUGP( REV, DBGLEV_TRACE, "CRestoreEvents::RefreshInterface()\n");
    }
}

void
CRestoreEvents::SynchPlayState()
{
    DEBUGP( REV, DBGLEV_TRACE, "CRestoreEvents::SynchPlayState()\n");
}

// Called after a CD is inserted and ready to play.
// For data CDs, this is after the first pass during content update.
// For audio CDs, it's after the second pass.
// Returns true if everything's fine, false if an error occurred.
bool
CRestoreEvents::HandleCDInsertion()
{
    // Tell the recording manager.
    DEBUGP( REV, DBGLEV_TRACE, "CRestoreEvents::HandleCDInsertion()\n");
    return true;
}

// returns -1 if event not handled
//          0 if handled and no further processing needed (override)
//          1 if handled and further processing needed
int
CRestoreEvents::HandleEvent( int key, void* data ) 
{
    if( !m_pUserInterface ) return -1;
    
    switch( key )
    {

        case EVENT_KEY_HOLD:
        {
            unsigned int keycode = (unsigned int)data;
            DEBUGP( REV, DBGLEV_INFO, "Key hold %d\n", keycode );
            
            switch( keycode )
            {
				case IR_KEY_MENU:
					DEBUGP( REV, DBGLEV_INFO, "IR_KEY_MENU hold ignored\n", keycode );
					break;

                default:                    
                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY;
                    Mesg.iData = keycode;
                    pt->MessageQueue()->Push(Mesg);
                    break;
            }
            break;
        }

        case EVENT_KEY_PRESS:
        {
            unsigned int keycode = (unsigned int)data;
            DEBUGP( REV, DBGLEV_INFO, "Key press %d\n", keycode );
            
            switch( keycode )
            {
                case KEY_CD_EJECT:
                {
static cyg_tick_count_t s_tick = 0;
                    cyg_tick_count_t tick = cyg_current_time();
                    if (!m_pMiniCDMgr->IsTrayOpen())
                    {
                        // Delay 5 seconds after a close to let the CD mount.
                        if (s_tick && (tick - s_tick < 500))
                        {
                            DEBUGP( REV, DBGLEV_INFO, "Ignoring eject keypress\n");
                            return 1;
                        }
                        s_tick = tick;
                    
                        m_pMiniCDMgr->OpenTray();
                    }
                    else
                    {
                        // Delay 3 seconds after an open to let the CD unmount.
                        if (s_tick && (tick - s_tick < 300))
                        {
                            DEBUGP( REV, DBGLEV_INFO, "Ignoring eject keypress\n");
                            return 1;
                        }
                        s_tick = tick;

						m_pMiniCDMgr->CloseTray();

                    }
                    break;
                }

                case IR_KEY_VOL_UP:
                    break;

                case IR_KEY_VOL_DOWN:
                    break;

                case KEY_BREAK_POINT:
                    // Fake a CD eject button press on the dharma.
                    CEventQueue::GetInstance()->PutEvent(EVENT_KEY_PRESS, (void*)KEY_CD_EJECT );
                    break;

                case IR_KEY_ZOOM:
                    break;

                /*
                case IR_KEY_1_misc:
                    {
                        // This is a way to test deleting cddb files
                        char cddb_idx_file[] = "A:\\cddb\\ecddb.idx";
                        STAT Stat;
                        if (pc_stat(cddb_idx_file, &Stat) != 0)
                            DEBUG( REV, DBGLEV_INFO, "No CDDB file [%s]\n", cddb_idx_file );
                        else if (!pc_unlink(cddb_idx_file))
                            DEBUG( REV, DBGLEV_INFO, "Unable to Delete CDDB file [%s]\n", cddb_idx_file );
                        else
                            DEBUG( REV, DBGLEV_INFO, "Deleted CDDB file [%s]\n", cddb_idx_file );
                    }
                    break;
                */

                default:
                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY;
                    Mesg.iData = (unsigned int)data;
                    pt->MessageQueue()->Push(Mesg);
                    break;
            }
            break;
        }

        case EVENT_KEY_RELEASE:
        {
            unsigned int keycode = (unsigned int)data;
            DEBUGP( REV, DBGLEV_INFO, "Key release %d\n", keycode );
            
            switch( keycode )
            {
                case IR_KEY_POWER:
                case KEY_POWER:
                    break;

                default:
                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY_RELEASE;
                    Mesg.iData = keycode;
                    pt->MessageQueue()->Push(Mesg);
                    break;
            }
            break;
        }

        case EVENT_STREAM_SET:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_STREAM_SET\n");
            return 1;

        case EVENT_STREAM_AUTOSET:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_STREAM_AUTOSET\n");
	        return 1;

        case EVENT_STREAM_PROGRESS:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_STREAM_PROGRESS\n");
            return 1;

        case EVENT_STREAM_END:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_STREAM_END\n");
            return 1;

        case EVENT_STREAM_FAIL:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_STREAM_FAIL\n");
            return 1;

        case EVENT_STREAM_PLAYING:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_STREAM_PLAYING\n");
            return 1;

        case EVENT_STREAM_PAUSED:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_STREAM_PAUSED\n");
            return 1;

        case EVENT_STREAM_STOPPED:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_STREAM_STOPPED\n");
            return 1;

        case EVENT_MEDIA_REMOVED:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_MEDIA_REMOVED\n");
            m_pUserInterface->NotifyMediaRemoved((int)data);  // needed?
            return 1;

        case EVENT_MEDIA_INSERTED:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_MEDIA_INSERTED\n");
            // Make sure that the CDDataSource knows that the tray is closed.
            // The user may have pushed the tray closed.
            m_pMiniCDMgr->TrayIsClosed();
            // This is a closed system, so a media insertion message means that the CD drive tray closed.
            // Check the CD data source to see if a CD is in the tray.
            if (m_pMiniCDMgr->GetMediaStatus(false) == ENOERR)
            {
                // There's a CD in there, so switch to CD mode.
                DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: CD Inserted\n");                
                // Stop playback and clear the playlist.
                m_pUserInterface->NotifyMediaInserted((int)data);
            }
            return 1;

        case EVENT_RESTORE_KICKSTART:
            // We want this message to be the equivalent of selecting 'Start Custom Restore'.
            // The function below will do this.
            m_pUserInterface->NotifyMediaInserted((int)data);
            break;
            
        case EVENT_CONTENT_UPDATE_BEGIN:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_CONTENT_UPDATE_BEGIN\n");
            return 1;

        case EVENT_CONTENT_UPDATE:
        {
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_CONTENT_UPDATE\n");
            break;
        }

        case EVENT_CONTENT_UPDATE_END:
        {
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_CONTENT_UPDATE_END\n");
            return 1;
        }   

        case EVENT_CONTENT_METADATA_UPDATE:
        {
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_CONTENT_METADATA_UPDATE\n");
            break;
        }

        case EVENT_CONTENT_METADATA_UPDATE_END:
            DEBUGP( REV, DBGLEV_INFO, "metadata update ended\n");
            return 1;

        case EVENT_CONTENT_UPDATE_ERROR:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_CONTENT_UPDATE_ERROR\n");
            return 1;

        case EVENT_RESTORE_CDDB:
        {
            CRestoreScreen* pRS = CRestoreScreen::GetInstance();

            PegThing *pt = NULL;
            pt->Presentation()->Add(pRS);

//            pRS->DoRestoreCDDB();
            pt->Presentation()->MoveFocusTree(pRS);
//            present.Remove(pRS);
            return 1;
        }

#ifndef NO_UPNP
		case EVENT_NETWORK_UP:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_NETWORK_UP %x\n",(int)data);
			break;
		case EVENT_NETWORK_DOWN:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_NETWORK_DOWN\n");
            break;
#endif  // NO_UPNP



#ifndef NO_UPNP
        case EVENT_IML_FOUND:
            DEBUGP( REV, DBGLEV_INFO, "Found FML\n");
            break;

        case EVENT_IML_CACHING_BEGIN:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_IML_CACHING_BEGIN\n");
            break;

        case EVENT_IML_CACHING_END:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_IML_CACHING_END\n");
            break;

        case EVENT_IML_BYEBYE:
            DEBUGP( REV, DBGLEV_TRACE, "RestoreEvent: EVENT_IML_BYEBYE\n");
            break;

        case QUERY_LIBRARY_RESULT:
            DEBUGP( REV, DBGLEV_INFO, "Received library query result\n");
            break;

        case QUERY_ARTISTS_RESULT:
        case QUERY_ALBUMS_RESULT:
        case QUERY_GENRES_RESULT:
        case QUERY_PLAYLISTS_RESULT:
            DEBUGP( REV, DBGLEV_INFO, "Received general query result\n");
            break;

        case QUERY_RADIO_STATIONS_RESULT:
            DEBUGP( REV, DBGLEV_INFO, "Received radio station query result\n");
            break;
#endif  // NO_UPNP

        default:
            return -1;
    };

    return -1;
}
