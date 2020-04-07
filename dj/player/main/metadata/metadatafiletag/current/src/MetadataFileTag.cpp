#include <main/metadata/metadatafiletag/MetadataFileTag.h>

#include <datastream/input/InputStream.h>
#include <content/common/Metadata.h>
#include <codec/mp3/id3v2/id3_tag.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <datastream/fatfile/FatFile.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <main/main/FatHelper.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_MD_FILETAG, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DBG_MD_FILETAG);

#define MFT_SIG "ID3V2EOF"
#define MFT_SIG_LEN 8
#define METADATA_MAX_CHARS 1024

#define ID3_V1_SIZE	128
#define ID3_V1_SIG_SIZE	4

#define MD_COUNT 5
#define MAX_ALIGN_CHARS (MD_COUNT * 3)

extern const char *gc_szID3v1GenreTable[];
const int gc_ID3v1GenreCount = 148;

struct tPresenceTag {
    char szSignature[MFT_SIG_LEN+1];
    int nTagSize;
};

static void AddID3v2String(IMetadata* pMetadata, int iAttributeID, ID3_Tag& tag, ID3_FrameID frameID);
static void SetTagFieldFromMetadata (IMetadata* pMetadata, int iAttributeID, ID3_Tag& id3tag, ID3_FrameID frameID);

CMetadataFileTag* CMetadataFileTag::m_pInstance = 0;

CMetadataFileTag::CMetadataFileTag()
{}

CMetadataFileTag::~CMetadataFileTag()
{}

CMetadataFileTag* CMetadataFileTag::GetInstance()
{
    if (!m_pInstance)
        m_pInstance = new CMetadataFileTag;
    return m_pInstance;
}

void CMetadataFileTag::Destroy()
{
    delete m_pInstance;
    m_pInstance = NULL;
}

int CMetadataFileTag::UpdateTag(const char* szUrl, IMetadata* pMetadata)
{
    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "mft:UpdateTag\n"); 
    CFatFile file;
    DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:opening %s\n",szUrl); 
    file.Open(FullFilenameFromURLInPlace(szUrl));
    UpdateTag(&file,pMetadata);
    file.Close();
}

// add or update a tag, overwriting any v1 tag found.
void CMetadataFileTag::UpdateTag(CFatFile* file, IMetadata* pMetadata)
{
  // check for an old MFT.
    int nOldTagSize = TagOffsetFromEnd(file);
    DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:ofe rpts %d\n",nOldTagSize); 
    if (!nOldTagSize)
    {
        // check for a v1 tag
        unsigned char bfV1Sig[ID3_V1_SIG_SIZE];
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:looking for v1\n"); 
        file->Seek(CFatFile::SeekEnd, -ID3_V1_SIZE );
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "...\n"); 
        int iValid = file->Read(bfV1Sig, ID3_V1_SIG_SIZE );
        if (iValid < 0)
        {
            DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "MFT:couldn't read for v1\n");
            file->Close();
            return;
        }
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:check v1 sig\n"); 
        if(bfV1Sig[0] == 'T' && bfV1Sig[1] == 'A' && bfV1Sig[2] == 'G')
        {
            DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:v1 sig\n"); 
            nOldTagSize = ID3_V1_SIZE;
        }
    }
    // position the file at the start of the old tag.
    file->Seek(CFatFile::SeekEnd, -nOldTagSize);
    DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:estimated tag size?\n"); 
    int nTagSize = TagSize(pMetadata);
    DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:ets %d\n",nTagSize); 
    if (nTagSize)
    {
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:alloc new tag buf\n"); 
        if (unsigned char* bfTag = new unsigned char[nTagSize+1+MAX_ALIGN_CHARS])
        {
            DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:byte aligning\n"); 
            unsigned char* bfAligned = bfTag;
            if ((int)bfAligned % 4)
                bfAligned += 4 - (int)bfAligned%4;
            memset(bfAligned, 0, nTagSize+1);
            DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:render to buf\n"); 
            nTagSize = Render(pMetadata,bfAligned,nTagSize+MAX_ALIGN_CHARS);
            int nTtlTagSize = nTagSize + sizeof(tPresenceTag);
            if (nTtlTagSize < nOldTagSize)
            {
                DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:new fits in old, truncate\n"); 
                // need to truncate the file a bit.
                file->Truncate(file->Length() - nOldTagSize + nTtlTagSize);
                // (epg,3/27/2002): TODO: verify trunating doesn't move the file cursor
                if (file->Seek(CFatFile::SeekCurrent, 0) != (file->Length() - nTtlTagSize))
                {
                    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "MFT:TruncMovedCursor!\n"); 
                    file->Seek(CFatFile::SeekEnd, -nTtlTagSize);
                }
                else {
                    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "MFT:TruncNoMoveCursor=>RemTest\n"); 
                }
            }
            DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:check cursor pos\n"); 
            int nTagPos = file->Seek(CFatFile::SeekCurrent, 0);
            DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:write tag to file\n"); 
            file->Write(bfAligned,nTagSize);
            tPresenceTag presence;
            presence.nTagSize = nTtlTagSize;
            DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:sign presence tag\n"); 
            memcpy (presence.szSignature, MFT_SIG, MFT_SIG_LEN);
            presence.szSignature[MFT_SIG_LEN] = 0;
            int nSigPos = file->Seek(CFatFile::SeekCurrent, 0);
            DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:write presence tag to file\n"); 
            file->Write((void*)&presence, sizeof (tPresenceTag));
            delete [] bfTag;
        }
        else {
            DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "MFT:NoAllocTag %d\n",nTagSize);
        }
    }
    else {
        DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "MFT:TagNull\n"); 
    }
    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "mft:UpdateDone\n");
}

