#include <main/util/filenamestore/FileNameStore.h>

#include <stdio.h>

#include <main/content/metakitcontentmanager/MetakitContentManager.h>
#include <core/playmanager/PlayManager.h>
#include <main/main/FatHelper.h>
#include <main/main/AppSettings.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_FILENAME_STORE, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_FILENAME_STORE);

/*      CFileNameStore       */

static const char* sc_szDirsTableNameBase = "Directories_";
static const char* sc_szFilesTableNameBase = "Files_";
static const char* sc_szFileFileNameTableLayout = "[Idx:I,Parent:I,ContentRecord:I,Name:S,LongName:S]";
static const char* sc_szDirFileNameTableLayout = "[Idx:I,Parent:I,Name:S,LongName:S]";

CFileNameStore::CFileNameStore(const char* szRootName, const char* szRootLongName, int nDataSourceID)
    : m_ipDirIdx("Idx"),
    m_ipDirParent("Parent"),
    m_spDirName("Name"),
    m_spDirLongName("LongName"),
    m_ipFileIdx("Idx"),
    m_ipFileParent("Parent"),
    m_ipContentRecord("ContentRecord"),
    m_spFileName("Name"),
    m_spFileLongName("LongName"),
    m_nDataSourceID(nDataSourceID),
    m_pContentIdxPtrVector(0)
{
    m_pRoot = new CDirFileNameNode(szRootName, szRootLongName);
}

CFileNameStore::~CFileNameStore()
{
    delete m_pRoot;
}

struct IdxNodePair {
    int idx;
    CDirFileNameNode *node;
};

typedef SimpleList<IdxNodePair*> IdxNodePairList;

// save the tree structure to disk.  it is assumed that only one tree per datasource id is needed on disk.
void CFileNameStore::Serialize()
{
    DEBUGP( DBG_FILENAME_STORE, DBGLEV_INFO,"fns: serialize\n");
    this->PrintStructureReport();
    c4_Storage* storage = ((CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager())->GetStorage();

    // open the views
    char* dirSchema = DirectoryTableSchema();
    char* fileSchema = FileTableSchema();
    m_vDirs = storage->GetAs(dirSchema);
    m_vFiles = storage->GetAs(FileTableSchema());
    delete [] dirSchema;
    delete [] fileSchema;
    
    // empty the views
    m_vDirs.SetSize(0);
    m_vFiles.SetSize(0);

    // push the first entry onto the list.
    IdxNodePairList dirs;
    IdxNodePair* pair = new IdxNodePair;
    pair->idx = 0;
    pair->node = m_pRoot;
    dirs.PushBack(pair);

    // init both index generators
    int nDirIdx = 1;
    int nFileIdx = 1;

    while (dirs.Size())
    {
        c4_Row row;
        pair = dirs.PopFront();
        int diridx = nDirIdx++;
        // add the directory entry to the dirs table.
        m_ipDirIdx(row) = diridx;
        m_ipDirParent(row) = pair->idx;
        m_spDirName(row) = pair->node->Name();
        m_spDirLongName(row) = pair->node->LongName();
        m_vDirs.Add(row);
        DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,"added dir %s : idx %d, parent %d\n",pair->node->LongName(),diridx,pair->idx);

        for (FileNodeListItr entry = pair->node->Children()->GetHead(); entry != pair->node->Children()->GetEnd(); ++entry)
        {
            if ((*entry)->IsDir())
            {
                IdxNodePair* subpair = new IdxNodePair;
                subpair->idx = diridx;
                subpair->node = (CDirFileNameNode*)(*entry);
                dirs.PushBack(subpair);
            }
            else
            {
                // add the file entry to the files table.
                c4_Row filerow;
                m_ipFileIdx(filerow) = (int)(*entry);
                m_ipFileParent(filerow) = diridx;
                m_ipContentRecord(filerow) = (int)(*entry)->GetContentRecord();
                m_spFileName(filerow) = (*entry)->Name();
                m_spFileLongName(filerow) = (*entry)->LongName();
                m_vFiles.Add(filerow);
                DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,"added file %s : idx %d, parent %d\n",(*entry)->LongName(),(int)(*entry),diridx);
            }
        }
        delete pair;
    }
}

