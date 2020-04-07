//==========================================================================
//
//      usbs_mass_storage.c
//
//      Support for USB-mass storage devices, slave-side.
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

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/cyg_trac.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/drv_api.h>

#include <pkgconf/io_usb_slave_mass_storage.h>

#define __ECOS 1
#include <cyg/io/usb/usbs_mass_storage.h>
#if defined(CYGPKG_IO_USB_SLAVE_MASS_STORAGE_DEVICE_ATAPI)
#include <cyg/io/usb/usbs_mass_storage_atapi.h>
#endif /* CYGPKG_IO_USB_SLAVE_MASS_STORAGE_DEVICE_ATAPI */
#if defined(CYGPKG_IO_USB_SLAVE_MASS_STORAGE_DEVICE_MMC)
#include <cyg/io/usb/usbs_mass_storage_mmc.h>
#endif /* CYGPKG_IO_USB_SLAVE_MASS_STORAGE_DEVICE_ATAPI */

#include <cyg/kernel/kapi.h>

#if 0
# define DBG(a) diag_printf a
#else
# define DBG(a)
#endif

// ----------------------------------------------------------------------------
// Static data.

//usbs_mass_storage usbs_mass_storage0;

// TODO Move these to usbs_mass_storage structure
//static unsigned char usbs_mass_storage_thread_stack[CYGPKG_IO_USB_MASS_STORAGE_THREAD_STACK_SIZE];
//static cyg_thread    usbs_mass_storage_thread;
//static cyg_handle_t  usbs_mass_storage_thread_handle;

// ----------------------------------------------------------------------------
// Internal functions.
static void usbs_mass_storage_thread_fn(cyg_addrword_t param);
static void tx_completion_fn(void* data, int result);
static void rx_completion_fn(void* data, int result);
static int no_data_transfer(usbs_mass_storage * ms);
static int data_in_transfer(usbs_mass_storage * ms);
static int data_out_transfer(usbs_mass_storage * ms);

// ----------------------------------------------------------------------------
// Initialization. This should be called explicitly by application code
// at an appropriate point in the system startup.
void
usbs_mass_storage_init(usbs_mass_storage_init_data* ms_init, usbs_control_endpoint* ctrl, usbs_rx_endpoint* rx, usbs_tx_endpoint* tx)
{
	usbs_mass_storage* ms = (usbs_mass_storage*) ms_init->mempool;

    ms->control_endpoint       = ctrl;
    ms->rx_endpoint            = rx;
    ms->rx_completion_fn       = &rx_completion_fn;
    cyg_semaphore_init(&ms->rx_completion_wait, 0);
    ms->tx_endpoint            = tx;
    ms->tx_completion_fn       = &tx_completion_fn;
    cyg_semaphore_init(&ms->tx_completion_wait, 0);
    
    // Install default handlers for some messages. Higher level code
    // may override this.
//    ctrl->state_change_fn       = &usbs_mass_storage_state_change_handler;
    ctrl->state_change_data     = (void*) ms;
    ctrl->class_control_fn      = &usbs_mass_storage_class_control_handler;
    ctrl->class_control_data    = (void*) ms;

    // Initialize the mass storage devices
    ms->protocol_data = (void*)( (char*)ms_init->mempool + sizeof( usbs_mass_storage ) );
    ms->protocol_data_len = ms_init->mempool_size - sizeof( usbs_mass_storage );
    
    usbs_ms_protocol_init( ms, ms_init );
    
    // Initialze the mass storage thread
    cyg_thread_create(CYGPKG_IO_USB_MASS_STORAGE_THREAD_PRIORITY,
		      usbs_mass_storage_thread_fn,
              (cyg_addrword_t)ms,
//		      (cyg_addrword_t)&usbs_mass_storage0,
		      "USB mass storage support",
		      ms->thread_data.usbs_ms_thread_stack,
		      CYGPKG_IO_USB_MASS_STORAGE_THREAD_STACK_SIZE,
		      &(ms->thread_data.usbs_ms_thread_handle),
		      &(ms->thread_data.usbs_ms_thread)
	);
    cyg_thread_resume(ms->thread_data.usbs_ms_thread_handle);
}

// ----------------------------------------------------------------------------
// Mass storage class protocol implementation.

