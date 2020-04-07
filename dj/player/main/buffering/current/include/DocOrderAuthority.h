#ifndef __DOC_ORDER_AUTHORITY__
#define __DOC_ORDER_AUTHORITY__

#include <util/datastructures/SimpleList.h>
#include <main/buffering/BufferTypes.h>

class IMediaContentRecord;

// report on the order of things to buffer.  highly project specific.
class CDocOrderAuthority 
{
public:
    CDocOrderAuthority();
    ~CDocOrderAuthority();

    // report on the ordering of documents to buffer
    void GetOrdering(StringList* plstUrls, int nBehind, int nAhead, IMediaContentRecord* mcr);

private:
    void GetDJOrdering(StringList* plstUrls, int nBehind, int nAhead, IMediaContentRecord* mcr);
};

#endif // __DOC_ORDER_AUTHORITY__
