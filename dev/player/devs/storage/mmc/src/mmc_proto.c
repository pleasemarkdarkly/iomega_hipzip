// mmc_proto.c: protocol layer for MMC interface
// Dan Conti 02/20/01 danc@iobjects.com
// (c) Interactive Objects. Mostly harmless.

// in general, the layers go like this:
//  mmc_dev.c    - ecos device layer
//  mmc_proto.c  - software protocol for MMC commands
//  mmc_util.c   - routines that send commands and process results
//  mmc_hw.c     - routines that interact with the hardware
//  mmc_hw_c0.S  - assembly routines for controller 0
//  mmc_hw_c1.S  - assembly routines for controller 1

#include "mmc_debug.h"

#include "mmc_proto.h"
#include "mmc_util.h"         // for mmcCommandAndResponse(), etc
#include "mmc_lowl.h"         // for MMCReceive, etc
#include "mmc_constants.h"    // for cmd, resp codes

#define ALLOW_MULTIBLOCK_OPS

cyg_uint32 mmcReset( mmc_controller_info_t* mci, cyg_uint32 setupInfo ) 
{
  int i, timeout;
  cyg_uint32 errcode;

  MENTER();
  
  mmcCommandAndResponse( mci,
			 MMC_CMD_GO_IDLE_STATE,
			 0L,
			 0,
			 MMC_RESP_0 );

  for( i = 0; i < (MMC_CID_LENGTH / 2); mci->card_info.last_response[i++] = 0 ) ;

  MMC_SYS_BUSY_WAIT( 10 );

  timeout = MMC_RESET_DELAY << 2;
  for( i = 0; i < timeout; i++ ) {
    errcode = mmcCommandAndResponse( mci,
				     MMC_CMD_SEND_OP_COND,
				     setupInfo,
				     0,
				     MMC_RESP_3 );

    if( errcode == MMC_ERR_NOT_RESPONDING ) {
      return errcode;
    }
    if( errcode == MMC_ERR_NONE ) {
      if( (mci->card_info.last_response[0] & 0x8000) ) {
	break;
      }
    }
  }

  // make sure we didn't hit the timeout
  if( i == timeout ) {
    return MMC_ERR_TIMEOUT;
  }

  for( i = 0; ; i++ ) {
    // loop on identify until we've found all the devices
    if( mmcIdentify( mci, i ) != MMC_ERR_NONE ) {
      break;
    }
  }

  MEXIT();

  if( i == 0 ) {
    MDEBUG("%s found no devices\n", __FUNCTION__ );
    return MMC_ERR_NO_DEVICES;
  }

  return MMC_ERR_NONE;
}

cyg_uint32 mmcIdentify( mmc_controller_info_t* mci, cyg_uint16 rca ) 
{
  cyg_uint32 errcode;

  errcode = mmcCommandAndResponse( mci,
				   MMC_CMD_ALL_SEND_CID,
				   0L,
				   0,
				   MMC_RESP_2 );
  if( errcode == MMC_ERR_NONE ) {
    errcode = mmcCommandAndResponse( mci,
				     MMC_CMD_SET_RELATIVE_ADDR,
				     ((cyg_uint32)(rca+1) << 16),
				     0,
				     MMC_RESP_1 );
  }
  
  return errcode;
}

cyg_uint32 mmcSetStandbyState( mmc_controller_info_t* mci, cyg_uint16 drca ) 
{
  cyg_uint32 errcode;

  errcode = mmcCommandAndResponse( mci,
				   MMC_CMD_SEND_STATUS,
				   ((cyg_uint32)drca) << 16,
				   0,
				   MMC_RESP_1 );
  if( errcode == MMC_ERR_NONE ) {
    int i = 10;
    while( mci->card_info.card_state == MMC_CARDSTATE_PRG && i-- > 0 ) {
      errcode = mmcGetStatus( mci, drca-1 );
      if( errcode != MMC_ERR_NONE ) {
	return errcode;
      }
      MMC_SYS_WAIT(1);
    }
    if( i == 0 ) {
      MDEBUG(" timeout waiting for card to leave program state\n");
    }
    
    if( mci->card_info.card_state != MMC_CARDSTATE_STANDBY ) {
      errcode = mmcCommandAndResponse( mci,
				       MMC_CMD_SELECT_DESELECT_CARD,
				       0L,
				       0,
				       MMC_RESP_1 );
    }
  }

  return errcode;
}

