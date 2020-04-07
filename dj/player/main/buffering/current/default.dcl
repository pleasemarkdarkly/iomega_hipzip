# default.dcl: default configuration for Pogo portable hard drive player application program on dharma
# edwardm@iobjects.com 8/10/01
# (c) Interactive Objects

name buffering
type dj

compile BufferWorker.cpp BufferDocument.cpp BufferFactory.cpp BufferReader.cpp
compile BufferWriter.cpp BufferAccountant.cpp BufferInStream.cpp
compile ReaderMsgPump.cpp WriterMsgPump.cpp
compile BufferInStreamImp.cpp DJConfig.cpp DocOrderAuthority.cpp
compile BufferDebug.cpp CircIndex.cpp 

export BufferAccountant.h BufferDocument.h BufferDocument.inl BufferFactory.h
export BufferInStream.h BufferTypes.h
export BufferWorker.h BufferWorker.inl CircIndex.h DJConfig.h ReaderMsgPump.h WriterMsgPump.h
export BufferReader.h BufferReader.inl BufferWriter.h
export BufferInStreamImp.h BufferInStreamImp.inl DocOrderAuthority.h
export BufferDebug.h
