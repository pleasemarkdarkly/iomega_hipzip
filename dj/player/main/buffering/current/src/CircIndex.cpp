#include <util/debug/debug.h>
#include <main/buffering/CircIndex.h>

/* CCircIndex */

CCircIndex::CCircIndex(int seed, int limit) : m_limit(limit), m_value(seed) 
{
}

CCircIndex::~CCircIndex() {}

int CCircIndex::Inc()
{
    if (++m_value > m_limit)
		m_value = 0;
	return m_value;
}

int CCircIndex::Dec()
{
  	if (--m_value < 0)
		m_value = m_limit;
  	return m_value;
}

int CCircIndex::Set(int value)
{
	m_value = value;
	return m_value;
}

int CCircIndex::Get()
{
	return m_value;
}

int CCircIndex::PastDistance(CCircIndex* past)
{
	int distance = m_value - past->Get();
	if (distance < 0)
		return -distance;
	return distance;
}

int CCircIndex::FutureDistance(CCircIndex* future)
{
	int distance = future->Get() - m_value;
	if (distance < 0)
		return -distance;
	return distance;
}
