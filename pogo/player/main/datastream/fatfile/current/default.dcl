# default.dcl: default configuration for the pogoplaylist module
# ericg@fullplaymedia.com 10/15/01

name pogofatfile
type datastream

requires input_datastream fatfile_datastream

export BufferedFileInputStream.h BufferedFileInputStreamImp.h CacheMan.h CircIndex.h 
export Consumer.h ConsumerMsgPump.h FileCache.h Producer.h ProducerMsgPump.h CacheReferee.h
export CacheMan.inl FileCache.inl Consumer.inl BufferedFileInputStreamImp.inl
export BufferingConfig.h CircIndex.inl

compile BufferedFileInputStream.cpp BufferedFileInputStreamImp.cpp CacheMan.cpp 
compile Consumer.cpp ConsumerMsgPump.cpp FileCache.cpp Producer.cpp ProducerMsgPump.cpp CacheReferee.cpp

