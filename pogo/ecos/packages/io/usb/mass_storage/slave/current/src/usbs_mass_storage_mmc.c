//==========================================================================
//
//      usbs_mass_storage_mmc.c
//
//      Support for USB-mass storage MMC devices, slave-side.
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    toddm@iobjects.com
// Contributors: toddm@iobjects.com
// Date:         2001-21-02
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/io_usb_slave_mass_storage.h>

//#define SYZYGY_BUG

#if defined(CYGPKG_IO_USB_SLAVE_MASS_STORAGE_DEVICE_MMC)

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/drv_api.h>

#include <cyg/io/usb/usbs_mass_storage.h>
#include <cyg/io/usb/usbs_mass_storage_mmc.h>
#include "usbs_mass_storage_mmc_misc.h"

typedef struct drive_geometry_s
{
    cyg_uint16  cyl;        /* Number of cylinders */
    cyg_uint16  hd;         /* Number of Heads */
    cyg_uint16  sec;        /* Sectors per track */
    cyg_uint16  bytes_p_sec;
    cyg_uint32  num_blks;         /* Total drive logical blocks */

    cyg_uint32      serial_len;
    unsigned char   serial_num[64];  /* Drive serial number */

    cyg_uint32      model_len;
    unsigned char   model_num[64];   /* Drive model number */  
} drive_geometry_t;

#define ERESET     0x29
#define EMEDCHG	   0x28
#define ENOMED	   0x3a

#define IO_BLK_GET_CONFIG_GEOMETRY        0x0301
#define IO_BLK_GET_CONFIG_MEDIA_STATUS    0x0302

#define IO_BLK_SET_CONFIG_RESET           0x0382
#define IO_BLK_SET_CONFIG_POWER_UP        0x0386

#if !defined(__DHARMA)
#error Dharma must be defined
#endif

#if 0
# include <cyg/infra/diag.h>
# define DBG(a) diag_printf a
#else
# define DBG(a)
#endif

#define CHECK_CONDITION -EIO

#define BLOCK_SIZE 512
//static char data_buffer[BLOCK_SIZE * 16];

typedef struct usbs_mmc_data_s {
    /* Setting this to one sector and trying to format the card exposes some interesting
       boundary conditions from a Win2K host. */
    char* data_buffer;
    int data_buffer_len;
    //    char data_buffer[BLOCK_SIZE * 128];
    cyg_io_handle_t lun_handle[MAX_LUNS + 1];

    sense_data_t sd[MAX_LUNS + 1];
    bool sd_valid[MAX_LUNS + 1];
    int max_luns;
    /* Bus could be reset while write_10 is in loop, but not receiving.  It will
       then miss the reset necessary to keep the mass storage thread in sync.  So
       use this variable to signal a reset.  Currently write_10 is the only command
       that receives data via multiple start_rx calls, so this is the only place
       this is necessary. */
    bool reset_receive;
} usbs_mmc_data_t;

#if defined(SYZYGY_BUG)
static int last_lun = 0;
#endif

static usbs_mmc_data_t* mmc_data;


static const inquiry_data_t inquiry_data =
{
    rmb: 1,
    additional_length: sizeof(inquiry_data_t) - 4,
    vendor_identification: "IObjects",
    product_identification: "Dharma board SDK",
    product_revision_level: "1.00"
};



#define SWAP_32(x) ((((x) & 0xff000000) >> 24) | \
                    (((x) & 0x00ff0000) >> 8) | \
                    (((x) & 0x0000ff00) << 8) | \
                    (((x) & 0x000000ff) << 24))
#define SWAP_16(x) ((((x) & 0xff00) >> 8) | \
                    (((x) & 0x00ff) << 8))

static int test_unit_ready(usbs_mass_storage * ms, int * bytes_processed);
static int request_sense(usbs_mass_storage * ms, int * bytes_processed);
static int inquiry(usbs_mass_storage * ms, int * bytes_processed);
static int mode_sense_6(usbs_mass_storage * ms, int * bytes_processed);
static int start_stop_unit(usbs_mass_storage * ms, int * bytes_processed);
static int read_format_capacities(usbs_mass_storage * ms, int * bytes_processed);
static int read_capacity(usbs_mass_storage * ms, int * bytes_processed);
static int read_10(usbs_mass_storage * ms, int * bytes_processed);
static int write_10(usbs_mass_storage * ms, int * bytes_processed);

static void set_sense_data(int lun, int code);
static sense_data_t * get_sense_data(int lun);
static sense_data_t * peek_sense_data(int lun);

