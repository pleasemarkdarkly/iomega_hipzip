//==========================================================================
//
//      usbs_dharma_pdiusbd12.c
//
//      Device driver for the Dharma PDIUSBD12 USB port.
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    toddm@iobjects.com
// Contributors: bartv
// Date:         2001-02-14
//
// This code implements support for the USB port on the Dharma PDIUSBD12.
//
//####DESCRIPTIONEND####
//==========================================================================

/* TODO
   - Transmit request is made for > 64 bytes on bulk endpoint.  Transmits
     some, then errors out.  Callback function called with error code, have no
     way of knowing how many bytes were actually transmitted.
     Possible solution: look at ep5.transmitted and call completion function twice,
     first with ep5.transmitted as the result, second with the error code as the
     result.  Re: mass storage, that will give a race condition for ms->tx_result.
   - Does not support multi configuration devices.  Move SET_CONFIGURATION
     request into upper layer in order to do that, but the catch is that
     the generic endpoints must be enabled, which is PDIUSBD12 specific.
     Also SET_FEATURE/CLEAR_FEATURE reference the configuration for determing
     self-powered and remote wakeup information.
*/

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/cyg_trac.h>
#include <cyg/infra/diag.h>

#include <pkgconf/system.h>
#include <pkgconf/hal_arm.h>
#include <pkgconf/devs_usb_dharma_pdiusbd12.h>

#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_cache.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_platform_ints.h>
#include <cyg/error/codes.h>

#include <cyg/io/usb/usb.h>
#include <cyg/io/usb/usbs.h>

// Debugging support. By default this driver operates mostly at
// DSR level, with the ISR doing a minimal amount of processing.
// However is also possible to run most of the code at thread-level,
// This is subject to some restrictions because the USB standard
// imposes timing constraints, e.g. some control operations such
// as SET-ADDRESS have to complete within 50ms. However it is
// very useful for debugging, specifically it allows you to put
// printf()'s in various places.
//
// Right now these configuration options are not exported to the
// user because running at DSR level is likely to be good enough
// for everybody not actively debugging this code. The options
// could be exported if necessary.
//#define CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_THREAD
#undef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_THREAD
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_THREAD
// Default stack size should be CYGNUM_HAL_STACK_SIZE_TYPICAL
# define CYGNUM_DEVS_USB_DHARMA_PDIUSBD12_THREAD_STACK_SIZE       4096
# define CYGNUM_DEVS_USB_DHARMA_PDIUSBD12_THREAD_PRIORITY         7
# include <cyg/kernel/kapi.h>
#endif

#if 0
# define DBG(a) diag_printf a
#else
# define DBG(a)
#endif

#undef FAILURES
#ifdef FAILURES
static volatile int ep4_failure = 7;
#endif

#undef STATS
#ifdef STATS
int ep4_receives = 0;
int ep4_errors = 0;
int ep5_transmits = 0;
int ep5_errors = 0;
# define INCR_STAT(a) (a) += 1
# define SET_STAT(a, b) (a) = (b)
#else
# define INCR_STAT(a)
# define SET_STAT(a, b)
#endif

// ----------------------------------------------------------------------------
// Start with definitions of the hardware. The use of a structure and
// a const base pointer should allow the compiler to do base/offset
// addressing and keep the hardware base address in a register. This
// is better than defining each hardware register via a separate
// address. 
//
// The USBS_CONTROL etc. macros allow for an alternative way of
// accessing the hardware if a better approach is presented, without
// having to rewrite all the code. Macros that correspond to registers
// are actually addresses, making it easier in the code to distinguish
// them from bit values: the & and * operators will just cancel out.

typedef struct usbs_dharma_pdiusbd12_hardware {
    volatile cyg_uint8 data;
    volatile cyg_uint8 command;
} usbs_dharma_pdiusbd12_hardware;

static usbs_dharma_pdiusbd12_hardware* const usbs_dharma_pdiusbd12_base =
(usbs_dharma_pdiusbd12_hardware* const) 0x40000000;
#define USBS_DATA       (&(usbs_dharma_pdiusbd12_base->data))
#define USBS_COMMAND    (&(usbs_dharma_pdiusbd12_base->command))

#define COMMAND_SELECT_ENDPOINT             0x00
#define COMMAND_READ_LAST_TRANSACTION_STATUS    0x40
#define COMMAND_SET_ENDPOINT_STATUS         0x40
#define COMMAND_GET_ENDPOINT_STATUS         0x80
#define COMMAND_SET_ADDRESS_ENABLE          0xD0
#define COMMAND_SET_ENDPOINT_ENABLE         0xD8
#define COMMAND_READ_BUFFER                 0xF0
#define COMMAND_WRITE_BUFFER                0xF0
#define COMMAND_ACK_SETUP                   0xF1
#define COMMAND_CLEAR_BUFFER                0xF2
#define COMMAND_SET_MODE                    0xF3
#define COMMAND_READ_INTERRUPT              0xF4
#define COMMAND_READ_FRAME_NUMBER           0xF5
#define COMMAND_VALIDATE_BUFFER             0xFA
#define COMMAND_SET_DMA                     0xFB

#define CONFIG1_NO_LAZY_CLOCK               0x02
#define CONFIG1_CLOCK_RUNNING               0x04
#define CONFIG1_INTERRUPT_MODE              0x08
#define CONFIG1_SOFT_CONNECT                0x10
#define CONFIG1_NONISO_MODE                 0x00
#define CONFIG1_ISOOUT_MODE                 0x40
#define CONFIG1_ISOIN_MODE                  0x80
#define CONFIG1_ISOIO_MODE                  0xC0
#define CONFIG2_CLOCK_12M                   0x03
#define CONFIG2_CLOCK_4M                    0x0B
#define CONFIG2_SET_TO_ONE                  0x40
#define CONFIG2_SOF_ONLY                    0x80

#define INTERRUPT_EP0                    0x0001
#define INTERRUPT_EP1                    0x0002
#define INTERRUPT_EP2                    0x0004
#define INTERRUPT_EP3                    0x0008
#define INTERRUPT_EP4                    0x0010
#define INTERRUPT_EP5                    0x0020
#define INTERRUPT_BUS_RESET              0x0040
#define INTERRUPT_SUSPEND                0x0080
#define INTERRUPT_DMA_EOT                0x0100

#define DMA_EP4_INT_ENABLE 0x40
#define DMA_EP5_INT_ENABLE 0x80

#define TRANSACTION_STATUS_DATA_RX_TX_SUCCESS   0x01
#define TRANSACTION_STATUS_ERROR_CODE_MASK      0x1E
#define TRANSACTION_STATUS_SETUP_PACKET         0x20
#define TRANSACTION_STATUS_DATA1_PACKET         0x40
#define TRANSACTION_STATUS_PREVIOUS_NOT_READ    0x80

#define EP_STATUS_FULL                0x01
#define EP_STALL                      0x02
#define EP_BUFFER0_FULL               0x20
#define EP_BUFFER1_FULL               0x60

#define EP0_BUFFER_SIZE 16
#define EP1_BUFFER_SIZE 16
#define EP4_BUFFER_SIZE 64
#define EP5_BUFFER_SIZE 64

// ----------------------------------------------------------------------------
// Static data. There is a data structure for each endpoint. The
// implementation is essentially a private class that inherits from
// common classes for control and data endpoints, but device drivers
// are supposed to be written in C so some ugliness is required.
//
// Devtab entries are defined in usbs_dharma_pdiusbd12_data.cxx to make sure
// that the linker does not garbage-collect them.

// Support for the interrupt handling code.
static cyg_interrupt usbs_dharma_pdiusbd12_intr_data;
static cyg_handle_t  usbs_dharma_pdiusbd12_intr_handle;
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
static volatile int  usbs_dharma_pdiusbd12_intr_reg;
#endif

// Endpoint 0 is always present, this module would not get compiled
// otherwise.
static void usbs_dharma_pdiusbd12_ep0_start(usbs_control_endpoint*);
static void usbs_dharma_pdiusbd12_poll(usbs_control_endpoint*);

typedef enum ep0_state {
    EP0_STATE_IDLE      = 0,
    EP0_STATE_IN        = 1,
    EP0_STATE_OUT       = 2
} ep0_state;

typedef struct ep0_impl {
    usbs_control_endpoint   common;
    ep0_state               ep_state;
    int                     length;
    int                     transmitted;
} ep0_impl;

