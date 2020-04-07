// (epg,2/14/2002): extension of playerscreen to handle events while recording from line-in

#ifndef __LINE_REC_EVENT_HANDLER_H
#define __LINE_REC_EVENT_HANDLER_H

#define LINEREC_NEWREC_DELAY_SECS (4)

#include <main/ui/UI.h>

struct PegMessage;
class CPlayerScreen;

class CLineRecEventHandler {
public:
    CLineRecEventHandler();
    ~CLineRecEventHandler();
    void InitPlayerScreenPtr(CPlayerScreen* s);
    // key event dispatcher
    SIGNED DispatchEvent(const PegMessage &Mesg);
    // same as hitting stop button while in this mode.
    void StopRecording();
    // cancel active recording, deleting the in-process file.
    void CancelRecording();
    // same as hitting pause button
    void PauseRecording();
    // unpause the recording
    void ResumeRecording();
    // stop recording b/c space is low
    void HandleSpaceLow();
    // stop recording the current file, move to the new track, and start recording.
    void JumpToNextFile();
private:
    void DisplayGainNotification();
    // recording event handlers
    SIGNED         HandleKeyPlay(const PegMessage &Mesg);
    SIGNED         HandleKeyStop(const PegMessage &Mesg);
    SIGNED         HandleKeyPause(const PegMessage &Mesg);
    SIGNED         HandleKeyUp(const PegMessage &Mesg);
    SIGNED         HandleKeyDown(const PegMessage &Mesg);
    SIGNED         HandleKeyZoom(const PegMessage &Mesg);
    SIGNED         HandleTrackProgress(const PegMessage& Mesg);
    SIGNED         HandleKeyFwdRelease(const PegMessage& Mesg);
    SIGNED         HandleMsgSystemMessage(const PegMessage &Mesg);
    SIGNED         HandleMsgClipDetected(const PegMessage &Mesg);
    SIGNED         HandleMsgSetUIViewMode(const PegMessage &Mesg);
    SIGNED         HandleMsgMusicSourceChanged(const PegMessage &Mesg);
    SIGNED         HandleKeyMenu(const PegMessage &Mesg);
    SIGNED         HandleKeyNotAvailable(const PegMessage &Mesg);
    CPlayerScreen* m_pPS;

    int m_iFwdCount;
    int m_iSecondsElapsed;
};

#endif // __LINE_REC_EVENT_HANDLER_H
