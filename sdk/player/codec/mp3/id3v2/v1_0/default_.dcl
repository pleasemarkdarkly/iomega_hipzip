# default.dcl: mp3 id3v2 info configuration
# edwardm@iobjects.com 08/18/01
# (c) Interactive Objects

name id3v2_mp3
type codec

requires debug_util

export id3_tag.h
export id3_error.h id3_externals.h id3_field.h id3_frame.h id3_header.h
export id3_header_frame.h id3_header_tag.h id3_int28.h id3_misc_support.h id3_tag.h id3_types.h
export id3_version.h

link default.a