cyg_uint32 mmcSetXferState( mmc_controller_info_t* mci, cyg_uint16 drca ) 
{
  cyg_uint32 errcode;

  errcode = mmcCommandAndResponse( mci,
				   MMC_CMD_SEND_STATUS,
				   ((cyg_uint32)drca << 16),
				   0,
				   MMC_RESP_1 );
  if( errcode == MMC_ERR_NONE ) {
    MDEBUG(" %s: card_state = %d\n", __FUNCTION__, mci->card_info.card_state );
    if( mci->card_info.card_state == MMC_CARDSTATE_STANDBY ) {
      errcode = mmcCommandAndResponse( mci,
				       MMC_CMD_SELECT_DESELECT_CARD,
				       ((cyg_uint32)(drca) << 16),
				       0,
				       MMC_RESP_1 );
    }
    else {
      int i = 10;
      // the trick is that we leave PRG state and automagically return to transfer state
      while( mci->card_info.card_state == MMC_CARDSTATE_PRG && i-- > 0 ) {
	errcode = mmcGetStatus( mci, drca-1 );
	if( errcode != MMC_ERR_NONE ) {
	  return errcode;
	}
	MMC_SYS_WAIT(1);
      }
      if( i == 0 ) {
	MDEBUG(" timeout waiting for card to leave program state\n");
      }
    }
  }

  return errcode;
}

cyg_uint32 mmcGetCardIdentification( mmc_controller_info_t* mci, cyg_uint16 rca,
				     cyg_uint8* mmcIdent )
{
  cyg_uint8* respbytes;
  cyg_uint32 errcode;
  cyg_uint16 i;

  MENTER();
  
  respbytes = (cyg_uint8*)mci->card_info.last_response;

  rca += 1;

  errcode = mmcSetStandbyState( mci, rca );
  // ok, on some cards, for some fuckin reason, you need two of these
  // better yet, no other cards seem to care if this is here.
  errcode = mmcSetStandbyState( mci, rca );
  if( errcode == MMC_ERR_NONE ) {
    errcode = mmcCommandAndResponse( mci,
				     MMC_CMD_SEND_CID,
				     ((cyg_uint32)(rca) << 16 ),
				     0,
				     MMC_RESP_2 );

    if( errcode == MMC_ERR_NONE ) {
      for( i = 0; i < (MMC_CID_LENGTH-1); i++ ) {
	mmcIdent[i] = respbytes[i+1];
      }
    } else {
      MDEBUG("%s: errcode %d returned from SEND_CID\n", __FUNCTION__,errcode );
    }
  }

  MEXIT();
  
  return errcode;
}

cyg_uint32 mmcGetConfiguration( mmc_controller_info_t* mci, cyg_uint16 rca,
				cyg_uint8* respCSD )
{
  cyg_uint8* respbytes;
  cyg_uint32 errcode;
  cyg_uint16 i;

  rca += 1;

  respbytes = (cyg_uint8*)mci->card_info.last_response;

  errcode = mmcSetStandbyState( mci, rca );
  if( errcode == MMC_ERR_NONE ) {
    errcode = mmcCommandAndResponse( mci,
				     MMC_CMD_SEND_CSD,
				     ((cyg_uint32)(rca) << 16 ),
				     0,
				     MMC_RESP_2 );

    if( errcode == MMC_ERR_NONE ) {
      MDEBUG("*** csd = ");
      for( i = 0; i < (MMC_CID_LENGTH - 1); i++ ) {
	MDEBUG("[%02x] ", respbytes[i+1]&0xff);
	respCSD[i] = respbytes[i+1];
      }
      MDEBUG("***\n");
    }
  }

  return errcode;
}

cyg_uint32 mmcConfigureBlockLength( mmc_controller_info_t* mci, cyg_uint16 rca , cyg_uint16 block_len ) 
{
  cyg_uint32 errcode;

  rca += 1;
  
  errcode = mmcSetXferState( mci, rca );

  if( errcode == MMC_ERR_NONE ) {
    MDEBUG(" %s setting block length\n", __FUNCTION__ );
    errcode = mmcCommandAndResponse( mci,
				     MMC_CMD_SET_BLOCKLEN,
				     (cyg_uint32)block_len,
				     0,
				     MMC_RESP_1 );

    if( errcode == MMC_ERR_NONE ) {
      mci->card_info.block_len = block_len;
    }
  }
  
  return errcode;
}

