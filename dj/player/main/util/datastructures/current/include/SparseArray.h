//
// SparseArray.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef SPARSEARRAY_H_
#define SPARSEARRAY_H_

#include <util/datastructures/SimpleList.h>
#include <util/datastructures/SimpleVector.h>

template<class DataType>
class SparseArray
{
public:

    //! Construct a new, empty vector.
	SparseArray(int iSize = 0)
        : m_iSize(iSize),
        m_iFilledSize(0)
	{
	}

    ~SparseArray()
        { Clear(); }

    //! Erase the nodes but don't delete the data.
    void Clear();

	//! Return the upper range for the array.
    //! This is not necessarily the number of filled blocks.
	int Size() const
	{
		return m_iSize;
	}

	//! Return the number of filled blocks in the list.
	int FilledSize() const
	{
		return m_iFilledSize;
	}

	//! Returns true if there are no items in the array, false otherwise.
	bool IsEmpty() const
	{
		return (m_iFilledSize == 0);
	}

    //! Adds the given array to the sparse array, starting at the given offset.
    void AddValues(SimpleVector<DataType>& svValues, int iStartIndex);

    //! Removes values from the given range from the sparse vector.
    void RemoveValues(int iStartIndex, int iItemCount);

    //! Removes values from the given range from the sparse vector.
    //! Removed values are added to a result vector, in case more memory management is needed.
    void RemoveValues(SimpleVector<DataType>& svValues, int iStartIndex, int iItemCount);

    //! Fills a vector with data in the sparse array.
    //! \param svValues A vector to hold the results.
    //! \param iStartIndex The starting index in the sparse array to get values for.
    //! \param iItemCount The number of items to get.
    //! \param dtNoValue The value to insert into the result vector if there is no data at an
    //! index in the sparse array.
    //! \retval The number of items in the result vector that have an actual value.
    int GetValues(SimpleVector<DataType>& svValues, int iStartIndex, int iItemCount, DataType dtNoValue);

    //! Fills a vector with all the data in the sparse array.
    //! This is useful when you need to delete all items in the array before clearing.
    void GetFilledValues(SimpleVector<DataType>& svValues);

    //! Fills a vector with the ranges of the filled indices in the sparse array.
    //! The format is { start index 1, size 1, start index 2, size 2, ... }
    void GetFilledRanges(SimpleVector<int>& svRanges);

    void Print() const;
    void PrintRanges() const;

private:

typedef SimpleVector<DataType> SADataVector;

typedef struct sa_value_node_s
{
    int             iStartIndex;
    SADataVector    svData;
} sa_value_node_t;

typedef SimpleList<sa_value_node_t*> SADataList;

    unsigned int    m_iSize;
    unsigned int    m_iFilledSize;

    SADataList      m_slData;
};









//! Erase the nodes but don't delete the data.
template<class DataType>
void
SparseArray<DataType>::Clear()
{
    while (!m_slData.IsEmpty())
    {
        delete m_slData.PopFront();
    }
}

