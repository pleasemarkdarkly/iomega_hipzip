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


compile adler32.c compress.c crc32.c deflate.c gzio.c
compile id3_field.cpp id3_field_binary.cpp id3_field_integer.cpp id3_field_string_ascii.cpp
compile id3_field_string_unicode.cpp id3_frame.cpp id3_frame_parse.cpp id3_frame_render.cpp
compile id3_header.cpp id3_header_frame.cpp id3_header_tag.cpp id3_int28.cpp id3_misc_support.cpp
compile id3_tag.cpp id3_tag_file.cpp id3_tag_find.cpp id3_tag_parse.cpp id3_tag_parse_lyrics3.cpp
compile id3_tag_parse_v1.cpp id3_tag_render.cpp id3_tag_sync.cpp infblock.c infcodes.c inffast.c  
compile inflate.c inftrees.c infutil.c uncompr.c uniwrap.cpp

arch adler32.o compress.o crc32.o deflate.o gzio.o
arch id3_field.o id3_field_binary.o id3_field_integer.o id3_field_string_ascii.o
arch id3_field_string_unicode.o id3_frame.o id3_frame_parse.o id3_frame_render.o
arch id3_header.o id3_header_frame.o id3_header_tag.o id3_int28.o id3_misc_support.o
arch id3_tag.o id3_tag_file.o id3_tag_find.o id3_tag_parse.o id3_tag_parse_lyrics3.o
arch id3_tag_parse_v1.o id3_tag_render.o id3_tag_sync.o infblock.o infcodes.o inffast.o  
arch inflate.o inftrees.o infutil.o uncompr.o uniwrap.o

dist include/id3_tag.h include/id3_error.h include/id3_externals.h include/id3_field.h
dist include/id3_frame.h include/id3_header.h include/id3_header_frame.h include/id3_header_tag.h
dist include/id3_int28.h include/id3_misc_support.h include/id3_tag.h include/id3_types.h
dist include/id3_version.h
dist default.a 