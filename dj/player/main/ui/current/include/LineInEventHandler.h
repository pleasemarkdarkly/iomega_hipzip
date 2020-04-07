// (epg,2/14/2002): extension of playerscreen to handle events while recording from line-in

#ifndef __LINE_IN_EVENT_HANDLER_H
#define __LINE_IN_EVENT_HANDLER_H

#include <main/ui/UI.h>
#include <playlist/common/Playlist.h>
#include <main/ui/TimeMenuScreen.h>

//#define IDLE_CODER_PICKLE_BYTES (1024*48)

struct PegMessage;
class CPlayerScreen;

class CLineInEventHandler {
public:
    CLineInEventHandler();
    ~CLineInEventHandler();
    void InitPlayerScreenPtr(CPlayerScreen* ps);
    // event dispatcher
    SIGNED DispatchEvent(const PegMessage &Mesg);
    // jump to the pass through track, a simulated track that acts as a placeholder in the playlist for passthrough mode.
    void SetPassthroughTrack(bool bHideMenus = true);
    // save the current playmode locally and throttle the global mode to NORMAL
    void SaveAndNormalizePlayMode();
    // save the current time mode locally and throttle the global mode to NORMAL
    void SaveAndNormalizeTimeViewMode();
    // restore the global playmode context to whatever was pushed in.
    void RestorePlayMode();
    // restore the global time mode context to whatever was pushed in.
    void RestoreTimeViewMode();
    // turns on and off passthrough fiq
    void EnterPassthroughMode(bool bHideMenus = true);
    void ExitPassthroughMode();
    // silence and restore passthrough fiq
    void MutePassthroughMode();
    void UnMutePassthroughMode();
    // pause the idle coder and save-off its state from SRAM
    void StopIdleCoder();
    // restore the idle coder's state, and resume its operations.
    void ResumeIdleCoder();
private:
    void SetLastPlaylistEntry();
    void SetFirstPlaylistEntry();

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
    SIGNED HandleKeyNext(const PegMessage &Mesg);
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
    SIGNED HandleKeyZoom(const PegMessage &Mesg);
    SIGNED HandleKeyFwdRelease(const PegMessage &Mesg);
    SIGNED HandleKeyRewRelease(const PegMessage &Mesg);
    SIGNED HandleKeyRecordRelease(const PegMessage &Mesg);
    SIGNED HandleMsgPlay(const PegMessage &Mesg);
    SIGNED HandleMsgStop(const PegMessage &Mesg);
    SIGNED HandleMsgNewTrack(const PegMessage &Mesg);
    SIGNED HandleMsgSystemMessage(const PegMessage &Mesg);
    SIGNED HandleMsgRefreshMetadata(const PegMessage &Mesg);
    SIGNED HandleMsgMultipleMetadata(const PegMessage &Mesg);
    SIGNED HandleMsgMediaInserted(const PegMessage &Mesg);
    SIGNED HandleMsgMediaRemoved(const PegMessage &Mesg);
    SIGNED HandleMsgClipDetected(const PegMessage &Mesg);
    SIGNED HandleMsgSetUIViewMode(const PegMessage &Mesg);
    SIGNED HandleMsgMusicSourceChanged(const PegMessage &Mesg);
    SIGNED HandleTimerScrollTitle(const PegMessage& Mesg);
    SIGNED HandleTimerScrollEnd(const PegMessage& Mesg);
    SIGNED HandleTrackProgress(const PegMessage& Mesg);
    SIGNED HandleTimerIR(const PegMessage &Mesg);
    SIGNED HandleTimerDoTrackChange(const PegMessage &Mesg);
    // record the start of a double-click, and notify the user to press again.
    void PrimeRecordDoubleClick();
    // notify the user that the gain has been changed.
    void DisplayGainNotification();

    // friend pointer
    CPlayerScreen* m_pPS;

    // state specific to line in mode
    bool m_bPassthroughMode;
    bool m_bIgnoreNewTrack;
    int m_nReRecordPrime;
    int m_nReRecordPrimeTimeout;
    // stored global playlist mode
    IPlaylist::PlaylistMode m_eGlobalPlaylistMode;
    // stored global time mode
    // dvb (07/23/02) taken out for now.  leave in since it might come back
    //CTimeMenuScreen::TimeViewMode m_eGlobalTimeViewMode;

    // Mimic NT/FWD and PT/REW behaviour of the PS
    int m_iRwdCount;
    int m_iFwdCount;
};

#endif // __LINE_IN_EVENT_HANDLER_H