//! Adds the given array to the sparse array, starting at the given offset.
template<class DataType>
void
SparseArray<DataType>::AddValues(SimpleVector<DataType>& svValues, int iStartIndex)
{
    SimpleListIterator<sa_value_node_t*> itNext = m_slData.GetHead();
    SimpleListIterator<sa_value_node_t*> itPrev = m_slData.GetEnd();
    while ((itNext != m_slData.GetEnd()) && ((*itNext)->iStartIndex < iStartIndex))
    {
        itPrev = itNext;
        ++itNext;
    }
    while ((itNext != m_slData.GetEnd()) && ((*itNext)->iStartIndex + (*itNext)->svData.Size() <= iStartIndex + svValues.Size()))
    {
        SimpleListIterator<sa_value_node_t*> itTemp = itNext;
        ++itNext;
        m_iFilledSize -= (*itTemp)->svData.Size();
        delete *itTemp;
        m_slData.Remove(itTemp);
    }

    sa_value_node_t* pValueNode = 0;
    if (itPrev != m_slData.GetEnd())
    {
        int iOverlapIndexStart = iStartIndex - (*itPrev)->iStartIndex;
        if (iOverlapIndexStart < (*itPrev)->svData.Size())
        {
            // The previous node's indices overlap with the new range, so overwrite the
            // overlapping values in the previous node and append the rest.
            int j = 0;
            for (int i = iOverlapIndexStart; i < (*itPrev)->svData.Size(); ++i)
            {
                (*itPrev)->svData[i] = svValues[j++];
            }
            m_iFilledSize += svValues.Size() - j;
            while (j < svValues.Size())
                (*itPrev)->svData.PushBack(svValues[j++]);
            pValueNode = *itPrev;
        }
        else if (iOverlapIndexStart == (*itPrev)->svData.Size())
        {
            // The last index of the previous node is one less than the start index of the
            // new range, so just append the values.
            for (int j = 0; j < svValues.Size(); ++j)
                (*itPrev)->svData.PushBack(svValues[j]);
            pValueNode = *itPrev;
            m_iFilledSize += svValues.Size();
        }
        else
        {
            // The previous node's indices don't overlap with the new range, so just
            // add a new node after the previous one.
            pValueNode = new sa_value_node_t;
            pValueNode->iStartIndex = iStartIndex;
            pValueNode->svData = svValues;
            m_slData.Insert(pValueNode, itPrev);
            m_iFilledSize += svValues.Size();
        }
    }
    else
    {
        // No previous node was found, so insert a new node at the beginning of the list.
        pValueNode = new sa_value_node_t;
        pValueNode->iStartIndex = iStartIndex;
        pValueNode->svData = svValues;
        m_slData.PushFront(pValueNode);
        m_iFilledSize += svValues.Size();
    }

    if ((itNext != m_slData.GetEnd()) && ((*itNext)->iStartIndex <= (pValueNode->iStartIndex + pValueNode->svData.Size())))
    {
        // The next node overlaps the new one, so append the non-overlapping data to the new node
        // and remove the next node from the list.
        int iAppendIndexStart =  pValueNode->iStartIndex + pValueNode->svData.Size() - (*itNext)->iStartIndex;
        m_iFilledSize -= iAppendIndexStart;
        for (int i = iAppendIndexStart; i < (*itNext)->svData.Size(); ++i)
            pValueNode->svData.PushBack((*itNext)->svData[i]);
        delete *itNext;
        m_slData.Remove(itNext);
    }
}


//! Removes values from the given range from the sparse vector.
template<class DataType>
void
SparseArray<DataType>::RemoveValues(int iStartIndex, int iItemCount)
{
    int iEndIndex = iStartIndex + iItemCount - 1;

    // Find the first node that has data we want to remove.
    SimpleListIterator<sa_value_node_t*> it = m_slData.GetHead();
    while ((it != m_slData.GetEnd()) && (iEndIndex >= (*it)->iStartIndex))
    {
        if ((*it)->iStartIndex >= iStartIndex)
        {
            if ((*it)->iStartIndex + (*it)->svData.Size() - 1 <= iEndIndex)
            {
                // The indices of this node fall entirely within the range to be deleted, so
                // remove the node from the list.
                SimpleListIterator<sa_value_node_t*> itTemp = it;
                ++it;
                m_iFilledSize -= (*itTemp)->svData.Size();
                delete *itTemp;
                m_slData.Remove(itTemp);
                continue;
            }
            else if ((*it)->iStartIndex <= iEndIndex)
            {
                // Truncate the front of the data block.
                int iRemove = iEndIndex - (*it)->iStartIndex + 1;
                m_iFilledSize -= iRemove;
                (*it)->iStartIndex += iRemove;
                (*it)->svData.Remove(0, iRemove);
            }
        }
        else if ((*it)->iStartIndex + (*it)->svData.Size() - 1 >= iStartIndex)
        {
            if ((*it)->iStartIndex + (*it)->svData.Size() - 1 <= iEndIndex)
            {
                // Truncate the end of the data block.
                int iRemove = (*it)->svData.Size() - iStartIndex + (*it)->iStartIndex;
                m_iFilledSize -= iRemove;
                (*it)->svData.Remove((*it)->svData.Size() - iRemove, iRemove);
            }
            else
            {
                // Break the node up into two separate chunks.
                sa_value_node_t* pValueNode = new sa_value_node_t;
                pValueNode->iStartIndex = iStartIndex + iItemCount;
                m_slData.Insert(pValueNode, it);
                m_iFilledSize -= (iStartIndex + iItemCount - 1);

                // Transfer the data from the end of the old node to the beginning of the new node.
                for (int i = iEndIndex + 1 - (*it)->iStartIndex; i < (*it)->svData.Size(); ++i)
                    pValueNode->svData.PushBack((*it)->svData[i]);
                int iRemove = (*it)->svData.Size() - iStartIndex + (*it)->iStartIndex;
                (*it)->svData.Remove((*it)->svData.Size() - iRemove, iRemove);
                ++it;
            }
        }
        ++it;
    }
}

