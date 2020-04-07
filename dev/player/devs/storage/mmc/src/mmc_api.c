// mmc_api.c: block access style API for mmc drives
// Dan Conti 02/20/01 danc@iobjects.com
// (c) Interactive Objects. This message will self destruct in 5 seconds.

#include <cyg/kernel/kapi.h>
#include <string.h>
#include <io/storage/blk_dev.h>

#include "mmc_proto.h"
#include "mmc_api.h"
#include "mmc_constants.h"

#include "mmc_debug.h"

// open a physical drive
// since we only deal with single card per controller interfaces, driveno->cno is a
// direct mapping. this is one of the only places we make this assumption
cyg_uint32 mmc_DriveOpen( mmc_controller_info_t* mci )
{
  cyg_uint32 errcode;
  cyg_uint32 ltemp;
  cyg_uint16 heads;
  cyg_uint16 sectors;
  cyg_uint16 driveno;
  //  cyg_uint8  buf[128];
  cyg_uint8  cid[17];
  cyg_uint8  csd[17];

  // technically, driveno should correspond to the device number on the given MMC
  // bus. however, since we assume one device per bus, driveno is always 0
  driveno = 0;

  // check to see if media is present first
  mci->media_present = (mmc_CardPresent( mci ) > 0);

  if( !mci->media_present ) {
    return MMC_ERR_NO_DEVICES;
  }

  errcode = mmcInit( mci );
  if( errcode != MMC_ERR_NONE ) {
    return errcode;
  }
  
#if 0
  mci->errorcode = mmcReset( mci, (cyg_uint32) driveno );
  if( errcode != MMC_ERR_NONE ) {
    return errcode;
  }
#endif

  errcode = mmcGetCardIdentification( mci, driveno, cid );
  if( errcode != MMC_ERR_NONE ) {
    MDEBUG("%s %d\n", __FUNCTION__, __LINE__ );
    return errcode;
  }

  //  errcode = mmcGetConfiguration( mci, driveno, (buf+64) );
  errcode = mmcGetConfiguration( mci, driveno, csd );
  if( errcode != MMC_ERR_NONE ) {
    MDEBUG("%s %d\n", __FUNCTION__, __LINE__ );
    return errcode;
  }
  
  // some magic parsing here
  heads   = (((cyg_uint16)csd[5]) & 0x0F);
  sectors = (cyg_uint16)(1 << heads);
  
  // set a default here. if we issue a command to change the block length, then the lower level
  // will keep this value in sync
  mci->card_info.block_len = MMC_DEFAULT_BLOCK_LEN;
  
  // TODO this next block of code is untested, its unlikely that it would ever occur
  if( sectors != MMC_DEFAULT_BLOCK_LEN ) {
    errcode = mmcConfigureBlockLength( mci, driveno, MMC_DEFAULT_BLOCK_LEN );
    if( errcode != MMC_ERR_NONE ) {
      MDEBUG("%s %d\n", __FUNCTION__, __LINE__ );
      return errcode;
    }

    errcode = mmcGetConfiguration( mci, driveno, csd);
    if( errcode != MMC_ERR_NONE ) {
      MDEBUG("%s %d\n", __FUNCTION__, __LINE__ );
      return errcode;
    }
  }

  // dc - 4/12/01 - determine if this is a sandisk card
  //  if so, we can make some special provisions in reading and writing to handle their slower throughput
  ltemp = (cyg_uint32) (csd[1] & 0x7f);
  // ok, in this byte, bits 6:3 are the mantissa
  ltemp >>= 3;
  // on sandisk cards, which are dick slow, the resulting value is 4
  if( ltemp >= 4 ) {
    mci->card_info.sandisk_card = 1;
  } else {
    mci->card_info.sandisk_card = 0;
  }
  
  // figure out the device capacity
  // this is all black magic copied from the sandisk code
  // technically i only need the calculation so i can tell total LBA
  sectors = (((cyg_uint16)csd[6]) & 0x03);
  sectors <<= 8;

  ltemp = ((cyg_uint32)csd[7]) + (cyg_uint32) sectors;
  ltemp <<= 2;
  sectors = ((cyg_uint16)(csd[8]) >> 6) & 0x03;
  ltemp += ((cyg_uint32) sectors + 1L );

  // device size multiplier. whatever that means
  heads = ((cyg_uint16)csd[9] & 0x03 );
  heads <<= 1;
  sectors = ((((cyg_uint16)csd[10]) >> 7) & 0x01);

  ltemp <<= ((heads + sectors + 2));

  heads = 2;
  if( ltemp > 0xFFFF ) {
    heads = 4;
  }
  
  mci->card_info.total_lba = ltemp;

  // clear the media change bit now that we have initialized the card
  mci->media_change = 0;

  return MMC_ERR_NONE;
}

