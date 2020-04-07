///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// * Neither name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// $Revision: 1.2 $
// $Date: 2000/09/06 20:21:02 $

#ifndef GENLIB_UTIL_DBLLIST_H
#define GENLIB_UTIL_DBLLIST_H

#include <assert.h>
#include <util/upnp/genlib/genexception.h>
#include <util/upnp/genlib/miscexceptions.h>

// similar to xdlist<T>; except replacing templates by void *
//   to reduce code size

class dblList;

class dblListNode
{
public:
    void *data;

private:
    dblListNode* next;
    dblListNode* prev;
    friend class dblList;
};

// callbacks for dblList

// returns true if a == b
typedef bool (*CompareDblItems)( void* a, void *b );

// destroys item (i.e. calls destructor and frees its mem)
typedef void (*FreeDblItem)( void* item );

class dblList
{
public:
    // if itemDestructorPtr is NULL, free() is used
	//
	// throws OutOfMemoryException
    dblList( FreeDblItem itemDestructorPtr = NULL,
             CompareDblItems comparatorPtr = NULL );

    virtual ~dblList();

	// throws OutOfMemoryException
    void addAfterTail( void* item );

	// throws OutOfMemoryException
    void addBeforeHead( void* item );

	// throws OutOfMemoryException
    void addAfter( dblListNode* node, void* item );

	// throws OutOfMemoryException
    void addBefore( dblListNode* node, void* item );

    void insertAfter( dblListNode* node, dblListNode* newNode );
    void insertBefore( dblListNode* node, dblListNode* newNode );

    void *getHead();
    void *getTail();

    int length() const;

    dblListNode* getFirstItem();
    dblListNode* getLastItem();
    dblListNode* next( dblListNode* node );
    dblListNode* prev( dblListNode* node );

    void remove( dblListNode* node, bool freeNodeMemory = true );
    void removeAll( bool freeNodeMemory = true );

    dblListNode* find( void* item );

private:
    dblListNode* head;
    dblListNode* tail;
    int len;
    FreeDblItem destroyItem;
    CompareDblItems compareItems;
};

#endif /* GENLIB_UTIL_DBLLIST_H */