// set the map of content record indexes to content record pointers, for an imminent deserialization.
void CFileNameStore::SetContentIdxPtrMap(IndexPtrVector* map)
{
    m_pContentIdxPtrVector = map;
}

// reconstruct a store from a two-table metakit form, translating index references into live pointers.  
// needs to be primed with a ContentIdxPtrMap first.
void CFileNameStore::DeSerialize()
{
    DEBUGP( DBG_FILENAME_STORE, DBGLEV_INFO,"fns: deserialize\n");
    if (!m_pContentIdxPtrVector)
        return;

    c4_Storage* storage = ((CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager())->GetStorage();

    // open the views
    char* dirSchema = DirectoryTableSchema();
    char* fileSchema = FileTableSchema();
    m_vDirs = storage->GetAs(dirSchema);
    m_vFiles = storage->GetAs(FileTableSchema());
    delete [] dirSchema;
    delete [] fileSchema;
    
    // dirs will represent directories that have already been put into the tree, but that might
    // have child dirs still in the directory table, referring to their indexes.
    IdxNodePairList dirs;
    IdxNodePair* parentpair = 0;

    int nDirRow, nFileRow = 0;
    int nDirIdx, nFileIdx;
    int nDirParentIdx, nFileParentIdx;
    const char *szDirName, *szDirLongName;
    const char *szFileName, *szFileLongName;
    CDirFileNameNode* pDirNode = 0;
    IFileNameNode* pFileNode = 0;

    parentpair = new IdxNodePair;
    nDirIdx = m_ipDirIdx(m_vDirs[0]);
    DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,"root dir %s : idx %d\n",m_pRoot->Name(),nDirIdx);
    parentpair->idx = nDirIdx;
    parentpair->node = this->m_pRoot;
    pDirNode = m_pRoot;

    // cycle through each row in the directory table, creating its entries in the tree.
    for (nDirRow = 0; nDirRow < m_vDirs.GetSize(); ++nDirRow)
    {
        // get this directory row
        nDirIdx = m_ipDirIdx(m_vDirs[nDirRow]);
        nDirParentIdx = m_ipDirParent(m_vDirs[nDirRow]);
        szDirName = m_spDirName(m_vDirs[nDirRow]);
        DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,"got dir %s : idx %d, parent %d\n",szDirName,nDirIdx, nDirParentIdx);
        szDirLongName = m_spDirLongName(m_vDirs[nDirRow]);        
        // discard entries from the head of the parent pair list, until we find this dir's parent node.
        if (nDirParentIdx)
            while (parentpair->idx != nDirParentIdx)
            {
                delete parentpair;
                parentpair = dirs.PopFront();
            }
        // instantiate the dir node (unless it's root, which exists already)
        if (nDirRow != 0)
        {
            pDirNode = new CDirFileNameNode(szDirName,szDirLongName);
            if (nDirParentIdx)
            {
                pDirNode->SetParent(parentpair->node);
                parentpair->node->AddChild(pDirNode);
            }
            // push this dir onto the stack of future potential parent dirs
            IdxNodePair* pair = new IdxNodePair;
            pair->idx = nDirIdx; pair->node = pDirNode;
            dirs.PushBack(pair);
        }
        // read all files that go into this directory.
        nFileParentIdx = m_ipFileParent(m_vFiles[nFileRow]);
        while (nFileParentIdx == nDirIdx)
        {
            // grab the file row
            nFileIdx = m_ipFileIdx(m_vFiles[nFileRow]);
            szFileName = m_spFileName(m_vFiles[nFileRow]);
            szFileLongName = m_spFileLongName(m_vFiles[nFileRow]);
            // create a node for the file
            pFileNode = new CFileFileNameNode(szFileName, szFileLongName);
            pFileNode->SetParent(pDirNode);
            pDirNode->AddChild(pFileNode);
            DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,"got file %s : idx %d, parent %d\n",szFileLongName,nFileIdx,nFileParentIdx);
            if ((int)m_ipContentRecord(m_vFiles[nFileRow]) != 0)
            {
                int idxRecord = (int)m_ipContentRecord(m_vFiles[nFileRow]);
                int iMapEntry = m_pContentIdxPtrVector->FindSorted((const void*)&idxRecord,&IdxPtrCompareToIdx);
                if (iMapEntry != -1)
                {
                    IMediaContentRecord* pMCR = (IMediaContentRecord*)(*m_pContentIdxPtrVector)[iMapEntry].ptr;
                    IFileNameRef* ref = pFileNode->GetRef();
                    pMCR->SetFileNameRef(ref);
                    delete ref;
                    pFileNode->SetContentRecord(pMCR);
                    m_ipContentRecord(m_vFiles[nFileRow]) = (int)pMCR;
                }
                else {
                    DEBUGP( DBG_FILENAME_STORE, DBGLEV_WARNING,"couldn't find media %d for %s\n",idxRecord,szFileLongName);
                }
            }
            // move to the next file entry and grab the parent index
            nFileParentIdx = m_ipFileParent(m_vFiles[++nFileRow]);
        }
    }
    PrintStructureReport();
    delete m_pContentIdxPtrVector;
    m_pContentIdxPtrVector = 0;
}