// return the length of the data in the file, not counting the tag
int CMetadataFileTag::FileLengthWithoutTag(CFatFile* file)
{
    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "mft:FileSizeWOTag\n"); 
    DBASSERT( DBG_MD_FILETAG, file != NULL, "MFT:NULL stream passed to FileLengthWithoutTag"); 
    int nFileSize = file->Length();
    int nTagSize = TagOffsetFromEnd(file);
    return nFileSize - nTagSize;
}

// return the offset of the tag from EOF, or zero if no tag is found.  position the file at the beginning of the tag, or EOF.
int CMetadataFileTag::TagOffsetFromEnd(CFatFile* file)
{
    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "mft:TagOffFrEOF\n"); 
    unsigned char bfPresence[sizeof(tPresenceTag)+1];
    tPresenceTag* pPT;
    int nTagSize = 0;
    if (file->Length() > sizeof(tPresenceTag))
    {
        // Remember where we are in the file.
        unsigned long ulCurrentPos = file->GetOffset();

        file->Seek(CFatFile::SeekEnd, -sizeof(tPresenceTag));
        file->Read(bfPresence, sizeof(tPresenceTag));
        pPT = (tPresenceTag*) bfPresence;
        if (!memcmp(pPT->szSignature,MFT_SIG,MFT_SIG_LEN))
        {
            nTagSize = pPT->nTagSize;
            // restrict the input stream from accessing the tag.
            file->SetSafeLen(file->Length() - nTagSize);
        }

        // Seek back to the current position.
        file->Seek(CFatFile::SeekStart, ulCurrentPos);
    }
    return nTagSize;
}