cyg_uint32 mmc_DriveClose( mmc_controller_info_t* mci ) 
{
  return MMC_ERR_NONE;
}

cyg_uint32 mmc_DriveWrite( mmc_controller_info_t* mci, mmc_block_request_t* blkreq ) 
{
  cyg_uint32 errcode;
  cyg_uint16 driveno;

  driveno = 0;  // TODO proper mappings

  if( !mci->media_present ) {
    return MMC_ERR_NO_DEVICES;
  }
  if( mci->media_change ) {
    return MMC_ERR_MEDIACHANGE;
  }
  
  if( blkreq->lba + blkreq->num_blks > mci->card_info.total_lba ) {
    return MMC_ERR_ADDRESS;
  }

  errcode = mmcWriteMultiple( mci, driveno, blkreq );

  return errcode;
}

cyg_uint32 mmc_DriveRead( mmc_controller_info_t* mci, mmc_block_request_t* blkreq )
{
  cyg_uint32 errcode;
  cyg_uint16 driveno;

  driveno = 0;  // TODO proper mappings

  if( !mci->media_present ) {
    return MMC_ERR_NO_DEVICES;
  }
  if( mci->media_change ) {
    return MMC_ERR_MEDIACHANGE;
  }
  
  if( blkreq->lba + blkreq->num_blks > mci->card_info.total_lba ) {
    return MMC_ERR_ADDRESS;
  }

  if( blkreq->num_blks == 1 ) {
    errcode = mmcRead( mci, driveno, blkreq );
  }
  else {
    errcode = mmcReadMultiple( mci, driveno, blkreq );
  }
  
  return errcode;
}

cyg_uint32 mmc_GetGeometry( mmc_controller_info_t* mci, drive_geometry_t* dg ) 
{
  cyg_uint32 errcode;
  cyg_uint16 driveno;
  cyg_uint8 buf[MMC_CID_LENGTH-1];

  driveno = 0;  // TODO proper mappings

  if( !mci->media_present ) {
    return MMC_ERR_NO_DEVICES;
  }
  if( mci->media_change ) {
    return MMC_ERR_MEDIACHANGE;
  }

  // the CID register takes 17 bytes
  errcode = mmcGetCardIdentification( mci, driveno, buf );
  if( errcode != MMC_ERR_NONE ) {
    MDEBUG("%s: card_state = %x, MMC_CARDSTATE_STANDBY = %x\n",
	   __FUNCTION__, mci->card_info.card_state, MMC_CARDSTATE_STANDBY );
    return errcode;
  }

  // lie about geometry since it doesn't matter
  dg->cyl = 1;
  dg->hd  = 1;
  dg->sec = mci->card_info.total_lba;
  dg->num_blks    = mci->card_info.total_lba;
  dg->bytes_p_sec = mci->card_info.block_len;

  strncpy( dg->model_num, &buf[3], 7 );
  dg->model_num[7] = 0;
  
  //  it is possible to use the whole 16 byte CID register as the serialnum,
  //  in which case there is no point to doing any of the below work, it should just
  //  be memcpy()'d

  // using the entire CID as a serial number. the last byte is always 01 so we discard
  memset( dg->serial_num, 0, sizeof( dg->serial_num ) );
  memcpy( dg->serial_num, buf, MMC_CID_LENGTH-1 );
  
  return errcode;
}

cyg_uint32 mmc_CardPresent( mmc_controller_info_t* mci ) 
{
  return mmcCheckCardPresent( mci );
}