//! Removes values from the given range from the sparse vector.
//! Removed values are added to a result vector, in case more memory management is needed.
template<class DataType>
void
SparseArray<DataType>::RemoveValues(SimpleVector<DataType>& svValues, int iStartIndex, int iItemCount)
{
#if 0
    int iEndIndex = iStartIndex + iItemCount - 1;

    // Find the first node that has data we want to remove.
    SimpleListIterator<sa_value_node_t*> it = m_slData.GetHead();
    while ((it != m_slData.GetEnd()) && (iEndIndex < (*it)->iStartIndex))
    {
        if ((*it)->iStartIndex > iStartIndex)
        {
            if ((*it)->iStartIndex + (*it)->svData.Size() <= iEndIndex)
            {
                // The indices of this node fall entirely within the range to be deleted, so
                // remove the node from the list.

            }
        }
        ++it;
    }
#endif    
}


template<class DataType>
int
SparseArray<DataType>::GetValues(SimpleVector<DataType>& svValues, int iStartIndex, int iItemCount, DataType dtNoValue)
{
    int iEndIndex = iStartIndex + iItemCount;
    int iFillCount = 0;

    // Find the first node that has data we want.
    SimpleListIterator<sa_value_node_t*> it = m_slData.GetHead();
    while ((it != m_slData.GetEnd()) && ((*it)->iStartIndex + (*it)->svData.Size() - 1 < iStartIndex))
        ++it;

    svValues.SetBlocks(svValues.Size() + iItemCount);
    int iCurrentIndex = iStartIndex;
    while (it != m_slData.GetEnd() && ((*it)->iStartIndex < iEndIndex))
    {
        // Fill up the items before the start of the next node with no value indicators.
        while (iCurrentIndex < (*it)->iStartIndex)
        {
            svValues.PushBack(dtNoValue);
            ++iCurrentIndex;
        }
        // Add items from this node to the result list.
        for (int i = iCurrentIndex - (*it)->iStartIndex; (i < (*it)->svData.Size()) && (iCurrentIndex < iEndIndex); ++i)
        {
            svValues.PushBack((*it)->svData[i]);
            ++iCurrentIndex; ++iFillCount;
        }
        ++it;
    }

    // Pad the leftover indices with no value indicators.
    while (iCurrentIndex++ < iEndIndex)
        svValues.PushBack(dtNoValue);

    return iFillCount;
}

//! Fills a vector with all the data in the sparse array.
//! This is useful when you need to delete all items in the array before clearing.
template<class DataType>
void
SparseArray<DataType>::GetFilledValues(SimpleVector<DataType>& svValues)
{
    for (SimpleListIterator<sa_value_node_t*> it = m_slData.GetHead(); it != m_slData.GetEnd(); ++it)
        svValues.Append((*it)->svData);
}

//! Fills a vector with the ranges of the filled indices in the sparse array.
//! The format is { start index 1, size 1, start index 2, size 2, ... }
template<class DataType>
void
SparseArray<DataType>::GetFilledRanges(SimpleVector<int>& svRanges)
{
    for (SimpleListIterator<sa_value_node_t*> it = m_slData.GetHead(); it != m_slData.GetEnd(); ++it)
    {
        svRanges.PushBack((*it)->iStartIndex);
        svRanges.PushBack((*it)->svData.Size());
    }
}


template<class DataType>
void
SparseArray<DataType>::Print() const
{
    SimpleListIterator<sa_value_node_t*> it;
    diag_printf("Filled size: %d\n", m_iFilledSize);
    for (it = m_slData.GetHead(); it != m_slData.GetEnd(); ++it)
        for (int i = 0; i < (*it)->svData.Size(); ++i)
            diag_printf("%d: %d\n", i + (*it)->iStartIndex, (*it)->svData[i]);
    diag_printf("\n");
}

template<class DataType>
void
SparseArray<DataType>::PrintRanges() const
{
    SimpleListIterator<sa_value_node_t*> it;
    diag_printf("Filled size: %d\n", m_iFilledSize);
    int i = 1;
    for (it = m_slData.GetHead(); it != m_slData.GetEnd(); ++it, ++i)
        diag_printf("Block %d: %d - %d\n", i, (*it)->iStartIndex, (*it)->iStartIndex + (*it)->svData.Size());
    diag_printf("\n");
}


#endif	// SPARSEARRAY_H_
