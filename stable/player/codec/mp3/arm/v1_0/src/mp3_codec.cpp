// Codec.cpp: implementation of the CCodec class.
//
//////////////////////////////////////////////////////////////////////

#include <cyg/kernel/kapi.h>
#include <cyg/infra/cyg_ass.h>
#include <string.h>
#include <ctype.h>

#include <datastream/input/InputStream.h>
#include <datastream/fatfile/FileInputStream.h>
#include <content/common/Metadata.h>
#include <util/debug/debug.h>
#include <util/rbuf/rbuf.h>
#include <util/tchar/tchar.h>
#include "mp3_codec.h"
#include "ID3v1Genre.h"
#include <codec/mp3/id3v2/id3_tag.h>
#include <_modules.h>

#ifdef DDOMOD_DJ_METADATAFILETAG
#include <datasource/fatdatasource/FatDataSource.h>
#include <main/metadata/metadatafiletag/MetadataFileTag.h>
#endif

DEBUG_MODULE(ARM_MP3);
DEBUG_USE_MODULE(ARM_MP3);

#define DEBUG_FUNCFAIL(x, y...) DEBUG(ARM_MP3, DBGLEV_ERROR, x, ##y)

#define FINDNEXTFRAME_ATTEMPTS 500

extern "C" void swap_bytes(void* buf,int n);

#define ID3_V1_SIZE	128

// reference parameters: you'd think they wouldn't include features that encourage side effects
static bool FindID3v1Headers(IInputStream& file, unsigned long& ID3v1Size, IMetadata* pMetadata);
static bool FindID3v2Headers(IInputStream& file, unsigned long& ID3v2Size, IMetadata* pMetadata);


REGISTER_CODEC( CMP3Codec, ARM_MP3_CODEC_ID );

//****************************************************************************
//
// The following is a mapping from the sample rate descriptor in the
// tMPEGHeader structure to the integer number of samples per second.
//
//****************************************************************************
const long lSRMap[] = { 11025, 12000,  8000, 0,
                        22050, 24000, 16000, 0,
                        44100, 48000, 32000, 0 };

//****************************************************************************
//
// The following is a mapping from the bitrate_index and ID bit in the MPEG
// sync header to a the bitrate of the frame.  The first 16 entries are for
// ID=0 (i.e. MPEG-2 or MPEG-2.5 half sample rate) and the second 16 entries
// are for ID=1 (i.e. MPEG-1 full sample rate).
//
//****************************************************************************
const long lBRMap[] = {	  0,  8,  16,  24,
                          32,  40,  48,  56,
                          64,  80,  96, 112,
                          128, 144, 160,   0,
                          0,  32,  40,  48,
                          56,  64,  80,  96,
                          112, 128, 160, 192,
                          224, 256, 320,   0 };