// add all file nodes, under the directory node passed in, to the list given.
void AppendFilesInDirRecursive(IFileNameNode* pNode, FileRefList* lstFiles)
{
    FileNodeList lstDirs;
    lstDirs.PushBack(pNode);
    while (lstDirs.Size())
    {
        IFileNameNode* node = lstDirs.PopFront();
        for (FileNodeListItr itr = node->Children()->GetHead(); itr != node->Children()->GetEnd(); ++itr)
        {
            if ((*itr)->IsDir())
                lstDirs.PushBack(*itr);
            else
            {
                DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE, "adding %s as new node\n",(*itr)->LongName()); 
                lstFiles->PushBack(new CStoreFileNameRef((*itr)));
            }
        }
    }
}

void AppendNodeToListRecursive(IFileNameNode* pNode, FileRefList* lstFiles)
{
    if (pNode->IsDir())
        AppendFilesInDirRecursive(pNode,lstFiles);
    else
    {
        DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE, "adding %s as new node\n",pNode->LongName()); 
        lstFiles->PushBack(new CStoreFileNameRef(pNode));
    }
}

// return a list of files that are in pNewStore, but not in this store.  
// transfer persistent content records into the new tree structure.
FileRefList* CFileNameStore::TransferOldAndListNewEntries(CFileNameStore* pNewStore)
{
    DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE, "fns:Transferring Old and Listing New Entries\n"); 
    CDirFileNameNode* root = m_pRoot;
    CDirFileNameNode* newRoot = pNewStore->m_pRoot;

    // a list of node references, to be returned
    FileRefList* newFiles = new FileRefList;
    // the list of directories not yet compared, both old and new.
    FileNodeList dirs, newDirs;
    // the pair of directories currently being compared, both old and new.
    IFileNameNode *dir, *newDir;
    // the pair of entries currently being considered, both old and new.
    FileNodeListItr entry, newEntry;

    dirs.PushBack(root);
    newDirs.PushBack(newRoot);

    while (dirs.Size() != 0)
    {
        // pop off a directory from each list of dirs todo.
        dir = dirs.PopFront();
        newDir = newDirs.PopFront();
        // in order to efficiently process things, the dir entries need to be sorted.
        ((CDirFileNameNode*)dir)->SortByLongName();
        ((CDirFileNameNode*)newDir)->SortByLongName();
        // iterate through each dir in tandem.
        entry = dir->Children()->GetHead();
        newEntry = newDir->Children()->GetHead();

        // while there are still entries to consider
        while (entry != dir->Children()->GetEnd() || newEntry != newDir->Children()->GetEnd())
        {
            // if the old entries are all gone, then all new entries are New.
            if (entry == dir->Children()->GetEnd())
            {
                AppendNodeToListRecursive(*newEntry, newFiles);
                ++newEntry;
                continue;
            }
            // if all new entries are gone, then just forget the rest of the old entries and move on.
            if (newEntry == newDir->Children()->GetEnd())
            {
                entry = dir->Children()->GetEnd();
                continue;
            }
            // compare the two entries
            int nStrDiff = strcmp((*entry)->m_szLongName, (*newEntry)->m_szLongName);
            // if the old dir entry is lesser than the new dir entry, just increment the old dir entry.
            if (nStrDiff < 0)
            {
                ++entry;
                continue;
            }
            // if the new dir entry is lesser than the old dir entry, add the new entry to the list of New Entries,
            // and increment the new entry.
            else if (nStrDiff > 0)
            {
                AppendNodeToListRecursive(*newEntry, newFiles);
                ++newEntry;
                continue;
            }
            // if the entries are the same,
            else
            {
                // if both entries are normal files
                if (!(*entry)->IsDir() && !(*newEntry)->IsDir())
                {
                    // transfer the old entry's media record to the new tree
                    if ((*entry)->GetContentRecord())
                    {
                        // point the media record to the new filename ref
                        IFileNameRef* newRef = (*newEntry)->GetRef();
                        (*entry)->GetContentRecord()->SetFileNameRef(newRef);
                        delete newRef;
                        // point the filename ref to its media record
                        (*newEntry)->SetContentRecord((*entry)->GetContentRecord());
                        // mark the media record as verfied so it doesn't get cull later on
                        DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE, "setting %s verified\n",(*entry)->LongName()); 
                        (*entry)->GetContentRecord()->SetVerified(true);
                        // clear out the old node's media record field.
                        (*entry)->SetContentRecord(NULL);
                    }
                    ++entry;
                    ++newEntry;
                    continue;
                }
                // if both entries are a dir node, then add them to the list of dirs to check.
                else if ((*entry)->IsDir() && (*newEntry)->IsDir())
                {
                    dirs.PushBack(*entry);
                    newDirs.PushBack(*newEntry);
                    ++entry;
                    ++newEntry;
                    continue;
                }
                // if the entries have the same name but only one is a dir, then add the new entry to the list of New Entries,
                // and increment both entries.
                else
                {
                    AppendNodeToListRecursive(*newEntry, newFiles);
                    ++entry;
                    ++newEntry;
                }
            }
        }
    }
    return newFiles;
}

