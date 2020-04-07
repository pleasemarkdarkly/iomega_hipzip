# default.dcl: basic configuration for rbuf support
# danc@iobjects.com 07/07/01
# (c) Interactive Objects

name rbuf
type util

requires debug_util eresult_util

export rbuf.h

link default.a

#header _rbuf.h start
#
#// shrinking this number will give a little performance boost
##define RBUF_NUM_READERS 3
#
#header _rbuf.h end