// only called for single blocks!!!!
cyg_uint32 mmcRead( mmc_controller_info_t* mci, cyg_uint16 rca, mmc_block_request_t* blkreq )
{
  cyg_uint32 errcode;

  errcode = mmcSetXferState( mci, (rca+1) );

  if( errcode != MMC_ERR_NONE ) {
    return errcode;
  }

  errcode = mmcCommandAndResponse( mci,
				   MMC_CMD_READ_BLOCK,
				   (blkreq->lba * (cyg_uint32)(mci->card_info.block_len)),
				   blkreq->num_blks,
				   MMC_RESP_1 );
  
  if( errcode != MMC_ERR_NONE ) {
    return errcode;
  }
  
  errcode = MMCReceiveData( mci, blkreq );
  
  return errcode;
}

cyg_uint32 mmcWrite( mmc_controller_info_t* mci, cyg_uint16 rca, mmc_block_request_t* blkreq )
{
  cyg_uint32 errcode;

  rca += 1;

  mmcSetXferState( mci, rca );

  errcode = mmcCommandAndResponse( mci,
				   MMC_CMD_WRITE_BLOCK,
				   (blkreq->lba * (cyg_uint32)(mci->card_info.block_len)),
				   blkreq->num_blks,
				   MMC_RESP_1 );

  if( errcode == MMC_ERR_NONE ) {
    // TODO actual write call here
    errcode = MMCSendData( mci, blkreq );
  }

  return errcode;
}

cyg_uint32 mmcStopTransmission( mmc_controller_info_t* mci ) 
{
  return mmcCommandAndResponse( mci,
				MMC_CMD_STOP_TRANSMISSION,
				0L,
				0,
				MMC_RESP_1 );
}
#ifdef ALLOW_MULTIBLOCK_OPS
cyg_uint32 mmcReadMultiple( mmc_controller_info_t* mci, cyg_uint16 rca, mmc_block_request_t* blkreq )
{
  cyg_uint32 errcode;

  errcode = mmcSetXferState( mci, (rca + 1) );

  if( errcode != MMC_ERR_NONE ) {
    MDEBUG(" %s: MMCSetXferState gave back %d\n", __FUNCTION__, errcode );
    return errcode;
  }
  
  errcode = mmcCommandAndResponse( mci,
				   MMC_CMD_READ_DAT_UNTIL_STOP,  // dont do read multiple block
				   (blkreq->lba * (cyg_uint32)(mci->card_info.block_len)),
				   blkreq->num_blks,
				   MMC_RESP_1 );
  
  if( errcode != MMC_ERR_NONE ) {
    MDEBUG(" %s: failed on read multiple block command\n", __FUNCTION__ );
    return errcode;
  }

  errcode = MMCReceiveData( mci, blkreq );

  // i wonder if you really need to do a stop transmission after a read
  // seems kind of silly, considering we specify exactly how much we are reading
  if( errcode ) {
    MDEBUG(" %s: MMCReceiveData gave back %d\n", __FUNCTION__, errcode );
    mmcStopTransmission( mci );
    return errcode;
  }

  errcode = mmcStopTransmission( mci );

  if( errcode != MMC_ERR_NONE ) {
    MDEBUG(" %s: MMCStopTransmission gave back %d\n", __FUNCTION__, errcode );
    return errcode;
  }

  return errcode;
}