void
usbs_ms_protocol_init( usbs_mass_storage* ms, usbs_mass_storage_init_data* ms_init )
{
    int status;
    int i;
    
    mmc_data = (usbs_mmc_data_t*)ms->protocol_data;
    
    if( ms->protocol_data_len < sizeof( usbs_mmc_data_t ) ) {
	DBG(("%s insufficient memory, expect overruns\n", __FUNCTION__));
    }

    mmc_data->max_luns = ms_init->num_luns-1;
    // figure out how much space we have left
    i = ms->protocol_data_len - sizeof( usbs_mmc_data_t );
    mmc_data->data_buffer = (char*)( mmc_data + 1 );
    // align the buffer size to a block
    mmc_data->data_buffer_len = (i - (i % BLOCK_SIZE) );
    if( mmc_data->data_buffer_len < BLOCK_SIZE ) {
	DBG(("%s insufficient memory, expect overruns\n", __FUNCTION__));
    }
    
    for (i = 0; i <= mmc_data->max_luns; ++i) {
	status = cyg_io_lookup(ms_init->lun_names[i], &(mmc_data->lun_handle[i]) );
	if (status < 0) {
	    DBG(("%s error lookup card %d\n", __FUNCTION__, status));
	}
#if 1
        else {
            cyg_io_set_config( mmc_data->lun_handle[i], IO_BLK_SET_CONFIG_POWER_UP, NULL, NULL );
        }
#endif
	memset(&(mmc_data->sd[i]), 0, sizeof(sense_data_t));
	mmc_data->sd_valid[i] = false;
    }
}

int
usbs_ms_protocol_do_command(usbs_mass_storage * ms, int * bytes_processed)
{
    int status;

    /* Only want this to do its work while in one of the commands, in other
       words, don't want this to be queued. */
    mmc_data->reset_receive = false;
    
    /* Decode the command type */
    switch(ms->cbw.CBWCB[0]) {
	case VERIFY_10:
	    /* Verify does the same thing as test unit ready because the operation is supposed to verify
	     * the medium and since the mmc card takes care of that itself, we can fake verify with just
	     * checking whether the medium is present. */
	case TEST_UNIT_READY: 
	{
	    status = test_unit_ready(ms, bytes_processed);
	    break;
	}

	case REQUEST_SENSE: 
	{
	    status = request_sense(ms, bytes_processed);
	    break;
	}

	case INQUIRY:
	{
	    status = inquiry(ms, bytes_processed);
	    break;
	}

	case MODE_SENSE_6:
	{
	    status = mode_sense_6(ms, bytes_processed);
	    break;
	}

	case START_STOP_UNIT:
	{
	    status = start_stop_unit(ms, bytes_processed);
	    break;
	}

	case READ_FORMAT_CAPACITIES:
	{
	    status = read_format_capacities(ms, bytes_processed);
	    break;
	}
	
	case READ_CAPACITY:
	{
	    status = read_capacity(ms, bytes_processed);
	    break;
	}

	case READ_10:
	{
	    status = read_10(ms, bytes_processed);
	    break;
	}

	case WRITE_10:
	{
	    status = write_10(ms, bytes_processed);
	    break;
	}

	default: 
	{
	    DBG(("Unsupported command %x\n", ms->cbw.CBWCB[0]));
	    set_sense_data(ms->cbw.bCBWLUN, -ENOSYS);
	    status = CHECK_CONDITION;
	    *bytes_processed = 0;
	    break;
	}
    }
    return status;
}

int
usbs_ms_protocol_check_cbw(cbw_t * cbw)
{
    if (cbw->bCBWLUN <= mmc_data->max_luns) {
	/* TODO Check dCBWCBLength */
	return true;
    }
    return false;
}

void
usbs_ms_protocol_reset_bus(void)
{
    int err;
    int len;
    int num_resets;
    int i;
    
    /* Reset both cards */
    for (i = 0; i <= mmc_data->max_luns; ++i) {
	len = 0;
	err = cyg_io_set_config(mmc_data->lun_handle[i], IO_BLK_SET_CONFIG_RESET, 0, &len);
	if (err < 0) {
	    /* Error resetting card. Try again a few times. */
	    num_resets = 0;
	    while (cyg_io_set_config(mmc_data->lun_handle[i], IO_BLK_SET_CONFIG_RESET, 0, &len) < 0 &&
		   ++num_resets < 3)
		;
	    DBG(("%s error resetting device %d\n", __FUNCTION__, err));
	}
	else {
	    DBG(("reset device %d\n", i));
	}
	mmc_data->reset_receive = true;
    }
}

