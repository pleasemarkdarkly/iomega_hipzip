/* CCircIndex inline */

inline CCircIndex::CCircIndex(int seed, int limit) : m_limit(limit), m_value(seed) {}

inline CCircIndex::~CCircIndex() {}

inline int CCircIndex::Inc()
{
	if (++m_value > m_limit)
		m_value = 0;
	return m_value;
}

inline int CCircIndex::Dec()
{
	if (--m_value < 0)
		m_value = m_limit;
	return m_value;
}

inline int CCircIndex::Set(int value)
{
	m_value = value;
	return m_value;
}

inline int CCircIndex::Get()
{
	return m_value;
}

inline int CCircIndex::PastDistance(CCircIndex* past)
{
	int distance = m_value - past->Get();
	if (distance < 0)
		return -distance;
	return distance;
}

inline int CCircIndex::FutureDistance(CCircIndex* future)
{
	int distance = future->Get() - m_value;
	if (distance < 0)
		return -distance;
	return distance;
}

