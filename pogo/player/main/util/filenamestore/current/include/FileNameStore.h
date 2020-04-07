#ifndef __FILE_NAME_STORE_INCLUDE_
#define __FILE_NAME_STORE_INCLUDE_

//#include <util/datastructures/SimpleList.h>
#include <util/metakit/mk4.h>
#include <util/metakit/mk4str.h>
#include <main/util/datastructures/SortList.h>
#include <util/datastructures/SimpleVector.h>

/* classes */
class CFileNameStore;
class IFileNameNode;
class CFileFileNameNode;
class CDirFileNameNode;
class CStoreFileNameRef;
class IFileNameRef;
class CSimpleFileNameRef;
class IContentRecord;

#ifndef NULL
#define NULL 0
#endif

/* types */
struct IndexPtrPair {
    int index;
    void* ptr;
};
typedef SortList<IFileNameNode*> FileNodeList;
typedef SimpleListIterator<IFileNameNode*> FileNodeListItr;
typedef SortList<IFileNameRef*> FileRefList;
typedef SimpleListIterator<IFileNameRef*> FileRefListItr;
typedef SortList<const char*> StringList;
typedef SimpleListIterator<const char*> StringListItr;
typedef SimpleVector<IndexPtrPair> IndexPtrVector;

/* functions */
int IndexPtrPairIndexLessThan(const void* left, const void* right);
int IdxPtrCompareToIdx(const IndexPtrPair& left, const void* pCompareData);

class CFileNameStore
{
public:
    CFileNameStore(const char* szRootName, const char* szRootLongName, int nDataSourceID);
    ~CFileNameStore();
    // save into a two-table metakit form, translating pointers to index references
    void Serialize();
    // set the map of content record indexes to content record pointers, for an imminent deserialization.
    void SetContentIdxPtrMap(IndexPtrVector*);
    // reconstruct a store from a two-table metakit form, translating index references into live pointers.  
    // needs to be primed with a ContentIdxPtrMap first.
    void DeSerialize();
    // mark all file nodes as unverified
    void MarkFilesUnverified();
    // return a list of files that are in pNewStore, but not in this store.
    FileRefList* TransferOldAndListNewEntries(CFileNameStore* pNewStore);
    // return a reference to the root entry
    CStoreFileNameRef* GetRoot();
    // add a node by url.  return a pointer to the new node, or the existing node if it was already there.
    IFileNameNode* AddNodeByURL(char* url);
    // find an entry by its path, and return a reference to it
    IFileNameRef* GetRefByPath(const char* path);
    // find an entry by its url, and return a reference to it
    IFileNameRef* GetRefByURL(const char* url);
    // return the id of the datasource that the store refers to.
    int GetDataSourceID();
    // return a count of all files and direcoties
    void FileCount(int* nDirs, int* nFiles);
    // print out a listing of all nodes in the store.
    void PrintStructureReport();
private:
    char* DirectoryTableSchema();
    char* FileTableSchema();
    // mk directory table
    c4_View m_vDirs;
    c4_IntProp m_ipDirIdx;
    c4_IntProp m_ipDirParent;
    c4_StringProp m_spDirName;
    c4_StringProp m_spDirLongName;
    // mk file table
    c4_View m_vFiles;
    c4_IntProp m_ipFileIdx;
    c4_IntProp m_ipFileParent;
    c4_IntProp m_ipContentRecord;
    c4_StringProp m_spFileName;
    c4_StringProp m_spFileLongName;

    CDirFileNameNode* m_pRoot;
    int m_nDataSourceID;
    IndexPtrVector* m_pContentIdxPtrVector;
};

