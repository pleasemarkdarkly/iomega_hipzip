#ifndef __METADATA_FILE_TAG
#define __METADATA_FILE_TAG

#include <codec/mp3/id3v2/id3_tag.h>

class IMediaContentRecord;
class CFatFile;
class IInputStream;
class IMetadata;

#define MFT_FRMHDR_SIG 0x47

struct tMetadataFrameHeader {
    unsigned char nSig;
    int nContentLength;
};

typedef enum tagAttributeType { 
    MD_ATTR_TYPE_TCHAR = 0,
    MD_ATTR_TYPE_INT
} tAttributeType;

class CMetadataFileTag {
public:
    // maintain a single instance
    static CMetadataFileTag* GetInstance();
    static void Destroy();
    // add or update a tag, overwriting any v1 tag found.
    void UpdateTag(CFatFile* file, IMetadata* pMetadata);
    // return the length of the data in the file, not counting the tag
    int FileLengthWithoutTag(CFatFile* file);
    // return the length of the tag if found, or zero if no tag is found.  parse the tag into pMetadata
    int FindAndParseTag(CFatFile* file, IMetadata* pMetadata);
    // appends an MFT to the end of the file found at szUrl
    int UpdateTag(const char* szUrl, IMetadata* pMetadata);
    // see if a file has a tag, and inform the stream of its safe data-length if so.
    void CheckStreamSafeLength(IInputStream* pInStream);
private:
    void PrintField(IMetadata* pMD, int nMDType, int nDataType);
    void PrintTag(IMetadata* pMD);

    int TagOffsetFromEnd(CFatFile* file);
    CMetadataFileTag();
    ~CMetadataFileTag();
    static CMetadataFileTag* m_pInstance;

    int FrameSize(IMetadata* pMetadata, int iAttributeID, tAttributeType eAttrType);
    int TagSize(IMetadata* pMetadata);
    int RenderFrame( IMetadata* pMetadata, int iAttributeID, unsigned char* bfTag, tAttributeType eAttrType);
    int ParseFrame( unsigned char* bfTag, int iAttributeID, IMetadata* pMetadata, tAttributeType eAttrType );
    int Render(IMetadata* pMetadata, unsigned char* bfTag, int nSize );
    void Parse(unsigned char* bfTag, IMetadata* pMetadata);
};

#endif // __METADATA_FILE_TAG
