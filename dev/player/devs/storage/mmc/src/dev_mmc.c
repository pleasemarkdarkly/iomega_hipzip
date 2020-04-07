#include <cyg/infra/diag.h>
#include <cyg/error/codes.h>
#include <cyg/io/devtab.h>
#include <cyg/io/config_keys.h>
#include <cyg/io/io.h>
#include <cyg/hal/hal_diag.h>

#include <errno.h>
//#include <dadio/io/pm.h>  // currently no PM support

// this effectively creates a circular dependency, but that's ok
#include <io/storage/blk_dev.h>
#include <devs/storage/mmc/_mmc_dev.h>

#include "mmc_drv.h"
#include "mmc_api.h"
#include "mmc_constants.h"
#include "mmc_phys.h"
#include "mmc_debug.h"

#define MMCDEBUG(s...) //diag_printf(##s)

//since static, can't place these in MMC.h
static bool dev_mmc_init(struct cyg_devtab_entry *tab);
static Cyg_ErrNo dev_mmc_lookup(struct cyg_devtab_entry **tab, struct cyg_devtab_entry *sub_tab,const char *name);
static Cyg_ErrNo dev_mmc_write(cyg_io_handle_t handle, const void *buf, cyg_uint32 *len, cyg_uint32 block);
static Cyg_ErrNo dev_mmc_read(cyg_io_handle_t handle, void *buf, cyg_uint32 *len, cyg_uint32 block);
static cyg_bool  dev_mmc_select(cyg_io_handle_t handle, cyg_uint32 Which, CYG_ADDRWORD Info);
static Cyg_ErrNo dev_mmc_get_config(cyg_io_handle_t handle, cyg_uint32 key, void *buf, cyg_uint32 *len);
static Cyg_ErrNo dev_mmc_set_config(cyg_io_handle_t handle, cyg_uint32 key, const void *buf, cyg_uint32 *len);

static mmc_controller_info_t mci_data[HW_MMC_NUM_DRIVES];

//init DEV struct var
BLOCK_DEVIO_TABLE(mmc_devio,
		  dev_mmc_write,
		  dev_mmc_read,
		  dev_mmc_select,
		  dev_mmc_get_config,
		  dev_mmc_set_config);
BLOCK_DEVTAB_ENTRY(dev_mmc_Zero,
		   BLOCK_DEV_HDA_NAME,
		   0,
		   &mmc_devio,
		   dev_mmc_init,
		   dev_mmc_lookup,
		   &mci_data[0]);

#if HW_MMC_NUM_DRIVES==2
BLOCK_DEVTAB_ENTRY(dev_mmc_One,
		   BLOCK_DEV_HDB_NAME,
		   0,
		   &mmc_devio,
		   dev_mmc_init,
		   dev_mmc_lookup,
		   &mci_data[1]);
#endif  // NUM_DRIVES==2

static Cyg_ErrNo dev_res_to_errno(cyg_uint8 dev_res);
static Cyg_ErrNo reset_and_open( mmc_controller_info_t* );

//static cyg_io_handle_t pm_devH = 0;

#if defined(HW_MMC_MEDIA_CHANGE_IRQ)
static cyg_interrupt mmc_interrupt_data;
static cyg_handle_t  mmc_interrupt_handle;
static cyg_uint32 mmc_card_detect_isr( cyg_vector_t, cyg_addrword_t );
static void       mmc_card_detect_dsr( cyg_vector_t, cyg_ucount32, cyg_addrword_t );
#endif

