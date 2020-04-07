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

// $Revision: 1.4 $
// $Date: 2000/07/29 21:50:38 $

#ifndef XDLIST_H
#define XDLIST_H

#include <assert.h>
#include <util/upnp/genlib/genexception.h>
#include <util/upnp/genlib/miscexceptions.h>

template <class T>
class xdlist;

template <class T>
class xdlistNode
{
public:
    T data;
    
private:
    friend class xdlist<T>;
    xdlistNode<T>* next;
    xdlistNode<T>* prev;
};

template <class T>
class xdlist
{
public:
    xdlist();
    virtual ~xdlist();
    
    void addAfterTail( T x );
    void addBeforeHead( T x );
    void addAfter( xdlistNode<T>* node, T item );
    void addBefore( xdlistNode<T>* node, T item );
    void insertAfter( xdlistNode<T>* node, xdlistNode<T>* newNode );
    void insertBefore( xdlistNode<T>* node, xdlistNode<T>* newNode );
    
    T getHead();
    T getTail();
    int length() const;
    
    xdlistNode<T>* getFirstItem();
    xdlistNode<T>* getLastItem();
    xdlistNode<T>* next( xdlistNode<T>* node );
    xdlistNode<T>* prev( xdlistNode<T>* node );
    
    void remove( xdlistNode<T>* node, bool freeNodeMemory = true );
    
    xdlistNode<T>* find( T item );
    
private:
    int len;
    xdlistNode<T> *head, *tail;
};

//////////
template <class T>
xdlist<T>::xdlist()
{
    head = new xdlistNode<T>;
    tail = new xdlistNode<T>;
    if ( head == NULL || tail == NULL )
    {
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfMemoryException( "xdlist<T>::xdlist()" );
    }
    
    head->next = tail;
    head->prev = NULL;
    tail->prev = head;
    tail->next = NULL;
    
    len = 0;
}

template <class T>
xdlist<T>::~xdlist()
{
    xdlistNode<T>* node, *node2;
    
    node = head;
    while ( node != NULL )
    {
        node2 = node->next;
        delete node;
        node = node2;
    }

}

template <class T>
void xdlist<T>::addBeforeHead( T x )
{
    addAfter( head, x );
}

template <class T>
void xdlist<T>::addAfterTail( T x )
{
    addBefore( tail, x );
}

template <class T>
void xdlist<T>::addAfter( xdlistNode<T>* node, T item )
{
    assert( node != NULL );
    
    xdlistNode<T>* newNode = new xdlistNode<T>;
    if ( node == NULL )
    {
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfMemoryException( "xdlist<T>::addAfter()" );
    }
    newNode->data = item;
    
    insertAfter( node, newNode );
}

template <class T>
void xdlist<T>::addBefore( xdlistNode<T>* node, T item )
{
    assert( node != NULL );
    
    xdlistNode<T>* newNode = new xdlistNode<T>;
    if ( newNode == NULL )
    {
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfMemoryException( "xdlist<T>::addBefore()" );
    }
    newNode->data = item;
    
    insertBefore( node, newNode );
}

template <class T>
void xdlist<T>::insertAfter( xdlistNode<T>* node, xdlistNode<T>* newNode )
{
    assert( node != NULL );
    assert( newNode != NULL );
    
    newNode->next = node->next;
    newNode->prev = node;
    
    node->next->prev = newNode;
    node->next = newNode;
    
    len++;
}

template <class T>
void xdlist<T>::insertBefore( xdlistNode<T>* node, xdlistNode<T>* newNode )
{
    assert( node != NULL );
    assert( newNode != NULL );
    
    newNode->next = node;
    newNode->prev = node->prev;

    node->prev->next = newNode;
    node->prev = newNode;

    len++;
}

template <class T>
T xdlist<T>::getHead()
{
    if ( len == 0 )
    {
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( "xdlist<T>::getHead()" );
    }
    
    return head->next->data;
}

template <class T>
T xdlist<T>::getTail()
{
    if ( len == 0 )
    {
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( "xdlist<T>::getTail()" );
    }
    
    return tail->prev->data;
}

template <class T>
int xdlist<T>::length() const
{
    return len;
}

template <class T>
xdlistNode<T>* xdlist<T>::getFirstItem()
{
    return head->next == tail ? NULL : head->next;
}

template <class T>
xdlistNode<T>* xdlist<T>::getLastItem()
{
    return tail->prev == head ? NULL : tail->prev;
}

template <class T>
xdlistNode<T>* xdlist<T>::next( xdlistNode<T>* node )
{
    if ( len == 0 || node == NULL )
    {
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( "xdlist<T>::next()" );
    }

    return node->next == tail ? NULL : node->next;
}

template <class T>
xdlistNode<T>* xdlist<T>::prev( xdlistNode<T>* node )
{
    if ( len == 0 || node == NULL )
    {
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( "xdlist<T>::prev()" );
    }

    return node->prev == head ? NULL : node->prev;
}


template <class T>
void xdlist<T>::remove( xdlistNode<T>* node, bool freeNodeMemory )
{
    assert( node != NULL );
    
    node->prev->next = node->next;
    node->next->prev = node->prev;
    len--;
    
    if ( freeNodeMemory )
    {
        delete node;
    }
}

template <class T>
xdlistNode<T>* xdlist<T>::find( T item )
{
    xdlistNode<T>* node;
    
    node = getFirstItem();
    while ( node != NULL )
    {
        if ( node->data == item )
        {
            return node;
        }
        node = next( node );
    }
    
    return NULL;
}

#endif /* XDLIST_H */
