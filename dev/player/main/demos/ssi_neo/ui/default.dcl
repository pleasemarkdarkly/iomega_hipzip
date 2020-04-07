# default.dcl: default configuration for hd, cd, and cf demo program on dharma
# edwardm@iobjects.com 8/10/01
# (c) Interactive Objects

name ui
type ssi_neo

requires peg_gui

compile Bitmaps.cpp Fonts.cpp OnScreenMenu.cpp PlayerScreen.cpp Screen.cpp Strings.cpp UI.cpp Latin_Font_12.cpp DACManager.cpp PEGUserInterface.cpp
compile BrowseMenuScreen.cpp BrowseArtistsMenuScreen.cpp BrowseArtistsAlbumsMenuScreen.cpp BrowseArtistsAlbumsTitlesMenuScreen.cpp BrowseAlbumsMenuScreen.cpp
compile BrowseAlbumsTitlesMenuScreen.cpp BrowseTitlesMenuScreen.cpp BrowseGenresMenuScreen.cpp BrowseGenresTitlesMenuScreen.cpp BrowseFoldersMenuScreen.cpp 
compile BrowseRecordingsMenuScreen.cpp DynamicMenuScreen.cpp MenuScreen.cpp ScrollingListScreen.cpp

export Bitmaps.h DACManager.h Fonts.h Keys.h Messages.h OnScreenMenu.h PlayerScreen.h PlayScreen.h Screen.h Strings.hpp UI.h PEGUserInterface.h
export BrowseMenuScreen.h BrowseArtistsMenuScreen.h BrowseArtistsAlbumsMenuScreen.h BrowseArtistsAlbumsTitlesMenuScreen.h BrowseAlbumsMenuScreen.h BrowseAlbumsTitlesMenuScreen.h \
export BrowseTitlesMenuScreen.h BrowseGenresMenuScreen.h BrowseGenresTitlesMenuScreen.h BrowseFoldersMenuScreen.h BrowseRecordingsMenuScreen.h DynamicMenuScreen.h
export MenuScreen.h ScrollingListScreen.h