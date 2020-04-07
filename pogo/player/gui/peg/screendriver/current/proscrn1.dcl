# proscrn1.dcl: config for the proscrn1 peg screen driver
# danb@iobjects.com 11/09/01

name screendriver
type peg

requires lcd_dev peg_gui

compile proscrn1.cpp

export proscrn1.hpp