static ep0_impl ep0 = {
    common:
    {
        state:                  USBS_STATE_POWERED,
        enumeration_data:       (usbs_enumeration_data*) 0,
        start_fn:               &usbs_dharma_pdiusbd12_ep0_start,
        poll_fn:                &usbs_dharma_pdiusbd12_poll,
        interrupt_vector:       CYGNUM_HAL_INTERRUPT_EINT1,
        control_buffer:         { 0, 0, 0, 0, 0, 0, 0, 0 },
        state_change_fn:        (void (*)(usbs_control_endpoint*, void*, usbs_state_change, int)) 0,
        state_change_data:      (void*) 0,
        standard_control_fn:    (usbs_control_return (*)(usbs_control_endpoint*, void*)) 0,
        standard_control_data:  (void*) 0,
        class_control_fn:       (usbs_control_return (*)(usbs_control_endpoint*, void*)) 0,
        class_control_data:     (void*) 0,
        vendor_control_fn:      (usbs_control_return (*)(usbs_control_endpoint*, void*)) 0,
        vendor_control_data:    (void*) 0,
        reserved_control_fn:    (usbs_control_return (*)(usbs_control_endpoint*, void*)) 0,
        reserved_control_data:  (void*) 0,
        buffer:                 (unsigned char*) 0,
        buffer_size:            0,
        fill_buffer_fn:         (void (*)(usbs_control_endpoint*)) 0,
        fill_data:              (void*) 0,
        fill_index:             0,
        complete_fn:            (usbs_control_return (*)(usbs_control_endpoint*, int)) 0
    },
    ep_state:           EP0_STATE_IDLE,
    length:             0,
    transmitted:        0
};

extern usbs_control_endpoint usbs_dharma_pdiusbd12_ep0 __attribute__((alias ("ep0")));

// Endpoint 4 is optional. If the application only involves control
// messages or only slave->host transfers then the endpoint 4
// support can be disabled.
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP4

typedef struct ep4_impl {
    usbs_rx_endpoint    common;
    int                 fetched;
    cyg_bool            using_buf_a;
} ep4_impl;

static void ep4_start_rx(usbs_rx_endpoint*);
static void ep4_set_halted(usbs_rx_endpoint*, cyg_bool);

static ep4_impl ep4 = {
    common: {
        start_rx_fn:        &ep4_start_rx,
        set_halted_fn:      &ep4_set_halted,
        complete_fn:        (void (*)(void*, int)) 0,
        complete_data:      (void*) 0,
        buffer:             (unsigned char*) 0,
        buffer_size:        0,
        halted:             0,
    },
    fetched:            -1,
    using_buf_a:        0
};

extern usbs_rx_endpoint usbs_dharma_pdiusbd12_ep4 __attribute__((alias ("ep4")));
#endif

// Endpoint 5 is optional. If the application only involves control
// messages or only host->slave transfers then the endpoint 5 support
// can be disabled.
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP5

typedef struct ep5_impl {
    usbs_tx_endpoint        common;
    int                     transmitted;
    int                     pkt_size;
} ep5_impl;

static void ep5_start_tx(usbs_tx_endpoint*);
static void ep5_set_halted(usbs_tx_endpoint*, cyg_bool);

static ep5_impl ep5 = {
    common: {
        start_tx_fn:        &ep5_start_tx,
        set_halted_fn:      &ep5_set_halted,
        complete_fn:        (void (*)(void*, int)) 0,
        complete_data:      (void*) 0,
        buffer:             (const unsigned char*) 0,
        buffer_size:        0,
        halted:             0,
    }, 
    transmitted:        0,
    pkt_size:           0
};

extern usbs_tx_endpoint usbs_dharma_pdiusbd12_ep5 __attribute__ ((alias ("ep5")));

#endif

// Write a value to a register
static void
usbs_dharma_pdiusbd12_write(volatile cyg_uint8* addr, cyg_uint8 value)
{
    volatile int delay = 0;
    
    *addr = value;

    // Delay ~500ns to comply with the timing specification of the PDIUSBD12
    //dc- factoring in instruction timing, we need to hold up for about 230ns
    // here. padding that to a (safer) 270ns, that's 19 cycles.
    // optimizing this loop should generate a "subs" and a "bne", or 2 cycles.
    // at this point flipping off debugging breaks this code
    do {
    } while( delay != 0 );
}

// Read a value from a register
static cyg_uint8
usbs_dharma_pdiusbd12_read(volatile cyg_uint8* addr)
{
    volatile int delay = 0;
    cyg_uint8 data;
    
    data = *addr;

    // Delay ~500ns to comply with the timing specification of the PDIUSBD12
    //dc- factoring in instruction timing, we need to hold up for about 230ns
    // here. padding that to a (safer) 270ns, that's 19 cycles.
    // at this point flipping off debugging breaks this code
    do {
    } while( delay != 0 );

    return data;
}

// ----------------------------------------------------------------------------
// Control transfers
//
// Endpoint 0 is rather more complicated than the others. This is
// partly due to the nature of the control protocol, for example it is
// bidirectional and transfer sizes are unpredictable.
//
// The USB standard imposes some timing constraints on endpoint 0, see
// section 9.2.6 of the spec. For example the set-address operation is
// supposed to take at most 50ms. In general the timings are reasonably
// generous so no special action is taken here. There could be problems
// when debugging, but that is pretty much inevitable.
//
// It is necessary to maintain a state for the control endpoint, the
// default state being idle. Control operations involve roughly the
// following sequence of events:
//
// 1) the host transmits a special setup token, indicating the start
//    of a control operation and possibly cancelling any existing control
//    operation that may be in progress. USB peripherals cannot NAK this
//    even if they are busy.
//
// 2) the setup operation is followed by an eight-byte packet from the host
//    that describes the specific control operation. This fits into the
//    Dharma PDIUSBD12's eight-byte buffer. There will be an endpoint 0
//    interrupt with the control in bit set.
//
// 3) the eight-byte packet is described in section 9.3 of the USB spec.
//    The first byte holds three fields, with the top bit indicating the
//    direction of subsequent data transfer. There are also two bytes
//    specifying the size of the subsequent transfer. Obviously the
//    packet also contains information such as the request type.
//
//    If the specified size is zero then the endpoint will remain in
//    its idle state. Otherwise the endpoint will switch to either
//    IN or OUT state, depending on the direction of subsequent data
//    transfers.
// 
// 4) some standard control operations can be handled by the code
//    here. Set-address involves setting the address register and
//    a change of state. Set-feature and clear-feature on the
//    data endpoints can be used in conjunction with endpoint-halt.
//    Get-status on the data endpoints tests the halt condition.
//    It is also possible for the hardware-specific code to
//    implement set-feature, clear-feature and get-status.
//
//    Other standard control operations will be handled by the
//    application-specific installed handler, if any, or by the
//    default handler usbs_handle_standard_control(). Class-specific
//    and vendor-specific functions require appropriate handlers to be
//    installed as well, If a particular request is not recognized
//    then a stall condition should be raised. This will not prevent
//    subsequent control operations, just the current one.
//
//    Data transfers on endpoint 0 involve at most eight bytes at
//    a time. More data will only be accepted if the control in
//    bit has been cleared via the read last transaction command, with the
//    hardware nak'ing OUT requests. To send data back to the host
//    the buffer should be filled and then the validate endpoint command
//    should be issued.
//
// It looks like processing all control packets at DSR level should be
// sufficient. During the data phase the hardware will NAK IN and
// OUT requests if the fifo is still empty/full, so timing is not
// an issue. Timing after receipt of the initial control message
// may be more important, e.g. the 50ms upper limit on processing
// the set-address control message, but this should still be ok.
// This decision may have to be re-examined in the light of
// experience.

// Init may get called during system startup or following a reset.
// During startup no work is needed since the hardware will
// have been reset and everything should be fine. After a reset
// the hardware will also be ok but there may be state information
// in ep0 that needs to be reset.
static void
usbs_dharma_pdiusbd12_ep0_init(void)
{
    if ((EP0_STATE_IDLE != ep0.ep_state) &&
        ((usbs_control_return (*)(usbs_control_endpoint*, int)) 0 != ep0.common.complete_fn)) {
        (*ep0.common.complete_fn)(&ep0.common, -EPIPE);
    }
    ep0.common.state            = USBS_STATE_POWERED;
    memset(ep0.common.control_buffer, 0, 8);
    ep0.common.buffer           = (unsigned char*) 0;
    ep0.common.buffer_size      = 0;
    ep0.common.fill_buffer_fn   = (void (*)(usbs_control_endpoint*)) 0;
    ep0.common.fill_data        = (void*) 0;
    ep0.common.fill_index       = 0;
    ep0.common.complete_fn      = (usbs_control_return (*)(usbs_control_endpoint*, int)) 0;
    ep0.ep_state                = EP0_STATE_IDLE;
    ep0.length                  = 0;
    ep0.transmitted             = 0;
}

