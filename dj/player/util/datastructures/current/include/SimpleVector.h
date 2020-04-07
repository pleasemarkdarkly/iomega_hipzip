//
// SimpleVector.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef SIMPLEVECTOR_H_
#define SIMPLEVECTOR_H_

#include <stdlib.h>  // qsort
#include <string.h>  // memcpy

// If defined, then memcpy is used when growing the array, skipping the data's assignment operator.
#define FAST_COPY

//! The SimpleVector is a template class for generically storing an array of items.
//! Its content can be randomly accessed by index.
//! The array starts empty, but grows as items are added to it.
template<class DataType>
class SimpleVector
{
public:

    //! Construct a new, empty vector.
    //! \param blockSize The number of items to grow the array by at a time.
    //! \param size The number of items to allocate initially.
    SimpleVector(unsigned int blockSize = 5, unsigned int size = 0)
        : m_iBlockSize(blockSize),
        m_cBlocks(size),
        m_cFilledBlocks(0)
    {
        if (m_cBlocks)
            m_pDataVector = (DataType*)malloc(m_cBlocks * sizeof(DataType));
        else m_pDataVector = NULL;
    }

    //! Copy an existing vector.
    SimpleVector(const SimpleVector& sv)
        : m_iBlockSize(sv.m_iBlockSize),
        m_cFilledBlocks(sv.m_cFilledBlocks)
    {
        if (m_cFilledBlocks)
        {
            m_cBlocks = sv.m_cBlocks;
            m_pDataVector = (DataType*)malloc(m_cBlocks * sizeof(DataType));
            memcpy(m_pDataVector, sv.m_pDataVector, m_cFilledBlocks * sizeof(DataType));
        }
        else
        {
            m_cBlocks = 0;
            m_pDataVector = 0;
        }
    }

    ~SimpleVector()
    {
        free(m_pDataVector);
    }

    //! Copy an existing vector.
    SimpleVector& operator=(const SimpleVector& sv)
    {
        m_iBlockSize = sv.m_iBlockSize;
        m_cFilledBlocks = sv.m_cFilledBlocks;
        free(m_pDataVector);
        if (m_cFilledBlocks)
        {
            m_cBlocks = sv.m_cBlocks;
            m_pDataVector = (DataType*)malloc(m_cBlocks * sizeof(DataType));
            memcpy(m_pDataVector, sv.m_pDataVector, m_cFilledBlocks * sizeof(DataType));
        }
        else
        {
            m_cBlocks = 0;
            m_pDataVector = 0;
        }
        return *this;
    }

    //! Gets the array's block size (the number of units it will allocate when
    //! it needs to extend the array).
    unsigned int GetBlockSize() const
        { return m_iBlockSize; }

    //! Sets the array's block size (the number of units it will allocate when
    //! it needs to extend the array).
    void SetBlockSize(unsigned int blockSize)
        { m_iBlockSize = blockSize; }

    //! Add a record to the end of the vector.
    //! If the vector is full, then grow it by the block size.
    void PushBack(DataType pData)
    {
        if (m_cFilledBlocks == m_cBlocks)
        {
            // The array is full, so copy the data to a temporary buffer,
            // allocate new memory for the array, and copy the data back.
            m_cBlocks += m_iBlockSize;
            m_pDataVector = (DataType*)realloc((void*)m_pDataVector, m_cBlocks * sizeof(DataType));
        }
        m_pDataVector[m_cFilledBlocks++] = pData;
    }

    //! Remove a record from the end of the vector
    //! Popping from an empty vector has undefined results.
    DataType PopBack()
    {
        return m_pDataVector[--m_cFilledBlocks];
    }