int
usbs_ms_protocol_get_max_lun(void)
{
    return mmc_data->max_luns;
}

static int
test_unit_ready(usbs_mass_storage * ms, int * bytes_processed)
    /* if can accept command without returning check condition status then good
       else return check condition and set up asc and ascq

       asc/ascq:
       illeagal request/lun not supported
       not ready/lun does not respond to selection
       not ready/medium not present
       not ready/lun not ready, cause not reportable
       not ready/lun is in process of becoming ready
       not ready/lun not ready, initializing command required
       not ready/lun not ready, manual intervention required
       not ready/lun not ready, format in progress
   
       p 161 spc-2 */
{
    sense_data_t * data;
    int status = ENOERR;
    int unused;
    
    *bytes_processed = 0;

    data = peek_sense_data(ms->cbw.bCBWLUN);
    if (data->sense_key == NO_SENSE) {
	unused = sizeof(unused);
	status = cyg_io_get_config(mmc_data->lun_handle[ms->cbw.bCBWLUN], IO_BLK_GET_CONFIG_MEDIA_STATUS, &unused, &unused);
	DBG(("0stat %d\n", status));
	set_sense_data(ms->cbw.bCBWLUN, status);
    }
    else {
	status = CHECK_CONDITION;
	DBG(("1stat %d\nsense_key %x asc %x\n", status, data->sense_key, data->asc));
    }
    return status;
}

static int
request_sense(usbs_mass_storage * ms, int * bytes_processed)
    /* if no sense data to return then asc/ascq: no sense/no additional sense information
   else if standby or idle power then asc/ascq: no sense/low power condition on and
       maintain power state
   else if invalid field in cdb or
       parity error in delivery subsystem or
       target malfunction then return check condition status

   request sense data:
       ili: check block length
       information: lba of last error
       additional sense length:
       sense key:
           no sense 0x00
	   recovered error 0x01
	   not ready 0x02
	   medium error 0x03
	   hardware error 0x04
	   illeagal request 0x05
	   unit attention 0x06
	   data protect 0x07
	   blank check 0x08
	   vendor specific 0x09
	   copy aborted 0x0a
	   aborted command 0x0b
	   volume overflow 0x0d
	   miscompare 0x0e
       
   p 134 spc-2 */
{
    int status = ENOERR;
    sense_data_t * sense_data;
    int len;
    
    if (ms->cbw.bmCBWFlags & CBW_DIRECTION) {

	DBG(("H %d\n", ms->cbw.bCBWLUN));
	
	/* Transmit sense data */
#if defined(SYZYGY_BUG)
	sense_data = get_sense_data(last_lun);
#else
	sense_data = get_sense_data(ms->cbw.bCBWLUN);
#endif
	len = ms->cbw.dCBWDataTransferLength < sizeof(sense_data_t) ? ms->cbw.dCBWDataTransferLength : sizeof(sense_data_t);
	usbs_start_tx_buffer(ms->tx_endpoint, (const void *)sense_data, len, ms->tx_completion_fn, ms);
	cyg_semaphore_wait(&ms->tx_completion_wait);
	if (ms->tx_result < 0) {
	    DBG(("%s error sending %d\n", __FUNCTION__, ms->tx_result));
	    status = ms->tx_result;
	    *bytes_processed = 0;
	}
	else {
	    *bytes_processed = ms->tx_result;
	}
    }
    else {
	*bytes_processed = 0;
	status = -EPHASE;
    }
    return status;
}

