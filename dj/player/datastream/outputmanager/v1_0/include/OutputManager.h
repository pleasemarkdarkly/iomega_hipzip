//
// OutputManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef __OUTPUTMANAGER_H__
#define __OUTPUTMANAGER_H__

class CRegistry;
class IOutputStream;
typedef struct output_table_s output_table_t;


//! The output manager provides a simple way for the media
//! player to locate and load available output streams in the system.
//! The output streams can then be incorporated into the playstream, and
//! the media player will take care of releasing them when it is
//! done with them. This is a singleton class.
class COutputManager
{
public:
    //! Get a pointer to the single instance of the output manager
    static COutputManager* GetInstance();

    //! Load an output stream based on output stream ID, return a pointer
    //! to the newly allocated object. If the requested output stream is not
    //! found, return NULL.
    IOutputStream* LoadOutputStream( unsigned int OutputID );
    
private:
    COutputManager();
    ~COutputManager();

    static COutputManager* m_pSingleton;

    CRegistry* m_pRegistry;
    output_table_t* m_pOutputTable;
    int m_iTableSize;
};

//@}

#endif // __OUTPUTMANAGER_H__