// add a file node by url.  return a pointer to the new node, or the existing node if it was already there.
IFileNameNode* CFileNameStore::AddNodeByURL(char* url)
{
    // remove any trailing slash
    if (url[strlen(url)-1] == '\\' || url[strlen(url)-1] == '/')
        url[strlen(url)-1] = 0;
    // break the url into pieces
    StringList lstPathTokens = TokenizePath((char*)url);
    IFileNameNode* walker = m_pRoot;
    // verify that the first token somewhat matches the root name.
    StringListItr tok = lstPathTokens.GetHead();
    // only compare up to the length of the shorter string
    int cmplen = strlen(walker->LongName());
    if (strlen(*tok) < cmplen)
        cmplen = strlen(*tok);
    if (tok == NULL || strncmp(walker->LongName(),(*tok),cmplen))
        return NULL;
    ++tok;
    // if the token length was less than the full longname, then assume there is another token in the longname (e.g., 'file://a:')
    if (cmplen < strlen(walker->LongName()))
        // and skip it.
        ++tok;
    // step into the tree to the parent of the new node.
    for (; tok != lstPathTokens.GetTail(); ++tok)
    {
        bool bFound = false;
        for (FileNodeListItr entry = walker->Children()->GetHead(); entry != walker->Children()->GetEnd(); ++entry)
        {
            if (!strcmp((*tok),(*entry)->LongName()))
            {
                walker = (*entry);
                bFound = true;
                break;
            }
        }
        if (!bFound)
            return NULL;
    }
    // now tok should be pointing at the filename, and walker should be pointing at the parent directory.
    if (walker->IsDir())
    {
        // scan through existing children nodes to makes sure the file doesn't already exist.
        for (FileNodeListItr entry = walker->Children()->GetHead(); entry != walker->Children()->GetEnd(); ++entry)
        {
            if (!strcmp((*tok),(*entry)->LongName()))
            {
                walker = (*entry);
                return walker;
            }
        }
        // look up the actual filesystem node to get the type, name, and shortname.
        DSTAT stat;
        if (pc_gfirst(&stat, LongPathFromFullURL(url)))
        {
            TrimStatNames(stat);
            char szName[PLAYLIST_STRING_SIZE];
            MakeShortFilename(stat, szName);
            char* szLongName = MakeLongFilename(stat);
            IFileNameNode* pNode = NULL;
            if (stat.fattribute & ADIRENT)
                // create a new directory node
                pNode = new CDirFileNameNode(szName, szLongName);
            else
                pNode = new CFileFileNameNode(szName, szLongName);
            free (szLongName);
            // add the new node to it's parent
            walker->AddChild(pNode);
            pc_gdone(&stat);
            return pNode;
        }
    }
    return NULL;
}

