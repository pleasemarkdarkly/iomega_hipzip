# default.dcl: default configuration for Pogo portable hard drive player application program on dharma
# edwardm@iobjects.com 8/10/01
# (c) Interactive Objects

name ui
type dj

requires peg_gui
# also requires the RECORD version of devs/audio

compile Bitmaps.cpp Fonts.cpp PlayerScreen.cpp Screen.cpp Strings.cpp UI.cpp PEGUserInterface.cpp
compile LibraryMenuScreen.cpp LibraryEntryMenuScreen.cpp SettingsMenuScreen.cpp PlaylistConstraint.cpp
compile PlayOrderMenuScreen.cpp TimeMenuScreen.cpp PlayerInfoMenuScreen.cpp
compile DynamicMenuScreen.cpp MenuScreen.cpp ScrollingListScreen.cpp SplashScreen.cpp QuickBrowseMenuScreen.cpp
compile CDTriageScreen.cpp SourceMenuScreen.cpp SystemToolsMenuScreen.cpp YesNoScreen.cpp
compile PlaylistSaveScreen.cpp BitrateMenuScreen.cpp InputGainMenuScreen.cpp
compile MultiString.cpp SystemMessageString.cpp MainMenuScreen.cpp VolumeMenuScreen.cpp
compile LineRecEventHandler.cpp PlaybackEventHandler.cpp LineInEventHandler.cpp
compile EditScreen.cpp AlertScreen.cpp InfoMenuScreen.cpp
compile EditIPScreen.cpp IPAddressString.cpp NetSettingsMenuScreen.cpp StaticSettingsMenuScreen.cpp
compile RestoreScreen.cpp PEGRestoreUserInterface.cpp RestoreMenuScreen.cpp RestoreOptionsMenuScreen.cpp
compile ProgressScreen.cpp AboutScreen.cpp HDInfoMenuScreen.cpp
compile CustomizeMenuScreen.cpp

export Bitmaps.h Fonts.h Keys.h Messages.h PlayerScreen.h Screen.h Strings.hpp UI.h PEGUserInterface.h
export LibraryMenuScreen.h LibraryEntryMenuScreen.h SettingsMenuScreen.h PlayOrderMenuScreen.h
export TimeMenuScreen.h PlayerInfoMenuScreen.h
export MenuScreen.h ScrollingListScreen.h DynamicMenuScreen.h SplashScreen.h PlaylistConstraint.h 
export QuickBrowseMenuScreen.h CDTriageScreen.h SourceMenuScreen.h SystemToolsMenuScreen.h YesNoScreen.h
export PlaylistSaveScreen.h BitrateMenuScreen.h InputGainMenuScreen.h
export MultiString.h SystemMessageString.h MainMenuScreen.h VolumeMenuScreen.h
export LineRecEventHandler.h PlaybackEventHandler.h
export LineInEventHandler.h Timers.h 
export EditScreen.h AlertScreen.h InfoMenuScreen.h
export EditIPScreen.h IPAddressString.h NetSettingsMenuScreen.h StaticSettingsMenuScreen.h
export RestoreScreen.h PEGRestoreUserInterface.h RestoreMenuScreen.h RestoreOptionsMenuScreen.h
export ProgressScreen.h PlaylistLoadEvents.h AboutScreen.h HDInfoMenuScreen.h
export CustomizeMenuScreen.h ContentDeleteEvents.h

build_flags -DLINE_RECORDER_ENABLED -DDISABLE_VOLUME_CONTROL