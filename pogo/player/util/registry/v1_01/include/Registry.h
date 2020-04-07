//
// Registry.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef REGISTRY_H_
#define REGISTRY_H_

#include <util/eresult/eresult.h>

//
// API error values
//
#define REGISTRY_ERROR_ZONE  0x4c

const ERESULT REG_ERR_NONE          = MAKE_ERESULT( SEVERITY_SUCCESS, REGISTRY_ERROR_ZONE, 0x00 );
const ERESULT REG_ERR_NOSPACE       = MAKE_ERESULT( SEVERITY_FAILED,  REGISTRY_ERROR_ZONE, 0x01 );
const ERESULT REG_ERR_OVERLAP       = MAKE_ERESULT( SEVERITY_FAILED,  REGISTRY_ERROR_ZONE, 0x02 );
const ERESULT REG_ERR_IO_FAIL       = MAKE_ERESULT( SEVERITY_FAILED,  REGISTRY_ERROR_ZONE, 0x03 );
const ERESULT REG_ERR_KEY_NOT_FOUND = MAKE_ERESULT( SEVERITY_FAILED,  REGISTRY_ERROR_ZONE, 0x04 );


// here is the layout of a RegKey:
// 0xffffffff
//     ||  ||
//     ||  ||
//     ||  ||
//     ||  \|
//     ||   \_ Name ID
//     \|
//      \_ Type ID

typedef unsigned int RegKey;

// Macros to access a regkey
#define REGKEY_CREATE( type, name )  (((((RegKey)type)&0xffff)<<16)|((((RegKey)name)&0xffff)))
#define REGKEY_TYPE( key )           ((key) >> 16)
#define REGKEY_NAME( key )           ((key) & 0xffff)


//
// Flag values for a registry item
//
#define REGFLAG_PERSISTENT   0x0001


//
// C API
//

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
  
    ERESULT registry_add_item( RegKey key, void* data, short flags, short length );
    void*   registry_find_by_key( RegKey key );
    ERESULT registry_find_by_type( RegKey type, RegKey* table, int size );
    void*    registry_remove_item( RegKey key );

#ifdef __cplusplus
};
#endif // __cplusplus


// fdecl
typedef struct registry_item_s registry_item_t;


//
// C++ API
//

#ifdef __cplusplus

// fdecl
class IInputStream;
class IOutputStream;
class CRegistryImp;


//
// CRegistry: physical central registry
//
class CRegistry
{
public:
  
    static CRegistry* GetInstance();

    // Add an item to the registry
    ERESULT AddItem( RegKey key, void* data, short flags=0, short length=4 );
    
    // Find an item by specific reg key
    void* FindByKey( RegKey key );

    // Find an item by specific reg key
    // Returns REG_ERR_NONE if the item was found,
    // or REG_ERR_KEY_NOT_FOUND if no matching key exists.
    ERESULT FindByKey( RegKey key, void** ppData );

    // Find an item(s) by type. populate the given table up to Size
    // elements, return the number of items found
    ERESULT FindByType( RegKey type, RegKey* pTable, int size );

    // Remove an item from the registry
    void* RemoveItem( RegKey key );

    ERESULT SaveState( IOutputStream* );
    ERESULT RestoreState( IInputStream* );
    
private:

    CRegistry();
    ~CRegistry();

    CRegistryImp*   m_pImp;

};

// CRegisterObject: object with a constructor
//  to handily register any item you pass it
class CRegisterObject
{
public:
    CRegisterObject( RegKey Key, void* Data ) {
        CRegistry::GetInstance()->AddItem( Key, Data );
    }
};


#endif // __cplusplus
  
#endif  // REGISTRY_H_
