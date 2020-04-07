//
// SortList.h
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef SORTLIST_H_
#define SORTLIST_H_

#include <util/datastructures/SimpleList.h>

//! The SortList is a template class for generically storing a doubly-linked list of items.
//! Its content can be traversed by the use of iterators.
template<class DataType> 
class SortList : public SimpleList<DataType>
{
public:
    typedef bool (SortListLessThanFunction)(void* left, void* right);

    SortList() : SimpleList<DataType>() {}

    SortList(const SortList& sl) : SimpleList<DataType>(sl) {}

    ~SortList() {}

    void Sort(SortListLessThanFunction lt) {
        MergeSort(lt);
    }

private:
    void MergeSort(SortListLessThanFunction lt);
};

#include <main/util/datastructures/SortList.inl>

#endif	// SORTLIST_H_