struct id3tag {
    char tag[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    unsigned char genre;
};


//////////////////////////////////////////////////////////////////////
//
//  CMP3Codec implementation
//

static const char* szErrorTable[] = 
{
    "No error",
    "No Syncword",
    "CRC Error",
    "Broken Frame",
    "End of bitstream",
    "Data overflow",
    "Can't allocate buffer",
    "Unsupported layer",
    "Frame discarded",
    "Reserved Sampling Frequency",
    "Forbidden bitrate",
    "Wrong MPEG build",
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMP3Codec::CMP3Codec() 
    : m_pInputStream(0),
      m_bIsVBR(false)
{
    DBEN( ARM_MP3 );

    unsigned int addr = (unsigned int) m_InstanceDataSpace;

    // align to 2k
    addr += 2047;
    addr = addr - (addr % 2048);
    
    // Initialize MPEG Instance information.
    m_pMPEGInstanceData = (tMPEGInstance *)addr;

    memset( (void*)m_pMPEGInstanceData,0, sizeof( tMPEGInstance ) );
    memset( (void*)&m_sBS, 0, sizeof( tMPEGBitstream ));
    memset( (void*)&m_sHdr, 0, sizeof( tMPEGHeader ) );
    memset( (void*) m_pcEncodedData,0, (MP3_MAX_BITS_REQUIRED / 8) );
    
    //    MP3Setup();

    DBEX( ARM_MP3 );
}

CMP3Codec::~CMP3Codec()
{
    DBEN( ARM_MP3 );
    DBEX( ARM_MP3 );
}

int
CMP3Codec::SetSong( IDataSource* pDataSource, IInputStream* pInputStream,
    stream_info_t& streamInfo, IMetadata* pMetadata )
{
    DBEN( ARM_MP3 );

    ERESULT res;
    
    m_pInputStream = pInputStream;

    //Double check input stream to be correct content
	
    // Get the length of the MP3 bitstream.
    m_ulFileLength = m_pInputStream->Length();
    m_ulFilePos = 0;

    m_ID3v1size = 0;
    m_ID3v2size = 0;
    
    // Initialize offsets into file to ignore id3v1 and id3v2 data
    //    if (bGetSongInfo && m_pInputStream->CanSeek())
    if( m_pInputStream->CanSeek() )
    {
        // mft takes precedence over other tags
#ifdef DDOMOD_DJ_METADATAFILETAG
        if ( (pMetadata != NULL)
            && (pDataSource->GetClassID() == FAT_DATA_SOURCE_CLASS_ID)
            && (CMetadataFileTag::GetInstance()->FindAndParseTag(((CFatFileInputStream*)m_pInputStream)->m_pFile, pMetadata) > 0) )
        {
            
            DEBUGP( ARM_MP3, DBGLEV_INFO, "mp3:mft detected\n");
        }
        else
#endif
        {
            if (pMetadata && !FindID3v1Headers(*m_pInputStream, m_ID3v1size, pMetadata))
            {
                DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
                return CODEC_FAIL;
            }
            if (!FindID3v2Headers(*m_pInputStream, m_ID3v2size, pMetadata))
            {
                DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
                return CODEC_FAIL;
            }
#if 0 // (epg,4/4/2002): a test of updating mft.
            if (pMetadata)
                CMetadataFileTag::GetInstance()->UpdateTag(((CFatFileInputStream*)m_pInputStream)->m_pFile, pMetadata);
#endif
        }

        // if we're not doing metadata, recall the safe length of the file from 
        if (!pMetadata)
        {


        }

        // Initialize the bitstream.
        res = _InitBitstream(m_ID3v2size);
        if (FAILED(res))
        {
            DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
            return CODEC_FAIL;
        }
    }
    else
    {
        // Initialize the bitstream.
        res = _InitBitstream(0);
        if (FAILED(res))
        {
            DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
            return CODEC_FAIL;
        }

        // Fill internal buffer to see if there's an ID3v2 tag at the beginning
        res = _FillInternalBuffer();
        if (FAILED(res)) {
            DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
            return CODEC_FAIL;
        }

        if (m_pcEncodedData[3] == 0x49 && m_pcEncodedData[2] == 0x44 && m_pcEncodedData[1] == 0x33)
        {
            m_ID3v2size = (static_cast<unsigned long>(m_pcEncodedData[5]) << 21 ) | 
                          (static_cast<unsigned long>(m_pcEncodedData[4]) << 14) | 
                          (static_cast<unsigned long>(m_pcEncodedData[11]) << 7) | 
                          static_cast<unsigned long>(m_pcEncodedData[10]);
            m_ID3v2size += 10;

            // Keep reading data until we reach the end of the ID3v2 tag.
            while (m_ID3v2size > m_ulFilePos)
            {
                m_ulValid = 0;
                res = _FillInternalBuffer();
                if (FAILED(res))
                {
                    DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
                    return CODEC_FAIL;
                }
            }
        }
    }

    // Find the first frame of the file.
    res = _FindNextFrame();
    if (FAILED(res))
    {
        // If we could not find the first frame of the file, then return an error.
        DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
        return res;
    }

    // Get the sample rate of the file.
    m_ulSampleRate = lSRMap[m_sHdr.sample_rate];

    // Get the number of channels in the file.
    m_ulChannels = m_sHdr.numchans;

    // Attempt to decode the VBR header.
    if(_DecodeVBRHeader() == 0)
    {
        // There is no VBR header on this file, so get the actual bitrate from the sync header.
        m_ulBitRate = lBRMap[((m_sHdr.packed_info & 0x00080000) >> 15) |
            ((m_sHdr.packed_info & 0x0000f000) >> 12)] * 1000;

        // If no bitrate is specified, then return an error.
        if(m_ulBitRate == 0)
        {
            DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
            return CODEC_BAD_FORMAT;
            //            return CODEC_FILE_WRONG_FORMAT;
        }

        // Indicate that this is not a VBR file.
        m_bIsVBR = false;
    }

    // Get the duration of the file.
    m_ulDuration = (((m_ulFileLength * 8) / m_ulBitRate) * 1000) +
                   ((((m_ulFileLength * 8) % m_ulBitRate) * 1000) / m_ulBitRate);


    // The time position is zero.
    m_TimePos = 0.0f;

    // Fill out stream info structure.
    streamInfo.Channels = m_sHdr.numchans;
    streamInfo.OutputChannels = 2;  // we support channel duplication
    streamInfo.SamplingFrequency = m_ulSampleRate;
    streamInfo.Bitrate = m_ulBitRate;
    streamInfo.Duration = m_ulDuration / 1000;
    if( m_ulFileLength && !streamInfo.Duration ) {
        streamInfo.Duration = 1;
    }
    
    if (pMetadata)
    {
        pMetadata->SetAttribute(MDA_DURATION, (void*)streamInfo.Duration);
        pMetadata->SetAttribute(MDA_SAMPLING_FREQUENCY, (void*)streamInfo.SamplingFrequency);
        pMetadata->SetAttribute(MDA_CHANNELS, (void*)streamInfo.Channels);
        pMetadata->SetAttribute(MDA_BITRATE, (void*)streamInfo.Bitrate);
    }

    // TODO For debugging only
    DEBUGP(ARM_MP3, DBGLEV_INFO, "%s Channels : %d\n", __FUNCTION__, m_sHdr.numchans);
    DEBUGP(ARM_MP3, DBGLEV_INFO, "%s Sampling Frequency : %d\n", __FUNCTION__, m_ulSampleRate);
    DEBUGP(ARM_MP3, DBGLEV_INFO, "%s Bit Rate : %d\n", __FUNCTION__, m_ulBitRate);

    // dc- tg 885; certain combinations fail to play due to a flaw in the arm mp3 decoder. as such, we need to fail to
    //     setsong on these, lest they be treated as valid songs that fail to play (rather than treating them as invalid songs)
    // These are all stereo streams at 32khz:
    //   224kbps, vbr
    //   320kbps, vbr
    //   320kbps, cbr
    // Unfortunately, with vbr, the bitrate we have is an average. so when we go to do a comparison, we can't tell exactly what bitrate we have-
    //  for example, the 224kbps vbr track in bug 885 shows up as ~ 230000bps
    if( m_sHdr.numchans == 2 && m_ulSampleRate == 32000 && m_ulBitRate == 320000 && !m_bIsVBR )
    {
        DEBUGP(ARM_MP3, DBGLEV_WARNING, "Unsupported format: 32khz stereo %d bps%s\n", m_ulBitRate, ( m_bIsVBR ? " vbr" : "" ) );
        return CODEC_BAD_FORMAT;
    }
    
    // Re-initialize the bitstream pointer structure.
    res = _InitBitstream(m_ID3v2size);
    if (FAILED(res))
    {
        DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
        return res;
    }
    
    DBEX( ARM_MP3 );
    
    return CODEC_NO_ERROR;
}

//int
//CMP3Codec::DecodeFrame(output_buffer_t* pBuffer, unsigned long& SamplesPerChannel, unsigned long& TimePos)
ERESULT
CMP3Codec::DecodeFrame( unsigned long& TimePos )
{   
    DBEN( ARM_MP3 );

    int Status;
    int iRetryCount = 2;

    // determine if we have space for a decode
    unsigned int Space = rb_write_avail( m_pWriteBuf );

    if( Space < (MP3_MAX_PCM_LENGTH*2*2) ) {
        // less then the max data we could get back, so dont
        // decode a frame
        return CODEC_NO_WORK;
    }
    
    // Loop over discarded frames.
    for (;;) {
	
        // Find the next frame in the bitstream.
        Status = _FindNextFrame();
        if (FAILED(Status)) {
            if( Status == CODEC_END_OF_FILE ) {
                rb_write_eof( m_pWriteBuf );
            } else {
                DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
            }
            return Status;
        }

        // Make sure there is enough data in the internal buffer.
        char * UpperBound = m_pcEncodedData + m_ulValid;
        char * LowerBound = (char *)m_sBS.bufptr;
        int DataNeeded = m_sHdr.bits_required / 8;
	
        // bitidx is actually where bitstream pointer is, so use <= instead of <.
        if ((int)(UpperBound - LowerBound) <= DataNeeded) {
	    
            // Get more data
            Status = _FillInternalBuffer();
            if (FAILED(Status)) {
                if( Status == CODEC_END_OF_FILE ) {
                    rb_write_eof( m_pWriteBuf );
                } else {
                    DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
                }
                return Status;
            }
        }

        // Decode the data frame.
        //	pBuffer->is_interleaved = false;
        tMPEGStatus MPEGStatus = MP3DecodeData(m_pMPEGInstanceData,
            m_DecodeBuffer[0],
            m_DecodeBuffer[1],
            &m_sBS);

        // 10/16/2002 - danb@fullplaymedia.com
        // It appears that the function MP3DecodeData in the MPEG library
        //   wants to overwrite data outside of it's allocated Data Space.
        //   So, as a patch, I've added some space for it to write into in the 
        //   class definition:  m_Bad_MPEG_Lib_Buffer[ ].  If this assert
        //   occurs and if we want to continue with this patch, the size of
        //   m_Bad_MPEG_Lib_Buffer[ ] should be increased.
        DBASSERT( ARM_MP3, m_pMPEGInstanceData != NULL, "MPEG lib overwrote class variables\n");

        if (MPEGStatus == eNoErr) {

            // Frame succesfully decoded
            break;
        } else {
            // We had an error; print the code and attempt to retry
            DEBUG(ARM_MP3, DBGLEV_WARNING, "fail: %s\n", szErrorTable[(int)MPEGStatus]);
            if( iRetryCount ) {
                iRetryCount--;
                continue;
            } else {
                // We could not decode the frame, so return an error.
                DEBUG_FUNCFAIL("-%s %d %d\n", __FUNCTION__, __LINE__, MPEGStatus);
                return CODEC_FAIL;
            }
        }
    }

    //    SamplesPerChannel = m_sHdr.samplesperchannel;
    unsigned int Bytes = m_sHdr.samplesperchannel * 2 * 2; // always output 2 channels of audio
    DBASSERT( ARM_MP3, Bytes <= Space, "something really strange just happened\n");

    unsigned int actual;
    rb_write_data( m_pWriteBuf, Bytes, &actual );
    
    short* pLeft = m_DecodeBuffer[0];
    short* pRight = (m_ulChannels == 2 ? m_DecodeBuffer[1] : m_DecodeBuffer[0]);
    short* ptr = (short*)rb_write_ptr( m_pWriteBuf );
    short* lim = ptr + (actual>>1);

    while( ptr < (lim-3) ) {
        *ptr++ = *pLeft++;
        *ptr++ = *pRight++;
        *ptr++ = *pLeft++;
        *ptr++ = *pRight++;
    }
    while( ptr < (lim-1) ) {
        *ptr++ = *pLeft++;
        *ptr++ = *pRight++;
    }

    rb_write_done( m_pWriteBuf, actual );
    
    if( actual < Bytes ) {
        Bytes -= actual;

        rb_write_data( m_pWriteBuf, Bytes, &actual );
        ptr = (short*)rb_write_ptr( m_pWriteBuf );
        lim = ptr + (actual>>1);

        while( ptr < (lim-3) ) {
            *ptr++ = *pLeft++;
            *ptr++ = *pRight++;
            *ptr++ = *pLeft++;
            *ptr++ = *pRight++;
        }
        while( ptr < (lim-1) ) {
            *ptr++ = *pLeft++;
            *ptr++ = *pRight++;
        }
        rb_write_done( m_pWriteBuf, actual );
    }
    
    // Increment the time based on the number of samples.
    m_TimePos += 1000.0f * static_cast<float>(m_sHdr.samplesperchannel) / static_cast<float>(m_ulSampleRate);
    TimePos = static_cast<unsigned long>(m_TimePos / 1000.0f);
    
    DBEX( ARM_MP3 );
    
    return CODEC_NO_ERROR;

}

/* This gets called from CMediaPlayer::Stop to seek to beginning of stream */
ERESULT
CMP3Codec::Seek(unsigned long &secSeek)
{
    DBEN( ARM_MP3 );

    if(secSeek * 1000 > m_ulDuration)
    {
        DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
        return CODEC_FAIL;
    }

    if( !m_pInputStream->CanSeek() ) {
        return CODEC_FAIL;
    }

    unsigned long ulMPEGDataSize = m_ulFileLength - m_ID3v2size - m_ID3v1size;
    unsigned long ulPos, ulPos2, ulPercent;
    unsigned long ulMSSeek = secSeek * 1000;

    // If this file uses VBR, then we use the seek point table to
    // compute the new position.
    if(m_bIsVBR)
    {
        // Convert the seek position into a percentage through the
        // file.
        ulPos = (ulMSSeek * 100) / m_ulDuration;

        // Compute the fractional percentage between this percentage
        // and the next.
        ulPercent = (((ulMSSeek * 100) % m_ulDuration) * 100) / m_ulDuration;

        // Get the two seek points that surround the requested
        // position.
        if(ulPos >= 99)
        {
            ulPos2 = 256;
        }
        else
        {
            ulPos2 = m_ucVBRSeek[ulPos + 1];
        }
        if(ulPos > 99)
        {
            ulPos = 256;
        }
        else
        {
            ulPos = m_ucVBRSeek[ulPos];
        }

        // Interpolate between the two seek points.
        ulPos = (ulPos * (m_ulFileLength / 256)) +
                (((ulPos2 - ulPos) * ulPercent *
                    (m_ulFileLength / 256)) / 100);
    }
    else
    {
        // Figure out the size of each frame based on the sample rate.
        unsigned long ulFrameSize;
        if(m_ulSampleRate >= 32000)
        {
            ulFrameSize = 1152;
        }
        else
        {
            ulFrameSize = 576;
        }

        // Compute the number of frames that occur before the requested
        // time position.
        ulPos = (((ulMSSeek / 1000) * m_ulSampleRate) / ulFrameSize) +
                (((ulMSSeek % 1000) * m_ulSampleRate) / (ulFrameSize * 1000));

        // Compute the time for the computed frame number.
        ulPos *= ulFrameSize;

        // Compute the file position based on the actual seek time.
        ulPos = ((ulPos / m_ulSampleRate) * (m_ulBitRate / 8)) +
                (((ulPos % m_ulSampleRate) * (m_ulBitRate / 8)) / m_ulSampleRate);
        if(ulPos)
            ulPos--;
    }

    // Initialize the bitstream to the position.
    int res = _InitBitstream(ulPos + m_ID3v2size);
    if (FAILED(res)) {
        DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
        return res;
    }
    

    // Find the next frame in the input MP3 bitstream.
    // TODO: this looks bad.
    int cFindFailed = 0;
    while(cFindFailed < FINDNEXTFRAME_ATTEMPTS)
    {
        // Look for the next frame.
        if (SUCCEEDED(_FindNextFrame()))
        {
            // Make sure that the packed_info for the frame we found
            // matches (where appropriate) the packed_info for the
            // first frame in the file.
            if((m_ulSampleRate == (unsigned)lSRMap[m_sHdr.sample_rate]) &&
                (m_ulChannels == (unsigned)m_sHdr.numchans))
                break;
        }

        // We could not find the next frame, so skip forward in the
        // input bitstream by a byte.
        ++cFindFailed;
        m_sBS.bitidx += 8;
        if(m_sBS.bitidx >= 32)
        {
            m_sBS.bufptr++;
            m_sBS.bitidx -= 32;
        }
    }
    if (cFindFailed >= FINDNEXTFRAME_ATTEMPTS) {
        DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
        return CODEC_FAIL;
    }
    

    // Skip back one word so that the frame header will be available
    // the next time the decoder is called.
    m_sBS.bufptr--;

    // Set time based on the new file position.
    m_TimePos = float(m_ulDuration) * float(ulPos)
                / float(ulMPEGDataSize );
	
    secSeek = (unsigned long)(m_TimePos / 1000.0f);

    // Success.
    DBEX( ARM_MP3 );
    return CODEC_NO_ERROR;
}

ERESULT
CMP3Codec::GetMetadata( IDataSource* pDataSource, IMetadata* pMetadata, IInputStream* pInputStream ) 
{
    if( !pInputStream || !pMetadata )
        return CODEC_FAIL;

    // If the client is interested in metadata that can only be obtained by setting the
    // track in the codec, then do just that.
    if (pMetadata->UsesAttribute(MDA_DURATION) ||
        pMetadata->UsesAttribute(MDA_SAMPLING_FREQUENCY) ||
        pMetadata->UsesAttribute(MDA_CHANNELS) ||
        pMetadata->UsesAttribute(MDA_BITRATE))
    {
        stream_info_t streamInfo;
        if (SUCCEEDED(SetSong(pDataSource, pInputStream, streamInfo, pMetadata)))
            return CODEC_NO_ERROR;
        else
            return CODEC_FAIL;
    }
    else
    {
#ifdef DDOMOD_DJ_METADATAFILETAG
        if ( (pDataSource->GetClassID() == FAT_DATA_SOURCE_CLASS_ID)
            && (CMetadataFileTag::GetInstance()->FindAndParseTag(((CFatFileInputStream*)pInputStream)->m_pFile, pMetadata) > 0) )
        {
            DEBUGP( ARM_MP3, DBGLEV_INFO, "mp3:mft\n"); 
        }
        else
#endif
        {
            if (pMetadata && !FindID3v1Headers(*pInputStream, m_ID3v1size, pMetadata))
            {
                DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
                return CODEC_FAIL;
            }
            if (!FindID3v2Headers(*pInputStream, m_ID3v2size, pMetadata))
            {
                DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
                return CODEC_FAIL;
            }
        }
    }
    return CODEC_NO_ERROR;
}

/////////////////////////////////////
//
// Helper functions
//

static void AddStrippedString(IMetadata* pMetadata, int iAttributeID, const char* szValue, int iValueLength);

bool
FindID3v1Headers(IInputStream& file, unsigned long& ID3v1Size, IMetadata* pMetadata)
{
    DBEN( ARM_MP3 );
    
    unsigned char buf[128];
    file.Seek(IInputStream::SeekEnd, -ID3_V1_SIZE );
    int iValid = file.Read(buf, ID3_V1_SIZE);
    if (iValid < 0)
        return false;
    if(buf[0] == 'T' && buf[1] == 'A' && buf[2] == 'G')
    {
        ID3v1Size = ID3_V1_SIZE;

        // Set ID3Info
        if (pMetadata)
        {
            id3tag* pTag = (id3tag*)buf;

            // copy title
            //            TCHAR tszValue[31];
            if (pTag->title[0] && pMetadata->UsesAttribute(MDA_TITLE))
            {
                AddStrippedString(pMetadata, MDA_TITLE, pTag->title, 30);
            }

            // copy artist
            if (pTag->artist[0] && pMetadata->UsesAttribute(MDA_ARTIST))
            {
                AddStrippedString(pMetadata, MDA_ARTIST, pTag->artist, 30);
            }

            // copy album
            if (pTag->album[0] && pMetadata->UsesAttribute(MDA_ALBUM))
            {
                AddStrippedString(pMetadata, MDA_ALBUM, pTag->album, 30);
            }

            // copy year
            if (pTag->year[0] && (pTag->year[0] != ' ') && pMetadata->UsesAttribute(MDA_YEAR))
            {
                int iYear = (pTag->year[0] - '0') * 1000 + (pTag->year[1] - '0') * 100 + (pTag->year[2] - '0') * 10 + pTag->year[3] - '0';
                pMetadata->SetAttribute(MDA_YEAR, (void*)iYear);
            }

            // copy genre
            if (pTag->genre && (pTag->genre < gc_ID3v1GenreCount) && pMetadata->UsesAttribute(MDA_GENRE))
            {
                TCHAR tszGenre[128];
                pMetadata->SetAttribute(MDA_GENRE, (void*)CharToTchar(tszGenre, gc_szID3v1GenreTable[pTag->genre]));
            }

            int iCommentLength = 30;
            // check for id3v1.1 track number
            if (!pTag->comment[28] && pTag->comment[29])
            {
                iCommentLength = 28;
                if (pMetadata->UsesAttribute(MDA_ALBUM_TRACK_NUMBER))
                    pMetadata->SetAttribute(MDA_ALBUM_TRACK_NUMBER, (void*)(int)pTag->comment[29]);
            }

            // copy comment
            if (pTag->comment[0] && pMetadata->UsesAttribute(MDA_COMMENT))
            {
                AddStrippedString(pMetadata, MDA_COMMENT, pTag->comment, iCommentLength);
            }

        }
    }
    else
        ID3v1Size = 0;

    DBEX( ARM_MP3 );
    
    return true;
}

void
AddStrippedString(IMetadata* pMetadata, int iAttributeID, const char* szValue, int iValueLength)
{
    TCHAR tszValue[31];
    // strip trailing spaces
    const char* pch = szValue + iValueLength - 1;
    while ((pch >= szValue) && isspace(*pch))
        --pch;
    if (pch >= szValue)
    {
        CharToTcharN(tszValue, szValue, pch - szValue + 1);
        tszValue[pch - szValue + 1] = '\0';
        pMetadata->SetAttribute(iAttributeID, (void*)tszValue);
    }
}

static void AddID3v2String(IMetadata* pMetadata, int iAttributeID, ID3_Tag& tag, ID3_FrameID frameID);

#define _ID3_V2_
#define PARSE_V2_GENRE  // If true, then the ID3v2 genre field will be parsed for (x), where x
// is the ID3v1 genre index.

bool
FindID3v2Headers(IInputStream& file, unsigned long& ID3v2Size, IMetadata* pMetadata)
{	
    //Check for ID3v2 tag
    unsigned char buf[128];

    file.Seek(IInputStream::SeekStart,0);   // (epg,2/6/2001): with the lookups rearranged, single buf may leave the file in the wrong place.
    int iValid = file.Read(buf, 10);
    if (iValid < 0)
        return false;

    if(buf[0] == 0x49 && buf[1] == 0x44 && buf[2] == 0x33)
    {
	    ID3v2Size = (static_cast<unsigned long>(buf[6]) << 21 ) | 
				    (static_cast<unsigned long>(buf[7]) << 14) | 
				    (static_cast<unsigned long>(buf[8]) << 7) | 
				    static_cast<unsigned long>(buf[9]);
	    ID3v2Size += 10;
    }
    else
	    ID3v2Size = 0;

    if (!ID3v2Size || !pMetadata)
        return true;

#ifdef _ID3_V2_

    file.Seek(IInputStream::SeekStart, 0);
    ID3_Tag  myTag(&file);

    AddID3v2String(pMetadata, MDA_TITLE, myTag, ID3FID_TITLE);
    AddID3v2String(pMetadata, MDA_ARTIST, myTag, ID3FID_LEADARTIST);
    AddID3v2String(pMetadata, MDA_ALBUM, myTag, ID3FID_ALBUM);
#ifdef PARSE_V2_GENRE
    ID3_Frame *myFrame;
    if (pMetadata->UsesAttribute(MDA_GENRE) && (myFrame = myTag.Find(ID3FID_CONTENTTYPE)))
    {
        TCHAR tszBuff[1024], *pch = tszBuff;
	    if (myFrame->Field(ID3FN_TEXT).Get(tszBuff, 1024))
        {
            if (*pch == '(')
            {
                int iGenreID = 0;
                while (*++pch && (*pch >= '0') && (*pch <= '9'))
                    iGenreID = iGenreID * 10 + *pch - '0';
                if ((*pch == ')') && (iGenreID < gc_ID3v1GenreCount))
                    pMetadata->SetAttribute(MDA_GENRE, (void*)CharToTchar(tszBuff, gc_szID3v1GenreTable[iGenreID]));
                else
                    pMetadata->SetAttribute(MDA_GENRE, tszBuff);
            }
            else
                pMetadata->SetAttribute(MDA_GENRE, tszBuff);
        }
    }
#else   // PARSE_V2_GENRE
    AddID3v2String(pMetadata, MDA_GENRE, myTag, ID3FID_CONTENTTYPE);
#endif  // PARSE_V2_GENRE

#if 0
#ifdef UNICODE
	// I would have thougth that the id3v2 people would be smart and
	// encode the tags in double bytes, but no, they didn't
	// so we have to do the same thing we did for id3v1 and see if a
	// character is over 7F and assume the next byte is the lower
	// byte of a two byte character...  this sucks!
	byte_to_double_byte_KSC5601(v2_info.szTitle);
	byte_to_double_byte_KSC5601(v2_info.szArtist);
	byte_to_double_byte_KSC5601(v2_info.szCopyright);
	byte_to_double_byte_KSC5601(v2_info.szGenre);

	// do we need to map these characters to unicode?
	convert_from_KSC5601(v2_info.szTitle);
	convert_from_KSC5601(v2_info.szArtist);
	convert_from_KSC5601(v2_info.szCopyright);
	convert_from_KSC5601(v2_info.szGenre);
#endif // #ifdef UNICODE

#endif  // 0

#endif // _ID3_V2_


	return true;
}


void
AddID3v2String(IMetadata* pMetadata, int iAttributeID, ID3_Tag& tag, ID3_FrameID frameID)
{
    ID3_Frame *myFrame;

    if (pMetadata->UsesAttribute(iAttributeID) && (myFrame = tag.Find(frameID)))
    {
        TCHAR tszBuff[1024];
	    if (myFrame->Field(ID3FN_TEXT).Get(tszBuff, 1024))
            pMetadata->SetAttribute(iAttributeID, tszBuff);
    }
}

// _InitBitstream initializes the stream to the given byte offset in the MP3 file.
ERESULT
CMP3Codec::_InitBitstream(unsigned long ulFilePos)
{
    DBEN( ARM_MP3 );
    int BufferPosition;

    //    if( m_ulFilePos & 0x01 ) {
    //        DEBUG( ARM_MP3, DBGLEV_ERROR, "odd offset request %d\n", ulFilePos );
    //    }
    
    // If requested position is already in local buffer
    unsigned int LowerBound = m_ulFilePos - m_ulValid;
    unsigned int UpperBound = m_ulFilePos;
    if (LowerBound <= ulFilePos &&
        ulFilePos <= UpperBound) {

        // Set buffer position
        BufferPosition = ulFilePos - LowerBound;
    }
    else {

        // Seek to the correct block of data in the file
        long Seek = m_ulFilePos;
        if( m_pInputStream->CanSeek() ) {
            if( ulFilePos & 0x01 ) {
                ulFilePos &= ~(0x01);
                DEBUG( ARM_MP3, DBGLEV_ERROR, "aligning seek dest to %d\n", ulFilePos );
            }
            Seek = m_pInputStream->Seek(IInputStream::SeekStart, ulFilePos );
        }
        
        if (Seek < 0) {
            DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
            return CODEC_FAIL;
        }

        // Update internal buffer state
        m_ulFilePos = Seek;
        m_ulValid = 0;

        // Set buffer position
        BufferPosition = 0;
    }

    // Re-initialize the MP3 library
    InitMP3Audio(m_pMPEGInstanceData);    

    // Initialize the bitstream pointer structure.
    m_sBS.bufptr = (unsigned int *)(m_pcEncodedData + (BufferPosition & ~0x3));
    m_sBS.bitidx = 8 * (BufferPosition & 0x3);

    DBEX( ARM_MP3 );
    return CODEC_NO_ERROR;
}

ERESULT
CMP3Codec::_FillInternalBuffer(void)
{
    DBEN( ARM_MP3 );

    // Make sure the bitstream pointer structure makes sense (i.e. it is
    // pointing to a valid bit in a word, not at the bit following the last
    // bit of the previous word).
    if(m_sBS.bitidx == 32)
    {
        m_sBS.bufptr++;
        m_sBS.bitidx = 0;
    }
    
    // If the bitstream pointer is not at the beginning of the local bitstream
    // buffer, then copy the remaining data back to the beginning.
    if((int)m_sBS.bufptr != (int)m_pcEncodedData)
    {
        // Copy the remainder of the data from the end of the local bitstream
        // buffer to the beginning.
        // dc- 7/19/02, assume a corrupt bufptr comes from the decoder library not liking the track
        //              and freaking out (rather than a semi-classic memory overrun) and exit gracefully
        if( (m_sBS.bufptr < (unsigned int*)m_pcEncodedData) ||
            (m_sBS.bufptr > (unsigned int*)(&m_pcEncodedData[sizeof(m_pcEncodedData)-1])) ) {
            DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
            return CODEC_FAIL;
        }
        
        //        DBASSERT(ARM_MP3, m_sBS.bufptr >= (unsigned int *)m_pcEncodedData, "Bufptr out of bounds\n");
        //        DBASSERT(ARM_MP3, m_sBS.bufptr < (unsigned int *)(&m_pcEncodedData[sizeof(m_pcEncodedData)]), "Bufptr out of bounds\n");
        //        DBASSERT(ARM_MP3, (sizeof(m_pcEncodedData) - ((int)m_sBS.bufptr - (int)m_pcEncodedData)) >= 0, "Bufptr out of bounds\n");

        memcpy(m_pcEncodedData, m_sBS.bufptr,
            sizeof(m_pcEncodedData) - ((int)m_sBS.bufptr - (int)m_pcEncodedData));

        // Update the count of valid bytes in the local bitstream buffer.
        m_ulValid -= (int)m_sBS.bufptr - (int)m_pcEncodedData;

        // Update the bitstream pointer structure.
        m_sBS.bufptr = (unsigned int *)m_pcEncodedData;
    }

    // See if there is any additional MP3 data that we can copy into the local
    // bitstream buffer.
    while (m_ulValid < sizeof(m_pcEncodedData)) {

        // Compute the number of bytes to read.
        unsigned long ulToRead = sizeof(m_pcEncodedData) - m_ulValid;

        // Read MP3 data into the end of the local bitstream buffer.
        long lRead = m_pInputStream->Read((unsigned char *)(m_pcEncodedData + m_ulValid), ulToRead);
        
        if (lRead < 0) {
            DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
            return CODEC_FAIL;
        }
	
        // If no more data in stream then return end of file
        if (lRead == 0) {
            // dc- only generate a fail message if this was an unexpected EOF
            if( m_ulFilePos != m_ulFileLength ) {
                DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
                return CODEC_FAIL;
            } else {
                return CODEC_END_OF_FILE;
            }
        }
	
        // Update the position in the file.
        m_ulFilePos += lRead;

        // Since the ARM MP3 decoder needs the bytes within each word swapped,
        // perform the byte swap now.
        lRead -= (lRead % 4);
        if (lRead > 0) {
            swap_bytes(m_pcEncodedData + m_ulValid, lRead);
        }

        // Update the count of valid bytes in the local bitstream buffer.
        m_ulValid += lRead;
    }

    DBEX( ARM_MP3 );
    return CODEC_NO_ERROR;
}

// _FindNextFrame finds the next frame in the input MP3 bitstream.
ERESULT
CMP3Codec::_FindNextFrame(void)
{
    DBEN( ARM_MP3 );

    tMPEGStatus MPEGStatus;
    int Status;

    // Fill internal buffer with enough data to find a frame
    Status = _FillInternalBuffer();
    if (FAILED(Status)) {
        // dc- dont print errors on valid EOF
        if( Status != CODEC_END_OF_FILE ) {
            DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
        }
        return Status;
    }

    // Record current bufptr and set number of bytes of the stream
    // that we will search for a valid frame header in.
    unsigned int * PreviousBufptr = m_sBS.bufptr;
    int BytesToSearch = MP3_MAX_BITS_REQUIRED / 8;

    // Begin search.
    for (;;) {
	
        // Find the next sync word.
        unsigned int ValidBits = (m_ulValid * 8) -
                                 (((m_sBS.bufptr - (unsigned int *)m_pcEncodedData) * 32) + m_sBS.bitidx);

        MPEGStatus = MP3SearchForSyncword(m_pMPEGInstanceData, &m_sBS, ValidBits);
        if (MPEGStatus != eNoErr)
        {
            // We could not find a sync word.  There's no point in trying again,
            // since we couldn't even find bits resembling a syncword.
            DEBUG_FUNCFAIL("-%s %d %d\n", __FUNCTION__, __LINE__, MPEGStatus);
            return CODEC_FAIL;
        }

        // m_sBS now points to the beginning of the syncword.
	
        // Update number of bytes to search.
        BytesToSearch -= m_sBS.bufptr - PreviousBufptr;
	
        // Check for enough data to decode info.
        char * UpperBound = m_pcEncodedData + m_ulValid;
        char * LowerBound = (char *)m_sBS.bufptr;
        int DataNeeded = (MP3_NINFOBITS / 8);
	
        // bitidx is actually where bitstream pointer is, so use <= instead of <.
        if ((int)(UpperBound - LowerBound) <= DataNeeded) {
	    
            // Get more data
            Status = _FillInternalBuffer();
            if (FAILED(Status)) {
                DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
                return Status;
            }
        }
	
        // Update pointer for recording number of bytes to search.
        PreviousBufptr = m_sBS.bufptr;
	
        // Decode the header.
        memset((void*)&m_sHdr, 0, sizeof(m_sHdr));
        MPEGStatus = MP3DecodeInfo(m_pMPEGInstanceData, &m_sBS, &m_sHdr);
        if (MPEGStatus != eNoErr)
        {
	    
            // We could not decode the header.  Update number of bytes
            // left to search.  If less than zero than return error,
            // otherwise search again.
            BytesToSearch -= m_sBS.bufptr - PreviousBufptr;
            if (BytesToSearch < 0) {
                DEBUG_FUNCFAIL("-%s %d %d\n", __FUNCTION__, __LINE__, MPEGStatus);
                return CODEC_FAIL;
            }
        }
        else {
	    
            // Success.
            DBEX( ARM_MP3 );
            return CODEC_NO_ERROR;
        }
    }
}

// _DecodeVBRHeader determines if the current frame is a Xing VBR header
// frame and decodes it if it is.
unsigned long
CMP3Codec::_DecodeVBRHeader()
{
    DBEN( ARM_MP3 );
    
    unsigned char *pucPtr;
    unsigned long ulFlags, ulFrames, ulIdx, ulFrameSize;

    // Get the offset to the beginning of the current frame.
    pucPtr = (unsigned char *)m_sBS.bufptr + (m_sBS.bitidx >> 3);

    // Determine the offset of the header.
    if(m_ulSampleRate >= 32000)
    {
        // This is a MPEG-1 file.  Is this a mono or stereo stream?
        if(m_ulChannels == 1)
        {
            // This is a mono stream, so the header starts at the 17st byte of
            // the frame data.
            pucPtr += 17;
        }
        else
        {
            // This is a stereo stream, so the header starts at the 32nd byte
            // of the frame data.
            pucPtr += 32;
        }

        // The frame size for MPEG-1 files is 1152.
        ulFrameSize = 1152;
    }
    else
    {
        // This is a MPEG-2 file.  Is this a mono or stereo stream?
        if(m_ulChannels == 1)
        {
            // This is a mono stream, so the header starts at the 9th byte of
            // the frame data.
            pucPtr += 9;
        }
        else
        {
            // This is a stereo stream, so the header starts at the 17th byte
            // of the frame.
            pucPtr += 17;
        }

        // The frame size for MPEG-2 files is 576.
        ulFrameSize = 576;
    }

    // See if the "Xing" signature appears in the frame.
    if ((pucPtr[3] != 'X') || (pucPtr[2] != 'i') ||
        (pucPtr[1] != 'n') || (pucPtr[0] != 'g'))
    {
        // There is no VBR header, or this is not a Xing VBR header, so return a failure.
        //        DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
        return(0);
    }

    // Skip past the signature.
    pucPtr += 4;

    // Get the header flags from the stream.
    ulFlags = (pucPtr[3] << 24) | (pucPtr[2] << 16) |
              (pucPtr[1] << 8) | pucPtr[0];
    pucPtr += 4;

    // If the number of frames exists in the header, then extract it now.
    if(ulFlags & 1)
    {
        ulFrames = (pucPtr[3] << 24) | (pucPtr[2] << 16) |
                   (pucPtr[1] << 8) | pucPtr[0];
        pucPtr += 4;
    }
    else
        // The count of frames does not exist, so we will not be able to
        // properly support this file.
    {
        DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
        return(0);
    }

    // If the file size exists in the header, then skip it.
    if(ulFlags & 2)
        pucPtr += 4;

    // If the seek point table exists in the header, then read it in now.
    if(ulFlags & 4)
    {
        // There are 100 entries in the table, so copy them one by one.
        for(ulIdx = 0; ulIdx < 100; ulIdx++)
            m_ucVBRSeek[ulIdx ^ 3] = *pucPtr++;

        // Since the seek point table exists in the header, indicate that this
        // is a VBR file.
        m_bIsVBR = 1;
    }
    else
    {
        //
        // Since there is no seek point table in the header, treat this file as
        // a non-VBR file.
        //
        m_bIsVBR = 0;
    }

    // Now, compute the bitrate from the file size and the count of frames in
    // the file.
    m_ulBitRate = ((m_ulFileLength / ulFrames) * m_ulSampleRate) / (ulFrameSize / 8);

    // Success.
    DBEX( ARM_MP3 );
    return(1);
}

void
CMP3Codec::Stats(void)
{
    diag_printf("sizeof(CMP3Codec): %d\n", sizeof(CMP3Codec));
}
