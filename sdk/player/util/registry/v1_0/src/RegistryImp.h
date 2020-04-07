// RegistryImp.h: system wide class registry. this header is both C and C++ safe
// danc@iobjects.com 07/08/01
// (c) Interactive Objects

#ifndef REGISTRYIMP_H_
#define REGISTRYIMP_H_

#include <util/registry/Registry.h> /* RegKey */
#include <util/datastructures/SimpleVector.h>
#include <util/eresult/eresult.h>

// fdecl
class IInputStream;
class IOutputStream;

//
// CRegistryImp: physical central registry
//
class CRegistryImp
{
public:

    CRegistryImp();
    ~CRegistryImp();
  
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

    SimpleVector<registry_item_t>   m_registry;
};

  
#endif // REGISTRYIMP_H_
