# default.dcl: default configuration for Pogo portable hard drive player application program on dharma
# edwardm@fullplaymedia.com 8/10/01
# (c) Fullplay Media Systems

name main
type pogo

requires eventq_util

build_flags -DINCREMENTAL_METAKIT -DENABLE_DAI_RECORD

compile main.cpp Events.cpp FatHelper.cpp usb_setup.c PlaylistConstraint.cpp Recorder.cpp Version.cpp ProgressWatcher.cpp

export FatHelper.h AppSettings.h usb_setup.h EventTypes.h Events.h Events.inl PlaylistConstraint.h Recorder.h Version.h KeyboardMap.h ProgressWatcher.h
