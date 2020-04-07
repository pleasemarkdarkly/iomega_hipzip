//
// SimpleList.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef SIMPLELIST_H_
#define SIMPLELIST_H_


// The DataNode is used internally by the SimpleList and SimpleListIterator to
// store data.
template<class DataType>
struct DataNode
{
    DataType	dataType;
    DataNode*	pNext;
    DataNode*	pPrev;

    DataNode(DataType initDataType) : dataType(initDataType) { }
};

template<class DataType> class SimpleList;

//! The SimpleListIterator is used to traverse a SimpleList.
template<class DataType>
class SimpleListIterator
{
public:

    SimpleListIterator(DataNode<DataType>* pDataNode = 0)
        : m_pDataNode(pDataNode)
    {
    }
    ~SimpleListIterator()
    {
    }

    //! Get access to the data stored at this position in the list.
    DataType& operator*()
    {
        return m_pDataNode->dataType;
    }

    //! Test for equality.
    bool operator==(const SimpleListIterator<DataType>& rhs)
    {
        return (m_pDataNode == rhs.m_pDataNode);
    }
    //! Test for inequality.
    bool operator!=(const SimpleListIterator<DataType>& rhs)
    {
        return (m_pDataNode != rhs.m_pDataNode);
    }

    //! Advance the iterator by the given number of indices.
    //! The results are undefined if the end of the list is passed.
    SimpleListIterator<DataType>& operator+=(int rhs)
    {
        while (rhs-- > 0)
            m_pDataNode = m_pDataNode->pNext;
        return *this;
    }

    //! Copy the given iterator and advance it by the given number of indices.
    //! The results are undefined if the end of the list is passed.
    friend SimpleListIterator<DataType> operator+(const SimpleListIterator<DataType>& lhs, int rhs)
    {
        SimpleListIterator<DataType> it = lhs;
        it += rhs;
        return it;
    }

    //! Go to the next element in the list.
    SimpleListIterator<DataType>& operator++()
    {
        m_pDataNode = m_pDataNode->pNext;
        return *this;
    }

    //! Rewind the iterator by the given number of indices.
    //! The results are undefined if the end of the list is passed.
    SimpleListIterator<DataType>& operator-=(int rhs)
    {
        while (rhs-- > 0)
            m_pDataNode = m_pDataNode->pPrev;
        return *this;
    }

    //! Copy the given iterator and rewind it by the given number of indices.
    //! The results are undefined if the end of the list is passed.
    friend SimpleListIterator<DataType> operator-(const SimpleListIterator<DataType>& lhs, int rhs)
    {
        SimpleListIterator<DataType> it = lhs;
        it -= rhs;
        return it;
    }

    //! Go to the previous element in the list.
    SimpleListIterator<DataType>& operator--()
    {
        m_pDataNode = m_pDataNode->pPrev;
        return *this;
    }

private:

    DataNode<DataType>*	m_pDataNode;

friend class SimpleList<DataType>;
};

//! The SimpleList is a template class for generically storing a doubly-linked list of items.
//! Its content can be traversed by the use of iterators.
template<class DataType>
class SimpleList
{
public:

    SimpleList()
        : m_pHead(0),
        m_pTail(0),
        m_iSize(0)
    {
    }
    SimpleList(const SimpleList& sl)
        : m_pHead(0),
        m_pTail(0),
        m_iSize(0)
    {
        Append(sl);
    }

    ~SimpleList()
    {
        Clear();
    }

    //! Copy an existing list.
    SimpleList& operator=(const SimpleList& sl)
    {
        Clear();
        Append(sl);
    }

    //! Append the contents of the given list to the end of this list.
    SimpleList& Append(const SimpleList& sl)
    {
        for (SimpleListIterator<DataType> it = sl.GetHead(); it != sl.GetEnd(); ++it)
            PushBack(*it);
        return *this;
    }

    //! Add an element to the beginning of the list.
    void PushFront(DataType data)
    {
        DataNode<DataType>* pNode = new DataNode<DataType>(data);
        pNode->pNext = m_pHead;
        pNode->pPrev = 0;
        if (m_pHead)
            m_pHead->pPrev = pNode;
        else
            m_pTail = pNode;
        m_pHead = pNode;
        ++m_iSize;
    }