// The start function is called by higher-level code when things have
// been set up, i.e. the enumeration data is available, appropriate
// handlers have been installed for the different types of control
// messages, and communication with the host is allowed to start. The
// next event that should happen is a reset operation from the host,
// so all other interrupts should be blocked. However it is likely
// that the hardware will detect a suspend state before the reset
// arrives, and hence the reset will act as a resume as well as a
// reset.
static void
usbs_dharma_pdiusbd12_ep0_start(usbs_control_endpoint* endpoint)
{
    CYG_ASSERT( endpoint == &ep0.common, "USB startup involves the wrong endpoint");
    
    // If there is additional platform-specific initialization to
    // perform, do it now. This macro can come from the platform HAL.
#ifdef DHARMA_PDIUSBD12_USB_PLATFORM_INIT
    DHARMA_PDIUSBD12_USB_PLATFORM_INIT;
#endif

    // Let's run
    // If PCMCIA is enabled, we must wait to unmask EINT1 as it is shared with several
    // other devices which may not be ready just yet.
    // TODO Make this conditional on interrupt sharing and move interrupt unmasking to
    // some later point in the user code after all devices are setup.  Also think about
    // how this should work with media removal/insertion on certain devices.
#ifndef CYGPKG_DEVS_PCMCIA_DHARMA
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);
#endif
}


// Filling the buffer with a reply to the host. This can be called
// immediately at the end of a control message, to prepare for
// the next IN token. It will also get called after each subsequent
// IN operation when the fifo has been emptied.
//
static void
usbs_dharma_pdiusbd12_ep0_write_buffer(void)
{
    char buffer[EP1_BUFFER_SIZE];
    int filled            = 0;
    int i;

    // ep0.length is the amount of data the host can accept
    // ep0.transmitted is the amount of data sent
    // ep0.buffer_size is the amount of data device can send

    // Fill local buffer so we know how much data will be sent.
    while (filled < sizeof(buffer)) {
        if (0 != ep0.common.buffer_size) {
	    CYG_ASSERT( filled < EP1_BUFFER_SIZE, "buffer overflow" );
	    buffer[filled++] = *ep0.common.buffer++;
            ep0.common.buffer_size--;
        } else if ((void (*)(usbs_control_endpoint*))0 != ep0.common.fill_buffer_fn) {
            (*ep0.common.fill_buffer_fn)(&ep0.common);
        } else {
            break;
        }
    }

    // Now transmit the buffer.
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 1);
    // Ignore optional read of endpoint, full/empty and stall conditions should be check
    // before calling this function.
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_WRITE_BUFFER);
    usbs_dharma_pdiusbd12_write(USBS_DATA, 0);
    usbs_dharma_pdiusbd12_write(USBS_DATA, filled);
    for (i = 0; i < filled; ++i) {
	usbs_dharma_pdiusbd12_write(USBS_DATA, buffer[i]);
    }
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_VALIDATE_BUFFER);
        
    // The following conditions are possible:
    // 1) amount transferred == amount requested, transfer complete.
    // 2) amount transferred < amount requested, this fill involved
    //    <sixteen bytes, transfer complete by definition of the protocol.
    // 3) amount transferred < amount requested but exactly sixteen
    //    bytes were sent this time. It will be necessary to send
    //    another packet of zero bytes to complete the transfer.
    ep0.transmitted += filled;
    if ((0 == ep0.common.buffer_size) &&
	((void (*)(usbs_control_endpoint*))0 == ep0.common.fill_buffer_fn) &&
	(filled < EP1_BUFFER_SIZE)) {

        ep0.ep_state    = EP0_STATE_IDLE;
        if ((usbs_control_return (*)(usbs_control_endpoint*, int))0 != ep0.common.complete_fn) {
            (void) (*ep0.common.complete_fn)(&ep0.common, 0);
        }
        ep0.common.buffer               = (unsigned char*) 0;
        ep0.common.buffer_size          = 0;
        ep0.common.fill_buffer_fn       = (void (*)(usbs_control_endpoint*)) 0;
    }
}

// Another utility function to read the buffer.
static int
usbs_dharma_pdiusbd12_ep0_read_buffer(unsigned char* buf, int count)
{
    int ep_status;
    int length;
    int i;
    
    CYG_ASSERT( (count >= 0) & (count <= EP0_BUFFER_SIZE), "EP0 write count must be in range" );

    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 0);
    ep_status = usbs_dharma_pdiusbd12_read(USBS_DATA);
    CYG_ASSERT( 0 == (EP_STALL & ep_status), "ep0 stalled" );
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_READ_BUFFER);
    usbs_dharma_pdiusbd12_read(USBS_DATA);
    length = usbs_dharma_pdiusbd12_read(USBS_DATA);

    for (i = 0; (i < length) && (i < count); ++i) {
	buf[i] = usbs_dharma_pdiusbd12_read(USBS_DATA);
    }

    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_CLEAR_BUFFER);
    
    return length;
}

// This is where all the hard work happens. It is a very large routine
// for a DSR, but in practice nearly all of it is nested if's and very
// little code actually gets executed. Note that there may be
// invocations of callback functions and the driver has no control
// over how much time those will take, but those callbacks should be
// simple.
static void
usbs_dharma_pdiusbd12_ep0_dsr(void)
{
    int transaction_status;
    int ep_status;
    
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_READ_LAST_TRANSACTION_STATUS + 0);
    transaction_status = usbs_dharma_pdiusbd12_read(USBS_DATA);
    // The interrupt bit is now cleared from the PDIUSBD12
    if (TRANSACTION_STATUS_DATA_RX_TX_SUCCESS == (transaction_status & TRANSACTION_STATUS_DATA_RX_TX_SUCCESS)) {
	
	usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 0);
	ep_status = usbs_dharma_pdiusbd12_read(USBS_DATA);
	CYG_ASSERT( (0 == (ep_status & EP_STALL)), "ep0 stalled" );
	
	// The endpoint can be in one of three states: IN, OUT, or IDLE.
	// For the first two it should mean that there is more data to be
	// transferred, which is pretty straightforward. IDLE means
	// that a new control message has arrived.
	if ((EP0_STATE_IDLE == ep0.ep_state) && (0 != (EP_STATUS_FULL & ep_status))) {
	
	    // A setup transfer.
	    if (0 != (transaction_status & TRANSACTION_STATUS_SETUP_PACKET)) {
	    
		int emptied = usbs_dharma_pdiusbd12_ep0_read_buffer(ep0.common.control_buffer,
								    sizeof(ep0.common.control_buffer));
	    
		if (sizeof(ep0.common.control_buffer) != emptied) {
		    // This indicates a serious problem somewhere. Respond by
		    // stalling. Hopefully the host will take some action that
		    // sorts out the mess.
		    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + 0);
		    usbs_dharma_pdiusbd12_write(USBS_DATA, 1);
		    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + 1);
		    usbs_dharma_pdiusbd12_write(USBS_DATA, 1);
		} else {
		    usbs_control_return result  = USBS_CONTROL_RETURN_UNKNOWN;
		    usb_devreq*         req     = (usb_devreq*) ep0.common.control_buffer;
		    int length, direction, protocol, recipient;
		
		    // Ack the setup token on control in and out and clear the out buffer.
		    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 0);
		    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_ACK_SETUP);
		    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_CLEAR_BUFFER);
		    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 1);
		    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_ACK_SETUP);
		
		    // Now we need to do some decoding of the data. A non-zero
		    // length field indicates that there will be a subsequent
		    // IN or OUT phase. The direction is controlled by the
		    // top bit of the first byte. The protocol is determined
		    // by other bits of the top byte.
		    length      = (req->length_hi << 8) | req->length_lo;
		    direction   = req->type & USB_DEVREQ_DIRECTION_MASK;
		    protocol    = req->type & USB_DEVREQ_TYPE_MASK;
		    recipient   = req->type & USB_DEVREQ_RECIPIENT_MASK;
		
#ifdef DBG
		    DBG(("ep0, new control request: type %x, code %x\n", req->type, req->request));
		    DBG(("     %s, length %d, value hi %x lo %x, index hi %x lo %x\n",
			 (USB_DEVREQ_DIRECTION_OUT == direction) ? "out" : "in",
			 length, req->value_hi, req->value_lo, req->index_hi, req->index_lo));
