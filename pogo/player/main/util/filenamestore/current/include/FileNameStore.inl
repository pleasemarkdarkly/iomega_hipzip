/*  CFileNameStore */

inline CStoreFileNameRef* CFileNameStore::GetRoot()
{ 
    return new CStoreFileNameRef((IFileNameNode*)m_pRoot); 
}

inline int CFileNameStore::GetDataSourceID()
{
    return m_nDataSourceID;
}

/*  CDirFileNameNode */

inline FileNodeList* CDirFileNameNode::Children()
{
    return &m_lstChildren;
}

inline bool CDirFileNameNode::IsDir()
{ 
    return true; 
}

inline IFileNameNode* CDirFileNameNode::Parent()
{
    return m_pParent;
}

inline void CDirFileNameNode::SetParent(IFileNameNode* parent)
{
    m_pParent = (CDirFileNameNode*)parent;
}

inline char* CDirFileNameNode::Name()
{
    return m_szName;
}

inline char* CDirFileNameNode::LongName()
{
    return m_szLongName;
}

inline IFileNameRef* CDirFileNameNode::GetRef()
{
    return new CStoreFileNameRef(this);
}

inline IContentRecord* CDirFileNameNode::GetContentRecord()
{
    return (IContentRecord*) NULL;
}

inline void CDirFileNameNode::SetContentRecord(IContentRecord* pCR)
{
    // a directory doesn't really have one
}

/*  CFileFileNameNode */

inline FileNodeList* CFileFileNameNode::Children()
{ 
    return 0; 
}

inline bool CFileFileNameNode::IsDir()
{ 
    return false; 
}

inline IFileNameNode* CFileFileNameNode::Parent()
{
    return m_pParent;
}

inline void CFileFileNameNode::SetParent(IFileNameNode* parent)
{
    m_pParent = (CDirFileNameNode*)parent;
}

inline char* CFileFileNameNode::Name()
{
    return m_szName;
}

inline char* CFileFileNameNode::LongName()
{
    return m_szLongName;
}

inline bool CFileFileNameNode::AddChild(IFileNameNode*)
{
    return false;
}

inline IFileNameRef* CFileFileNameNode::GetRef()
{
    return new CStoreFileNameRef(this);
}

inline IContentRecord* CFileFileNameNode::GetContentRecord()
{
    return m_pContentRecord;
}

inline void CFileFileNameNode::SetContentRecord(IContentRecord* pMR)
{
    m_pContentRecord = pMR;
}

/*  CStoreFileNameRef */

inline IFileNameNode* CStoreFileNameRef::Parent()
{
    return m_pNode->Parent();
}

inline bool CStoreFileNameRef::IsSame(CStoreFileNameRef* pOther)
{
    return (m_pNode == pOther->m_pNode);
}

inline char* CStoreFileNameRef::Name()
{
    return m_pNode->Name();
}

inline char* CStoreFileNameRef::LongName()
{
    return m_pNode->LongName();
}

inline IFileNameNode* CStoreFileNameRef::operator*()
{
    return m_pNode;
}

inline bool CStoreFileNameRef::IsSame(IFileNameRef* other) 
{ 
    return ( **((CStoreFileNameRef*)other) == m_pNode ); 
}