    //! Remove a record from the specified position in the vector.
    //! This is an inefficient operation and should be used sparingly.
    DataType Remove(unsigned int index)
    {
        DataType temp = m_pDataVector[index];
#ifdef FAST_COPY
        memmove(&(m_pDataVector[index]), &(m_pDataVector[index + 1]), (m_cFilledBlocks - index - 1) * sizeof(DataType));
#else
        for (unsigned int i = index; i < m_cFilledBlocks - 1; ++i)
            m_pDataVector[i] = m_pDataVector[i + 1];
#endif
        --m_cFilledBlocks;
        return temp;
    }

    //! Remove a range of records from the specified position in the vector.
    //! This is an inefficient operation and should be used sparingly.
    DataType Remove(unsigned int index, unsigned int size)
    {
        DataType temp = m_pDataVector[index];
#ifdef FAST_COPY
        memmove(&(m_pDataVector[index]), &(m_pDataVector[index + size]), (m_cFilledBlocks - size) * sizeof(DataType));
#else
        for (unsigned int i = index; i < m_cFilledBlocks - size; ++i)
            m_pDataVector[i] = m_pDataVector[i + size];
#endif
        m_cFilledBlocks -= size;
        return temp;
    }

    //! Clear the vector but don't delete the data.
    void Clear()
    {
        m_cFilledBlocks = 0;
    }

    //! Return the number of filled blocks in the list.
    int Size() const
    {
        return m_cFilledBlocks;
    }

    //! Set the number of allocated blocks.
    //! If less than the current size, then the array will be truncated.
    int SetBlocks(int size)
    {
        m_pDataVector = (DataType*)realloc((void*)m_pDataVector, size * sizeof(DataType));
        m_cBlocks = size;
    }

    //! Returns true if there are no items in the array, false otherwise.
    bool IsEmpty() const
    {
        return (m_cFilledBlocks == 0);
    }

    //! Returns a const reference to the item at the given index.
    const DataType& operator[](int pos) const
    {
        return m_pDataVector[pos];
    }

    //! Returns a reference to the item at the given index.
    DataType& operator[](int pos)
    {
        return m_pDataVector[pos];
    }

//! A CompareDataFunction should return true if a <= b, false otherwise.
typedef bool CompareDataFunction(const DataType& a, const DataType& b);

    //! Sort the vector in ascending order, based on the comparison function.
    void Sort(CompareDataFunction& compareDataFunction)
    {
        for (int i = 1; i < Size(); ++i)
        {
            for (int j = i; j > 0; --j)
                if (!compareDataFunction(m_pDataVector[j - 1], m_pDataVector[j]))
                {
                    DataType swap = m_pDataVector[j];
                    m_pDataVector[j] = m_pDataVector[j - 1];
                    m_pDataVector[j - 1] = swap;
                }
                else
                    break;
        }
    }

// A QCompareDataFunction should return 
//      < 0 if *a < *b, 
//        0 if *a = *b,
//      > 0 if *a > *b.
typedef int QCompareDataFunction(const void* a, const void* b);

    //! Sort the vector in ascending order, based on the comparison function.
    void QSort(QCompareDataFunction& qCompareDataFunction)
    {
        qsort( (void*)m_pDataVector,Size(),sizeof(DataType),qCompareDataFunction);
    }

    //! Searches for a matching value in the vector.
    //! Returns the index of the item if found, -1 otherwise.
    int Find(const DataType& data)
    {
        for (int i = 0; i < Size(); ++i)
            if (m_pDataVector[i] == data)
                return i;
        return -1;
    }

typedef int SortedFindCompareFunction(const DataType& a, const void* pCompareData);

    //! Searches for a matching value in the sorted vector.
    //! Returns the index of the item if found, -1 otherwise.
    int FindSorted(const void* pCompareData, SortedFindCompareFunction sortedFindCompareFunction)
    {
        int iFloor = 0;
        int iCeiling = Size() - 1;
        while (1)
        {
            if (iCeiling < iFloor)
                return -1;
            int iCompare = iFloor + (iCeiling - iFloor) / 2;
            int diff = sortedFindCompareFunction(m_pDataVector[iCompare],pCompareData);
            if (diff == 0)
                return iCompare;
            if (diff < 0)
                iCeiling = iCompare - 1;
            else 
                iFloor = iCompare + 1;
        }
    }

