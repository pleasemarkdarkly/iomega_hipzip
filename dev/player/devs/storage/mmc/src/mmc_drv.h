// mmc_drv.h: mmc driver structures
// Dan Conti 02/20/01 danc@iobjects.com
// (c) Interactive Objects. We funky like that, G.

#ifndef __MMC_DRV_H__
#define __MMC_DRV_H__

#include <cyg/kernel/kapidata.h>
#include "mmc_constants.h"

// this corresponds to a set of routines used to do low level interaction
// with an mmc card
typedef struct mmc_function_set_s 
{
  cyg_uint32 (*exchange_data)( cyg_uint8, cyg_uint8* );
  void       (*send_command) ( cyg_uint32, cyg_uint16, cyg_uint16 );
  cyg_uint32 (*get_response) ( cyg_uint8*, cyg_uint16 );
  cyg_uint8  (*get_data)     ( void );
  void       (*send_data)    ( cyg_uint8 );
  cyg_uint32 (*receive)      ( cyg_uint8*, cyg_uint16, cyg_uint16 );
  cyg_uint32 (*transmit)     ( cyg_uint8*, cyg_uint16 );
} mmc_function_set_t;

// a structure corresponding to card specific data
typedef struct mmc_card_info_s
{
  cyg_uint32  total_lba;
  cyg_uint16  block_len;
  cyg_uint16  last_response[9];
  cyg_uint16  card_state:14,
    card_ready:1,
    sandisk_card:1;
} mmc_card_info_t;

typedef void (*ms_cb_type)(int,int);

// data for a given controller. each controller has a single instance of this
typedef struct mmc_controller_info_s
{
  const mmc_function_set_t* mmc_functions;
  mmc_card_info_t card_info;  // only one card per controller for now
  cyg_mutex_t card_mutex;
  ms_cb_type media_status_cb;
  cyg_uint8 controllerno;
  cyg_uint8 media_present:1,
    media_change:1,
    power_state:1,
    clock_rate:1,
    do_card_reset:1,
    unused:3;
} mmc_controller_info_t;

typedef struct mmc_block_request_s
{
  void* buf;
  cyg_uint32 lba;
  cyg_uint32 num_blks;
} __attribute((packed)) mmc_block_request_t;


#endif // __MMC_DRV_H__
