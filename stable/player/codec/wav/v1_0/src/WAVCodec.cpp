// WAVCodec.cpp: .wav codec implementation
// danc@iobjects.com 12/06/01
// (c) Interactive Objects


#include <_modules.h>
#include <datastream/input/InputStream.h>
#include <content/common/Metadata.h>
#include "WAVCodec.h"
#ifdef DDOMOD_DJ_METADATAFILETAG
#include <main/metadata/metadatafiletag/MetadataFileTag.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datastream/fatfile/FileInputStream.h>
#endif

#include <string.h>   // strncmp

#include <util/debug/debug.h>

DEBUG_MODULE(DBG_WAV_CODEC);
DEBUG_USE_MODULE(DBG_WAV_CODEC);

REGISTER_CODEC( CWAVCodec, CODEC_WAV_KEY );

typedef struct wav_header_s
{
    char RIFF[4];   // should be "RIFF"
    unsigned int   FileSize;
    char WAVE[4];   // should be "WAVE"
    unsigned int   Chunk1ID;
    unsigned int   Chunk1Size;
    unsigned short AudioFormat;
    unsigned short NumChannels;
    unsigned int   SampleRate;
    unsigned int   ByteRate;
    unsigned short BlockAlign;
    unsigned short BitsPerSample;
    unsigned int   Chunk2ID;
    unsigned int   Chunk2Size; // data length
} wav_header_t;


#define WAV_HEADER_SIZE 44


ERESULT CWAVCodec::SetSong( IDataSource* pDataSource, IInputStream* pInputStream,
                            stream_info_t& streamInfo, IMetadata* pMetadata = 0 ) 
{
    unsigned char pWaveHeader[ WAV_HEADER_SIZE ] __attribute__((aligned(4)));;
    if( WAV_HEADER_SIZE > pInputStream->Read( pWaveHeader, WAV_HEADER_SIZE ) ) {
        return CODEC_BAD_FORMAT;
    }

    m_iGranuleSize = 2 * pInputStream->GetInputUnitSize();

    wav_header_t* pHeader = (wav_header_t*)(&pWaveHeader[0]);

    if( strncmp( pHeader->RIFF, "RIFF", 4 ) != 0 ||
        strncmp( pHeader->WAVE, "WAVE", 4 ) != 0 ) {
        return CODEC_BAD_FORMAT;
    }

    // only accept 16 bit PCM
    if( pHeader->BitsPerSample != 16 ) {
        return CODEC_BAD_FORMAT;
    }

    // figure out number of channels and sample rate
    m_uiNumChannels = (unsigned int)pHeader->NumChannels;
    streamInfo.OutputChannels = streamInfo.Channels = m_uiNumChannels;
    
    streamInfo.SamplingFrequency = m_uiSampleRate = pHeader->SampleRate;
    
    // calculate the duration
    streamInfo.Duration = pHeader->Chunk2Size / (m_uiNumChannels * 2 /* bytes per sample */ * m_uiSampleRate);
    streamInfo.Bitrate  = pHeader->ByteRate * 8;

    if( pMetadata ) {
#ifdef DDOMOD_DJ_METADATAFILETAG
        if ( (pDataSource->GetClassID() == FAT_DATA_SOURCE_CLASS_ID)
            && (CMetadataFileTag::GetInstance()->FindAndParseTag(((CFatFileInputStream*)m_pInputStream)->m_pFile, pMetadata) > 0) )
                DEBUGP( DBG_WAV_CODEC, DBGLEV_INFO, "wav:mft\n"); 
#endif
        pMetadata->SetAttribute( MDA_DURATION, (void*)streamInfo.Duration );
        pMetadata->SetAttribute( MDA_SAMPLING_FREQUENCY, (void*)streamInfo.SamplingFrequency );
        pMetadata->SetAttribute( MDA_CHANNELS, (void*)streamInfo.Channels );
        pMetadata->SetAttribute( MDA_BITRATE,  (void*)streamInfo.Bitrate );
    }

    m_pInputStream = pInputStream;
    m_fTimePos = 0;
    return CODEC_NO_ERROR;
}

ERESULT CWAVCodec::GetMetadata( IDataSource* pDataSource, IMetadata* pMetadata,
                                IInputStream* pInputStream ) 
{
    if( !pInputStream || !pMetadata ) {
        return CODEC_FAIL;
    }

    stream_info_t unused;
    if (pMetadata->UsesAttribute(MDA_DURATION) ||
            pMetadata->UsesAttribute(MDA_SAMPLING_FREQUENCY) ||
            pMetadata->UsesAttribute(MDA_CHANNELS) ||
            pMetadata->UsesAttribute(MDA_BITRATE))
        return this->SetSong( pDataSource, pInputStream, unused, pMetadata );

#ifdef DDOMOD_DJ_METADATAFILETAG
    // process MFTs from the hard drive.
    if ( (pDataSource->GetClassID() == FAT_DATA_SOURCE_CLASS_ID)
        && (pMetadata && CMetadataFileTag::GetInstance()->FindAndParseTag(((CFatFileInputStream*)pInputStream)->m_pFile, pMetadata) > 0) )
            DEBUGP( DBG_WAV_CODEC, DBGLEV_INFO, "wav:mft\n"); 
#endif

    return CODEC_NO_ERROR;
}