// return the stock directory table schema, personalized to the data source id
char* CFileNameStore::DirectoryTableSchema()
{
    int len = strlen (sc_szDirsTableNameBase) + strlen (sc_szDirFileNameTableLayout) + 2;
    if (m_nDataSourceID > 9)
        ++len;
    DBASSERT( DBG_FILENAME_STORE, m_nDataSourceID < 100, "Unexpectedly large data source id\n");
    char* schema  = new char[len];
    sprintf(schema,"%s%i",sc_szDirsTableNameBase,m_nDataSourceID);
    strcat(schema,sc_szDirFileNameTableLayout);
    return schema;
}

// return the stock file table schema, personalized to the data source id
char* CFileNameStore::FileTableSchema()
{
    int len = strlen (sc_szFilesTableNameBase) + strlen (sc_szFileFileNameTableLayout) + 2;
    if (m_nDataSourceID > 9)
        ++len;
    DBASSERT( DBG_FILENAME_STORE, m_nDataSourceID < 100, "Unexpectedly large data source id\n");
    char* schema  = new char[len];
    sprintf(schema,"%s%i",sc_szFilesTableNameBase,m_nDataSourceID);
    strcat(schema,sc_szFileFileNameTableLayout);
    return schema;
}

// print out a report of all entries stored
void CFileNameStore::PrintStructureReport()
{
    // walk the entire tree, depth-first, and report on every node.
    m_pRoot->PrintReportRecursive(0);
}

IFileNameRef* CFileNameStore::GetRefByPath(const char* path)
{
    StringList lstPathTokens = TokenizePath((char*)path);
    IFileNameNode* walker = m_pRoot;
    // verify that the first token matches the root name.
    StringListItr tok = lstPathTokens.GetHead();
    if (tok == NULL || strcmp(walker->Name(),(*tok)))
        return NULL;
    ++tok;
    for (; tok != lstPathTokens.GetEnd(); ++tok)
    {
        bool bFound = false;
        for (FileNodeListItr entry = walker->Children()->GetHead(); entry != walker->Children()->GetEnd(); ++entry)
        {
            if (!strcmp((*tok),(*entry)->Name()))
            {
                walker = (*entry);
                bFound = true;
                break;
            }
        }
        if (!bFound)
            return NULL;
    }
    return new CStoreFileNameRef(walker);
}

IFileNameRef* CFileNameStore::GetRefByURL(const char* url)
{
    StringList lstPathTokens = TokenizePath((char*)url);
    IFileNameNode* walker = m_pRoot;
    // verify that the first token somewhat matches the root name.
    StringListItr tok = lstPathTokens.GetHead();
    // only compare up to the length of the shorter string
    int cmplen = strlen(walker->LongName());
    if (strlen(*tok) < cmplen)
        cmplen = strlen(*tok);
    if (tok == NULL || strncmp(walker->LongName(),(*tok),cmplen))
        return NULL;
    ++tok;
    // if the token length was less than the full longname, then assume there is another token in the longname (e.g., 'file://a:')
    if (cmplen < strlen(walker->LongName()))
        // and skip it.
        ++tok;
    for (; tok != lstPathTokens.GetEnd(); ++tok)
    {
        bool bFound = false;
        if (walker->Children() != NULL)
            for (FileNodeListItr entry = walker->Children()->GetHead(); entry != walker->Children()->GetEnd(); ++entry)
            {
                if (!strcmp((*tok),(*entry)->LongName()))
                {
                    walker = (*entry);
                    bFound = true;
                    break;
                }
            }
        if (!bFound)
            return NULL;
    }
    return new CStoreFileNameRef(walker);
}

