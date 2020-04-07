# default.dcl: default configuration for qsound filter
# danc@iobjects.com 07/26/01
# (c) Interactive Objects

name qsound
type filter

requires filter_datastream

export QSoundFilterKeys.h

compile QSoundFilter.cpp volksq2x.S volksq2xfilter.S