#endif
		
		    if (USB_DEVREQ_TYPE_STANDARD == protocol) {
		    
			// First see if the request can be handled entirely in
			// this module.
			if (USB_DEVREQ_SET_ADDRESS == req->request) {
			    // The USB device address should be in value_lo.
			    // No more data is expected.
			    int address = req->value_lo;
			    if ((0 != length) || (address > 127)) {
				result = USBS_CONTROL_RETURN_STALL;
			    } else {
				int old_state = ep0.common.state;
				ep0.common.state = USBS_STATE_ADDRESSED;
				usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ADDRESS_ENABLE);
				usbs_dharma_pdiusbd12_write(USBS_DATA, 0x80 | address);
				if ((void (*)(usbs_control_endpoint*, void*, usbs_state_change, int))0 !=
				    ep0.common.state_change_fn) {
				    (*ep0.common.state_change_fn)(&ep0.common, ep0.common.state_change_data,
								  USBS_STATE_CHANGE_ADDRESSED, old_state);
				}
				result = USBS_CONTROL_RETURN_HANDLED;
			    }
			} else if (USB_DEVREQ_GET_STATUS == req->request) {
			    // GET_STATUS on the device as a whole is used to
			    // check the remote-wakeup and self-powered bits.
			    // GET_STATUS on an endpoint is used to determine
			    // the halted condition.
			    // GET_STATUS on anything else has to be left to
			    // other code.
			    if (USB_DEVREQ_RECIPIENT_DEVICE == recipient) {
				// The host should expect two bytes back.
				if ((2 == length) && (USB_DEVREQ_DIRECTION_IN == direction)) {
				    CYG_ASSERT( 1 == ep0.common.enumeration_data->device.number_configurations, \
						"Higher level code should have handled this request");
				    ep0.common.control_buffer[0] =
					((ep0.common.enumeration_data->configurations[0].attributes &
					  USB_CONFIGURATION_DESCRIPTOR_ATTR_SELF_POWERED) >> 6) |
					((ep0.common.enumeration_data->configurations[0].attributes &
					  USB_CONFIGURATION_DESCRIPTOR_ATTR_REMOTE_WAKEUP) >> 4);
				    ep0.common.control_buffer[1] = 0;
				    ep0.common.buffer            = ep0.common.control_buffer;
				    ep0.common.buffer_size       = 2;
				    result                       = USBS_CONTROL_RETURN_HANDLED;
				} else {
				    result = USBS_CONTROL_RETURN_STALL;
				}
			    
			    } else if (USB_DEVREQ_RECIPIENT_ENDPOINT == recipient) {
				if ((2 == length) && (USB_DEVREQ_DIRECTION_IN == direction)) {
				    int endpoint = (req->index_lo & USB_DEVREQ_INDEX_ENDPOINT_MASK) * 2;
				    if (req->index_lo & USB_DEVREQ_INDEX_DIRECTION_IN) {
					++endpoint;
				    }
				    if (0 == endpoint || 1 == endpoint) {
					usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + endpoint);
					if (EP_STALL & usbs_dharma_pdiusbd12_read(USBS_DATA)) {
					    ep0.common.control_buffer[0] = 1;
					} else {
					    ep0.common.control_buffer[0] = 0;
					}
					result = USBS_CONTROL_RETURN_HANDLED;
				    }
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP4
				    else if ((4 == endpoint) &&
					     (USBS_STATE_CONFIGURED == (ep0.common.state & USBS_STATE_MASK))) {
				    
					ep0.common.control_buffer[0] = ep4.common.halted;
					result = USBS_CONTROL_RETURN_HANDLED;
				    
				    }
#endif                            
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP5
				    else if ((5 == endpoint) &&
					     (USBS_STATE_CONFIGURED == (ep0.common.state & USBS_STATE_MASK))) {
				    
					ep0.common.control_buffer[0] = ep5.common.halted;
					result = USBS_CONTROL_RETURN_HANDLED;
				    
				    }
#endif                            
				    else {
					// An invalid endpoint has been specified or the
					// endpoint can only be examined in configured state.
					result = USBS_CONTROL_RETURN_STALL;
				    }
				    if (USBS_CONTROL_RETURN_HANDLED == result) {
					ep0.common.control_buffer[1] = 0;
					ep0.common.buffer            = ep0.common.control_buffer;
					ep0.common.buffer_size       = 2;
				    }
				} else {
				    result = USBS_CONTROL_RETURN_STALL;
				}
			    }// Endpoint or device get-status
			
			} else if (USB_DEVREQ_SET_CONFIGURATION == req->request) {

			    // Changing to configuration 0 means a state change from
			    // configured to addressed. Changing to anything else means a
			    // state change to configured. Both involve invoking the
			    // state change callback. If there are multiple configurations
			    // to choose from then this request has to be handled at
			    // a higher level. 
			    int old_state = ep0.common.state;
			    if (0 == req->value_lo) {
				ep0.common.state = USBS_STATE_ADDRESSED;
				// Disable the generic endpoints
				usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_ENABLE);
				usbs_dharma_pdiusbd12_write(USBS_DATA, 0);
				if ((void (*)(usbs_control_endpoint*, void*, usbs_state_change, int))0 !=
				    ep0.common.state_change_fn) {
				    (*ep0.common.state_change_fn)(&ep0.common, ep0.common.state_change_data,
								  USBS_STATE_CHANGE_DECONFIGURED, old_state);
				}
				result = USBS_CONTROL_RETURN_HANDLED;
			    
			    } else {
				CYG_ASSERT(1 == ep0.common.enumeration_data->device.number_configurations, \
					   "Higher level code should have handled this request");
				if (req->value_lo == ep0.common.enumeration_data->configurations[0].configuration_id) {
				    ep0.common.state = USBS_STATE_CONFIGURED;
				    // Disable the generic endpoints...
				    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_ENABLE);
				    usbs_dharma_pdiusbd12_write(USBS_DATA, 0);
				    // then enable the generic endpoints
				    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_ENABLE);
				    usbs_dharma_pdiusbd12_write(USBS_DATA, 1);
				    if ((void (*)(usbs_control_endpoint*, void*, usbs_state_change, int))0 !=
					ep0.common.state_change_fn) {
					(*ep0.common.state_change_fn)(&ep0.common, ep0.common.state_change_data,
								      USBS_STATE_CHANGE_CONFIGURED, old_state);
				    }
				    result = USBS_CONTROL_RETURN_HANDLED;
				} else {
				    result = USBS_CONTROL_RETURN_STALL;
				}
			    }			
		    
			} else if (USB_DEVREQ_CLEAR_FEATURE == req->request) {
			
			    // CLEAR_FEATURE operates in much the same way as
			    // GET_STATUS
			    if (USB_DEVREQ_RECIPIENT_DEVICE == recipient) {
				// No data should be transferred, and only remote-wakeup can be cleared.
				if ((0 != length) || (USB_DEVREQ_FEATURE_DEVICE_REMOTE_WAKEUP != req->value_lo)) {
				    result = USBS_CONTROL_RETURN_STALL;
				} else {
				    CYG_ASSERT( 1 == ep0.common.enumeration_data->device.number_configurations, \
						"Higher level code should have handled this request");
				    // Clearing remote-wakeup is a no-op.  Technically it's not supported in this
				    // driver.
				    result = USBS_CONTROL_RETURN_HANDLED;
				}
			    } else if (USB_DEVREQ_RECIPIENT_ENDPOINT == recipient) {
				// The only feature that can be cleared is endpoint-halt, no data should be transferred.
				if ((0 != length) || (USB_DEVREQ_FEATURE_ENDPOINT_HALT != req->value_lo)) {
				    result = USBS_CONTROL_RETURN_STALL;
				} else {
				    int endpoint = (req->index_lo & USB_DEVREQ_INDEX_ENDPOINT_MASK) * 2;
				    if (req->index_lo & USB_DEVREQ_INDEX_DIRECTION_IN) {
					++endpoint;
				    }
				    if (0 == endpoint || 1 == endpoint) {
					usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + endpoint);
					usbs_dharma_pdiusbd12_write(USBS_DATA, 0);
					result = USBS_CONTROL_RETURN_HANDLED;
				    }
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP4
				    else if ((4 == endpoint) &&
					     (USBS_STATE_CONFIGURED == (ep0.common.state & USBS_STATE_MASK))) {
					ep4_set_halted(&ep4.common, false);
					result = USBS_CONTROL_RETURN_HANDLED;
				    
				    }
#endif
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP5
				    else if ((5 == endpoint) &&
					     (USBS_STATE_CONFIGURED == (ep0.common.state & USBS_STATE_MASK))) {
					ep5_set_halted(&ep5.common, false);
					result = USBS_CONTROL_RETURN_HANDLED;
                                    
				    }
#endif
				    else {
					// Invalid endpoint or not in configured state.
					result = USBS_CONTROL_RETURN_STALL;
				    }
				}
			    }   // Endpoint or device clear-feature
			
			} else if (USB_DEVREQ_SET_FEATURE == req->request) {
			
			    // SET_FEATURE also operates in much the same way as
			    // GET_STATUS
			    if (USB_DEVREQ_RECIPIENT_DEVICE == recipient) {

				// Don't support remote wakeup for now
				result = USBS_CONTROL_RETURN_STALL;				
#if 0
				if ((0 != length) ||
				    (USB_DEVREQ_FEATURE_DEVICE_REMOTE_WAKEUP != req->value_lo)) {
				    result = USBS_CONTROL_RETURN_STALL;
				} else {
				    CYG_ASSERT( 1 == ep0.common.enumeration_data->device.number_configurations, \
						"Higher level code should have handled this request");
				    ep0.common.enumeration_data->configurations[0].attributes |=
					USB_CONFIGURATION_DESCRIPTOR_ATTR_REMOTE_WAKEUP;
				    result = USBS_CONTROL_RETURN_HANDLED;
				}
#endif
			    } else if (USB_DEVREQ_RECIPIENT_ENDPOINT == recipient) {
				int endpoint = (req->index_lo & USB_DEVREQ_INDEX_ENDPOINT_MASK) * 2;
				if (req->index_lo & USB_DEVREQ_INDEX_DIRECTION_IN) {
				    ++endpoint;
				}
				// Only the halt condition can be set, and no data should be transferred.
				// Halting endpoint 0 should probably be disallowed although the
				// standard does not explicitly say so.
				if ((0 != length) ||
				    (USB_DEVREQ_FEATURE_ENDPOINT_HALT != req->value_lo) ||
				    ((USBS_STATE_CONFIGURED != (ep0.common.state & USBS_STATE_MASK)) &&
				     ((0 != endpoint) && (1 != endpoint)))) {
				
				    result = USBS_CONTROL_RETURN_STALL;
				
				} else {
				    if (0 == endpoint || 1 == endpoint) {
					usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + endpoint);
					usbs_dharma_pdiusbd12_write(USBS_DATA, 1);
					result = USBS_CONTROL_RETURN_HANDLED;
				    }
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP4
				    else if (4 == endpoint) {
					ep4_set_halted(&ep4.common, true);
					result = USBS_CONTROL_RETURN_HANDLED;
				    }
#endif                            
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP5
				    else if (5 == endpoint) {
					ep5_set_halted(&ep5.common, true);
					result = USBS_CONTROL_RETURN_HANDLED;
				    }
#endif                            
				    else {
					result = USBS_CONTROL_RETURN_STALL;
				    }
				}
			    } // Endpoint or device set-feature
			    
			} else if (USB_DEVREQ_SYNCH_FRAME == req->request) {

			    if ((2 == length) && (USB_DEVREQ_DIRECTION_IN == direction)) {
				usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_READ_FRAME_NUMBER);
				ep0.common.control_buffer[0] = usbs_dharma_pdiusbd12_read(USBS_DATA);
				ep0.common.control_buffer[1] = usbs_dharma_pdiusbd12_read(USBS_DATA);
				ep0.common.buffer            = ep0.common.control_buffer;
				ep0.common.buffer_size       = 2;
				result                       = USBS_CONTROL_RETURN_HANDLED;
			    } else {
				result = USBS_CONTROL_RETURN_STALL;
			    }

			}
			
			// If the result has not been handled yet, pass it to
			// the install callback function (if any).
			if (USBS_CONTROL_RETURN_UNKNOWN == result) {
			    if ((usbs_control_return (*)(usbs_control_endpoint*, void*))0 !=
				ep0.common.standard_control_fn) {
				result = (*ep0.common.standard_control_fn)(&ep0.common, ep0.common.standard_control_data);
			    }
			}
		    
			// If the result has still not been handled, leave it to
			// the default implementation in the USB slave common place.
			if (USBS_CONTROL_RETURN_UNKNOWN == result) {
			    result = usbs_handle_standard_control(&ep0.common);
			}
		    
		    } else {
			// The other three types of control message can be
			// handled by similar code.
			usbs_control_return (*callback_fn)(usbs_control_endpoint*, void*);
			void* callback_arg;
			DBG(("non-standard control request %x", req->request));
		    
			if (USB_DEVREQ_TYPE_CLASS == protocol) {
			    callback_fn  = ep0.common.class_control_fn;
			    callback_arg = ep0.common.class_control_data;
			} else if (USB_DEVREQ_TYPE_VENDOR == protocol) {
			    callback_fn  = ep0.common.vendor_control_fn;
			    callback_arg = ep0.common.vendor_control_data;
			} else {
			    callback_fn  = ep0.common.reserved_control_fn;
			    callback_arg = ep0.common.reserved_control_data;
			}
		    
			if ((usbs_control_return (*)(usbs_control_endpoint*, void*)) 0 == callback_fn) {
			    result = USBS_CONTROL_RETURN_STALL;
			} else {
			    result = (*callback_fn)(&ep0.common, callback_arg);
			}
		    }
		    DBG(("Control request done, %d\n", result));
		
		    if (USBS_CONTROL_RETURN_HANDLED != result) {
			// This control request cannot be handled. Generate a stall.
			usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + 0);
			usbs_dharma_pdiusbd12_write(USBS_DATA, 1);
			usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + 1);
			usbs_dharma_pdiusbd12_write(USBS_DATA, 1);
		    } else {
			// The control request has been handled. Is there any more
			// data to be transferred?
			if (0 == length) {
			    if (USB_DEVREQ_DIRECTION_OUT == direction) {
				// Ack host by sending a 0 length packet.
				ep0.transmitted = 0;
				ep0.length      = 0;
				ep0.ep_state = EP0_STATE_IN;
				usbs_dharma_pdiusbd12_ep0_write_buffer();
			    }
			} else {
			    // The endpoint should now go into IN or OUT mode while the
			    // remaining data is transferred.
			    ep0.transmitted     = 0;
			    ep0.length          = length;
			    if (USB_DEVREQ_DIRECTION_OUT == direction) {
				// Wait for the next packet from the host.
				ep0.ep_state = EP0_STATE_OUT;
				CYG_ASSERT( (unsigned char*) 0 != ep0.common.buffer, \
					    "A receive buffer should have been provided");
				CYG_ASSERT( (usbs_control_return (*)(usbs_control_endpoint*, int))0 !=
					    ep0.common.complete_fn, \
					    "A completion function should be provided for OUT control messages");
			    } else {
				ep0.ep_state = EP0_STATE_IN;
				usbs_dharma_pdiusbd12_ep0_write_buffer();
			    }
			}   // Control message handled
		    }   // Received 8-byte control message
		}
	    } else {
	    
		// Ignore non setup packets in idle state
		usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 0);
		usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_CLEAR_BUFFER);
	    
		usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_ACK_SETUP);
		usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 1);
		usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_ACK_SETUP);
	    }
        
	} else if ((EP0_STATE_OUT == ep0.ep_state) && (0 != (EP_STATUS_FULL & ep_status))) {
	
	    // A host->device transfer. Higher level code must have
	    // provided a suitable buffer.
	    CYG_ASSERT( (unsigned char*)0 != ep0.common.buffer, "A receive buffer should have been provided" );
	    CYG_ASSERT( ep0.common.buffer_size - ep0.transmitted >= 0, "Negative buffer space remaining" );
	
	    ep0.transmitted += usbs_dharma_pdiusbd12_ep0_read_buffer(ep0.common.buffer + ep0.transmitted,
								     ep0.common.buffer_size - ep0.transmitted);

	    if (ep0.transmitted != ep0.length) {
		// The host is not allowed to send more data than it
		// indicated in the original control message, and all
		// messages until the last one should be full size.
		CYG_ASSERT( ep0.transmitted < ep0.length, "The host must not send more data than expected");
		CYG_ASSERT( 0 == (ep0.transmitted % EP0_BUFFER_SIZE), \
			    "All OUT packets until the last one should be full-size");
	    } else {
		// The whole transfer is now complete. Invoke the
		// completion function, and based on its return value
		// either generate a stall or complete the message.
		usbs_control_return result;
            
		CYG_ASSERT( (usbs_control_return (*)(usbs_control_endpoint*, int))0 != ep0.common.complete_fn, \
			    "A completion function should be provided for OUT control messages");

		result = (*ep0.common.complete_fn)(&ep0.common, 0);
		ep0.common.buffer           = (unsigned char*) 0;
		ep0.common.buffer_size      = 0;
		ep0.common.complete_fn      = (usbs_control_return (*)(usbs_control_endpoint*, int)) 0;
            
		if (USBS_CONTROL_RETURN_HANDLED != result) {
		    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + 0);
		    usbs_dharma_pdiusbd12_write(USBS_DATA, 1);
		    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + 1);
		    usbs_dharma_pdiusbd12_write(USBS_DATA, 1);
		}
	    }
	}
	
    } else {
	DBG(("%s TS: %x\n", __FUNCTION__, transaction_status));
	// Ignore transaction errors
    }
    
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT1);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);
} // ep0_dsr

