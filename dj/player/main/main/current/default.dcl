# default.dcl: default configuration for Pogo portable hard drive player application program on dharma
# edwardm@iobjects.com 8/10/01
# (c) Interactive Objects

name main
type dj

requires eventq_util idlecoder_extras filecopier_extras ecos_support_dj
# also requires the RECORD version of devs/audio

compile main.cpp DJPlayerState.cpp DJEvents.cpp FatHelper.cpp IsoHelper.cpp
compile LED.cpp LEDStateManager.cpp Recording.cpp LineRecorder.cpp SpaceMgr.cpp
compile CDCache.cpp CDDirGen.cpp RestoreEvents.cpp MiniCDMgr.cpp
compile ProgressWatcher.cpp UpdateManager.cpp DJHelper.cpp

export AppSettings.h DJPlayerState.h EventTypes.h Events.h FatHelper.h IsoHelper.h 
export LED.h LEDStateManager.h Recording.h RecordingEvents.h LineRecorder.h SpaceMgr.h
export DJEvents.h RestoreEvents.h MiniCDMgr.h 
export ProgressWatcher.h UpdateManager.h DJHelper.h

