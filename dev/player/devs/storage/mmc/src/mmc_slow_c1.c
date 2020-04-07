// mmc_slow_c1.c: slower routines for controller 1
// Dan Conti 02/21/01 danc@iobjects.com
// (c) Interactive Objects. All your base are belong to us.

#include <cyg/kernel/kapi.h>
#include "mmc_phys.h"
#include "mmc_slow.h"
#include "mmc_constants.h"

#include "mmc_debug.h"

#define WAIT()  { int delay; for( delay = 0x20; delay; delay-- ); }

cyg_uint32 MMCExchangeDataSlow_c1(cyg_uint8 odata, cyg_uint8 *idata)
{
  // bic everything but the cmd and data lines
  odata &= (HW_MMC_CMD1 | HW_MMC_DATA1);

  MMC_SET_PORT( HW_MMC_PORT1, odata );
  WAIT();
  MMC_SET_PORT( HW_MMC_PORT1, odata | HW_MMC_CLK1 );
  WAIT();
  *idata = MMC_TEST_PORT( HW_MMC_PORT1 );
  MMC_SET_PORT( HW_MMC_PORT1, odata );
  WAIT();
  return (MMC_ERR_NONE);
}

void MMCSendCommandSlow_c1( cyg_uint32 Arg, cyg_uint16 Cmd, cyg_uint16 crcDATA )
{
  cyg_uint8   cmd_bytes[MMC_COMMAND_LENGTH];
  cyg_uint16  i;
  cyg_uint8   nByte, tmp;
  cyg_uint8   tranByte;
  cyg_int16   nBit;

  MENTER();
  
  for (i = 0; i < 2; i++)
    MMCExchangeDataSlow_c1( HW_MMC_DATA1 | HW_MMC_CMD1, &nByte );

  cmd_bytes[0] = (cyg_uint8)(Cmd);

    /* MSB first */
  for (i = 4; i > 0; i--)
  {
    cmd_bytes[i] = (cyg_uint8)(Arg & 0xFFL);
    Arg >>= 8;
  }

  cmd_bytes[5] = (cyg_uint8)crcDATA;

  for( i = 0; i < MMC_COMMAND_LENGTH; i++ )
  {
    tranByte = cmd_bytes[i];

    for( nBit = 0x80; nBit ; nBit >>= 1 )
    {
      nByte = (cyg_uint8)( (HW_MMC_DATA1|HW_MMC_CLK1) | ( tranByte & nBit ? HW_MMC_CMD1 : 0x00 ));
      MMCExchangeDataSlow_c1( nByte, &tmp );
    }
  }
  
  MEXIT();
}

cyg_uint32 MMCGetResponseSlow_c1( cyg_uint8 *resp_bytes, cyg_uint16 respBitLength)
{
  cyg_uint16 i,x;
  cyg_uint8 iByte, bitmask;

  MENTER();
  iByte = 0;

  MMC_DDR_CMD1_INPUT( );

  for( i = 0xFFFF; i ; i-- ) {
    MMCExchangeDataSlow_c1( HW_MMC_CMD1 | HW_MMC_DATA1 , &iByte );

    if ( !(iByte & HW_MMC_CMD1) )  /* start bit of cmd's response detected */
      break;
    if( i == 1 ) {
      MMC_DDR_CMD1_OUTPUT( );
      return MMC_ERR_NOT_RESPONDING;
    }
  }

  bitmask = 0x80;
  x = 0;

  for (i = 0; i < respBitLength; i++)
  {   /* Receiving & storing CMD_response */
    resp_bytes[x] |= ( iByte & HW_MMC_CMD1 ? bitmask : 0 );
    bitmask >>= 1;
    if( !bitmask ) {
      bitmask = 0x80;
      x++;
    }
    MMCExchangeDataSlow_c1( HW_MMC_CMD1 | HW_MMC_DATA1, &iByte );
  }
  MMC_DDR_CMD1_OUTPUT( );

  MEXIT();
  
  return MMC_ERR_NONE;
}



