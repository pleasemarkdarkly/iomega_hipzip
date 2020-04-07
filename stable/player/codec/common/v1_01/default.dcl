# default.dcl: basic codec configuration
# danc@iobjects.com 07/10/01
# (c) Interactive Objects

name common
type codec

requires input_datastream rbuf_util registry_util eresult_util ident_util common_content

export Codec.h codec_vects.h codec_workspace.h

dist include/Codec.h include/codec_vects.h
