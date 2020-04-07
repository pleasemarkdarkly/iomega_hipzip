# default.dcl: default configuration for Pogo portable hard drive player application program on dharma
# edwardm@fullplaymedia.com 8/10/01
# (c) Fullplay Media Systems

name ui
type pogo

requires peg_gui

compile Bitmaps.cpp Fonts.cpp PlayerScreen.cpp Screen.cpp Strings.cpp UI.cpp PEGUserInterface.cpp SetMenuScreen.cpp
compile BrowseMenuScreen.cpp SetupMenuScreen.cpp GenreMenuScreen.cpp PlaylistMenuScreen.cpp FolderMenuScreen.cpp
compile PlayOrderMenuScreen.cpp ToneMenuScreen.cpp TimeMenuScreen.cpp BacklightMenuScreen.cpp PlayerInfoMenuScreen.cpp SystemMessageScreen.cpp
compile DiskInfoMenuScreen.cpp BitrateMenuScreen.cpp InputSelectMenuScreen.cpp

compile DynamicMenuScreen.cpp MenuScreen.cpp ScrollingListScreen.cpp SplashScreen.cpp 
#compile Latin_Font_9.cpp # I think this one is moved to Fonts.cpp.

export UserInterface.h
export Bitmaps.h Fonts.h Keys.h Messages.h OnScreenMenu.h PlayerScreen.h PlayScreen.h Screen.h Strings.hpp UI.h PEGUserInterface.h
export BrowseMenuScreen.h SetupMenuScreen.h GenreMenuScreen.h PlaylistMenuScreen.h FolderMenuScreen.h PlayOrderMenuScreen.h ToneMenuScreen.h
export TimeMenuScreen.h BacklightMenuScreen.h PlayerInfoMenuScreen.h SystemMessageScreen.h
export MenuScreen.h ScrollingListScreen.h DynamicMenuScreen.h SetMenuScreen.h SplashScreen.h
export DiskInfoMenuScreen.h BitrateMenuScreen.h InputSelectMenuScreen.h