static int
inquiry(usbs_mass_storage * ms, int * bytes_processed)
    /* evpd: if vpd is not supported then check condition (illeagal request/invalid field in cdb)
   cmdddt: if command support data is not supported then check condition (illeagal request/invalid field in cdb)

   data should be returned even though device is not ready for other commands
   vendor id:
   product id:
   product revision level:
   
   p 87 spc-2 */
{
    inquiry_cmd_t * cmd = (inquiry_cmd_t *)ms->cbw.CBWCB;
    int status = ENOERR;

    /* Do not clear pending unit attention condition */

    if (ms->cbw.bmCBWFlags & CBW_DIRECTION) {
    
	if (cmd->evpd && cmd->cmddt == 0) {
	    /* Return optional vital product data in cmd->page_op_code field */
	    DBG(("%s return optional vital product data\n", __FUNCTION__));
	    
	    switch (cmd->page_op_code) {
#define NUM_SUPPORTED_PAGES 2
		case SUPPORTED_VPD_PAGES:
		{
		    cyg_uint8 page[sizeof(supported_vpd_t) + NUM_SUPPORTED_PAGES];
		    supported_vpd_t * vpd = (supported_vpd_t *)&page[0];

		    memset(&page, 0, sizeof(page));
		    vpd->peripheral_device_type = 0x0;
		    vpd->peripheral_qualifier = 0x0;
		    vpd->page_code = SUPPORTED_VPD_PAGES;
		    vpd->page_length = sizeof(page) - 4;
		    vpd->supported_page_list[0] = SUPPORTED_VPD_PAGES;
		    vpd->supported_page_list[1] = UNIT_SERIAL_NUMBER_PAGE;

		    usbs_start_tx_buffer(ms->tx_endpoint, (const void *)&page, sizeof(page),
					 ms->tx_completion_fn, ms);
		    cyg_semaphore_wait(&ms->tx_completion_wait);
		    if (ms->tx_result < 0) {
			DBG(("%s error sending %d\n", __FUNCTION__, ms->tx_result));
			status = ms->tx_result;
			*bytes_processed = 0;
		    }
		    else {
			*bytes_processed = ms->tx_result;
		    }
		    break;
		}
		
		case UNIT_SERIAL_NUMBER_PAGE:
		{
		    drive_geometry_t dg;
		    int dg_stat;
		    int len;

		    len = sizeof(dg);
		    dg_stat = cyg_io_get_config(mmc_data->lun_handle[ms->cbw.bCBWLUN], IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len);
                    // unconditional scoping block, stack trickery here
                    {
                        cyg_uint8 page[sizeof(unit_serial_number_t) + dg.serial_len];
                        unit_serial_number_t * sn = (unit_serial_number_t *)&page[0];
		    
                        memset(&page, 0, sizeof(page));
                        sn->peripheral_device_type = 0x0;
                        sn->peripheral_qualifier = 0x0;
                        sn->page_code = UNIT_SERIAL_NUMBER_PAGE;
                        sn->page_length = sizeof(page) - 4;
                        
                        if (dg_stat == ENOERR) {
                            memcpy(&sn->product_serial_number, &dg.serial_num, dg.serial_len);
                        }
                        else {
                            memset(&sn->product_serial_number, 0x20, dg.serial_len);
                        }

                        usbs_start_tx_buffer(ms->tx_endpoint, (const void *)&page, sizeof(page),
					 ms->tx_completion_fn, ms);
                    }
                    
                    cyg_semaphore_wait(&ms->tx_completion_wait);
                        
                    DBG(("%s ms->tx_result %d\n", __FUNCTION__, ms->tx_result));
                    if (ms->tx_result < 0) {
                        status = ms->tx_result;
                        *bytes_processed = 0;
                    }
                    else {
                        *bytes_processed = ms->tx_result;
                    }
                    
		    DBG(("%s status: %d\n", __FUNCTION__, status));
		    break;
		}
		
		default: 
		{
		    DBG(("Unsupported vital product data page %x\n", cmd->page_op_code));
		    *bytes_processed = 0;
		    set_sense_data(ms->cbw.bCBWLUN, -EINVAL);
		    status = CHECK_CONDITION;
		    break;
		}
	    }
	}
	else if (cmd->cmddt && cmd->evpd == 0) {
	    /* Return optional command support data in cmd->code field */
	    DBG(("%s return optional command support data\n", __FUNCTION__));
	    
	    /* If cmd->code not supported then */
	    *bytes_processed = 0;
	    set_sense_data(ms->cbw.bCBWLUN, -EINVAL);
	    status = CHECK_CONDITION;
	}
	else if (cmd->cmddt == 0 && cmd->evpd == 0) {
	    /* Return standard inquiry data */
	    DBG(("%s return standard inquiry data\n", __FUNCTION__));
	    
	    if (cmd->page_op_code == 0) {
		usbs_start_tx_buffer(ms->tx_endpoint, (const void *)&inquiry_data, sizeof(inquiry_data),
				     ms->tx_completion_fn, ms);
		cyg_semaphore_wait(&ms->tx_completion_wait);
		if (ms->tx_result < 0) {
		    DBG(("%s error sending %d\n", __FUNCTION__, ms->tx_result));
		    status = ms->tx_result;
		    *bytes_processed = 0;
		}
		else {
		    *bytes_processed = ms->tx_result;
		}
	    }
	    else {
		DBG(("%s:%d invalid field\n", __FUNCTION__, __LINE__));
		*bytes_processed = 0;
		set_sense_data(ms->cbw.bCBWLUN, -EINVAL);
		status = CHECK_CONDITION;
	    }
	}
	else {
	    DBG(("%s:%d invalid field\n", __FUNCTION__, __LINE__));
	    *bytes_processed = 0;
	    set_sense_data(ms->cbw.bCBWLUN, -EINVAL);
	    status = CHECK_CONDITION;
	}
    }
    else {
	*bytes_processed = 0;
	status = -EPHASE;
    }

    return status;
}