// only do mutex locking in threadsafe mode
#if defined(DEV_MMC_THREADSAFE)
#define LOCK_RESOURCE(x)   cyg_mutex_lock(x)
#define UNLOCK_RESOURCE(x) cyg_mutex_unlock(x)
#else
#define LOCK_RESOURCE(x)
#define UNLOCK_RESOURCE(x)
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool dev_mmc_init(struct cyg_devtab_entry* tab)
{
  mmc_controller_info_t* mci = (mmc_controller_info_t*) tab->priv;
  int i;
  static bool initialized = false;
    
  MMCDEBUG("mmc init\n");
  if (!initialized) {
#if defined(HW_MMC_MEDIA_CHANGE_IRQ)
    cyg_drv_interrupt_create( HW_MMC_CD_INT,
			      40,
			      0,
			      (cyg_ISR_t*) mmc_card_detect_isr,
			      (cyg_DSR_t*) mmc_card_detect_dsr,
			      &mmc_interrupt_handle,
			      &mmc_interrupt_data );
    cyg_interrupt_attach( mmc_interrupt_handle );
    cyg_interrupt_acknowledge( HW_MMC_CD_INT );
    cyg_interrupt_unmask( HW_MMC_CD_INT );
#endif
    for( i = 0; i < HW_MMC_NUM_DRIVES; i++ ) {
      mci_data[i].controllerno = i;
      mci_data[i].media_status_cb = 0;
#if defined(DEV_MMC_THREADSAFE)
      cyg_mutex_init( &mci_data[i].card_mutex );
#endif
      
    }
    initialized = true;
  }

  // optional ?
  // perhaps i could set do_card_reset here instead ?
  //  reset_and_open( mci );
  mci->media_present = mmc_CardPresent( mci );
  if( mci->media_present ) {
    mci->do_card_reset = 1;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static Cyg_ErrNo dev_mmc_lookup(struct cyg_devtab_entry **tab,struct cyg_devtab_entry *sub_tab,const char *name)
{
  Cyg_ErrNo res=ENOERR;

  MMCDEBUG("mmc lookup [%s]\n", name);
  //  pm_devH=(cyg_io_handle_t)sub_tab;//pass subtab dev entry to pm_h handle
  return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static Cyg_ErrNo dev_mmc_write(cyg_io_handle_t handle, const void * buf, cyg_uint32 *len, cyg_uint32 block)
{
  mmc_block_request_t blk_req;
  cyg_devtab_entry_t* tab=(cyg_devtab_entry_t*)handle;
  mmc_controller_info_t* mci = (mmc_controller_info_t*) tab->priv;
    
  Cyg_ErrNo res = ENOERR;
  cyg_uint32 dev_res;
  
  blk_req.buf = (void *)buf;
  blk_req.lba = block;
  blk_req.num_blks = *len / 512;

  MMCDEBUG("mmc write\n");
  LOCK_RESOURCE( &mci->card_mutex );

  if( mci->do_card_reset ) {
    res = reset_and_open( mci );
    mci->do_card_reset = 0;
  }
  if( res == ENOERR ) {
    dev_res = mmc_DriveWrite( mci, &blk_req );
    res = dev_res_to_errno( dev_res );
    if( res == -EMEDCHG ) {
      mci->do_card_reset = 1;
    } else if( res != ENOERR && res != -ENOMED ) {
      // 4/13/01 dc retry if failed
      dev_res = mmc_DriveWrite( mci, &blk_req );
      res = dev_res_to_errno( dev_res );
    }
  }
  
  UNLOCK_RESOURCE( &mci->card_mutex );

  return res;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static Cyg_ErrNo dev_mmc_read(cyg_io_handle_t handle,void * buf,cyg_uint32 *len, cyg_uint32 block)
{
  mmc_block_request_t blk_req;
  cyg_devtab_entry_t* tab=(cyg_devtab_entry_t*)handle;
  mmc_controller_info_t* mci = (mmc_controller_info_t*) tab->priv;
  
  Cyg_ErrNo res=ENOERR;
  cyg_uint8 dev_res;

  blk_req.buf = buf;
  blk_req.lba = block;
  blk_req.num_blks = *len / 512;
  
  MMCDEBUG("mmc read\n");
  LOCK_RESOURCE( &mci->card_mutex );

  if( mci->do_card_reset ) {
    res = reset_and_open( mci );
    mci->do_card_reset = 0;
  }
  if( res == ENOERR ) {
    dev_res = mmc_DriveRead( mci, &blk_req );
    res = dev_res_to_errno( dev_res );
    if( res == -EMEDCHG ) {
      mci->do_card_reset = 1;
    } else if( res != ENOERR && res != -ENOMED ) {
      // 4/13/01 dc retry the read if it fails
      diag_printf(" mmc phys read retry\n");
      dev_res = mmc_DriveRead( mci, &blk_req );
      res = dev_res_to_errno( dev_res );
    }
  }
  
  UNLOCK_RESOURCE( &mci->card_mutex );
  
  return res;
}

static cyg_bool dev_mmc_select(cyg_io_handle_t handle, cyg_uint32 Which, CYG_ADDRWORD Info)
{
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static Cyg_ErrNo dev_mmc_get_config(cyg_io_handle_t handle,cyg_uint32 key,void *buf,cyg_uint32 *len)
{
  cyg_devtab_entry_t* tab=(cyg_devtab_entry_t*)handle;
  mmc_controller_info_t* mci = (mmc_controller_info_t*) tab->priv;
  Cyg_ErrNo res=ENOERR;//init returned res
  cyg_uint8 dev_res;
  
  LOCK_RESOURCE( &mci->card_mutex );
  switch(key)
  {
    case IO_BLK_GET_CONFIG_GEOMETRY:
    {
      drive_geometry_t* pGeomDesc = (drive_geometry_t*)buf;
      
      MMCDEBUG("get geometry\n");
      if( mci->do_card_reset ) {
	res = reset_and_open( mci );
	mci->do_card_reset = 0;
      }
      if( res == ENOERR ) {
	dev_res = mmc_GetGeometry( mci, pGeomDesc );
	res=dev_res_to_errno(dev_res);
	if( res == -EMEDCHG ) {
	  mci->do_card_reset = 1;
	} else if( res != ENOERR && res != -ENOMED ) {
	  // 4/13/01 dc retry if failed
	  dev_res = mmc_GetGeometry( mci, pGeomDesc );
	  res = dev_res_to_errno( dev_res );
	}
      }
      break;
    }
	  
    case IO_BLK_GET_CONFIG_MEDIA_STATUS:
    {
      MMCDEBUG("media status\n");
      if( mci->do_card_reset ) {
	res = reset_and_open( mci );
	mci->do_card_reset = 0;
      }
      if( res == ENOERR ) {
	if( !mmc_CardPresent( mci ) ) {
	  res = -ENOMED;
	} else if( mci->media_change ) {
	  mci->do_card_reset = 1;
	  res = -EMEDCHG;
	} else {
	  res = ENOERR;
	}
      }
      break;
    }
      
    default:
      res = -ENOSYS;
      break;
  }
  
  UNLOCK_RESOURCE( &mci->card_mutex );

  return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static Cyg_ErrNo dev_mmc_set_config(cyg_io_handle_t handle, cyg_uint32 key, const void *buf, cyg_uint32 *len)
{
  cyg_devtab_entry_t* tab=(cyg_devtab_entry_t*) handle;
  mmc_controller_info_t* mci = (mmc_controller_info_t*) tab->priv;
  Cyg_ErrNo res=ENOERR;//init returned res
  //  cyg_uint8 dev_res;
  //  cyg_uint32 _len;
  //  int delay;
  
  LOCK_RESOURCE(&mci->card_mutex);//lock mutex,binary semaphore
  switch(key)
  {
    case IO_BLK_SET_CONFIG_MEDIA_STATUS_CB:
    {
      MMCDEBUG("set media status cb\n");
      mci->media_status_cb = (ms_cb_type)(*(unsigned int*)buf);
      break;
    }
    case IO_BLK_SET_CONFIG_RESET:
    {
      res = reset_and_open( mci );
      break;
    }
    case IO_BLK_SET_CONFIG_POWER_DOWN:
    {
      break;
    }
    case IO_BLK_SET_CONFIG_POWER_UP:
    {
      break;
    }
      /* TODO power up/down are waiting for hardware fix ,2000/09/05 */
      
#if 0
    case IO_PM_SET_CONFIG_POWER_DOWN: //use GPIO driver as sub tab?key TBD//PE0 is used to power down/up MMC
      MDEBUG("set pwr down\n");
      /* put the drive in reset mode */
      *(volatile cyg_uint8*)PEDR|=0x01;
      break;
      
    case IO_PM_SET_CONFIG_POWER_UP://key TBD //PE0 is used to power down/up MMC
      MDEBUG("set pwr up\n");
      *(volatile cyg_uint8*)PEDR&=~0x01;
      for(delay=0;delay<100000;delay++);//TODO TBD,can I call cyg_thread_delay here ?
      /* TODO probably should get rid of this.  however, can't test what will happen
       * with the drive out of reset, i.e. whether the reads and writes will fail and
       * call reset_and_open on their own to re-init the drive.  if that is the case,
       * then definitely get rid of this */
      res = reset_and_open( mci );
      break;
      
    case IO_PM_SET_CONFIG_REGISTER://TBD
    case IO_PM_SET_CONFIG_UNREGISTER:
      UNLOCK_RESOURCE(&mci->card_mutex); /* unlock mutex before returning from this function */
      _len = sizeof(cyg_io_handle_t);
      return cyg_io_set_config(pm_devH, key, handle, &_len);
#endif

    default:
      res = -ENOSYS;
      break;
  }
  UNLOCK_RESOURCE(&mci->card_mutex);
  return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static Cyg_ErrNo dev_res_to_errno(cyg_uint8 dev_res)
{
  // TODO: convert this to a const table since the mmc error
  // codes are all 0+

  //  diag_printf(" dev_res = %d\n", dev_res );
  switch (dev_res)
  {
    case MMC_ERR_NONE:
      return ENOERR;
    case MMC_ERR_NOT_RESPONDING:
    case MMC_ERR_TIMEOUT:
      return -ETIME;
    case MMC_ERR_NO_DEVICES:
      return -ENOMED;
    case MMC_ERR_ADDRESS:
      return -EINVAL;
    case MMC_ERR_BAD_DATA:
    case MMC_ERR_NO_CRC:
    case MMC_ERR_CRC_INVALID:
      return -EIO;
    case MMC_ERR_MEDIACHANGE:
      return -EMEDCHG;
    case MMC_ERR_GENERAL:
      return -EIO;
    default:
      MDEBUG("Unaccounted for error %x\n", dev_res);
      return -EIO;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static Cyg_ErrNo reset_and_open( mmc_controller_info_t* mci )
{
  cyg_uint8 dev_res;
  Cyg_ErrNo res = ENOERR;

  dev_res = mmc_DriveOpen( mci );
  res = dev_res_to_errno(dev_res);
    
  return res;
}


#if defined(HW_MMC_MEDIA_CHANGE_IRQ)
#define DEBUG_MED_CHG(s...) //diag_printf(##s)
static cyg_uint32 mmc_card_detect_isr( cyg_vector_t vector, cyg_addrword_t data) 
{
  cyg_drv_interrupt_mask( HW_MMC_CD_INT );
  return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
}
static void mmc_card_detect_dsr( cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data) 
{
  cyg_uint32 cd;
  DEBUG_MED_CHG("dsr\n");

  // allow a little time for the GPIO to debounce
  hal_delay_us(1200);
  
  if( ( cd = mmc_CardPresent( &mci_data[0] ) ) != mci_data[0].media_present ) {
    mci_data[0].media_present = cd;
    mci_data[0].media_change = 1;
    DEBUG_MED_CHG("card 0 mc %d\n", cd );
    if( mci_data[0].media_status_cb ) {
      mci_data[0].media_status_cb(0, cd);
    }
  }
#if HW_MMC_NUM_DRIVES==2
  if( ( cd = mmc_CardPresent( &mci_data[1] ) ) != mci_data[1].media_present ) {
    mci_data[1].media_present = cd;
    mci_data[1].media_change = 1;
    DEBUG_MED_CHG("card 1 mc %d\n", cd );
    if( mci_data[1].media_status_cb ) {
      mci_data[1].media_status_cb(1, cd);
    }
  }
#endif // NUM_DRIVES==2
  cyg_drv_interrupt_unmask( HW_MMC_CD_INT );
}
#endif // MEDIA_CHANGE_IRQ