    //! Add an element to the end of the list.
    void PushBack(DataType data)
    {
        DataNode<DataType>* pNode = new DataNode<DataType>(data);
        pNode->pPrev = m_pTail;
        pNode->pNext = 0;
        if (m_pTail)
            m_pTail->pNext = pNode;
        else
            m_pHead = pNode;
        m_pTail = pNode;
        ++m_iSize;
    }

    //! Inserts the data type into the list at the zero-based index.
    //! The results are undefined if the index is out-of-range.
    void Insert(DataType data, int index)
    {
        if (index)
        {
            DataNode<DataType>* pTemp = m_pHead;
            while (--index)
                pTemp = pTemp->pNext;

            DataNode<DataType>* pNode = new DataNode<DataType>(data);
            pNode->pPrev = pTemp;
            pNode->pNext = pTemp->pNext;
            if (pTemp->pNext)
                pTemp->pNext->pPrev = pNode;
            else
                m_pTail = pNode;
            pTemp->pNext = pNode;
            ++m_iSize;
        }
        else
            PushFront(data);
    }

    //! Inserts the data type into the list after the given iterator.
    //! If the iterator is the end-of-list marker, then the data is appended to the list.
    void Insert(DataType data, SimpleListIterator<DataType>& it)
    {
        if (it != GetEnd())
        {
            DataNode<DataType>* pNode = new DataNode<DataType>(data);
            pNode->pPrev = it.m_pDataNode;
            pNode->pNext = it.m_pDataNode->pNext;
            if (it.m_pDataNode->pNext)
                it.m_pDataNode->pNext->pPrev = pNode;
            else
                m_pTail = pNode;
            it.m_pDataNode->pNext = pNode;
            ++m_iSize;
        }
        else
            PushBack(data);
    }

    //! Erase the nodes but don't delete the data.
    void Clear()
    {
        while (m_pHead)
        {
            DataNode<DataType>* pTemp = m_pHead->pNext;
            delete m_pHead;
            m_pHead = pTemp;
        }
        m_pTail = 0;
        m_iSize = 0;
    }

    //! Returns the number of nodes in the list.
    int Size() const
    {
        return m_iSize;
    }

    //! Returns true if there are no items in the list, false otherwise.
    bool IsEmpty() const
    {
        return (m_pHead == 0);
    }

    //! Pops the first item off the list and returns it.
    //! The results are undefined if the list is empty.
    DataType PopFront()
    {
        DataNode<DataType>* pTemp = m_pHead;
        if (m_pHead = m_pHead->pNext)
            m_pHead->pPrev = 0;
        else
            m_pTail = 0;
        DataType dataType = pTemp->dataType;
        delete pTemp;
        --m_iSize;
        return dataType;
    }

    //! Pops the last item off the list and returns it.
    //! The results are undefined if the list is empty.
    DataType PopBack()
    {
        DataNode<DataType>* pTemp = m_pTail;
        if (m_pTail = m_pTail->pPrev)
            m_pTail->pNext = 0;
        else
            m_pHead = 0;
        DataType dataType = pTemp->dataType;
        delete pTemp;
        --m_iSize;
        return dataType;
    }

    //! Removes a node from the list.
    //! The iterator is invalid after this operation.
    DataType Remove(SimpleListIterator<DataType> it)
    {
        DataNode<DataType>* pNode = it.m_pDataNode;
        if (pNode->pPrev)
            pNode->pPrev->pNext = pNode->pNext;
        else
            m_pHead = pNode->pNext;
        if (pNode->pNext)
            pNode->pNext->pPrev = pNode->pPrev;
        else
            m_pTail = pNode->pPrev;
        DataType dataType = pNode->dataType;
        delete pNode;
        --m_iSize;
        return dataType;
    }

    //! Return an iterator that points to the first element in the list.
    SimpleListIterator<DataType> GetHead() const
    {
        return SimpleListIterator<DataType>(m_pHead);
    }
    //! Return an iterator that points to the last element in the list.
    SimpleListIterator<DataType> GetTail() const
    {
        return SimpleListIterator<DataType>(m_pTail);
    }
    //! Return an iterator that points to an empty element.
    //! Use this to test if an iterator has passed the end of the list.
    SimpleListIterator<DataType> GetEnd() const
    {
        return SimpleListIterator<DataType>(0);
    }

protected:

    DataNode<DataType>* m_pHead;
    DataNode<DataType>* m_pTail;
    int m_iSize;

};


#endif	// SIMPLELIST_H_
