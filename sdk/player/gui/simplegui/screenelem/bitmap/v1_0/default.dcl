# default.dcl: default configuration for serial UI
# danc@iobjects.com 07/28/01
# (c) Interactive Objects

name bitmap
type screenelem

export Bitmap.h

compile Bitmap.cpp BitmapData.cpp

arch Bitmap.o BitmapData.o

dist include/Bitmap.h default.a