class IFileNameNode
{
public:
    IFileNameNode() {;}
    virtual ~IFileNameNode() {;}
    virtual bool IsDir() = 0;
    virtual IFileNameRef* GetRef() = 0;
    virtual FileNodeList* Children() = 0;
    virtual IFileNameNode* Parent() = 0;
    virtual void SetParent(IFileNameNode*) = 0;
    virtual char* Name() = 0;
    virtual char* LongName() = 0;
    virtual bool AddChild(IFileNameNode*) = 0;
    virtual void PrintReportRecursive(int nFolderDepth) = 0;
    virtual IContentRecord* GetContentRecord() = 0;
    virtual void SetContentRecord(IContentRecord*) = 0;
    virtual void FileCountRecursive(int* nDirs, int* nFiles) = 0;
protected:
    char* m_szName;
    char* m_szLongName;    
    friend class CFileNameStore;
    friend class CStoreFileNameRef;
};

class CFileFileNameNode : public IFileNameNode
{
public:
    CFileFileNameNode(const char* szName, const char* szLongName);
    ~CFileFileNameNode();
    bool IsDir();
    IFileNameRef* GetRef();
    FileNodeList* Children();
    IFileNameNode* Parent();
    void SetParent(IFileNameNode*);
    char* Name();
    char* LongName();
    bool AddChild(IFileNameNode*);
    void PrintReportRecursive(int nFolderDepth);
    IContentRecord* GetContentRecord();
    void SetContentRecord(IContentRecord*);
    void FileCountRecursive(int* nDirs, int* nFiles);
private:
    CDirFileNameNode* m_pParent;
    IContentRecord* m_pContentRecord;
};

class CDirFileNameNode : public IFileNameNode
{
public:
    CDirFileNameNode(const char* szName, const char* szLongName);
    ~CDirFileNameNode();
    bool AddChild(IFileNameNode*);
    bool IsDir();
    IFileNameRef* GetRef();
    FileNodeList* Children();
    IFileNameNode* Parent();
    void SetParent(IFileNameNode*);
    char* Name();
    char* LongName();
    void PrintReportRecursive(int nFolderDepth);
    IContentRecord* GetContentRecord();
    void SetContentRecord(IContentRecord*);
    // sort directory entries alphabetically by LongName;
    void SortByLongName();
    // sort directory entries by whether they are dirs or normal files (dirs first)
    void SortByIsDir();
    void FileCountRecursive(int* nDirs, int* nFiles);
private:
    CDirFileNameNode* m_pParent;
    FileNodeList m_lstChildren;
};

class IFileNameRef
{
public:
    IFileNameRef() {;}
    virtual ~IFileNameRef() {;}
    virtual char* Name() = 0;
    virtual char* LongName() = 0;
    virtual char* URL() = 0;
    virtual char* Path() = 0;
    virtual bool DynamicAlloc() = 0;
    virtual bool IsSame(IFileNameRef* other) = 0;
};

class CStoreFileNameRef : public IFileNameRef
{
public:
    CStoreFileNameRef(IFileNameNode*);
    CStoreFileNameRef();
    ~CStoreFileNameRef();
    char* Name();
    char* LongName();
    char* URL();
    char* Path();
    FileRefList Children();
    IFileNameNode* Parent();
    bool IsSame(CStoreFileNameRef* pOther);
    IFileNameNode* operator*();
    bool DynamicAlloc() { return true; }
    bool IsSame(IFileNameRef* other);
private:
    
    IFileNameNode* m_pNode;
};

class CSimpleFileNameRef : public IFileNameRef
{
public:
    CSimpleFileNameRef();
    ~CSimpleFileNameRef();

    // (epg,11/30/2001): TODO: Impl name and long name, probably from FilenameFromURL & FilenameFromPath (?)
    char* Name() { return (char*)0;}
    char* LongName() { return (char*)0;}
    char* URL() { return m_szUrl; }
    char* Path() { return m_szPath; }
    bool DynamicAlloc() { return false; }
    bool IsSame(IFileNameRef* other) { return (other == this); }
private:
    char* m_szUrl;
    char* m_szPath;
};

#include "FileNameStore.inl"

#endif // __FILE_NAME_STORE_INCLUDE_