static int
mode_sense_6(usbs_mass_storage * ms, int * bytes_processed)
    /* return mode pages
   
   p 102 spc-2 */
{
    mode_sense_6_cmd_t * cmd = (mode_sense_6_cmd_t *)ms->cbw.CBWCB;
    int status = ENOERR;

    if (ms->cbw.bmCBWFlags & CBW_DIRECTION) {
	
	switch (cmd->page_code) {
	    case 0x00:
	    {
		/* This is copied from Clik */
		cyg_uint8 mode_param_list[sizeof(mode_param_header_6_t) + sizeof(block_descriptor_t)];
		mode_param_header_6_t * header = (mode_param_header_6_t *)&mode_param_list[0];
		block_descriptor_t * block = (block_descriptor_t *)&mode_param_list[sizeof(mode_param_header_6_t)];

		memset(mode_param_list, 0, sizeof(mode_param_list));
	    
		header->mode_data_length = sizeof(mode_param_list) - 1;
		header->medium_type = 0;
		header->device_specific = 0;
		header->block_descriptor_length = sizeof(block_descriptor_t);

		block->number_of_blocks = 0;
		block->density_code = 0;
		block->block_length_msb = 0;
		block->block_length = SWAP_16(BLOCK_SIZE);
	    
		/* Send mode parameter list */
		usbs_start_tx_buffer(ms->tx_endpoint, (const void *)mode_param_list,
				     sizeof(mode_param_list), ms->tx_completion_fn, ms);
		cyg_semaphore_wait(&ms->tx_completion_wait);
		if (ms->tx_result < 0) {
		    DBG(("%s error sending %d\n", __FUNCTION__, ms->tx_result));
		    status = ms->tx_result;
		    *bytes_processed = 0;
		}
		else {
		    *bytes_processed = ms->tx_result;
		}
		break;
	    }
	
	    case 0x3f: 
	    {
		/* Return all supported mode pages */
		/* TODO Add pages */
		cyg_uint8 mode_param_list[sizeof(mode_param_header_6_t) + sizeof(block_descriptor_t)];
		mode_param_header_6_t * header = (mode_param_header_6_t *)&mode_param_list[0];
		block_descriptor_t * block = (block_descriptor_t *)&mode_param_list[sizeof(mode_param_header_6_t)];
	    
		header->mode_data_length = sizeof(mode_param_list) - 1;
		header->medium_type = 0;
		header->device_specific = 0;
		header->block_descriptor_length = sizeof(block_descriptor_t);
	    
		block->number_of_blocks = 0;
		block->density_code = 0;
		block->block_length_msb = 0;
		block->block_length = SWAP_16(BLOCK_SIZE);
	    
		/* Send mode parameter list */
		usbs_start_tx_buffer(ms->tx_endpoint, (const void *)mode_param_list,
				     sizeof(mode_param_list), ms->tx_completion_fn, ms);
		cyg_semaphore_wait(&ms->tx_completion_wait);
		if (ms->tx_result < 0) {
		    DBG(("%s error sending %d\n", __FUNCTION__, ms->tx_result));
		    status = ms->tx_result;
		    *bytes_processed = 0;
		}
		else {
		    *bytes_processed = ms->tx_result;
		}
		break;
	    }

	    default: 
	    {
		DBG(("%s unsupported mode page %x\n", __FUNCTION__, cmd->page_code));
		*bytes_processed = 0;
		set_sense_data(ms->cbw.bCBWLUN, -EINVAL);
		status = CHECK_CONDITION;
		break;
	    }
	}
    }
    else {
	*bytes_processed = 0;
	status = -EPHASE;
    }
    return status;
}

static int
start_stop_unit(usbs_mass_storage * ms, int * bytes_processed)
    /* enable/disable device, set power conditions
   power condition: active/idle/standby/sleep
   
   p 53 sbc */
{
    int status = ENOERR;

    DBG(("%s not doing anything\n", __FUNCTION__));
    
    return status;
}

