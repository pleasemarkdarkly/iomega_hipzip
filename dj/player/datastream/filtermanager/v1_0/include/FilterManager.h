//
// FilterManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef __FILTERMANAGER_H__
#define __FILTERMANAGER_H__

//
// fdecl
//
class CRegistry;
class IFilter;
typedef struct filter_table_s filter_table_t;

// CFilterManager
//! The filter manager provides a simple way for the media
//! player to locate and load available filters in the system.
//! The filters can then be incorporated into the playstream, and
//! the media player will take care of releasing them when it is
//! done with them. This is a singleton class.
class CFilterManager 
{
public:
    //! Get a pointer to the single filter manager
    static CFilterManager* GetInstance();

    //! Load a filter by key, returning a pointer to the
    //! newly created instance. If the filter is not found,
    //! return NULL.
    IFilter* LoadFilter( unsigned int FilterID );
    
private:
    CFilterManager();
    ~CFilterManager();

    static CFilterManager* m_pSingleton;

    CRegistry* m_pRegistry;
    filter_table_t* m_pFilterTable;
    int m_iTableSize;
};

//@}

#endif // __FILTERMANAGER_H__