// return the length of the tag if found, or zero if no tag is found.  parse the tag into pMetadata
int CMetadataFileTag::FindAndParseTag(CFatFile* file, IMetadata* pMetadata)
{

    // verify file
    DBASSERT( DBG_MD_FILETAG, file != NULL, "MFT:NULL stream passed to FindAndParseTag"); 
    int nFileStartOffset = file->GetOffset();

    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "mft:FindAndParseTag\n"); 
    int nTagSize = TagOffsetFromEnd(file);
    DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:tag offset from end %d\n",nTagSize); 
    if (nTagSize)
    {
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft: seeking for read\n"); 
        int nPos = file->Seek(CFatFile::SeekEnd, -nTagSize);
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:seeked %d\n",nPos); 
        int nTagPos = file->Seek(CFatFile::SeekCurrent, 0);
        unsigned char bfTag[nTagSize+1+4];
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:aligning for read\n"); 
        unsigned char* bfAligned = bfTag;
        if ((int)bfAligned % 4)
            bfAligned += 4 - (int)bfAligned % 4;
        memset (bfTag, 0, nTagSize+1);
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:reading at %d for %d bytes\n",file->GetOffset(),nTagSize+1); 
        file->Read((void*)bfTag,nTagSize);
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:parsing\n"); 
        this->Parse(bfTag, pMetadata);
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:parse done tagsize %d\n",nTagSize); 
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:rew fatfile %d\n",nFileStartOffset); 
        int nSought = file->Seek(CFatFile::SeekStart, nFileStartOffset);
        DEBUGP( DBG_MD_FILETAG, nSought != nFileStartOffset, "MFT:sk zero got %d!\n",nSought); 
        return nTagSize;
    }
    else
    {
        // no tag found
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:no tag found\n"); 
        file->Seek(CFatFile::SeekStart, nFileStartOffset);
        return 0;
    }
}

int CMetadataFileTag::FrameSize(IMetadata* pMetadata, int iAttributeID, tAttributeType eAttrType)
{
    if (eAttrType == MD_ATTR_TYPE_TCHAR)
    {
        TCHAR* tszData;
        if (pMetadata && SUCCEEDED(pMetadata->GetAttribute(iAttributeID, (void**) &tszData)))
        {
            int nSize = (tstrlen(tszData) + 1) * sizeof (TCHAR) + sizeof(tMetadataFrameHeader);
            DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:framesize %d\n",nSize); 
            return nSize;
        }
        else
            return sizeof(tMetadataFrameHeader);
    }
    else
    {
        int nSize = sizeof (int) + sizeof(tMetadataFrameHeader);
        DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:framesize %d\n",nSize); 
        return nSize;
    }
}

int CMetadataFileTag::TagSize(IMetadata* pMetadata)
{
    int nSize = FrameSize(pMetadata, MDA_GENRE, MD_ATTR_TYPE_TCHAR) +
                FrameSize(pMetadata, MDA_ARTIST, MD_ATTR_TYPE_TCHAR) + 
                FrameSize(pMetadata, MDA_ALBUM, MD_ATTR_TYPE_TCHAR) + 
                FrameSize(pMetadata, MDA_TITLE, MD_ATTR_TYPE_TCHAR) + 
                FrameSize(pMetadata, MDA_ALBUM_TRACK_NUMBER, MD_ATTR_TYPE_INT);
    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "mft:TagSize %d\n",nSize); 
    return nSize;
}

// return the number of bytes rendered to bfTag.  place pMetadata[iAttributeID] into bfTag
int CMetadataFileTag::RenderFrame( IMetadata* pMetadata, int iAttributeID, unsigned char* bfTag, tAttributeType eAttributeType)
{
    int nBytesRendered = 0;
    // waste bytes until we get to a word boundary.
    if ((int)bfTag % 4)
    {
        nBytesRendered += 4 - (int)bfTag%4;
        bfTag += 4 - (int)bfTag%4;
    }

    tMetadataFrameHeader* pFrmHeader;
    pFrmHeader = (tMetadataFrameHeader*) bfTag;
    nBytesRendered += sizeof(tMetadataFrameHeader);
    void* pData;
    int nContentLen = 0;
    pFrmHeader->nSig = MFT_FRMHDR_SIG;
    if (pMetadata && SUCCEEDED(pMetadata->GetAttribute(iAttributeID, &pData)))
    {
        if (eAttributeType == MD_ATTR_TYPE_TCHAR)
        {
            nContentLen = (tstrlen((TCHAR*)pData) + 1) * sizeof(TCHAR);
            memcpy((void*)(bfTag+sizeof(tMetadataFrameHeader)), pData, nContentLen);
        }
        else if (eAttributeType == MD_ATTR_TYPE_INT)
        {
            nContentLen = sizeof(int);
            memcpy((void*)(bfTag+sizeof(tMetadataFrameHeader)), &pData, sizeof (int));
        }
        else
            DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "MFT:UnrecognizedMDType\n"); 
        pFrmHeader->nContentLength = nContentLen;
        nBytesRendered += nContentLen;
    }
    else
    {
        DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "MFT:failed to get attribute %d\n",iAttributeID); 
        pFrmHeader->nContentLength = 0;
    }
    return nBytesRendered;
}


