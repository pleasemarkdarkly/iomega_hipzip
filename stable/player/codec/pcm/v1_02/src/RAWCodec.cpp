// RAWCodec.cpp: straight PCM support
// danc@iobjects.com 12/06/01
// (c) Interactive Objects

#include <_modules.h>
#include <codec/pcm/RAWCodec.h>
#include <content/common/Metadata.h>
#include <datastream/input/InputStream.h>
#ifdef DDOMOD_DJ_METADATAFILETAG
#include <main/metadata/metadatafiletag/MetadataFileTag.h>
#endif
#include <datasource/fatdatasource/FatDataSource.h>
#include <datastream/fatfile/FileInputStream.h>

#include <util/rbuf/rbuf.h>

#include <util/debug/debug.h>
DEBUG_MODULE(DBG_RAW_CODEC);
DEBUG_USE_MODULE(DBG_RAW_CODEC);

CRAWCodec::CRAWCodec() 
{
    m_uiSampleRate = 0;
    m_uiNumChannels = 0;
    m_fTimePos = 0;
    
    m_pInputStream = NULL;
    m_pWriteBuf = NULL;
}

CRAWCodec::~CRAWCodec()         
{
}

ERESULT CRAWCodec::DecodeFrame( unsigned long& TimePos ) 
{
    if( m_pInputStream == NULL || m_pWriteBuf == NULL ) {
        return CODEC_FAIL;
    }
    
    unsigned int Space = (signed)rb_write_avail( m_pWriteBuf );

    if( Space >= (unsigned)m_iGranuleSize ) {
        unsigned int actual;
        rb_write_data( m_pWriteBuf, Space, &actual );

        int Read = m_pInputStream->Read( rb_write_ptr( m_pWriteBuf ), actual );
        if( Read < 0 )
        {
            DEBUG(DBG_RAW_CODEC, DBGLEV_ERROR, "Error in read, returning CODEC_FAIL\n");
            return CODEC_FAIL;
        } else if( Read == 0 ) {
            rb_write_eof( m_pWriteBuf );
            return CODEC_END_OF_FILE;
        } else if( Read < actual ) {
            actual = Read;
        }

        rb_write_done( m_pWriteBuf, actual );

        if( actual < Space ) {
            unsigned int amt = Space - actual;
            if( amt >= (unsigned)m_iGranuleSize ) {
                rb_write_data( m_pWriteBuf, amt, &actual );
                unsigned int Read = m_pInputStream->Read( rb_write_ptr( m_pWriteBuf ), actual );
                if( Read < 0 )
                {
                    DEBUG(DBG_RAW_CODEC, DBGLEV_ERROR, "Error in read, returning CODEC_FAIL\n");
                    return CODEC_FAIL;
                }
                else if( Read == 0 ) {
                    rb_write_eof( m_pWriteBuf );
                    return CODEC_END_OF_FILE;
                } else if( Read < actual ) {
                    actual = Read;
                }
                rb_write_done( m_pWriteBuf, actual );
            }
        }
        m_fTimePos += (1000.0f * static_cast<float>(Space / m_uiNumChannels / 2) / m_uiSampleRate);
        TimePos = static_cast<unsigned long>(m_fTimePos / 1000.0f);
        return CODEC_NO_ERROR;
    }
    else {
        return CODEC_NO_WORK;
    }
}

ERESULT CRAWCodec::SetSong( IDataSource* pDataSource, IInputStream* pInputStream,
                            stream_info_t& streamInfo, IMetadata* pMetadata ) 
{
    m_pInputStream = pInputStream;

    // we dont know anything about the stream... so...
    streamInfo.OutputChannels = streamInfo.Channels = m_uiNumChannels = 2;
    streamInfo.SamplingFrequency = m_uiSampleRate   = 44100;
    streamInfo.Duration = pInputStream->Length() / (2 /* channels */ * 2 /* bytes per sample */ * 44100 /* sampling freq */);
    streamInfo.Bitrate = 2 /* channels */ * 2 /* bytes per sample */ * 44100 /* sampling freq */ * 8 /* bits per byte */;
    m_iGranuleSize = 2 * m_pInputStream->GetInputUnitSize();

    if (pMetadata)
    {
#ifdef DDOMOD_DJ_METADATAFILETAG
        if ( (pDataSource->GetClassID() == FAT_DATA_SOURCE_CLASS_ID)
            && (CMetadataFileTag::GetInstance()->FindAndParseTag(((CFatFileInputStream*)m_pInputStream)->m_pFile, pMetadata) > 0) )
                DEBUGP( DBG_RAW_CODEC, DBGLEV_INFO, "raw:mft\n"); 
#endif
        pMetadata->SetAttribute(MDA_DURATION, (void*)streamInfo.Duration);
        pMetadata->SetAttribute(MDA_SAMPLING_FREQUENCY, (void*)streamInfo.SamplingFrequency);
        pMetadata->SetAttribute(MDA_CHANNELS, (void*)streamInfo.Channels);
        pMetadata->SetAttribute(MDA_BITRATE, (void*)streamInfo.Bitrate);

    }
    
    m_fTimePos = 0;
    return CODEC_NO_ERROR;
}

ERESULT CRAWCodec::Seek( unsigned long& secSeek ) 
{
    if( !m_pInputStream->CanSeek() ) {
        return CODEC_FAIL;
    }
    
    // convert seconds to byte offset
    unsigned long offset = (secSeek * (m_uiSampleRate * m_uiNumChannels * 2));

    // convert offset to a media-happy value
    if( offset % m_iGranuleSize ) {
        offset -= (offset % m_iGranuleSize);
    }
    unsigned long actual = m_pInputStream->Seek( IInputStream::SeekStart, offset );
    
    m_fTimePos = (1000.0f * static_cast<float>(actual / m_uiNumChannels / 2) / m_uiSampleRate);
    secSeek = static_cast<unsigned long>(m_fTimePos / 1000.0f);

    return CODEC_FAIL;
}

ERESULT CRAWCodec::GetMetadata( IDataSource* pDataSource, IMetadata* pMetadata, IInputStream* pInputStream ) 
{
    if( !pInputStream || !pMetadata )
        return CODEC_FAIL;

#ifdef DDOMOD_DJ_METADATAFILETAG
    // process MFTs from the hard drive.
    if ( (pDataSource->GetClassID() == FAT_DATA_SOURCE_CLASS_ID)
        && (pMetadata && CMetadataFileTag::GetInstance()->FindAndParseTag(((CFatFileInputStream*)pInputStream)->m_pFile, pMetadata) > 0) )
            DEBUGP( DBG_RAW_CODEC, DBGLEV_INFO, "raw:mft\n"); 
#endif
    // The only metadata that can be retrieved is stream info.
    pMetadata->SetAttribute(MDA_DURATION,
                            (void*)(pInputStream->Length() / (2 /* channels */ * 2 /* bytes per sample */ * 44100 /* sampling freq */)));
    pMetadata->SetAttribute(MDA_SAMPLING_FREQUENCY, (void*)44100);
    pMetadata->SetAttribute(MDA_CHANNELS, (void*)2);
    pMetadata->SetAttribute(MDA_BITRATE,
                            (void*)(2 /* channels */ * 2 /* bytes per sample */ * 44100 /* sampling freq */ * 8 /* bits per byte */));
    return CODEC_NO_ERROR;
}

int CRAWCodec::GetOutputUnitSize() const 
{
    return m_iGranuleSize;
}

void CRAWCodec::Stats() 
{}