// return a count of all files and direcoties
void CFileNameStore::FileCount(int* pnDirs, int* pnFiles)
{
    *pnDirs = *pnFiles = 0;
    m_pRoot->FileCountRecursive(pnDirs, pnFiles);
}

/*      CFileFileNameNode       */

CFileFileNameNode::CFileFileNameNode(const char* szName, const char* szLongName) : IFileNameNode() , m_pParent(0), m_pContentRecord(0)
{
    m_szName = new char[strlen(szName)+1];
    strcpy (m_szName,szName);
    m_szLongName = new char[strlen(szLongName)+1];
    strcpy (m_szLongName,szLongName);
}

CFileFileNameNode::~CFileFileNameNode() 
{
    delete [] m_szName;
    delete [] m_szLongName;
}

void CFileFileNameNode::PrintReportRecursive(int nFolderDepth)
{
    for (int i = 0; i < nFolderDepth; ++i) {
        DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,".");
    }
    DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,"%s (%d)\n",m_szName,(int)m_pContentRecord);
}

void CFileFileNameNode::FileCountRecursive(int* pnDirs, int* pnFiles)
{
    ++*pnFiles;
}

/*      CDirFileNameNode       */

CDirFileNameNode::CDirFileNameNode(const char* szName, const char* szLongName) : m_pParent(0)
{
    m_szName = new char[strlen(szName)+1];
    strcpy (m_szName,szName);
    m_szLongName = new char[strlen(szLongName)+1];
    strcpy (m_szLongName,szLongName);
}

CDirFileNameNode::~CDirFileNameNode() 
{
    for (FileNodeListItr itr = m_lstChildren.GetHead(); itr != m_lstChildren.GetEnd(); ++itr)
        delete (*itr);
    delete [] m_szName;
    delete [] m_szLongName;
}

bool CDirFileNameNode::AddChild(IFileNameNode* pChild)
{
    pChild->SetParent(this);
    m_lstChildren.PushBack(pChild);
    return true;
}

void CDirFileNameNode::PrintReportRecursive(int nFolderDepth)
{
    for (int i = 0; i < nFolderDepth; ++i) {
        DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,".");
    }
    DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,"[%s]\n",m_szName);
    for (FileNodeListItr itrChild = m_lstChildren.GetHead(); itrChild != m_lstChildren.GetEnd(); ++itrChild)
        (*itrChild)->PrintReportRecursive(nFolderDepth+1);
}

void CDirFileNameNode::FileCountRecursive(int* pnDirs, int* pnFiles)
{
    ++*pnDirs;
    for (FileNodeListItr itrChild = m_lstChildren.GetHead(); itrChild != m_lstChildren.GetEnd(); ++itrChild)
        (*itrChild)->FileCountRecursive(pnDirs,pnFiles);
}

// given the file name nodes left and right, return whether left has an alphabetically lesser long name.
inline bool LongNameLessThan(void* left, void* right)
{
    IFileNameNode* ndLeft = (IFileNameNode*)left;
    IFileNameNode* ndRight = (IFileNameNode*)right;
    const char* szLeft = ndLeft->LongName();
    if (!szLeft)
        szLeft = ndLeft->Name();
    const char* szRight = ndRight->LongName();
    if (!szRight)
        szRight = ndRight->Name();
    return (strcmp(szLeft, szRight) < 0);
}