static void
usbs_mass_storage_thread_fn(cyg_addrword_t param)
{
    usbs_mass_storage * ms = (usbs_mass_storage *)param;
    int status;

    for (;;) {
	// Get next CBW
	usbs_start_rx_buffer(ms->rx_endpoint, (unsigned char *)&(ms->cbw), sizeof(ms->cbw), ms->rx_completion_fn, ms);
	cyg_semaphore_wait(&ms->rx_completion_wait);
	status = ms->rx_result;
	if (status >= 0) {
	    // Make sure CBW is valid and meaningful
	    if (status == sizeof(ms->cbw) && ms->cbw.dCBWSignature == CBW_SIGNATURE &&
		usbs_ms_protocol_check_cbw(&ms->cbw)) {
		if (ms->cbw.dCBWDataTransferLength == 0) {
		    status = no_data_transfer(ms);
		} else if (ms->cbw.bmCBWFlags & CBW_DIRECTION) {
		    status = data_in_transfer(ms);
		} else {
		    status = data_out_transfer(ms);
		}
		if (status != -EPIPE) {   
		    // Send CSW
		    ms->csw.dCSWSignature = CSW_SIGNATURE;
		    ms->csw.dCSWTag = ms->cbw.dCBWTag;
		    do {
			usbs_start_tx_buffer(ms->tx_endpoint, (unsigned char *)&(ms->csw), sizeof(ms->csw),
					     ms->tx_completion_fn, ms);
			cyg_semaphore_wait(&ms->tx_completion_wait);
			status = ms->tx_result;
			if (status == -EAGAIN) {
			    // Endpoint is stalled, wait for it to become unstalled
			    usbs_start_tx_endpoint_wait(ms->tx_endpoint, ms->tx_completion_fn, ms);
			    cyg_semaphore_wait(&ms->tx_completion_wait);
			    status = ms->tx_result;
			} else if (status == -EPIPE) {
			    break;
			}
		    } while (status != sizeof(ms->csw));
		}
	    } else {
		// CBW is invalid or unmeaningful
		usbs_set_tx_endpoint_halted(ms->tx_endpoint, true);
		usbs_set_rx_endpoint_halted(ms->rx_endpoint, true);
		// Wait for reset recovery
		// The fall through here should be ok, since the host will issue Clear Feature
		// HALT to Bulk-Out endpoint last, so this will just loop and then wait till
		// that endpoint is unstalled, and the reset recovery is over.
		DBG(("%s reset recovery\n", __FUNCTION__));
	    }
	} else {
	    // Receive CBW failed
	    DBG(("Error receiving CBW %d\n", status));
	    if (status == -EAGAIN) {
		// Endpoint is stalled, wait for it to become unstalled
		usbs_start_rx_endpoint_wait(ms->rx_endpoint, ms->rx_completion_fn, ms);
		cyg_semaphore_wait(&ms->rx_completion_wait);
		status = ms->rx_result;
	    }
	}
    }
}

static int
no_data_transfer(usbs_mass_storage * ms)
{
    int bytes_processed;
    int status;
    
    bytes_processed = 0;
    status = usbs_ms_protocol_do_command(ms, &bytes_processed); 
    if (bytes_processed == 0) {
	if (status == ENOERR) {
	    ms->csw.bCSWStatus = CSW_PASS;
	} else if (status == -EPHASE) {
	    ms->csw.bCSWStatus = CSW_PHASE_ERROR;
	} else {
	    ms->csw.bCSWStatus = CSW_FAIL;
	}
	ms->csw.dCSWDataResidue = 0;
    } else {
	ms->csw.bCSWStatus = CSW_PHASE_ERROR;
    }

    /* This should only have to be done for PHASE ERRORs, but at least
       with Win2K mass storage driver it has to be done for FAIL also. */
    if (ms->csw.bCSWStatus != CSW_PASS) {
	usbs_set_tx_endpoint_halted(ms->tx_endpoint, true);
    }

    return status;
}

static int
data_in_transfer(usbs_mass_storage * ms)
{
    int bytes_processed;
    int status;
    
    bytes_processed = 0;
    status = usbs_ms_protocol_do_command(ms, &bytes_processed);
    if (bytes_processed == ms->cbw.dCBWDataTransferLength) {
	if (status == ENOERR) {
	    ms->csw.bCSWStatus = CSW_PASS;
	} else if (status == -EPHASE) {
	    ms->csw.bCSWStatus = CSW_PHASE_ERROR;
	} else {
	    ms->csw.bCSWStatus = CSW_FAIL;
	}
	ms->csw.dCSWDataResidue = 0;
    }
    else if (bytes_processed < ms->cbw.dCBWDataTransferLength) {
	if (status == ENOERR) {
	    ms->csw.bCSWStatus = CSW_PASS;
	} else if (status == -EPHASE) {
	    ms->csw.bCSWStatus = CSW_PHASE_ERROR;
	} else {
	    ms->csw.bCSWStatus = CSW_FAIL;
	}
	ms->csw.dCSWDataResidue = ms->cbw.dCBWDataTransferLength - bytes_processed;
    }
    else {
	ms->csw.bCSWStatus = CSW_PHASE_ERROR;
    }

    if (ms->csw.bCSWStatus != CSW_PASS ||
	ms->csw.dCSWDataResidue != 0) {
	usbs_set_tx_endpoint_halted(ms->tx_endpoint, true);
    }

    return status;
}

