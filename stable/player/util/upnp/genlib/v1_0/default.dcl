# default.dcl: default configuration for upngenlib interface
# robl@iobjects.com 10/16/01
# (c) Interactive Objects

name upnpgenlib
type util

build_flags -DINCLUDE_CLIENT_APIS

export charreader.h charwriter.h client_table.h dbllist.h fileexceptions.h genexception.h gmtdate.h
export http_client.h interrupts.h memreader.h miniserver.h miscexceptions.h
export mystring.h netall.h netexception.h netreader.h outofmemoryexception.h parseutil2.h
export readwrite.h scheduler.h server.h service_table.h statuscodes.h timer_thread.h tokenizer.h
export tpool.h urlconfig.h util.h utilall.h xdlist.h xstring.h noexceptions.h encode.h

compile client_table.cpp dbllist.cpp genexception.cpp gmtdate.cpp http_client.cpp interrupts.cpp
compile memreader.cpp miniserver.cpp mystring.c netreader.cpp 
compile parseutil2.cpp readwrite.cpp scheduler.cpp server.cpp service_table.cpp statuscodes.cpp timer_thread.cpp
compile tokenizer.cpp tpool.cpp urlconfig.cpp xstring.cpp encode.cpp