static void
usbs_dharma_pdiusbd12_ep1_dsr(void)
{
    int transaction_status;
    int ep_status;

    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_READ_LAST_TRANSACTION_STATUS + 1);
    transaction_status = usbs_dharma_pdiusbd12_read(USBS_DATA);
    // The interrupt bit is now cleared from the PDIUSBD12
    //cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT1);
    //cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);

#ifdef DBG
    if (0 == (transaction_status & TRANSACTION_STATUS_DATA_RX_TX_SUCCESS)) {
	DBG(("%s TS: %x\n", __FUNCTION__, transaction_status));
    }
#endif
    
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 1);
    ep_status = usbs_dharma_pdiusbd12_read(USBS_DATA);
    
    // The endpoint should be in the IN state only here.
    if ((EP0_STATE_IN == ep0.ep_state) && (0 == (EP_STATUS_FULL & ep_status))) {
	
	usbs_dharma_pdiusbd12_ep0_write_buffer();
	
    }

    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT1);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);
}

#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP4
// ----------------------------------------------------------------------------
// Endpoint 4 is used for OUT transfers, i.e. receive operations.

// Complete a transfer. This takes care of invoking the completion
// callback and resetting the buffer.
static void
ep4_rx_complete(int result)
{
    void (*complete_fn)(void*, int)  = ep4.common.complete_fn;
    void* complete_data = ep4.common.complete_data;

#ifdef DBG
    if (-EAGAIN == result) {
	DBG(("%s stall\n", __FUNCTION__));
    }
#endif
    
    ep4.common.buffer           = (unsigned char*) 0;
    ep4.common.buffer_size      = 0;
    ep4.common.complete_fn      = (void (*)(void*, int)) 0;
    ep4.common.complete_data    = (void*) 0;
    ep4.fetched                 = -1;
    
    if ((void (*)(void*, int))0 != complete_fn) {
	(*complete_fn)(complete_data, result);
    }
}