static int
data_out_transfer(usbs_mass_storage * ms)
{
    int bytes_processed;
    int status;

    bytes_processed = 0;
    status = usbs_ms_protocol_do_command(ms, &bytes_processed);
    if (bytes_processed <= ms->cbw.dCBWDataTransferLength) {
	if (bytes_processed != ms->cbw.dCBWDataTransferLength) {
	    usbs_set_rx_endpoint_halted(ms->rx_endpoint, true);
	}
	if (status == ENOERR) {
	    ms->csw.bCSWStatus = CSW_PASS;
	} else if (status == -EPHASE) {
	    ms->csw.bCSWStatus = CSW_PHASE_ERROR;
	} else {
	    ms->csw.bCSWStatus = CSW_FAIL;
	}
	ms->csw.dCSWDataResidue = ms->cbw.dCBWDataTransferLength - bytes_processed;
    } else {
	if (bytes_processed != ms->cbw.dCBWDataTransferLength) {
	    usbs_set_rx_endpoint_halted(ms->rx_endpoint, true);
	}
	ms->csw.bCSWStatus = CSW_PHASE_ERROR;
    }

    if (ms->csw.bCSWStatus != CSW_PASS) {
	usbs_set_tx_endpoint_halted(ms->tx_endpoint, true);
    }

    return status;
}

// ---------------------------------------------------------------------------
// Basic receive and transmit completion functions.

static void
tx_completion_fn(void* data, int result)
{
    usbs_mass_storage * ms = (usbs_mass_storage *)data;
    ms->tx_result = result;
    cyg_semaphore_post(&ms->tx_completion_wait);
}

static void
rx_completion_fn(void* data, int result)
{
    usbs_mass_storage * ms = (usbs_mass_storage *)data;    
    ms->rx_result = result;
    cyg_semaphore_post(&ms->rx_completion_wait);
}

// ----------------------------------------------------------------------------
// Control operations. These callbacks will typically be invoked
// in DSR context.

#define USBS_MASS_STORAGE_CONTROL_RESET        0xFF
#define USBS_MASS_STORAGE_CONTROL_GET_MAX_LUN  0xFE

usbs_control_return
usbs_mass_storage_class_control_handler(usbs_control_endpoint* endpoint, void* callback_data)
{
    usbs_control_return result = USBS_CONTROL_RETURN_STALL;
    
    usbs_mass_storage* ms = (usbs_mass_storage*)   callback_data;
    usb_devreq* devreq    = (usb_devreq*) endpoint->control_buffer;
    int         size      = (devreq->length_hi << 8) + devreq->length_lo;
    
    CYG_ASSERT(endpoint == ms->control_endpoint, "USB mass storage control messages correctly routed");

    if (USBS_MASS_STORAGE_CONTROL_RESET == devreq->request) {
	if (0 == size) {
	    DBG(("mass storage reset\n"));
	    usbs_ms_protocol_reset_bus();
	    result = USBS_CONTROL_RETURN_HANDLED;
	}
    } else if (USBS_MASS_STORAGE_CONTROL_GET_MAX_LUN == devreq->request) {
	if (1 == size) {
	    DBG(("get max lun\n"));
	    endpoint->buffer[0]   = usbs_ms_protocol_get_max_lun();
	    endpoint->buffer_size = 1;
	    result = USBS_CONTROL_RETURN_HANDLED;
	}
    }

    return result;
}

// State changes. As far as the mass storage code is concerned, if there
// is a change to CONFIGURED state then the device has come up,
// otherwise if there is a change from CONFIGURED state it has gone
// down. All other state changes are irrelevant.
void
usbs_mass_storage_state_change_handler(usbs_control_endpoint* endpoint, void* callback_data,
				       usbs_state_change change, int old_state)
{
    usbs_mass_storage* ms       = (usbs_mass_storage*) callback_data;
    CYG_ASSERT(endpoint == ms->control_endpoint, "USB mass storage state changes correctly routed");

    // Win2k class driver likes to reset the device and not issue the class request to reset,
    // so we need to do a protocol reset call here.
    if (USBS_STATE_CHANGE_RESET == change) {
	DBG(("bus reset\n"));
	usbs_ms_protocol_reset_bus();
    }

#if 0
    if (USBS_STATE_CHANGE_CONFIGURED == change) {
        if (USBS_STATE_CONFIGURED != old_state) {
            usbs_mass_storage_enable(ms);
        }
    } else if ((USBS_STATE_CHANGE_RESUMED == change) && (USBS_STATE_CONFIGURED == (USBS_STATE_MASK & old_state))) {
        usbs_mass_storage_enable(ms);
    } else if (eth->host_up) {
        usbs_mass_storage_disable(ms);
    }
#endif
}

// dc - this needs to call into the hardware driver to disable the interrupt,
//      otherwise we will continue to get interrupts which will make bad mem references
void
usbs_mass_storage_disable( usbs_mass_storage* ms )
{
	cyg_thread_suspend( ms->thread_data.usbs_ms_thread_handle );
    cyg_semaphore_destroy( &ms->rx_completion_wait );
    cyg_semaphore_destroy( &ms->tx_completion_wait );
	cyg_thread_delete( ms->thread_data.usbs_ms_thread_handle );
}