static int
read_format_capacities(usbs_mass_storage * ms, int * bytes_processed)
    /* No media: capacity list header + maximum capacity header
       Media: capacity list header + current capacity header + formattable capacity descriptor */
{    
    int status = ENOERR;
    sense_data_t * sense_data;
    drive_geometry_t dg;
    int len;
    cyg_uint8 data[sizeof(capacity_list_header_t) +
		  sizeof(current_capacity_t) +
		  sizeof(formattable_capacity_t)];
    capacity_list_header_t * hdr = (capacity_list_header_t *)&data[0];
    current_capacity_t * current = (current_capacity_t *)&data[sizeof(capacity_list_header_t)];
    formattable_capacity_t * format = (formattable_capacity_t *)&data[sizeof(capacity_list_header_t) +
								     sizeof(current_capacity_t)];

    if (ms->cbw.bmCBWFlags & CBW_DIRECTION) {
	
	sense_data = peek_sense_data(ms->cbw.bCBWLUN);
	if (sense_data->sense_key == UNIT_ATTENTION) {
	    status = CHECK_CONDITION;
	}
	else {
	    memset(data, 0, sizeof(data));
	    len = sizeof(dg);
	    status = cyg_io_get_config(mmc_data->lun_handle[ms->cbw.bCBWLUN], IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len);
	    if (status == ENOERR) {
		hdr->list_length = sizeof(current_capacity_t) + sizeof(formattable_capacity_t);
		current->descriptor_code = FORMATTED_MEDIA;
		current->number_of_blocks = SWAP_32(dg.num_blks);
		current->block_length = SWAP_16(dg.bytes_p_sec);
		format->number_of_blocks = SWAP_32(dg.num_blks);
		format->format_type = 0x00;	/* param is block size */
		format->param = SWAP_16(dg.bytes_p_sec);
		usbs_start_tx_buffer(ms->tx_endpoint, (const void *)data, sizeof(capacity_list_header_t) +
				     sizeof(current_capacity_t) + sizeof(formattable_capacity_t), ms->tx_completion_fn, ms);
		cyg_semaphore_wait(&ms->tx_completion_wait);
		if (ms->tx_result < 0) {
		    DBG(("%s error sending %d\n", __FUNCTION__, ms->tx_result));
		    status = ms->tx_result;
		    *bytes_processed = 0;
		}
		else {
		    *bytes_processed = ms->tx_result;
		}
	    }
	    else {
		/* Error getting geometry */
		DBG(("%s error getting geometry %d\n", __FUNCTION__, status));
		if (status == -ENOMED) {
		    hdr->list_length = sizeof(current_capacity_t);
		    current->descriptor_code = NO_MEDIA_PRESENT;
		    current->number_of_blocks = SWAP_32(0x40000); /* TODO Assume 256MB for now */
		    current->block_length = SWAP_16(BLOCK_SIZE);
		    usbs_start_tx_buffer(ms->tx_endpoint, (const void *)data, sizeof(capacity_list_header_t) +
					 sizeof(current_capacity_t), ms->tx_completion_fn, ms);
		    cyg_semaphore_wait(&ms->tx_completion_wait);
		    if (ms->tx_result < 0) {
			DBG(("%s error sending %d\n", __FUNCTION__, ms->tx_result));
			status = ms->tx_result;
			*bytes_processed = 0;
		    }
		    else {
			status = ENOERR;
			*bytes_processed = ms->tx_result;
		    }
		}
		else {
		    *bytes_processed = 0;
		    set_sense_data(ms->cbw.bCBWLUN, status);
		    status = CHECK_CONDITION;
		}
	    }
	}
    }
    else {
	*bytes_processed = 0;
	status = -EPHASE;
    }
    return status;
}

static int
read_capacity(usbs_mass_storage * ms, int * bytes_processed)
    /* p 42 sbc */
{
    read_capacity_cmd_t * cmd = (read_capacity_cmd_t *)ms->cbw.CBWCB;
    read_capacity_data_t data;
    drive_geometry_t dg;
    int len;
    int status = ENOERR;

    if (ms->cbw.bmCBWFlags & CBW_DIRECTION) {

	if (cmd->reladr) {
	    DBG(("%s reladr set, should not be, ignoring\n", __FUNCTION__, __LINE__));
	}
	
	if (cmd->pmi == 0 && cmd->lba != 0) {
	    *bytes_processed = 0;
	    set_sense_data(ms->cbw.bCBWLUN, -EINVAL);
	    status = CHECK_CONDITION;
	}
	else {
	    /* PMI bit is ignored, return last LBA of medium */
	    len = sizeof(dg);
	    status = cyg_io_get_config(mmc_data->lun_handle[ms->cbw.bCBWLUN], IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len);
	    if (status == ENOERR) {
		memset(&data, 0, sizeof(data));
		data.lba = SWAP_32(dg.num_blks - 1);
		data.blk_len = SWAP_32(dg.bytes_p_sec);
		usbs_start_tx_buffer(ms->tx_endpoint, (const void *)&data, sizeof(data), ms->tx_completion_fn, ms);
		cyg_semaphore_wait(&ms->tx_completion_wait);
		if (ms->tx_result < 0) {
		    DBG(("%s error sending %d\n", __FUNCTION__, ms->tx_result));
		    status = ms->tx_result;
		    *bytes_processed = 0;
		}
		else {
		    *bytes_processed = ms->tx_result;
		}
	    }
	    else {
		/* Error getting geometry */
		DBG(("%s error getting geometry %d\n", __FUNCTION__, status));

		*bytes_processed = 0;
		set_sense_data(ms->cbw.bCBWLUN, status);
		status = CHECK_CONDITION;
	    }
	}
    }
    else {
	*bytes_processed = 0;
	status = -EPHASE;
    }
    return status;
}

