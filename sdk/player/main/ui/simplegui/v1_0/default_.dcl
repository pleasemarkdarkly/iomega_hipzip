# default.dcl: default configuration for simple GUI
# edwardm@iobjects.com 08/23/01
# (c) Interactive Objects

name simplegui
type ui

requires common_ui show_simplegui screen_simplegui
requires font_simplegui common_simplegui
requires textlabel_screenelem bitmap_screenelem
requires common_codec codecmanager_codec playmanager_core
requires common_content registry_util

export SimpleGUIUserInterface.h

compile SimpleGUIUserInterface.cpp