static void
ep4_process_packet(void)
{
    int ep_status;
    int length;
    int i;
    
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 4);
    ep_status = usbs_dharma_pdiusbd12_read(USBS_DATA);
    while ((ep4.fetched < ep4.common.buffer_size) &&
	   (EP_STALL != (ep_status & EP_STALL)) &&
	   (EP_STATUS_FULL == (ep_status & EP_STATUS_FULL))) {
	INCR_STAT(ep4_receives);
	usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_READ_BUFFER);
	usbs_dharma_pdiusbd12_read(USBS_DATA);
	length = usbs_dharma_pdiusbd12_read(USBS_DATA);
        
        // dc- since the read call takes so long, unrolling this loop does not help
	for (i = 0; i < length; i++) {
	    // TODO Put this in a big fat warning in the documentation somewhere, no
	    // bounds checking on buffer, assume it's a multiple of 64 bytes.
	    ep4.common.buffer[ep4.fetched++] = usbs_dharma_pdiusbd12_read(USBS_DATA);
	}
        
	usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_CLEAR_BUFFER);

	usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 4);
	ep_status = usbs_dharma_pdiusbd12_read(USBS_DATA);
    }

    if (EP_STALL == (ep_status & EP_STALL)) {
	ep4_rx_complete(-EAGAIN);
    } else if (ep4.fetched == ep4.common.buffer_size) {
	ep4_rx_complete(ep4.fetched);
    } else if (ep4.fetched > ep4.common.buffer_size) {
	// TODO Return -EMSGSIZE if too big? But then
	// have no way of knowing how much data was sent.
	ep4_rx_complete(ep4.fetched); 
    }
}

// Start a transmission. This functionality is overloaded to cope with
// waiting for stalls to complete.
static void
ep4_start_rx(usbs_rx_endpoint* endpoint)
{
    CYG_ASSERT( endpoint == &ep4.common, "USB data transfer involves the wrong endpoint");

    cyg_drv_dsr_lock();

    // This indicates the start of a transfer.
    ep4.fetched = 0;
    ep4_process_packet();
    
    cyg_drv_dsr_unlock();
}

static void
ep4_set_halted(usbs_rx_endpoint* endpoint, cyg_bool new_value)
{
    CYG_ASSERT( endpoint == &ep4.common, "USB set-stall operation involves the wrong endpoint");

    cyg_drv_dsr_lock();
    
    if (ep4.common.halted != new_value) {
	if (new_value) {
	    // The endpoint should be stalled. There is a potential race
	    // condition here with a current transfer. Updating the
	    // stalled flag means that the dsr will do nothing.
	    ep4.common.halted = true;
	
	    // Now perform the actual stall.
	    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + 4);
	    usbs_dharma_pdiusbd12_write(USBS_DATA, 1);
	    
	    // And abort any current transfer.
	    ep4_rx_complete(-EAGAIN);
	    
	} else {
	    // The stall condition should be cleared. First take care of
	    // things at the hardware level so that a new transfer is
	    // allowed.
	    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + 4);
	    usbs_dharma_pdiusbd12_write(USBS_DATA, 0);
	    
	    // Now allow new transfers to begin.
	    ep4.common.halted = false;
	    ep4_rx_complete(0);
	}
    }
    
    cyg_drv_dsr_unlock();
}

// The DSR is invoked following an interrupt.
static void
usbs_dharma_pdiusbd12_ep4_dsr(void)
{
    int transaction_status;

    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_READ_LAST_TRANSACTION_STATUS + 4);
    transaction_status = usbs_dharma_pdiusbd12_read(USBS_DATA);
    // The interrupt bit is now cleared from the PDIUSBD12

    if (0 == (transaction_status & TRANSACTION_STATUS_DATA_RX_TX_SUCCESS)) {
	DBG(("%s TS: %x\n", __FUNCTION__, transaction_status));
    } else {
	if ((0 != ep4.common.buffer) && (0 <= ep4.fetched)) {
	    ep4_process_packet();
	}
    }

    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT1);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);
}

// Initialization.
//
// This may get called during system start-up or following a reset
// from the host.
static void
usbs_dharma_pdiusbd12_ep4_init(void)
{
    // Endpoints should never be halted during a start-up.
    ep4.common.halted           = false;

    // If there has been a reset and there was a receive in progress,
    // abort it. This also takes care of sorting out the endpoint
    // fields ready for the next rx.
    ep4_rx_complete(-EPIPE);
}

#endif  // CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP4


#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP5
// ----------------------------------------------------------------------------
// Endpoint 5 is used for IN transfers, i.e. transmitting data to the
// host. 
//

static void
ep5_process_packet(void)
{
    int ep_status;
    int i;
    
    ep5.pkt_size = ep5.common.buffer_size - ep5.transmitted;
    if (ep5.pkt_size > EP5_BUFFER_SIZE) {
	ep5.pkt_size = EP5_BUFFER_SIZE;
    }

    if (ep5.pkt_size == 0) {
	DBG(("ep5.pkt_size == 0\n"));
    }
    
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SELECT_ENDPOINT + 5);
    ep_status = usbs_dharma_pdiusbd12_read(USBS_DATA);
    CYG_ASSERT( 0 == (ep_status & EP_STALL), "ep5_process_packet, stalled" );
    CYG_ASSERT( 0 == (ep_status & EP_STATUS_FULL), "ep5_process_packet, full" );
    
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_WRITE_BUFFER);
    usbs_dharma_pdiusbd12_write(USBS_DATA, 0);
    usbs_dharma_pdiusbd12_write(USBS_DATA, ep5.pkt_size);    
    
    for (i = 0; i < ep5.pkt_size; i++) {
	usbs_dharma_pdiusbd12_write(USBS_DATA, ep5.common.buffer[ep5.transmitted + i]);
    }

    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_VALIDATE_BUFFER);    
}