static int
read_10(usbs_mass_storage * ms, int * bytes_processed)
    /* transfer length: number of blocks
   p 41 sbc */
{
    read_10_cmd_t * cmd = (read_10_cmd_t *)ms->cbw.CBWCB;
    int sector;
    int end_sector;
    int num_blks, num_bytes;
    int status = ENOERR;
    
    if (ms->cbw.bmCBWFlags & CBW_DIRECTION) {
	
	sector = SWAP_32(cmd->lba);
	end_sector = sector + (cmd->transfer_length == 0 ? 256 : SWAP_16(cmd->transfer_length));
	
	*bytes_processed = 0;
	while (status >= 0 && sector < end_sector) {
	    /* Read block */
	    num_blks = (end_sector - sector) < ((mmc_data->data_buffer_len) / BLOCK_SIZE) ? (end_sector - sector) :
		((mmc_data->data_buffer_len) / BLOCK_SIZE);
            num_bytes = num_blks * BLOCK_SIZE;
	    status = cyg_io_bread(mmc_data->lun_handle[ms->cbw.bCBWLUN], mmc_data->data_buffer, &num_bytes, sector);
	    if (status == ENOERR) {
		/* Transmit it and update bytes processed */
		usbs_start_tx_buffer(ms->tx_endpoint, mmc_data->data_buffer, num_bytes, ms->tx_completion_fn, ms);
		cyg_semaphore_wait(&ms->tx_completion_wait);
		if (ms->tx_result < 0) {
		    /* Transmit failed */
		    DBG(("%s error sending %d\n", __FUNCTION__, ms->tx_result));
		    status = ms->tx_result;
		}
		else {
		    *bytes_processed += ms->tx_result;
		    sector += (ms->tx_result / BLOCK_SIZE);
		}
	    }
	    else {
		/* Read failed */
		DBG(("%s error reading from device %d\n", __FUNCTION__, status));
		set_sense_data(ms->cbw.bCBWLUN, status);
		status = CHECK_CONDITION;
	    }
	}
    }
    else {
	*bytes_processed = 0;
	status = -EPHASE;
    }
    return status;
}

static int
write_10(usbs_mass_storage * ms, int * bytes_processed)
    /* same as read(10)
   p 57 sbc */
{
    write_10_cmd_t * cmd = (write_10_cmd_t *)ms->cbw.CBWCB;
    int sector;
    int end_sector;
    int num_blks, num_bytes;
    int status = ENOERR;
    
    if ((ms->cbw.bmCBWFlags & CBW_DIRECTION) == 0) {
	
	sector = SWAP_32(cmd->lba);
	end_sector = sector + (cmd->transfer_length == 0 ? 256 : SWAP_16(cmd->transfer_length));

	//DBG(("%s %x:%x\n", __FUNCTION__, sector, end_sector));
    
	*bytes_processed = 0;
	while (status >= 0 && sector < end_sector && !mmc_data->reset_receive) {
	    /* Receive block and update bytes processed */
	    num_blks = (end_sector - sector) < ((mmc_data->data_buffer_len) / BLOCK_SIZE) ? (end_sector - sector) :
		((mmc_data->data_buffer_len) / BLOCK_SIZE);
            num_bytes = num_blks * BLOCK_SIZE;
	    usbs_start_rx_buffer(ms->rx_endpoint, mmc_data->data_buffer, num_bytes, ms->rx_completion_fn, ms);
	    cyg_semaphore_wait(&ms->rx_completion_wait);
	    if (ms->rx_result < 0) {
		/* Transmit failed */
		DBG(("%s error receiving %d\n", __FUNCTION__, ms->rx_result));
		status = ms->rx_result;
	    }
	    else {
		*bytes_processed += ms->rx_result;

		/* Write block */
		if (ms->rx_result % BLOCK_SIZE != 0) {
		    /* Did not receive an entire block from the host.  Things are sufficiently
		       fubar'd at this point, so assume that host will retry the write and
		       send the whole block again later. */
		    DBG(("%s ignoring partial block\n", __FUNCTION__));
		}
                num_bytes = ms->rx_result;
		num_blks = num_bytes / BLOCK_SIZE;
		//DBG(("%s   %x:%x\n", __FUNCTION__, num_blks, sector));
		status = cyg_io_bwrite(mmc_data->lun_handle[ms->cbw.bCBWLUN], mmc_data->data_buffer, &num_bytes, sector);
		if (status == ENOERR) {
		    sector += num_blks;
		}
		else {
		    /* Write failed */
		    DBG(("%s error writing device %d\n", __FUNCTION__, status));
		    set_sense_data(ms->cbw.bCBWLUN, status);
		    status = CHECK_CONDITION;
		}
	    }
	}
	if (mmc_data->reset_receive) {
	    status = -EPIPE;
	}
    }
    else {
	*bytes_processed = 0;
	status = -EPHASE;
    }
    return status;
}

