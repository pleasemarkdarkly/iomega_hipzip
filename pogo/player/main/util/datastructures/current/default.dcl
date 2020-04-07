# default.dcl: default configuration for templated data structures
# edwardm@fullplaymedia.com 7/07/01

name datastructures
type pogoutil

export SortList.h SortList.inl

compile SortList.cpp

dist include/SortList.h 