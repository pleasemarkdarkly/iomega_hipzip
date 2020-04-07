# default.dcl: default configuration for templated data structures
# edwardm@fullplaymedia.com 7/07/01

name filenamestore
type pogoutil

export FileNameStore.h FileNameStore.inl

compile FileNameStore.cpp

dist include/FileNameStore.h 