// (epg,2/14/2002): extension of playerscreen to handle events while recording from line-in

#ifndef __PLAYBACK_EVENT_HANDLER_H
#define __PLAYBACK_EVENT_HANDLER_H

#include <main/ui/UI.h>

struct PegMessage;
class CPlayerScreen;
class CLibraryMenuScreen;
class CLibraryEntryMenuScreen;
class CDJPlayerState;

class CPlaybackEventHandler {
public:
    CPlaybackEventHandler();
    ~CPlaybackEventHandler();
    void InitPlayerScreenPtr(CPlayerScreen* s);
    // event dispatcher
    SIGNED DispatchEvent(const PegMessage &Mesg);
private:
    // playback event handlers
    SIGNED HandleKeyPlay(const PegMessage &Mesg);
    SIGNED HandleKeyPause(const PegMessage &Mesg);
    SIGNED HandleKeyStop(const PegMessage &Mesg);
    SIGNED HandleKeyExit(const PegMessage &Mesg);
    SIGNED HandleKeyFwd(const PegMessage &Mesg);
    SIGNED HandleKeyRew(const PegMessage &Mesg);
    SIGNED HandleKeyUp(const PegMessage &Mesg);
    SIGNED HandleKeyDown(const PegMessage &Mesg);
    SIGNED HandleKeyUp10(const PegMessage &Mesg);
    SIGNED HandleKeyDown10(const PegMessage &Mesg);
    SIGNED HandleKeyRight(const PegMessage &Mesg);
    SIGNED HandleKeySave(const PegMessage &Mesg);
    SIGNED HandleKeyClear(const PegMessage &Mesg);
    SIGNED HandleKeyDelete(const PegMessage &Mesg);
    SIGNED HandleKeyAdd(const PegMessage &Mesg);
    SIGNED HandleKeyMenu(const PegMessage &Mesg);
    SIGNED HandleKeyRecord(const PegMessage &Mesg);
    SIGNED HandleKeyGenre(const PegMessage &Mesg);
    SIGNED HandleKeyArtist(const PegMessage &Mesg);
    SIGNED HandleKeyAlbum(const PegMessage &Mesg);
    SIGNED HandleKeyPlaylist(const PegMessage &Mesg);
    SIGNED HandleKeyRadio(const PegMessage &Mesg);
    SIGNED HandleKeySource(const PegMessage &Mesg);
    SIGNED HandleKeyPlayMode(const PegMessage &Mesg);
    SIGNED HandleKeyInfo(const PegMessage &Mesg);
    SIGNED HandleKeyMute(const PegMessage &Mesg);
    SIGNED HandleKeyZoom(const PegMessage &Mesg);
    SIGNED HandleKeyFwdRelease(const PegMessage &Mesg);
    SIGNED HandleKeyRewRelease(const PegMessage &Mesg);
    SIGNED HandleMsgPlay(const PegMessage &Mesg);
    SIGNED HandleMsgStop(const PegMessage &Mesg);
    SIGNED HandleMsgNewTrack(const PegMessage &Mesg);
    SIGNED HandleMsgPlayMode(const PegMessage &Mesg);
    SIGNED HandleMsgSystemMessage(const PegMessage &Mesg);
    SIGNED HandleMsgRefreshMetadata(const PegMessage &Mesg);
    SIGNED HandleMsgMultipleMetadata(const PegMessage &Mesg);
    SIGNED HandleMsgCDTrayOpened(const PegMessage &Mesg);
    SIGNED HandleMsgCDTrayClosed(const PegMessage &Mesg);
    SIGNED HandleMsgMediaInserted(const PegMessage &Mesg);
    SIGNED HandleMsgMediaRemoved(const PegMessage &Mesg);
    SIGNED HandleMsgSetUIViewMode(const PegMessage &Mesg);
    SIGNED HandleMsgMusicSourceChanged(const PegMessage &Mesg);
    SIGNED HandleMsgPlaylistCleared(const PegMessage &Mesg);
    SIGNED HandleTimerScrollTitle(const PegMessage& Mesg);
    SIGNED HandleTimerScrollEnd(const PegMessage& Mesg);
    SIGNED HandleTrackProgress(const PegMessage& Mesg);
    SIGNED HandleTimerIR(const PegMessage &Mesg);
    SIGNED HandleTimerDoTrackChange(const PegMessage &Mesg);
    // friend pointers
    CPlayerScreen* m_pPS;
    CDJPlayerState* m_pDJPS;
    CLibraryMenuScreen* m_pLMS;
    CLibraryEntryMenuScreen* m_pLEMS;
    // Checks to see if the CD is currently browsable.
    bool CanBrowseCD();
};

#endif // __PLAYBACK_EVENT_HANDLER_H