// compare left and right according to whether they are directories (dirs considered sorted earlier than normal files)
inline bool IsDirLessThan(void* left, void* right)
{
    IFileNameNode* ndLeft = (IFileNameNode*)left;
    IFileNameNode* ndRight = (IFileNameNode*)right;
    if (!ndLeft->IsDir() && ndRight->IsDir())
        return false;
    return true;
}

// sort directory entries alphabetically by LongName;
void CDirFileNameNode::SortByLongName()
{
    if (m_lstChildren.Size() > 1)
        m_lstChildren.Sort(LongNameLessThan);
}

// sort directory entries by whether they are dirs or normal files (dirs first)
void CDirFileNameNode::SortByIsDir()
{
    if (m_lstChildren.Size() > 1)
        m_lstChildren.Sort(IsDirLessThan);
}

/*      CStoreFileNameRef        */

CStoreFileNameRef::CStoreFileNameRef(IFileNameNode* node) : m_pNode(node)
{
}

CStoreFileNameRef::~CStoreFileNameRef()
{
}

char* CStoreFileNameRef::URL()
{
    // climb up and get the string length required
    int len = 0;
    IFileNameNode* node = m_pNode;
    while (node)
    {
        // add in the segment length, plus one for the segment separator
        len += strlen (node->LongName()) + 1;
        node = node->Parent();
    }
    // allocate the string space.  we don't have to add one for the null term here,
    // since we added one char for separator space per segment: one too many.
    char* url = new char[len];
    url[len-1] = 0;
    // climb up and fill in the url contents
    node = m_pNode;
    while (node)
    {
        // get the length of the segment
        int seglen = strlen(node->LongName());
        // copy the segment into place
        memcpy(url+len-seglen-1,node->LongName(),seglen);
        // place a separator between segments, if this is not the root segment.
        if (node->Parent())
            url[len-1-seglen-1] = '\\';
        // back up the len counter the length of the segment, plus one for the separator.
        len -= (seglen+1);
        // move up the tree hierarchy.
        node = node->Parent();
    }
    DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,"fns:url %s\n",url);        
    return url;
}

char* CStoreFileNameRef::Path()
{
    // climb up and get the string length required
    int len = 0;
    IFileNameNode* node = m_pNode;
    while (node)
    {
        // add in the segment length, plus one for the segment separator
        len += strlen (node->Name()) + 1;
        node = node->Parent();
    }
    // allocate the string space.  we don't have to add one for the null term here,
    // since we added one char for separator space per segment: one too many.
    char* path = new char[len];
    path[len-1] = 0;
    // climb up and fill in the path contents
    node = m_pNode;
    while (node)
    {
        // get the length of the segment
        int seglen = strlen(node->Name());
        // copy the segment into place
        memcpy(path+len-seglen-1,node->Name(),seglen);
        // place a separator between segments, if this is not the root segment.
        if (node->Parent())
            path[len-1-seglen-1] = '\\';
        // back up the len counter the length of the segment, plus one for the separator.
        len -= (seglen+1);
        // move up the tree hierarchy.
        node = node->Parent();
    }
    DEBUGP( DBG_FILENAME_STORE, DBGLEV_TRACE,"fns:path %s\n",path);
    return path;
}

FileRefList CStoreFileNameRef::Children()
{
    FileRefList children;
    for (FileNodeListItr it = m_pNode->Children()->GetHead(); it != m_pNode->Children()->GetEnd(); ++it)
        children.PushBack(new CStoreFileNameRef(*it));
    return children;
}

/*          Miscellaneous helper functions          */

int IndexPtrPairIndexLessThan(const void* left, const void* right)
{
    IndexPtrPair *prLeft = (IndexPtrPair*) left;
    IndexPtrPair *prRight = (IndexPtrPair*) right;
    return (prLeft->index - prRight->index);
}

int IdxPtrCompareToIdx(const IndexPtrPair& left, const void* pCompareData)
{
    int iRight = *(int*)pCompareData;
    //DEBUGP( DBG_FILENAME_STORE, DBGLEV_INFO,"L=%d, R=%d => %d\n",left.index,iRight,(iRight-left.index));
    return (iRight - left.index);
}