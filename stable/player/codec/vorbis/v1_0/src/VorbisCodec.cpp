// VorbisCodec.cpp: Ogg/Vorbis I codec
// monty@xiph.org 20020106
// (c) Interactive Objects

#include "../src/codec.h"
#include "../src/vorbisfile.h"
#include <codec/vorbis/VorbisCodec.h>
#include <content/common/Metadata.h>
#include <datastream/input/InputStream.h>

#include <util/rbuf/rbuf.h>
#include <cyg/infra/diag.h>

REGISTER_CODEC( CVorbisCodec, CODEC_VORBIS_KEY );

CVorbisCodec::CVorbisCodec(){
  memset(&m_VorbisFile,0,sizeof(m_VorbisFile));
    
  m_pInputStream = NULL;
  m_pWriteBuf    = NULL;
  m_eof          = 0;

}

CVorbisCodec::~CVorbisCodec(){
  ov_clear(&m_VorbisFile);
}

// This is not a single frame decoding; Dharma assumes that the same
// amount of data comes out each time (D'oh).  So we have to buffer
// and simulate.
// The extra copy sucks, but there's no alternative in the current API.

ERESULT CVorbisCodec::DecodeFrame( unsigned long& TimePos ) {
  if( m_pInputStream == NULL || m_pWriteBuf == NULL ) {
    return CODEC_FAIL;
  }
  
  if(m_eof)return CODEC_END_OF_FILE;

  int linknum;
  unsigned int totalSpace = rb_write_avail( m_pWriteBuf );
  unsigned int nowSpace,usedSpace;
  
  if(totalSpace==0){
    return CODEC_NO_WORK;
  }

  while(totalSpace){
    rb_write_data( m_pWriteBuf, totalSpace, &nowSpace );
    char *ptr=(char *)rb_write_ptr( m_pWriteBuf );
    usedSpace=0;
    
    while(nowSpace){
      long ret=ov_read(&m_VorbisFile,
		       ptr+usedSpace,
		       nowSpace,
		       &linknum);
      if(ret==0)goto eof; /* actually pad it out */
      if(ret>-1){
	usedSpace+=ret;
	totalSpace-=ret;
	nowSpace-=ret;
      }
    }
    
    rb_write_done( m_pWriteBuf, usedSpace );
  }
  
  TimePos= static_cast<unsigned long>(ov_time_tell(&m_VorbisFile)/1000);
  return CODEC_NO_ERROR;
  
 eof:
  m_eof=1;
  return CODEC_NO_ERROR;
  
}

static size_t ov_read_wrapper(void *ptr, size_t size, size_t nmemb, 
			      void *datasource){

  return((IInputStream *)datasource)->Read(ptr,size*nmemb);
}

static int ov_seek_wrapper(void *datasource, ogg_int64_t offset, int whence){
  switch(whence){
  case SEEK_SET:
    return ((IInputStream *)datasource)->Seek(IInputStream::SeekStart,offset);
  case SEEK_CUR:
    return ((IInputStream *)datasource)->Seek(IInputStream::SeekCurrent,offset);
  case SEEK_END:
    return ((IInputStream *)datasource)->Seek(IInputStream::SeekEnd,offset);
  }

  return(-1);
}

static long ov_tell_wrapper(void *datasource){
  return ((IInputStream *)datasource)->Position();
}

static int ov_close_wrapper(void *dummy){
  return(0);
}

ERESULT CVorbisCodec::SetSong( IDataSource* pDS, IInputStream* pInputStream, 
			       stream_info_t& streamInfo, 
			       IMetadata* pMetadata ) {

  if(m_pInputStream)ov_clear(&m_VorbisFile);
  
  m_pInputStream = pInputStream;
  
  // open the stream
  ov_callbacks callbacks={
    ov_read_wrapper,
    ov_seek_wrapper,
    ov_close_wrapper,
    ov_tell_wrapper,
  };
  
  int ret=ov_open_callbacks(pInputStream,&m_VorbisFile,NULL,0,callbacks);
  if(ret==-1){
    m_pInputStream = NULL;
    return CODEC_BAD_FORMAT;
  }
  
  // set the details for streamdata
  vorbis_info *vi=ov_info(&m_VorbisFile,0);
  
  streamInfo.OutputChannels = streamInfo.Channels = vi->channels;
  streamInfo.SamplingFrequency = vi->rate;
  streamInfo.Duration = ov_time_total(&m_VorbisFile,-1)/1000;
  
  streamInfo.Bitrate = ov_bitrate(&m_VorbisFile,-1);
  
  if (pMetadata){
    pMetadata->SetAttribute(MDA_DURATION, (void*)streamInfo.Duration);
    pMetadata->SetAttribute(MDA_SAMPLING_FREQUENCY, (void*)streamInfo.SamplingFrequency);
    pMetadata->SetAttribute(MDA_CHANNELS, (void*)streamInfo.Channels);
    pMetadata->SetAttribute(MDA_BITRATE, (void*)streamInfo.Bitrate);
  }
    
  return CODEC_NO_ERROR;
}

ERESULT CVorbisCodec::Seek( unsigned long& secSeek ) {
  if(!ov_seekable(&m_VorbisFile)) return CODEC_FAIL;
  int ret=ov_time_seek(&m_VorbisFile,secSeek*1000);
  if(ret)return CODEC_FAIL;
  return CODEC_NO_ERROR;
}

// be maximally pessimal right now and create a new instance
ERESULT CVorbisCodec::GetMetadata( IDataSource* pDS, IMetadata* pMetadata, 
				   IInputStream* pInputStream ) {
  if( !pInputStream || !pMetadata )
    return CODEC_FAIL;

  OggVorbis_File v;
  // open the stream
  ov_callbacks callbacks={
    ov_read_wrapper,
    ov_seek_wrapper,
    ov_close_wrapper,
    ov_tell_wrapper,
  };
  
  int ret=ov_open_callbacks(pInputStream,&v,NULL,0,callbacks);
  if(ret==-1)return CODEC_BAD_FORMAT;
  
  // set the details for streamdata
  vorbis_info *vi=ov_info(&v,0);
  
  
  unsigned long ch   =vi->channels;
  unsigned long rate = vi->rate;
  unsigned long dur  = ov_time_total(&v,-1);
  unsigned long br   = ov_bitrate(&v,-1);
  
  ov_clear(&v);

  // stream metadata only right now.  Add comments after demo
  pMetadata->SetAttribute(MDA_DURATION, (void *)dur);
  pMetadata->SetAttribute(MDA_SAMPLING_FREQUENCY, (void *)rate);
  pMetadata->SetAttribute(MDA_CHANNELS, (void *)ch);
  pMetadata->SetAttribute(MDA_BITRATE, (void *)br);
  return CODEC_NO_ERROR;
}

rbuf_writer_t* CVorbisCodec::GetWriteBuf() const 
{
  return m_pWriteBuf;
}
void CVorbisCodec::SetWriteBuf( rbuf_writer_t* pWriteBuf ) 
{
  m_pWriteBuf = pWriteBuf;
}

int CVorbisCodec::GetOutputUnitSize() const 
{
  return CHUNKSIZE;
}

IInputStream* CVorbisCodec::GetInputStream() const 
{
  return m_pInputStream;
}

void CVorbisCodec::Stats() 
{}