// return the number of bytes consumed by this frame.  read the tag buffer and populate pMetadata[iAttributeID]
int CMetadataFileTag::ParseFrame( unsigned char* bfTag, int iAttributeID, IMetadata* pMetadata, tAttributeType eAttrType )
{
    int nBytesConsumed = 0;
    // waste bytes until we get to a word boundary.
    if ((int)bfTag % 4)
    {
        nBytesConsumed += 4 - (int)bfTag%4;
        bfTag += 4 - (int)bfTag%4;
    }

    tMetadataFrameHeader* pFrmHeader;
    pFrmHeader = (tMetadataFrameHeader*) bfTag;
    // If we don't have the signature, no bytes have been consumed, so return 0.  This should
    // normally never happen, but it has been seen with a filesystem that's been messed up.
    if (pFrmHeader->nSig != MFT_FRMHDR_SIG)
        return 0;
    nBytesConsumed  += sizeof(tMetadataFrameHeader);
    int nContentLen  = pFrmHeader->nContentLength;
    if (nContentLen > 0)
    {
        if (eAttrType == MD_ATTR_TYPE_TCHAR)
        {
            unsigned char bfTransfer[nContentLen];
            memcpy(bfTransfer,bfTag+sizeof(tMetadataFrameHeader),nContentLen);
            if (pMetadata)
                pMetadata->SetAttribute(iAttributeID, (void*)bfTransfer);
        }
        else if (eAttrType == MD_ATTR_TYPE_INT)
        {
            int nValue;
            DBASSERT( DBG_MD_FILETAG, nContentLen == sizeof (int), "MFT: size mismatch\n"); 
            memcpy(&nValue,bfTag+sizeof(tMetadataFrameHeader),nContentLen);
            if (pMetadata)
                pMetadata->SetAttribute(iAttributeID, (void*)nValue);
        }
    }
    else
    {
        // leave the attribute unset if it isn't found.
        DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "MFT:attr %d empty\n"); 
    }
    nBytesConsumed += nContentLen;
    return nBytesConsumed;
}

void CMetadataFileTag::PrintField(IMetadata* pMD, int nMDType, int nDataType)
{
    switch (nDataType)
    {
        case MD_ATTR_TYPE_TCHAR:
        {
            void* pData;
            if (SUCCEEDED(pMD->GetAttribute(nMDType,&pData)))
            {
                int len = tstrlen((TCHAR*)pData);
                char temp[len+1];
                TcharToChar(temp,(TCHAR*)pData);
                DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "%d: %s\n",nMDType,temp); 
            }
            else
                DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:MD%d(sz): fail\n"); 
            break;
        }
        case MD_ATTR_TYPE_INT:
        {
            void* pData;
            if (SUCCEEDED(pMD->GetAttribute(nMDType,&pData)))
            {
                DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "%d: %d\n",nMDType,(int)pData); 
            }
            else
                DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:MD%d(int): fail\n"); 
            break;
        }
        default:
            DBASSERT( DBG_MD_FILETAG, TRUE, "MFT: unrecognized md datatype %d\n",nDataType); 
            break;

    }
}