static void
set_sense_data(int lun, int code)
{
    DBG(("E %d %d\n", lun, code));
#if defined(SYZYGY_BUG)
    last_lun = lun;
#endif
    switch (code) {
	case ENOERR: 
	{
	    mmc_data->sd[lun].sense_key = NO_SENSE;
	    mmc_data->sd[lun].asc = NO_ADDITIONAL_SENSE_INFORMATION;
	    break;
	}
		
	case -ERESET:
	{
	    mmc_data->sd[lun].sense_key = UNIT_ATTENTION;
	    mmc_data->sd[lun].asc = POWER_ON_BUS_RESET;
	    break;
	}
	
	case -EMEDCHG:
	{
	    mmc_data->sd[lun].sense_key = UNIT_ATTENTION;
	    mmc_data->sd[lun].asc = MEDIUM_CHANGED;
	    break;
	}
	
	case -ENOMED:
	{
	    mmc_data->sd[lun].sense_key = NOT_READY;
	    mmc_data->sd[lun].asc = MEDIUM_NOT_PRESENT;
	    break;
	}

	case -EINVAL:
	{
	    mmc_data->sd[lun].sense_key = ILLEGAL_REQUEST;
	    mmc_data->sd[lun].asc = INVALID_FIELD_IN_CDB;
	    break;
	}

	case -ENOSYS:
	{
	    mmc_data->sd[lun].sense_key = ILLEGAL_REQUEST;
	    mmc_data->sd[lun].asc = INVALID_COMMAND_OPERATION_CODE;
	    break;
	}
	
	default:
	{
	    mmc_data->sd[lun].sense_key = MEDIUM_ERROR;
	    mmc_data->sd[lun].asc = NO_ADDITIONAL_SENSE_INFORMATION;
	    break;
	}
    }
    mmc_data->sd[lun].additional_length = sizeof(sense_data_t) - 7;
    mmc_data->sd_valid[lun] = true;
    DBG(("E %d %d\n", lun, mmc_data->sd_valid[lun]));
}

static void
get_current_sense_data(int lun)
{
    int status;
    int len;
    
    if (mmc_data->sd_valid[lun] == false) {
	DBG(("A %d\n", lun));
	len = 0;
	status = cyg_io_get_config(mmc_data->lun_handle[lun], IO_BLK_GET_CONFIG_MEDIA_STATUS, 0, &len);
	set_sense_data(lun, status);
	mmc_data->sd[lun].additional_length = sizeof(sense_data_t) - 7; /* No additional sense data */
    }
    mmc_data->sd[lun].response_code = CURRENT_ERRORS;
}

static sense_data_t *
get_sense_data(int lun)
{
    DBG(("C %d %d\n", lun, mmc_data->sd_valid[lun]));
    get_current_sense_data(lun);
    mmc_data->sd_valid[lun] = false;
    return &mmc_data->sd[lun];
}

static sense_data_t *
peek_sense_data(int lun)
{
    DBG(("B %d %d\n", lun, mmc_data->sd_valid[lun]));
    get_current_sense_data(lun);
    return &mmc_data->sd[lun];
}

#endif /* CYGPKG_IO_USB_SLAVE_MASS_STORAGE_DEVICE_MMC */

