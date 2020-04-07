// mmc_util.c: utility routines used by the protocol layer
// Dan Conti 02/20/01 danc@iobjects.com
// (c) Interactive Objects. Buy the software and nobody gets hurt.

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include "mmc_util.h"
#include "mmc_lowl.h"

#include "mmc_debug.h"

static cyg_uint16 CalculateCmdCRC( cyg_uint16, cyg_uint32 );
static cyg_uint16 CalculateCRC7( cyg_uint8*, cyg_uint16, cyg_uint16, cyg_uint8 );
static cyg_uint32 mmcReceiveResponse( mmc_controller_info_t*, cyg_uint16 );
static cyg_uint32 mmcAnalyzeResponse( mmc_controller_info_t* );

cyg_uint32 mmcCommandAndResponse( mmc_controller_info_t* mci, cyg_uint16 cmd,
				  cyg_uint32 arg, cyg_uint16 blks,
				  cyg_uint16 resp_type )
{
  cyg_uint32 errcode;
  cyg_uint16 i,lim;
  cyg_uint16 crc;

  crc = CalculateCmdCRC( cmd, arg );
  errcode = MMCSendCommand( mci, cmd, arg, crc );
  
  if( errcode != MMC_ERR_NONE ) {
    diag_printf("mmcsendcommand failed\n");
    return errcode;
  }

  // resp type 0 == no response
  if( resp_type == MMC_RESP_0 ) {
    return MMC_ERR_NONE;
  }

  lim = MMC_CID_LENGTH >> 1;
  for( i = 0; i < lim; i++ ) {
    mci->card_info.last_response[i] = 0;
  }

  errcode = mmcReceiveResponse( mci, resp_type );
  if( errcode == MMC_ERR_NONE && resp_type == MMC_RESP_1 ) {
    errcode = mmcAnalyzeResponse( mci );
  }
    
  return errcode;
}

static cyg_uint32 mmcReceiveResponse( mmc_controller_info_t* mci, cyg_uint16 resp_type ) 
{
  cyg_uint16 resplen;
  
  if( resp_type == MMC_RESP_2 ) {
    resplen = MMC_RESP2_BITLEN;
    //    resplen = MMC_CID_LENGTH - 1;
  } else {
    resplen = MMC_RESP1_3_BITLEN;
  }

  return MMCGetResponse( mci, resplen );
}

static cyg_uint32 mmcAnalyzeResponse( mmc_controller_info_t* mci ) 
{
  cyg_uint8* respbuff;
  cyg_uint32 status;

  respbuff = (cyg_uint8*) mci->card_info.last_response;

  status =
    ( (respbuff[1] & 0xff) << 24 ) |
    ( (respbuff[2] & 0xff) << 16 ) |
    ( (respbuff[3] & 0xff) <<  8 ) |
    ( (respbuff[4] & 0xff) <<  0 );

  
  mci->card_info.card_state = (cyg_uint16)(respbuff[3] & 0x1f );
  mci->card_info.card_ready = mci->card_info.card_state & 0x01;
  mci->card_info.card_state >>= 1;
  

  MDEBUG(" AnalyzeResponse: status = 0x%08x\n", status);
  status >>= 15;
  if( status == 0 ) {
    return MMC_ERR_NONE;
  }

  // there are a lot of error codes here. rather than actual
  // parse codes that we will never use, just give back a general
  return MMC_ERR_GENERAL;
}

// this routine is called from the ASM routines exclusively
cyg_uint16 CalculateDataCRC16( cyg_uint8* buf, cyg_uint16 len ) 
{
  cyg_uint8   *ppData;
  cyg_uint32  rreg, rtemp;
  cyg_uint16  ibyte, ibit;
  
  rreg = 0L;
  ppData = (cyg_uint8 *)buf;
  
  for (ibyte = 0; ibyte < len; ibyte++)
  {
    rtemp = ((cyg_uint32)ppData[ibyte]) << (0x10 - 7);
    for (ibit = 0; ibit < 8; ibit++)
    {
      rreg <<= 1;
      rreg ^= ( ((rtemp ^ rreg) & 0x10000L) ? 0x11021L : 0);
      rtemp <<= 1;
    }
  }
  
  return ((cyg_uint16)(rreg & 0x0FFFFL));
}

static cyg_uint16 CalculateCRC7( cyg_uint8* buf, cyg_uint16 offset, cyg_uint16 length, cyg_uint8 is_response )
{
  cyg_uint16  ibyte, ibit;
  cyg_uint16  rreg, crc_byte, dtmp;

  if ( is_response )
    crc_byte = 0x89;
  else
    crc_byte = 0x09;

  buf    += offset;
  length -= offset;
  
  rreg = 0;
  
  for (ibyte = 0; ibyte < (length-1); ibyte++)
  {
    dtmp = (cyg_uint16)( buf[ibyte] ); 
    for (ibit = 0; ibit < 8; ibit++)
    {
      rreg <<= 1;
      dtmp &= 0xFF;
      rreg ^= (((dtmp ^ rreg) & 0x80) ? crc_byte : 0);
      dtmp <<= 1;
    }
  }
  
  return (rreg&0xFF);
}

// take cmd - MMC_CMD_STARTBIT and index against this table
// this makes the assumption that you only have one card per bus. :)
static const cyg_uint16 mmc_crc_table[] =  
{
  0x0095,  // GO_IDLE_STATE
  0x0143,  // SEND_OP_COND
  0x004D,  // ALL_SEND_CID
  0x017F,  // SET_RELATIVE_ADDR
  0x0000,  // SET_DSR
  0x0000,  // 0x05 unused
  0x0000,  // 0x06 unused
  0x0000,  // SELECT_DESELECT_CARD // sometimes arg varies here
  0x0000,  // 0x08 unused
  0x00F1,  // SEND_CSD
  0x0045,  // SEND_CID
  0x0000,  // 0x0b unused
  0x0000,  // STOP_TRANSMISSION
  0x0153,  // SEND_STATUS
  0x0000,  // SET_BUS_WIDTH_REGISTER
  0x0000,  // GO_INACTIVE_STATE
};
static const cyg_uint16 num_crc_entries = sizeof( mmc_crc_table ) / sizeof( mmc_crc_table[0] );

static cyg_uint16 CalculateCmdCRC( cyg_uint16 cmd, cyg_uint32 args ) 
{
  cyg_uint16 crc;
  cyg_uint8 buf[MMC_COMMAND_LENGTH];

  // return a cached CRC if available
#if 1
  if( (cmd-MMC_CMD_STARTBIT) < num_crc_entries && mmc_crc_table[ cmd-MMC_CMD_STARTBIT ] ) {
    return mmc_crc_table[ cmd-MMC_CMD_STARTBIT ];
  }
#endif
  
  buf[0] = (cyg_uint8) cmd;
  buf[1] = (cyg_uint8) ((args >> 24) & 0xff);
  buf[2] = (cyg_uint8) ((args >> 16) & 0xff);
  buf[3] = (cyg_uint8) ((args >>  8) & 0xff);
  buf[4] = (cyg_uint8) ((args >>  0) & 0xff);

  crc = CalculateCRC7( buf, 0, MMC_COMMAND_LENGTH, 0 );

  // this makes it the crc + end bit
  crc = (crc << 1) | 0x01;

  return crc;
}