void CMetadataFileTag::PrintTag(IMetadata* pMD)
{
    PrintField(pMD, MDA_GENRE, MD_ATTR_TYPE_TCHAR);
    PrintField(pMD, MDA_ARTIST, MD_ATTR_TYPE_TCHAR);
    PrintField(pMD, MDA_ALBUM, MD_ATTR_TYPE_TCHAR);
    PrintField(pMD, MDA_TITLE, MD_ATTR_TYPE_TCHAR);
    PrintField(pMD, MDA_ALBUM_TRACK_NUMBER, MD_ATTR_TYPE_INT);
}

// return the count of bytes actually rendered to the buffer.  this varies slightly b/c of word alignment.
int CMetadataFileTag::Render(IMetadata* pMetadata, unsigned char* bfTag, int nSize)
{
    int nPos = 0;
    nPos += RenderFrame(pMetadata, MDA_GENRE, bfTag + nPos, MD_ATTR_TYPE_TCHAR);
    nPos += RenderFrame(pMetadata, MDA_ARTIST, bfTag + nPos, MD_ATTR_TYPE_TCHAR);
    nPos += RenderFrame(pMetadata, MDA_ALBUM, bfTag + nPos, MD_ATTR_TYPE_TCHAR);
    nPos += RenderFrame(pMetadata, MDA_TITLE, bfTag + nPos, MD_ATTR_TYPE_TCHAR);
    nPos += RenderFrame(pMetadata, MDA_ALBUM_TRACK_NUMBER, bfTag + nPos, MD_ATTR_TYPE_INT);
    DBASSERT( DBG_MD_FILETAG, nPos <= nSize, "MFT:RenderFrame overflow\n");
    return nPos;
}

void CMetadataFileTag::Parse(unsigned char* bfTag, IMetadata* pMetadata)
{
    int nPos = 0;
    nPos += ParseFrame( bfTag + nPos, MDA_GENRE, pMetadata, MD_ATTR_TYPE_TCHAR);
    nPos += ParseFrame( bfTag + nPos, MDA_ARTIST, pMetadata, MD_ATTR_TYPE_TCHAR);
    nPos += ParseFrame( bfTag + nPos, MDA_ALBUM, pMetadata, MD_ATTR_TYPE_TCHAR);
    nPos += ParseFrame( bfTag + nPos, MDA_TITLE, pMetadata, MD_ATTR_TYPE_TCHAR);
    nPos += ParseFrame( bfTag + nPos, MDA_ALBUM_TRACK_NUMBER, pMetadata, MD_ATTR_TYPE_INT);
    DEBUGP( DBG_MD_FILETAG, DBGLEV_TRACE, "mft:finished parsing %d bytes\n",nPos); 
}

// (epg,4/2/2002): generic buffer dumpers
#if 0
void Dump40Bytes(unsigned char* bf)
{
    for (int i = 0; i < 40; ++i)
    {
        DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "%02x ",bf[i]); 
        if (i && i % 20 == 0) {
            DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "\n"); 
        }
    }
    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "\n"); 
}
void DumpBuf(unsigned char* bf, int nSize)
{
    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "\n"); 
    int nLines = nSize / 40;
    nLines += (nSize % 40) ? 1 : 0;
    DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "size %d dumping %d lines\n",nSize,nLines); 
    for (int i = 0; i < nLines; ++i)
    {
        for (int j =0; j < 40; ++j)
        {
            DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "%02d ",(j+40*i) % 40); 
        }
        DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "\n"); 
        for (int j =0; j < 40; ++j)
        {
            DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "%02x ",bf[j+40*i]); 
        }
        DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "\n"); 
        for (int j =0; j < 40; ++j)
        {
            DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "---"); 
        }
        DEBUGP( DBG_MD_FILETAG, DBGLEV_INFO, "\n"); 
    }
}
#endif

void CMetadataFileTag::CheckStreamSafeLength(IInputStream* pInStream)
{
    TagOffsetFromEnd(((CFatFileInputStream*)pInStream)->m_pFile);
}

