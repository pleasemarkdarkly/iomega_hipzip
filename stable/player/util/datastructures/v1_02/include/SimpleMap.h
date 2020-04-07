//
// SimpleMap.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef SIMPLEMAP_H_
#define SIMPLEMAP_H_

#include <util/datastructures/SimpleVector.h>

//! The SimpleMap is a template class for generically storing elements indexed by keys.
//! The IndexType must have comparison operators < and >.
template<class IndexType, class DataType>
class SimpleMap
{
private:
    
    class SimpleMapEntry
    {
    public:
        SimpleMapEntry(IndexType key, DataType value)
            : m_key(key), m_value(value)
            { }
        ~SimpleMapEntry()
            { }
        const IndexType GetKey()  const
            { return m_key; }
        DataType GetValue()
            { return m_value; }
        
        void SetValue(DataType value)
            { m_value = value; }
        
    private:
        IndexType	m_key;
        DataType	m_value;
    };
    
public:
    
    SimpleMap()
        { }
    ~SimpleMap()
        { Clear(); }
    
    int Size() const
        { return m_vEntries.Size(); }

    //! Gets a key and value given an index.
    //! If the index is out-of-range then false is returned.
    bool GetEntry(int index, IndexType* pKey, DataType* pValue)
    {
        if (index >= Size())
            return false;
        *pKey = m_vEntries[index]->GetKey();
        *pValue = m_vEntries[index]->GetValue();
        return true;
    }

    //! Adds an entry to the dictionary.
    //! If a key with the same name already exists, then its value is overwritten.
    void AddEntry(IndexType key, DataType value)
    {
        int lower = 0, higher = Size() - 1;

        while (lower <= higher)
        {
            int middle = (lower + higher) / 2;

            if (m_vEntries[middle]->GetKey() < key)
                lower = middle + 1;
            else if(m_vEntries[middle]->GetKey() > key)
                higher = middle - 1;
            else
            {
                // The key is already in the map, so overwrite the value.
                m_vEntries[middle]->SetValue(value);
                return;
            }
        }
        m_vEntries.Insert(new SimpleMapEntry(key, value), lower);

    }
    
    //! Gets a value given a key.
    //! If the key isn't in the map then false is returned.
    bool FindEntry(const IndexType key, DataType* pValue)
    {
        int index = FindEntryIndex(key);
        if (index != -1)
        {
            *pValue = m_vEntries[index]->GetValue();
            return true;
        }
        else
            return false;
    }
    
    //! Gets a value given a key.
    //! If the key isn't in the dictionary then false is returned.
    bool FindEntry(const IndexType key, DataType* pValue) const
    {
        int index = FindEntryIndex(key);
        if (index != -1)
        {
            *pValue = m_vEntries[index]->GetValue();
            return true;
        }
        else
            return false;
    }

    //! Removes the entry at the given index from the map.
	//! This is an inefficient operation and should be used sparingly.
    void RemoveEntryAtIndex(int index, IndexType* pKey, DataType* pValue)
    {
        SimpleMapEntry* pEntry = m_vEntries.Remove(index);
        *pKey = pEntry->GetKey();
        *pValue = pEntry->GetValue();
        delete pEntry;
    }

    //! Removes the entry with the given key from the map.
	//! This is an inefficient operation and should be used sparingly.
    //! If the key isn't in the map then false is returned.
    bool RemoveEntryByKey(const IndexType key, DataType* pValue)
    {
        int index = FindEntryIndex(key);
        if (index != -1)
        {
            SimpleMapEntry* pEntry = m_vEntries.Remove(index);
            *pValue = pEntry->GetValue();
            delete pEntry;
            return true;
        }
        else
            return false;
    }

    //! Clears the map of all entries.
    void Clear()
    {
        while (!m_vEntries.IsEmpty())
            delete m_vEntries.PopBack();
    }
    
    
private:
    
    SimpleVector<SimpleMapEntry*>	m_vEntries;


    // Gets an index given a key.
    // If the key isn't in the dictionary then -1 is returned.
    int FindEntryIndex(const IndexType key) const
    {
        int lower = 0, higher = Size() - 1;

        while (lower <= higher)
        {
            int middle = (lower + higher) / 2;

            if (m_vEntries[middle]->GetKey() < key)
                lower = middle + 1;
            else if(m_vEntries[middle]->GetKey() > key)
                higher = middle - 1;
            else
            {
                // Found
                return middle;
            }
        }
        return -1;
    }
};


#endif	// SIMPLEMAP_H_