// A utility routine for completing a transfer. This takes care of the
// callback as well as resetting the buffer.
static void
ep5_tx_complete(int result)
{
    void (*complete_fn)(void*, int)  = ep5.common.complete_fn;
    void* complete_data = ep5.common.complete_data;

#ifdef DBG
    if (-EAGAIN == result) {
	DBG(("%s stall\n", __FUNCTION__));
    }
#endif
    
    ep5.common.buffer           = (unsigned char*) 0;
    ep5.common.buffer_size      = 0;
    ep5.common.complete_fn      = (void (*)(void*, int)) 0;
    ep5.common.complete_data    = (void*) 0;

    if ((void (*)(void*, int))0 != complete_fn) {
	(*complete_fn)(complete_data, result);
    }
}


// The exported interface to start a transmission.
static void
ep5_start_tx(usbs_tx_endpoint* endpoint)
{
    CYG_ASSERT( endpoint == &ep5.common, "USB data transfer involves the wrong endpoint");

    cyg_drv_dsr_lock();

    // TODO Assumption is made now that endpoint will not be stalled between ep5_start_tx
    // and ep5_tx_complete, so no need to check for that in ep5_process_packet.
    if (ep5.common.halted && (0 != ep5.common.buffer_size)) {
	ep5_tx_complete(-EAGAIN);
    } else if (0 == ep5.common.buffer_size && 0 == ep5.common.buffer) {
	// A check to see if the endpoint is halted.
	if (0 == ep5.common.halted) {
	    ep5_tx_complete(0);
	}
    } else {
	ep5.transmitted = 0;
	ep5_process_packet();
    }
    
    cyg_drv_dsr_unlock();
}

static void
ep5_set_halted(usbs_tx_endpoint* endpoint, cyg_bool new_value)
{
    CYG_ASSERT(endpoint == &ep5.common, "USB set-stall operation involves the wrong endpoint");

    cyg_drv_dsr_lock();
    
    if (ep5.common.halted != new_value) {
	if (new_value) {
	    ep5.common.halted = true;
	    
	    // Now perform the actual stall.
	    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + 5);
	    usbs_dharma_pdiusbd12_write(USBS_DATA, 1);
	    
	    // And abort any current transfer.
	    ep5_tx_complete(-EAGAIN);
	    
	} else {
	    // First take care of the hardware so that a new transfer is
	    // allowed. Then allow new transfers to begin and inform
	    // higher-level code.
	    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_ENDPOINT_STATUS + 5);
	    usbs_dharma_pdiusbd12_write(USBS_DATA, 0);
	    ep5.common.halted = false;
	    ep5_tx_complete(0);
	}
    }

    cyg_drv_dsr_unlock();
}

// The dsr will be invoked when the transmit-packet-complete bit is
// set. Typically this happens when a packet has been completed
// (surprise surprise) but it can also happen for error conditions.
static void
usbs_dharma_pdiusbd12_ep5_dsr(void)
{
    int transaction_status;

    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_READ_LAST_TRANSACTION_STATUS + 5);
    transaction_status = usbs_dharma_pdiusbd12_read(USBS_DATA);
    // The interrupt bit is now cleared from the PDIUSBD12
    CYG_ASSERT( 0 == (transaction_status & TRANSACTION_STATUS_PREVIOUS_NOT_READ), \
		"ep5_dsr, previous transaction not read" );
    
    if (0 == (transaction_status & TRANSACTION_STATUS_DATA_RX_TX_SUCCESS)) {
	DBG(("%s TS: %x\n", __FUNCTION__, transaction_status));
    } else {
	INCR_STAT(ep5_transmits);
	ep5.transmitted += ep5.pkt_size;
	if (ep5.transmitted < ep5.common.buffer_size) {
	    ep5_process_packet();
	} else {
	    ep5_tx_complete(ep5.transmitted);
	}
    }
    
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT1);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);
}

// Endpoint 5 initialization.
//
// This may be called during system start-up or following a reset
// from the host.
static void
usbs_dharma_pdiusbd12_ep5_init(void)
{
    // Endpoints should never be halted after a reset
    ep5.common.halted   = false;

    // If there has been a reset and there was a receive in progress,
    // abort it. This also takes care of clearing the endpoint
    // structure fields.
    ep5_tx_complete(-EPIPE);
}

#endif // CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP5

// ----------------------------------------------------------------------------
// Interrupt handling
//
// As much work as possible is deferred to the DSR (or to the debug
// thread). Interrupts for the endpoints are never a problem: the
// variuos packet-complete etc. bits ensure that the endpoints
// remain quiescent until the relevant interrupt has been serviced.
// Suspend and resume are more complicated. A suspend means that
// there has been no activity for 3ms, which should be enough
// time for the whole thing to be handled. A resume means that there
// has been bus activity after a suspend, and again it is infrequent.
//
// Reset appears to be much more complicated. A reset means that the
// host is holding the USB lines to a specific state for 10ms. This is
// detected by the hardware, causing the USB controller to be reset
// (i.e. any pending transfers are discarded, etc.). The reset bit in
// the status register will be set, and an interrupt will be raised.
// Now, in theory the correct thing to do is to process this
// interrupt, block reset interrupts for the duration of these 10ms,
// and wait for further activity such as the control message to set
// the address.
//
// In practice this does not seem to work. Possibly the USB controller
// gets reset continuously while the external reset signal is applied,
// but I have not been able to confirm this. Messing about with the
// reset interrupt control bit causes the system to go off into
// never-never land. 10ms is too short a time to allow for manual
// debugging of what happens. So for now the interrupt source is
// blocked at the interrupt mask level and the dsr will do the
// right thing. This causes a significant number of spurious interrupts
// for the duration of the reset signal and not a lot else can happen.


// Perform reset operations on all endpoints that have been
// configured in. It is convenient to keep this in a separate
// routine to allow for polling, where manipulating the
// interrupt controller mask is a bad idea.
static void
usbs_dharma_pdiusbd12_handle_reset(void)
{
    int old_state;

    // Any state change must be reported to higher-level code
    old_state = ep0.common.state;
    ep0.common.state = USBS_STATE_DEFAULT;
    if ((void (*)(usbs_control_endpoint*, void*, usbs_state_change, int))0 != ep0.common.state_change_fn) {
	(*ep0.common.state_change_fn)(&ep0.common, ep0.common.state_change_data,
				      USBS_STATE_CHANGE_RESET, old_state);
    }

    // Reinitialize all the endpoints that have been configured in.
    usbs_dharma_pdiusbd12_ep0_init();
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP4
    usbs_dharma_pdiusbd12_ep4_init();
#endif    
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP5
    usbs_dharma_pdiusbd12_ep5_init();
#endif
}

// The DSR. This can be invoked directly by poll(), or via the usual
// interrupt subsystem.
static void
usbs_dharma_pdiusbd12_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    int * intr_reg = (int *)data;
#else
    int interrupt = 0;
#endif
    
    CYG_ASSERT(CYGNUM_HAL_INTERRUPT_EINT1 == vector, "USB DSR should only be invoked for USB interrupts" );
#if !defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    CYG_ASSERT(0 == data, "The DHARMA_PDIUSBD12 USB DSR needs no global data pointer");
#endif
    
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    // Interrupt status comes from ISR
    //DBG(("IR: %x\n", *intr_reg));
#else
    // Read interrupt status. This will unmask the interrupt for everything
    // except packet transfers.
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_READ_INTERRUPT);
    interrupt = usbs_dharma_pdiusbd12_read(USBS_DATA);
    interrupt |= usbs_dharma_pdiusbd12_read(USBS_DATA) << 8;

    //DBG(("IR: %x\n", interrupt));
#endif

