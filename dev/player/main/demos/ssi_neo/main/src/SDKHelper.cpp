#include <main/demos/ssi_neo/main/SDKHelper.h>
#include <main/demos/ssi_neo/main/AppSettings.h>
//#include <main/demos/ssi_neo/ui/SystemMessageScreen.h>
#include <main/demos/ssi_neo/ui/PlayerScreen.h>
//#include <main/demos/ssi_neo/ui/PlaylistConstraint.h>

#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <util/registry/Registry.h>
#include <content/metakitcontentmanager/MetakitContentManager.h>
#include <datasource/datasourcemanager/DataSourceManager.h>

static CMetakitContentManager* s_mcm = 0; 

// avoid weird static-init-order issues
void InitSDKHelper()
{
    s_mcm = (CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager(); 

}

// (epg,10/17/2001): TODO: this isn't outfitted for ssi land yet.
extern "C"
void EnterUSBMode()
{
    // stop playing
    CMediaPlayer::GetInstance()->Deconfigure();
    // remove any constraints
    //PlaylistConstraint::GetInstance()->Constrain();
    // clear the playlist
    CPlayManager::GetInstance()->GetPlaylist()->Clear();    
    // save settings
    IOutputStream* pOS;
    if ((pOS = CDataSourceManager::GetInstance()->OpenOutputStream(SAVE_SETTINGS_PATH)))
    {
        s_mcm->SaveStateToStream(pOS);
    }
    // bring up the SystemMessageScreen
    //CSystemMessageScreen::GetSystemMessageScreen()->ShowScreen(CPlayerScreen::GetPlayerScreen(), CSystemMessageScreen::USB_CONNECTED);
}

extern "C"
void ExitUSBMode()
{
    //CSystemMessageScreen::GetSystemMessageScreen()->HideScreen();
    CPlayManager::GetInstance()->RefreshAllContent( );
}