    //! Insert a record at the specified index in the vector.
    //! If the index is out-of-range, then false is returned.
    bool Insert(DataType pData, unsigned int index)
    {
        if (index > m_cFilledBlocks + 1)
            return false;

        if (m_cFilledBlocks == m_cBlocks)
        {
            // The array is full, so copy the data to a temporary buffer,
            // allocate new memory for the array, and copy the data back.
            m_cBlocks += m_iBlockSize;
            DataType* pNewVector = (DataType*)malloc(m_cBlocks * sizeof(DataType));
#ifdef FAST_COPY
            if (index > 0)
                memcpy(pNewVector, m_pDataVector, index * sizeof(DataType));
            pNewVector[index] = pData;
            if (index < m_cFilledBlocks)
                memcpy(&(pNewVector[index + 1]), &(m_pDataVector[index]), (m_cFilledBlocks - index + 1) * sizeof(DataType));
#else   // FAST_COPY
            for (int i = 0; i < index; ++i)
                pNewVector[i] = m_pDataVector[i];
            pNewVector[index] = pData;
            for (int i = index; i < m_cFilledBlocks; ++i)
                pNewVector[i + 1] = m_pDataVector[i];
#endif  // FAST_COPY
            ++m_cFilledBlocks;

            free(m_pDataVector);
            m_pDataVector = pNewVector;
        }
        else
        {
            for (unsigned int i = m_cFilledBlocks; i > index; --i)
                m_pDataVector[i] = m_pDataVector[i - 1];
            m_pDataVector[index] = pData;
            ++m_cFilledBlocks;
        }

        return true;
    }

    //! Insert a record in the sorted array in its proper position, based on the comparison function.
    bool Insert(DataType pData, CompareDataFunction& compareDataFunction)
    {
        for (int i = 0; i < Size(); ++i)
            if (compareDataFunction(pData, m_pDataVector[i]))
                return Insert(pData, i);
        PushBack(pData);
        return true;
    }

    //! Append the contents of one simple vector to the end of this one.
    void Append(const SimpleVector<DataType>& sv)
    {
        int iNeededBlocks = (sv.Size() - (m_cBlocks - m_cFilledBlocks)) / m_iBlockSize + 1;
        if (iNeededBlocks > 0)
        {
            m_cBlocks += (m_iBlockSize * iNeededBlocks);
            m_pDataVector = (DataType*)realloc((void*)m_pDataVector, m_cBlocks * sizeof(DataType));
        }
#ifdef FAST_COPY
        memcpy(&(m_pDataVector[m_cFilledBlocks]), sv.m_pDataVector, sv.Size() * sizeof(DataType));
#else   // FAST_COPY
        for (unsigned int i = 0; i < sv.Size(); ++i)
            m_pDataVector[m_cFilledBlocks + i] = sv.m_pDataVector[i];
#endif  // FAST_COPY
        m_cFilledBlocks += sv.Size();
    }

    //! Moves a record in the array to the new index, and pushes the records
    //! in between the two indices up or down accordingly.
    //! No range-checking is done, so make sure the indices are in bounds.
    void Move(int indexA, int indexB)
    {
        DataType temp = m_pDataVector[indexA];
        if (indexA > indexB)
        {
            for (int i = indexA; i > indexB; --i)
                m_pDataVector[i] = m_pDataVector[i - 1];
            m_pDataVector[indexB] = temp;
        }
        else
        {
            for (int i = indexA; i < indexB; ++i)
                m_pDataVector[i] = m_pDataVector[i + 1];
            m_pDataVector[indexB] = temp;
        }
    }

private:

    DataType*       m_pDataVector;
    unsigned int    m_iBlockSize;
    unsigned int    m_cBlocks;
    unsigned int    m_cFilledBlocks;

};



#endif	// SIMPLEVECTOR_H_