cyg_uint32 mmcWriteMultiple( mmc_controller_info_t* mci, cyg_uint16 rca, mmc_block_request_t* blkreq )
{
  cyg_uint32 errcode;
  int i;
#if 1
  mmc_block_request_t single_block;
#endif

  errcode = mmcSetXferState( mci, (rca+1) );

  if( errcode != MMC_ERR_NONE ) {
    return errcode;
  }

  errcode = mmcCommandAndResponse( mci,
				   MMC_CMD_WRITE_MULTIPLE_BLOCK,
				   //MMC_CMD_WRITE_DAT_UNTIL_STOP,
				   (blkreq->lba * (cyg_uint32)(mci->card_info.block_len)),
				   blkreq->num_blks,
				   MMC_RESP_1 );
  
  if( errcode != MMC_ERR_NONE ) {
    return errcode;
  }
#if 1
  single_block.buf = blkreq->buf;
  single_block.lba = blkreq->lba;
  single_block.num_blks = 1;
#endif
  MDEBUG(" blkreq->num_blks = %d\n", blkreq->num_blks );

#if 1
  for( i = 0; i < blkreq->num_blks; i++ ) {
    errcode = MMCSendData( mci, &single_block );

    MDEBUG(" MMCSendData gave back %d\n", errcode );
    if( errcode != MMC_ERR_NONE ) {
      break;
    }

    single_block.lba++;
    single_block.buf = (void*)(((char*)single_block.buf) + mci->card_info.block_len);
  }
#else
  errcode = MMCSendData( mci, blkreq );
#endif
  if( errcode != MMC_ERR_NONE ) {
    mmcStopTransmission( mci );
    return errcode;
  }

  errcode = mmcStopTransmission( mci );
  if( errcode != MMC_ERR_NONE ) {
    return errcode;
  }

  return errcode;
}
#else  // ALLOW_MULTIBLOCK_OPS
cyg_uint32 mmcWriteMultiple( mmc_controller_info_t* mci, cyg_uint16 rca, mmc_block_request_t* blkreq ) 
{
  mmc_block_request_t singleblk;
  cyg_uint32 errcode;
  int i;
  
  MENTER();

  errcode = mmcSetXferState( mci, (rca+1) );

  if( errcode != MMC_ERR_NONE ) {
    return errcode;
  }

  singleblk.buf = blkreq->buf;
  singleblk.lba = blkreq->lba;
  singleblk.num_blks = 1;
  
  for( i = 0; i < blkreq->num_blks && errcode == MMC_ERR_NONE; i++ ) {
    errcode = mmcCommandAndResponse( mci,
				     MMC_CMD_WRITE_BLOCK,
				     (singleblk.lba * (cyg_uint32)(mci->card_info.block_len)),
				     singleblk.num_blks,
				     MMC_RESP_1 );

    if( errcode == MMC_ERR_NONE ) {
      errcode = MMCSendData( mci, &singleblk );
    }

    singleblk.lba++;
    singleblk.buf += mci->card_info.block_len;
  }
  
  MEXIT();
  return errcode;
}
cyg_uint32 mmcReadMultiple( mmc_controller_info_t* mci, cyg_uint16 rca, mmc_block_request_t* blkreq )
{
  mmc_block_request_t singleblk;
  cyg_uint32 errcode;
  int i;

  MENTER();

  errcode = mmcSetXferState( mci, (rca+1) );

  if( errcode != MMC_ERR_NONE ) {
    return errcode;
  }

  singleblk.buf = blkreq->buf;
  singleblk.lba = blkreq->lba;
  singleblk.num_blks = 1;

  for( i = 0; i < blkreq->num_blks && errcode == MMC_ERR_NONE; i++ ) {
    errcode = mmcCommandAndResponse( mci,
				     MMC_CMD_READ_BLOCK,
				     (singleblk.lba * (cyg_uint32)(mci->card_info.block_len)),
				     singleblk.num_blks,
				     MMC_RESP_1 );
    if( errcode == MMC_ERR_NONE ) {
      errcode = MMCReceiveData( mci, &singleblk );
    }

    singleblk.lba++;
    singleblk.buf += mci->card_info.block_len;
  }

  MEXIT();
  return errcode;
}

#endif // ALLOW_MULTIBLOCK_OPS

cyg_uint32 mmcGetStatus( mmc_controller_info_t* mci, cyg_uint16 rca ) 
{
  return mmcCommandAndResponse( mci,
				MMC_CMD_SEND_STATUS,
				(((cyg_uint32)rca+1) << 16),
				0,
				MMC_RESP_1 );
}

cyg_uint32 mmcInit( mmc_controller_info_t* mci ) 
{
  cyg_uint32 errcode;

  MENTER();
  
  MMCHardwareInit( mci );

  errcode = mmcReset( mci, 0xFFC000 );
  if( errcode != MMC_ERR_NONE ) {
    return errcode;
  }
  
  MMCSelectClock( mci, MMC_CLOCK_FAST );

  MEXIT();
  
  return errcode;
}

// not used
cyg_uint32 mmcConfigDevice( mmc_controller_info_t* mci ) 
{
  return mmcIdentify( mci, mci->controllerno );
}

cyg_uint32 mmcCheckCardPresent( mmc_controller_info_t* mci ) 
{
  return MMCCardPresent( mci );
}



  
