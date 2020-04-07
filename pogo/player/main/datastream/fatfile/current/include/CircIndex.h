#ifndef __PIS_INCLUDE_CIRCINDEX_DEFINED__ 
#define __PIS_INCLUDE_CIRCINDEX_DEFINED__

class CCircIndex
{
public:
	CCircIndex(int seed, int limit);
	~CCircIndex();
	int Inc();
	int Dec();
	int Set(int value);
	int Get();
	int PastDistance(CCircIndex* past);
	int FutureDistance(CCircIndex* future);
private:
	int m_limit;
	int m_value;
};

#include "CircIndex.inl"

#endif // __PIS_INCLUDE_CIRCINDEX_DEFINED__
