// mmc_lowl.c: hardware oriented routines for MMC interface
// Dan Conti 02/20/01 danc@iobjects.com
// (c) Interactive Objects, crazy like that

#include <cyg/kernel/kapi.h>
#include "mmc_drv.h"
#include "mmc_lowl.h"
#include "mmc_phys.h"
#include "mmc_util.h"

#include "mmc_fast.h"
#include "mmc_slow.h"

#include "mmc_debug.h"

// fdecl
static const mmc_function_set_t mmc_functions[] =
{
  // mmc_routines[0], slow routines for controller 0
  {
    &MMCExchangeDataSlow_c0,
    &MMCSendCommandSlow_c0,
    &MMCGetResponseSlow_c0,
    NULL,
    NULL,
    NULL,
    NULL,
  },
  // mmc_routines[1], fast routines for controller 0
  {
    NULL,
    &MMCSendCommand_c0,
    &MMCGetResponse_c0,
    &MMCGetData_c0,
    &MMCSendData_c0,
    &MMCReceive_c0,
    &MMCTransmit_c0,
  },
  // mmc_routines[2], slow routines for controller 1
  {
    &MMCExchangeDataSlow_c1,
    &MMCSendCommandSlow_c1,
    &MMCGetResponseSlow_c1,
    NULL,
    NULL,
    NULL,
    NULL,
  },
  // mmc_routines[3], fast routines for controller 1
  {
    NULL,
    &MMCSendCommand_c1,
    &MMCGetResponse_c1,
    &MMCGetData_c1,
    &MMCSendData_c1,
    &MMCReceive_c1,
    &MMCTransmit_c1,
  },
};

// note - doing this power cycles both cards right now, since they share
//        a pin for vcc control
static void MMCPowerOnOff( mmc_controller_info_t* mci, cyg_uint8 power_state ) 
{
  MENTER();
  MDEBUG(" power for controller %d: %s\n", mci->controllerno, power_state ? "on" : "off"  );
  if( power_state ) {
    if( mci->controllerno == 0 ) {
      HW_MMC_POWER_ON0();
    } else {
#if !defined(HW_MMC_SHARED_RESET)
      HW_MMC_POWER_ON1();
#endif
    }
  } else {
    if( mci->controllerno == 0 ) {
      HW_MMC_POWER_OFF0();
    } else {
#if !defined(HW_MMC_SHARED_RESET)
      HW_MMC_POWER_OFF1();
#endif
    }
  }
  mci->power_state = power_state;
  MEXIT();
}

void MMCSelectClock( mmc_controller_info_t* mci, cyg_uint8 clock_rate ) 
{
  int findex = (mci->controllerno << 1) | clock_rate;

  MENTER();

  mci->mmc_functions = &mmc_functions[ findex ];
  mci->clock_rate = clock_rate;

  // adjust pin directions
  if( mci->controllerno == 0 ) {
    MMC_DDR_CLK0_OUTPUT();
    MMC_DDR_CMD0_OUTPUT();
    MMC_DDR_DATA0_INPUT();
  } else {
    MMC_DDR_CLK1_OUTPUT();
    MMC_DDR_CMD1_OUTPUT();
    MMC_DDR_DATA1_INPUT();
  }

  MEXIT();
}

void MMCHardwareInit( mmc_controller_info_t* mci )
{
  MENTER();

  // set the card detect pins as inputs
  MMC_DDR_CARD0_INPUT();
  MMC_DDR_CARD1_INPUT();

  // use the "slow" routines"
  MMCSelectClock( mci, MMC_CLOCK_SLOW );

  // hard reset the interface
  MMCPowerOnOff( mci, 0 );
  MMC_SYS_BUSY_WAIT( 3 );
  MMCPowerOnOff( mci, 1 );
  MMC_SYS_BUSY_WAIT( 3 );

  // get the mmc bus up and going
  MMCBusStart( mci );
  
  MEXIT();
}

void MMCResetController( mmc_controller_info_t* mci ) 
{
  MMCPowerOnOff( mci, 1 );
  MMC_SYS_BUSY_WAIT( 3 );
}