#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    if (*intr_reg) {
	if (*intr_reg & INTERRUPT_SUSPEND) {
#else
	    if (interrupt) {
		if (interrupt & INTERRUPT_SUSPEND) {
#endif
	    // Process suspend state
	    int old_state       = ep0.common.state;
	    ep0.common.state   |= USBS_STATE_SUSPENDED;
	    if ((void (*)(usbs_control_endpoint*, void*, usbs_state_change, int))0 != ep0.common.state_change_fn) {
		(*ep0.common.state_change_fn)(&ep0.common, ep0.common.state_change_data,
					      USBS_STATE_CHANGE_SUSPENDED, old_state);
	    }
	    cyg_drv_interrupt_acknowledge(vector);
	    cyg_drv_interrupt_unmask(vector);
	} else {
	    // Notify resume state change if necessary
	    if (USBS_STATE_SUSPENDED & ep0.common.state) {
		int old_state = ep0.common.state;
		ep0.common.state &= ~USBS_STATE_SUSPENDED;
		if ((void (*)(usbs_control_endpoint*, void*, usbs_state_change, int))0 != ep0.common.state_change_fn) {
		    (*ep0.common.state_change_fn)(&ep0.common, ep0.common.state_change_data,
						  USBS_STATE_CHANGE_RESUMED, old_state);
		}
	    }
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
	    if (*intr_reg & INTERRUPT_BUS_RESET) {
#else
		if (interrupt & INTERRUPT_BUS_RESET) {
#endif
		// Reset is special, since it invalidates everything else.
		usbs_dharma_pdiusbd12_handle_reset();
		cyg_drv_interrupt_acknowledge(vector);
		cyg_drv_interrupt_unmask(vector);
	    } else {
		// Now process endpoint interrupts. Control operations on
		// endpoint 0 may have side effects on the other endpoints
		// so it is better to leave them until last.  Order is alos
		// important in that transmit must be processed before receive
		// else the state gets confused.
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP5
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
		if (*intr_reg & INTERRUPT_EP5) {
#else
		    if (interrupt & INTERRUPT_EP5) {
#endif
		    usbs_dharma_pdiusbd12_ep5_dsr();
		}
#endif
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP4
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
		if (*intr_reg & INTERRUPT_EP4) {
#else
		    if (interrupt & INTERRUPT_EP4) {
#endif
		    usbs_dharma_pdiusbd12_ep4_dsr();
		}
#endif
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
		if (*intr_reg & INTERRUPT_EP1) {
#else
		    if (interrupt & INTERRUPT_EP1) {
#endif
		    usbs_dharma_pdiusbd12_ep1_dsr();
		}
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
		if (*intr_reg & INTERRUPT_EP0) {
#else
		    if (interrupt & INTERRUPT_EP0) {
#endif
		    usbs_dharma_pdiusbd12_ep0_dsr();
		}		
#if 0
		// Ignore the following interrupts
		if (*intr_reg & INTERRUPT_DMA_EOT ||
		    *intr_reg & INTERRUPT_EP2 ||
		    *intr_reg & INTERRUPT_EP3) {
		    cyg_drv_interrupt_acknowledge(vector);
		    cyg_drv_interrupt_unmask(vector);
		}
#endif
	    }
	}
    } else {
	cyg_drv_interrupt_acknowledge(vector);
	cyg_drv_interrupt_unmask(vector);
    }
}

// ----------------------------------------------------------------------------
// Optionally the USB code can do most of its processing in a thread
// rather than in a DSR.
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_THREAD
static unsigned char usbs_dharma_pdiusbd12_thread_stack[CYGNUM_DEVS_USB_DHARMA_PDIUSBD12_THREAD_STACK_SIZE];
static cyg_thread    usbs_dharma_pdiusbd12_thread;
static cyg_handle_t  usbs_dharma_pdiusbd12_thread_handle;
static cyg_sem_t     usbs_dharma_pdiusbd12_sem;


static void
usbs_dharma_pdiusbd12_thread_fn(cyg_addrword_t param)
{
    for (;;) {
	cyg_semaphore_wait(&usbs_dharma_pdiusbd12_sem);
	usbs_dharma_pdiusbd12_dsr(CYGNUM_HAL_INTERRUPT_EINT1, 0, 0);
    }
    CYG_UNUSED_PARAM(cyg_addrword_t, param);
}

static void
usbs_dharma_pdiusbd12_thread_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    CYG_ASSERT( 0 != isr_status_bits, "DSR's should only be scheduled when there is work to do");
    cyg_semaphore_post(&usbs_dharma_pdiusbd12_sem);
    
    CYG_UNUSED_PARAM(cyg_vector_t, vector);
    CYG_UNUSED_PARAM(cyg_ucount32, count);
    CYG_UNUSED_PARAM(cyg_addrword_t, data);
}

#endif

// ----------------------------------------------------------------------------
// The interrupt handler. This does as little as possible.
static cyg_uint32
usbs_dharma_pdiusbd12_isr(cyg_vector_t vector, cyg_addrword_t data)
{
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    int * intr_reg = (int *)data;
#endif
    
    CYG_ASSERT(CYGNUM_HAL_INTERRUPT_EINT1 == vector, "USB ISR should only be invoked for USB interrupts" );
#if !defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    CYG_ASSERT(0 == data, "The DHARMA_PDIUSBD12 USB ISR needs no global data pointer" );
#endif

#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    // Read interrupt status. This will unmask the interrupt for everything
    // except packet transfers.
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_READ_INTERRUPT);
    *intr_reg = usbs_dharma_pdiusbd12_read(USBS_DATA);
    *intr_reg |= usbs_dharma_pdiusbd12_read(USBS_DATA) << 8;

    if (*intr_reg == 0) {
	// Interrupt source not USB
	//diag_printf("USB0\n");
	return 0;
    }
    else {
#endif
	//diag_printf("USB1\n");
	cyg_drv_interrupt_mask(vector);
	return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    }
#endif
}

// ----------------------------------------------------------------------------
// Polling support. This acts mostly like the interrupt handler: it
// sets the isr status bits and causes the dsr to run. Reset has to be
// handled specially: polling does nothing as long as reset is asserted.

static void
usbs_dharma_pdiusbd12_poll(usbs_control_endpoint* endpoint)
{
    CYG_ASSERT( endpoint == &ep0.common, "USB poll involves the wrong endpoint");

    if (*(volatile cyg_uint32 *)INTSR1 & INTSR1_EINT1) {
	usbs_dharma_pdiusbd12_dsr(CYGNUM_HAL_INTERRUPT_EINT1, 0, (cyg_addrword_t) 0);
    }
}


// ----------------------------------------------------------------------------
// Initialization.

void
usbs_dharma_pdiusbd12_init(void)
{
    // Activate the hardware
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_MODE);
    usbs_dharma_pdiusbd12_write(USBS_DATA, CONFIG1_SOFT_CONNECT | CONFIG1_NONISO_MODE);
    usbs_dharma_pdiusbd12_write(USBS_DATA, CONFIG2_SET_TO_ONE | CONFIG2_CLOCK_4M);
    // TODO Move the below to ep4_init and ep5_init
    usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_DMA);
    usbs_dharma_pdiusbd12_write(USBS_DATA, DMA_EP4_INT_ENABLE | DMA_EP5_INT_ENABLE);    
    
    usbs_dharma_pdiusbd12_ep0_init();
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP4
    usbs_dharma_pdiusbd12_ep4_init();
#endif
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_EP5
    usbs_dharma_pdiusbd12_ep5_init();
#endif

    // If processing is supposed to happen in a thread rather
    // than in DSR, initialize the threads.
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_THREAD
    cyg_semaphore_init(&usbs_dharma_pdiusbd12_sem, 0);
    cyg_thread_create(CYGNUM_DEVS_USB_DHARMA_PDIUSBD12_THREAD_PRIORITY,
		      &usbs_dharma_pdiusbd12_thread_fn,
		      0,
		      "Dharma PDIUSBD12 USB support",
		      usbs_dharma_pdiusbd12_thread_stack,
		      CYGNUM_DEVS_USB_DHARMA_PDIUSBD12_THREAD_STACK_SIZE,
		      &usbs_dharma_pdiusbd12_thread_handle,
		      &usbs_dharma_pdiusbd12_thread
	);
    cyg_thread_resume(usbs_dharma_pdiusbd12_thread_handle);
#endif
    
    // It is also possible and desirable to install the interrupt
    // handler here, even though there will be no interrupts for a
    // while yet.
    cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_EINT1,
			     10,        // priority
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
			     &usbs_dharma_pdiusbd12_intr_reg, // data
#else
			     0,
#endif
			     &usbs_dharma_pdiusbd12_isr,
#ifdef CYGPKG_DEVS_USB_DHARMA_PDIUSBD12_THREAD
			     &usbs_dharma_pdiusbd12_thread_dsr,
#else                             
			     &usbs_dharma_pdiusbd12_dsr,
#endif                             
			     &usbs_dharma_pdiusbd12_intr_handle,
			     &usbs_dharma_pdiusbd12_intr_data);
    cyg_drv_interrupt_attach(usbs_dharma_pdiusbd12_intr_handle);
    // Wait till interface is started before unmasking interrupt
}
