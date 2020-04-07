
#ifndef __BUFFER_TYPES__
#define __BUFFER_TYPES__

#include <util/datastructures/SimpleList.h>

typedef SimpleList<char*> StringList;
typedef SimpleListIterator<char*> StringListIterator;
typedef void SetIntFn(int);

class CBufferAccountant;
class CBufferCore;
class CBufferFactory;
class CBuffering;
class CBufferOutputStream;
class CBuffferThreading;
class CBufferWorker;
class CBufferDocument;

//! Enumeration for seek origin
enum eInputSeekPos
{
    SeekStart,
    SeekCurrent,
    SeekEnd,
};

#endif // __BUFFER_TYPES__
