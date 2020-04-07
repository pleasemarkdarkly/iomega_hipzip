// mmc_slow_c0.c: slower routines for controller 0
// Dan Conti 02/21/01 danc@iobjects.com
// (c) Interactive Objects. We use special herbs and spices.

#include <cyg/kernel/kapi.h>
#include "mmc_phys.h"
#include "mmc_slow.h"
#include "mmc_constants.h"

#include "mmc_debug.h"

// this wait is painfully long, but the sandisk cards seem to get unhappy
// if we shrink it any
#define WAIT()  { int delay; for( delay = 0x20; delay; delay-- ) ; }


cyg_uint32 MMCExchangeDataSlow_c0(cyg_uint8 odata, cyg_uint8 *idata)
{
  odata &= ( HW_MMC_CMD0 | HW_MMC_DATA0 );

  // preserve the rest of the port
  odata |= ( MMC_TEST_PORT( HW_MMC_PORT0 ) & ~(HW_MMC_CMD0|HW_MMC_DATA0));

  MMC_SET_PORT( HW_MMC_PORT0, odata );
  WAIT();
  MMC_SET_PORT( HW_MMC_PORT0, (odata | HW_MMC_CLK0) );
  WAIT();
  *idata = MMC_TEST_PORT( HW_MMC_PORT0 );
  MMC_SET_PORT( HW_MMC_PORT0, odata );
  WAIT();
  return (MMC_ERR_NONE);
}

void MMCSendCommandSlow_c0( cyg_uint32 Arg, cyg_uint16 Cmd, cyg_uint16 crcDATA )
{
  cyg_uint8   cmd_bytes[MMC_COMMAND_LENGTH];
  cyg_uint16  i;
  cyg_uint8   nByte, tmp;
  cyg_uint8   tranByte;
  cyg_int16   nBit;

  MENTER();
  
  for (i = 0; i < 2; i++)
    MMCExchangeDataSlow_c0( HW_MMC_CMD0 | HW_MMC_DATA0, &nByte );

  cmd_bytes[0] = (cyg_uint8)(Cmd);

  // MSB first
  for (i = 4; i > 0; i--)
  {
    cmd_bytes[i] = (cyg_uint8)(Arg & 0xFFL);
    Arg >>= 8;
  }

  cmd_bytes[5] = (cyg_uint8)crcDATA;
  MDEBUG("%s: %02x %02x %02x %02x %02x %02x\n", __FUNCTION__,
	 cmd_bytes[0], cmd_bytes[1], cmd_bytes[2],
	 cmd_bytes[3], cmd_bytes[4], cmd_bytes[5] );

  for( i = 0; i < MMC_COMMAND_LENGTH; i++ )
  {
    tranByte = cmd_bytes[i];

    for( nBit = 0x80; nBit ; nBit >>= 1 )
    {
      nByte = (cyg_uint8)( (HW_MMC_DATA0|HW_MMC_CLK0) | ( tranByte & nBit ? HW_MMC_CMD0 : 0x00 ));
      MMCExchangeDataSlow_c0( nByte, &tmp );
    }
  }
  
  MEXIT();
}

cyg_uint32 MMCGetResponseSlow_c0( cyg_uint8 *resp_bytes, cyg_uint16 respBitLength)
{
  cyg_uint16 i,x;
  cyg_uint8 iByte, bitmask;

  MENTER();

  // switch the ddr so we can read the resp of the cmd line
  MMC_DDR_CMD0_INPUT( );

  iByte = 0;

  for( i = 0xFFFF; i ; i-- ) {
    MMCExchangeDataSlow_c0( HW_MMC_CMD0 | HW_MMC_DATA0, &iByte );

    if ( !(iByte & HW_MMC_CMD0) )  /* start bit of cmd's response detected */
      break;
    if( i == 1 ) {
      MMC_DDR_CMD0_OUTPUT( );
      MDEBUG("MMCGetResponse didn't get a start bit on the CMD line\n");
      return MMC_ERR_NOT_RESPONDING;
    }
  }

  bitmask = 0x80;
  x = 0;

  // receive and store the response
  // clock a bit at a time
  for (i = 0; i < respBitLength; i++)
  {
    resp_bytes[x] |= ( iByte & HW_MMC_CMD0 ? bitmask : 0 );
    bitmask >>= 1;
    if( !bitmask ) {
      bitmask = 0x80;
      x++;
    }
    MMCExchangeDataSlow_c0( HW_MMC_CMD0 | HW_MMC_DATA0, &iByte );
  }

  // flip the ddr back
  MMC_DDR_CMD0_OUTPUT( );

  // clock the card 8 times
  for( i = 0; i < 8; i++ ) {
    MMCExchangeDataSlow_c0( 0xff, &iByte );
  }
  
  MEXIT();
  
  return MMC_ERR_NONE;
}