int MMCCheckCardBusy( mmc_controller_info_t* mci )
{
  int i;
  int byte;

  MENTER();
  if( mci->controllerno == 0 ) {
    for( i = 0; i < 0xfff; i++ ) {
      MMC_READ_PORT0(byte);
      if( byte & HW_MMC_DATA0 ) {
	return 0;
      }
    }
  } else {
    for( i = 0; i < 0xfff; i++ ) {
      MMC_READ_PORT1(byte);
      if( byte & HW_MMC_DATA1 ) {
	return 0;
      }
    }
  }
  return 1;
}

int MMCCardPresent( mmc_controller_info_t* mci )
{
  int ret;
  MENTER();
  
  if( mci->controllerno == 0 ) {
    ret = MMC_TEST_CARD0_PRESENT();
  } else {
    ret = MMC_TEST_CARD1_PRESENT();
  }
  
  MEXIT();
  // map to boolean
  return ( ret ? 1 : 0 );
}

int MMCBusStart( mmc_controller_info_t* mci ) 
{
  int i;
  cyg_uint8 data;
  //  MMCResetController( mci );  // turn power on, redundant
  //  MMCSelectClock( mci, MMC_CLOCK_SLOW ); // clock down, redundant

  MENTER();
  // bus initialization involves sending 80 clocks to the bus

  for( i = 0; i < 80 ; i++ ) {
    mci->mmc_functions->exchange_data( 0xff, &data );
  }
  
  MEXIT();
  return MMC_ERR_NONE;
}

__inline__ cyg_uint32 MMCReceiveData( mmc_controller_info_t* mci, mmc_block_request_t* blkreq ) 
{
  cyg_uint32 errcode = MMC_ERR_NONE;
  
  MDEBUG("+%s blkreq->num_blks = %d mci->card_info.block_len = %d blkreq->buf = 0x%08x blkreq->lba = %d\n",
	 __FUNCTION__, blkreq->num_blks, mci->card_info.block_len, blkreq->buf, blkreq->lba);

  if( mci->card_info.sandisk_card ) {
    int i, len = mci->card_info.block_len;
    cyg_uint8* pBuf = blkreq->buf;

    for( i = 0; i < blkreq->num_blks &&
	   mci->mmc_functions->receive( pBuf, len, (i == 0 ? 1 : 0) ) == MMC_ERR_NONE;
	 pBuf += len, i++ ) {

      // on sandisk cards, we need roughly 120 ticks between reads
      // the compiler converts this loop into a subs and bpl, so ticks = loop lim * 2
      if( i < (blkreq->num_blks-1) ) {
      	hal_delay_us( 500 );
	//	for( z = 0; z < 1000; z++ ) ;
      }
    }

    if( i < blkreq->num_blks || errcode != MMC_ERR_NONE ) {
      diag_printf(" choke on block %d\n", i );
      errcode = MMC_ERR_NOT_RESPONDING;
    }
  } else {
    errcode = mci->mmc_functions->receive( blkreq->buf, blkreq->num_blks * mci->card_info.block_len, 1 );
  }
  
  return errcode;
}

cyg_uint32 MMCSendData( mmc_controller_info_t* mci, mmc_block_request_t* blkreq ) 
{
  MDEBUG("+%s blkreq->num_blks = %d mci->card_info.block_len = %d blkreq->buf = 0x%08x blkreq->lba = %d\n",
	 __FUNCTION__, blkreq->num_blks, mci->card_info.block_len, blkreq->buf, blkreq->lba);
  return mci->mmc_functions->transmit( blkreq->buf, blkreq->num_blks * mci->card_info.block_len );
}

cyg_uint32 MMCSendCommand( mmc_controller_info_t* mci, cyg_uint16 cmd,
			   cyg_uint32 arg, cyg_uint16 crc ) 
{
  MDEBUG("+%s %x\n", __FUNCTION__, cmd);
  mci->mmc_functions->send_command( arg, cmd, crc );
  return MMC_ERR_NONE;
}

cyg_uint32 MMCGetResponse( mmc_controller_info_t* mci, cyg_uint16 bitlen ) 
{
  MENTER();
  return mci->mmc_functions->get_response( (cyg_uint8*)mci->card_info.last_response, bitlen );
}

