#ifndef CYGONCE_USBS_MASS_STORAGE_H_
#define  CYGONCE_USBS_MASS_STORAGE_H_
//==========================================================================
//
//      include/usbs_mass_storage.h
//
//      Description of the USB slave-side mass storage support
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
// Purpose:
// Description:  USB slave-side mass storage support
//
//
//####DESCRIPTIONEND####
//==========================================================================

#ifdef __cplusplus
extern "C" {
#endif
    
// The public interface depends on configuration options.
#include <pkgconf/io_usb_slave_mass_storage.h>

#define CYGPKG_IO_USB_MASS_STORAGE_THREAD_STACK_SIZE (4096)
#define CYGPKG_IO_USB_MASS_STORAGE_THREAD_PRIORITY   8

// Define the interface in terms of eCos data types.
#include <cyg/infra/cyg_type.h>
    
// The generic USB support
#include <cyg/io/usb/usbs.h>

#include <cyg/kernel/kapi.h>
    
// ----------------------------------------------------------------------------
// Mass storage class structures
    
#define CBW_SIGNATURE 0x43425355
#define CSW_SIGNATURE 0x53425355

#define CBW_DIRECTION 0x80

#define CSW_PASS 0x00
#define CSW_FAIL 0x01
#define CSW_PHASE_ERROR 0x02

typedef struct cbw
{
    cyg_uint32 dCBWSignature;
    cyg_uint32 dCBWTag;
    cyg_uint32 dCBWDataTransferLength;
    cyg_uint8 bmCBWFlags;
    cyg_uint8 bCBWLUN : 4;
    cyg_uint8 Reserved0 : 4;
    cyg_uint8 bCBWCBLength : 5;
    cyg_uint8 Reserved1 :    3;
    cyg_uint8 CBWCB[16];
} __attribute__ ((packed)) cbw_t;

typedef struct csw
{
    cyg_uint32 dCSWSignature;
    cyg_uint32 dCSWTag;
    cyg_uint32 dCSWDataResidue;
    cyg_uint8 bCSWStatus;
} __attribute__ ((packed)) csw_t;

typedef struct usbs_ms_thread_data_s
{
	unsigned char usbs_ms_thread_stack[CYGPKG_IO_USB_MASS_STORAGE_THREAD_STACK_SIZE];
	cyg_thread    usbs_ms_thread;
	cyg_handle_t  usbs_ms_thread_handle;
} usbs_ms_thread_data_t;
    
// ----------------------------------------------------------------------------
// This data structure serves two purposes. First, it keeps track of
// the information needed by the low-level USB ethernet code, for
// example which endpoints should be used for incoming and outgoing
// packets. Second, if the support for the TCP/IP stack is enabled
// then there are additional fields to support that (e.g. for keeping
// track of statistics).
//
// Arguably the two uses should be separated into distinct data
// structures. That would make it possible to instantiate multiple
// low-level USB-ethernet devices but only have a network driver for
// one of them. Achieving that flexibility would require some extra
// indirection, affecting performance and code-size, and it is not
// clear that that flexibility would ever prove useful. For now having
// a single data structure seems more appropriate.

typedef struct usbs_mass_storage {

    // What endpoints should be used for communication?
    usbs_control_endpoint*      control_endpoint;
    
    usbs_rx_endpoint*           rx_endpoint;
    void (*rx_completion_fn)(void *, int);
    cyg_sem_t                   rx_completion_wait;
    int                         rx_result;
    
    usbs_tx_endpoint*           tx_endpoint;
    void (*tx_completion_fn)(void *, int);
    cyg_sem_t                   tx_completion_wait;
    int                         tx_result;

	// thread data
    usbs_ms_thread_data_t       thread_data;

    // Protocol wrappers
    cbw_t cbw;
    csw_t csw;

    void*                       protocol_data;
    int				protocol_data_len;
} usbs_mass_storage;

typedef struct usbs_mass_storage_init_data {
    void* mempool;
    int   mempool_size;
    char** lun_names;
    int   num_luns;
} usbs_mass_storage_init_data;

// The package automatically instantiates one USB ethernet device.
extern usbs_mass_storage usbs_mass_storage0;

// ----------------------------------------------------------------------------
// A C interface to the low-level USB code.
    
// Initialize the USBS-mass storage support for a particular usbs_mass_storage device.
// This associates a usbs_mass_storage structure with specific endpoints.
extern void usbs_mass_storage_init(usbs_mass_storage_init_data*, usbs_control_endpoint*, usbs_rx_endpoint*,
				   usbs_tx_endpoint*);
    
// Start an asynchronous transmit of a single buffer. When the transmit
// has completed the callback function (if any) will be invoked with the
// specified pointer. NOTE: figure out what to do about error reporting.
extern void usbs_mass_storage_start_tx(usbs_mass_storage*, unsigned char*,
				       void (*)(usbs_mass_storage*, void*, int), void*);

// Start an asynchronous receive of a single buffer. When a
// single buffer has been received or when some sort of
// error occurs the callback function will be invoked.
extern void usbs_mass_storage_start_rx(usbs_mass_storage*, unsigned char*,
				       void (*)(usbs_mass_storage*, void*, int), void*);

// The handler for application class control messages. The init call
// will install this in the control endpoint by default. However the
// handler is fairly dumb: it assumes that all application control
// messages are for the mass storage interface and does not bother to
// check the control message's destination. This is fine for simple
// USB mass storage devices, but for any kind of multi-function peripheral
// higher-level code will have to perform multiplexing and invoke this
// handler only when appropriate.
extern usbs_control_return usbs_mass_storage_class_control_handler(usbs_control_endpoint*, void*);

// Similarly a handler for state change messages. Installing this
// means that the mass storage code will have sufficient knowledge about
// the state of the USB connection for simple mass storage-only
// peripherals, but not for anything more complicated. In the latter
// case higher-level code will need to keep track of which
// configuration, interfaces, etc. are currently active and explicitly
// enable or disable the mass storage device using the functions below.
extern void usbs_mass_storage_state_change_handler(usbs_control_endpoint*, void*, usbs_state_change, int);
extern void usbs_mass_storage_disable(usbs_mass_storage*);
extern void usbs_mass_storage_enable(usbs_mass_storage*);    
    
#ifdef __cplusplus
}; // extern "C"
#endif

#endif // CYGONCE_USBS_MASS_STORAGE_H_
