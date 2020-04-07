// mp3_codec.h: mp3 codec implementation
// danc@iobjects.com 07/20/01
// (c) Interactive Objects

#ifndef __ARM_MP3_CODEC_H__
#define	__ARM_MP3_CODEC_H__

#include <codec/common/Codec.h>    // codec base class

// local headers
#include "mpgdata.h"
#include "mpgaudio.h"

#define ARM_MP3_CODEC_ID  0x0f

// fdecl
class IInputStream;

class CMP3Codec : public ICodec
{
public:
    CMP3Codec();
    ~CMP3Codec();

    ERESULT DecodeFrame( unsigned long& TimePos );
    ERESULT SetSong( IDataSource* pDataSource, IInputStream* pInputStream,
                     stream_info_t& streamInfo, IMetadata* pMetadata = 0 );
    ERESULT Seek( unsigned long& secSeek );

    ERESULT GetMetadata( IDataSource* pDataSource, IMetadata* pMetadata,
                         IInputStream* pInputStream = 0 );
    
    rbuf_writer_t* GetWriteBuf() const  { return m_pWriteBuf;  }
    void SetWriteBuf( rbuf_writer_t* pWriteBuf ) { m_pWriteBuf = pWriteBuf; }

    int GetOutputUnitSize() const { return (MP3_MAX_PCM_LENGTH * 2 * 2);  }

    IInputStream* GetInputStream() const { return m_pInputStream; }
    
    void Stats(void);

    DEFINE_CODEC( "ARM MPEG-1 Layer 3", ARM_MP3_CODEC_ID, true, "mp3" );
    //    int DecodeFrame(output_buffer_t* pBuffer, unsigned long& SamplesPerChannel, unsigned long& TimePos);	
    //    int SetSong(CInputStream* pSongFile, song_info_t& info, bool bGetSongInfo);
    //    int Seek(unsigned long &secSeek);
    //    const char* GetCodecName() const
    //		{ return "MPEG-1 Layer 3"; }
    //    int GetCodecID() const
    //		{ return MP3_CODEC_ID; }
    
private:

    ERESULT _FindNextFrame(void);
    ERESULT _InitBitstream(unsigned long ulFilePos);
    unsigned long _DecodeVBRHeader(void);
    ERESULT _FillInternalBuffer(void);
        
    IInputStream*       m_pInputStream;
    rbuf_writer_t*      m_pWriteBuf;

    short               m_DecodeBuffer[2][MP3_MAX_PCM_LENGTH];
    
    char                m_InstanceDataSpace[ 2047 + sizeof( tMPEGInstance ) ];

                        // 10/16/2002 - danb@fullplaymedia.com
                        // It appears that the function MP3DecodeData in the MPEG library
                        //   wants to overwrite data outside of it's allocated Data Space above.
                        //   So, as a patch, I've added some space for it to write into so 
                        //   that valid variables don't get written over.  An assert is placed on
                        //   m_pMPEGInstanceData (checking that it's not NULL).  If this assert
                        //   occurs, if we want to continue with this patch, the size of this 
                        //   buffer should be increased.
    char                m_Bad_MPEG_Lib_Buffer[ 100 ];

    tMPEGInstance*      m_pMPEGInstanceData;
    char                m_pcEncodedData[MP3_MAX_BITS_REQUIRED / 8];// A buffer to contain the encoded MP3 audio.
    tMPEGBitstream      m_sBS;              // The MP3 bitstream pointer.
    tMPEGHeader         m_sHdr;             // The decoded MP3 header information.
    unsigned long       m_ulFileLength;     // The length of the encoded MP3 file.
    unsigned long       m_ulFilePos;        // The current offset into the MP3 file, indicating the
                                            // number of bytes which have been read from the file into
                                            // the encoded data buffer.
    unsigned long       m_ulValid;          // The number of bytes in pcEncodedData which contain 
                                            // valid encoded MP3 data.
    unsigned long       m_ulBitRate;        // The bit rate of the MP3 file.
    float               m_TimePos;          // Position in the song in milliseconds
    unsigned long       m_ulDuration;       // The length of the file in seconds.
    unsigned long       m_ulSampleRate;     // The sample rate of the decoded PCM stream.
    unsigned long       m_ulChannels;       // The number of channels in the file.
    unsigned char       m_ucVBRSeek[100];   // The seek points for VBR files.

    unsigned long       m_ID3v2size;
    unsigned long       m_ID3v1size;
    bool                m_bIsVBR;           // True if the file uses VBR.
    bool                m_bNearEOF;
};


#endif // __ARM_MP3_CODEC_H__
