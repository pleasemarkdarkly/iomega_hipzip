//==========================================================================
//
//      usb/usb_io.c
//
//      Stand-alone USB logical I/O support for RedBoot
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// The contents of this file are subject to the Red Hat eCos Public License 
// Version 1.1 (the "License"); you may not use this file except in         
// compliance with the License.  You may obtain a copy of the License at    
// http://www.redhat.com/                                                   
//                                                                          
// Software distributed under the License is distributed on an "AS IS"      
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the 
// License for the specific language governing rights and limitations under 
// the License.                                                             
//                                                                          
// The Original Code is eCos - Embedded Configurable Operating System,      
// released September 30, 1998.                                             
//                                                                          
// The Initial Developer of the Original Code is Red Hat.                   
// Portions created by Red Hat are                                          
// Copyright (C) 1998, 1999, 2000, 2001 Red Hat, Inc.                             
// All Rights Reserved.                                                     
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    gthomas
// Contributors: gthomas
// Date:         2000-07-14
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <redboot.h>
#include <cyg/hal/hal_misc.h>   // Helper functions
//#include <cyg/hal/hal_if.h>     // HAL I/O interfaces
#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_intr.h>

#include <usb/usb.h>
// TODO This is dharma only
#include <cyg/hal/hal_platform_ints.h>
#include <cyg/io/usb/usbs_pdiusbd12.h>

#define USB_CHANNEL CYGNUM_HAL_VIRTUAL_VECTOR_COMM_CHANNELS

//#define TRACE_USB
//#define DEBUG_USB

#ifdef DEBUG_USB
int show_usb = 1;
#endif 

static usbs_state_change state = USBS_STATE_DETACHED;
static usbs_state_change old_state = USBS_STATE_DETACHED;
static int _timeout = 500;
static int orig_console, orig_debug;

static int in_buflen = 0;
static unsigned char in_buf[256];
static unsigned char *in_bufp;
static int out_buflen = 0;
static unsigned char out_buf[256];
static unsigned char *out_bufp;

static usb_cmd_t cmd;
static usb_stat_t stat;

// Functions in this module
static void usb_io_flush(void);
static void usb_io_revert_console(void);
static void usb_io_putc(void*, cyg_uint8);

// Special characters used by Telnet - must be interpretted here
#define TELNET_IAC    0xFF // Interpret as command (escape)
#define TELNET_IP     0xF4 // Interrupt process
#define TELNET_WONT   0xFC // I Won't do it
#define TELNET_DO     0xFD // Will you XXX
#define TELNET_TM     0x06 // Time marker (special DO/WONT after IP)

static cyg_bool
_usb_io_getc_nonblock(void* __ch_data, cyg_uint8* ch)
{
    int n;
    
    //TODO This doesn't block
#ifdef TRACE_USB
    {
	int old_console;
	old_console = start_console();  
	printf("%s:%d\n", __FUNCTION__, __LINE__);  
	end_console(old_console);
    }
#endif

    if (in_buflen == 0) {
	// Write command
	cmd.DataTransferLength = sizeof(in_buf);
	cmd.Flags = 0x00;
	n = __usb_write(&cmd, sizeof(cmd));
	
	// Read data
	n = __usb_read(in_buf, cmd.DataTransferLength);

	// Read status
	n = __usb_read(&stat, sizeof(stat));
	in_bufp = in_buf;
	in_buflen = sizeof(in_buf) - stat.DataResidue;

#ifdef DEBUG_USB
        if (show_usb && (in_buflen > 0)) {
            int old_console;
            old_console = start_console();  
            printf("%s:%d %d\n", __FUNCTION__, __LINE__, in_buflen);  
            dump_buf(in_buf, in_buflen);  
            end_console(old_console);
        }
#endif // DEBUG_USB
    }
    if (in_buflen) {
        *ch = *in_bufp++;
        in_buflen--;
        return true;
    } else {
        return false;
    }
}

static cyg_bool
usb_io_getc_nonblock(void* __ch_data, cyg_uint8* ch)
{
    cyg_uint8 esc;

#ifdef TRACE_USB
    {
	int old_console;
	old_console = start_console();  
	printf("%s:%d\n", __FUNCTION__, __LINE__);  
	end_console(old_console);
    }
#endif

    if (!_usb_io_getc_nonblock(__ch_data, ch))
        return false;

    if (gdb_active || *ch != TELNET_IAC)
        return true;

    // Telnet escape - need to read/handle more
    while (!_usb_io_getc_nonblock(__ch_data, &esc)) ;

    switch (esc) {
    case TELNET_IAC:
        // The other special case - escaped escape
        return true;
    case TELNET_IP:
        // Special case for ^C == Interrupt Process
        *ch = 0x03;  
        // Just in case the other end needs synchronizing
        usb_io_putc(__ch_data, TELNET_IAC);
        usb_io_putc(__ch_data, TELNET_WONT);
        usb_io_putc(__ch_data, TELNET_TM);
        usb_io_flush();
        return true;
    case TELNET_DO:
        // Telnet DO option
        while (!_usb_io_getc_nonblock(__ch_data, &esc)) ;                
        // Respond with WONT option
        usb_io_putc(__ch_data, TELNET_IAC);
        usb_io_putc(__ch_data, TELNET_WONT);
        usb_io_putc(__ch_data, esc);
        return false;  // Ignore this whole thing!
    default:
        return false;
    }
}

static cyg_uint8
usb_io_getc(void* __ch_data)
{
    cyg_uint8 ch;
    int idle_timeout = 10;  // 10ms

#ifdef TRACE_USB
    {
	int old_console;
	old_console = start_console();  
	printf("%s:%d\n", __FUNCTION__, __LINE__);  
	end_console(old_console);
    }
#endif

    usb_io_flush();  // Make sure any output has been sent
    
    CYGARC_HAL_SAVE_GP();
    while (true) {
        if (usb_io_getc_nonblock(__ch_data, &ch)) break;
        if (--idle_timeout == 0) {
            usb_io_flush();
            idle_timeout = 10;
        } else {
            MS_TICKS_DELAY();
        }
    }
    CYGARC_HAL_RESTORE_GP();
    return ch;
}

static void
usb_io_flush(void)
{
    int n;

#ifdef TRACE_USB
    {
	int old_console;
	old_console = start_console();  
	printf("%s:%d\n", __FUNCTION__, __LINE__);  
	end_console(old_console);
    }
#endif

    if (out_buflen) {
#ifdef DEBUG_USB
        if (show_usb) {
            int old_console;
            old_console = start_console();  
            printf("%s:%d\n", __FUNCTION__, __LINE__);
            dump_buf(out_buf, out_buflen);  
            end_console(old_console);
        }
#endif // SHOW_USB
	
	// Write command
	cmd.DataTransferLength = out_buflen;
	cmd.Flags = CMD_FLAGS_DIRECTION;
	n = __usb_write(&cmd, sizeof(cmd));

	// Write data
	n = __usb_write(out_buf, out_buflen);
	
	// Read status
	n = __usb_read(&stat, sizeof(stat));
	if (stat.Status == STATUS_SUCCESS) {
	}
    }
    
    out_bufp = out_buf;  out_buflen = 0;
}

static void
usb_io_putc(void* __ch_data, cyg_uint8 c)
{
    static bool have_dollar, have_hash;
    static int hash_count;

#ifdef TRACE_USB
    {
	int old_console;
	old_console = start_console();  
	printf("%s:%d\n", __FUNCTION__, __LINE__);  
	end_console(old_console);
    }
#endif
    
    *out_bufp++ = c;
    if (c == '$') have_dollar = true;
    if (have_dollar && (c == '#')) {
        have_hash = true;
        hash_count = 0;
    }
    if ((++out_buflen == sizeof(out_buf)) || 
        (have_hash && (++hash_count == 3))) {
        usb_io_flush();
        have_dollar = false;
    }
}

static void
usb_io_write(void* __ch_data, const cyg_uint8* __buf, cyg_uint32 __len)
{
    int old_console;

    old_console = start_console();
    printf("%s.%d\n", __FUNCTION__, __LINE__);
    end_console(old_console);
#if 0
    CYGARC_HAL_SAVE_GP();

    while(__len-- > 0)
        usb_io_putc(__ch_data, *__buf++);

    CYGARC_HAL_RESTORE_GP();
#endif
}

static void
usb_io_read(void* __ch_data, cyg_uint8* __buf, cyg_uint32 __len)
{
    int old_console;

    old_console = start_console();
    printf("%s.%d\n", __FUNCTION__, __LINE__);
    end_console(old_console);
#if 0
    CYGARC_HAL_SAVE_GP();

    while(__len-- > 0)
        *__buf++ = usb_io_getc(__ch_data);

    CYGARC_HAL_RESTORE_GP();
#endif
}

static cyg_bool
usb_io_getc_timeout(void* __ch_data, cyg_uint8* ch)
{
    int delay_count;
    cyg_bool res;
    CYGARC_HAL_SAVE_GP();

#ifdef TRACE_USB
    {
	int old_console;
	old_console = start_console();  
	printf("%s:%d\n", __FUNCTION__, __LINE__);  
	end_console(old_console);
    }
#endif
    
    usb_io_flush();  // Make sure any output has been sent
    delay_count = _timeout;

    for(;;) {
        res = usb_io_getc_nonblock(__ch_data, ch);
        if (res || 0 == delay_count--)
            break;
        MS_TICKS_DELAY();
    }

    CYGARC_HAL_RESTORE_GP();
    return res;
}

static int
usb_io_control(void *__ch_data, __comm_control_cmd_t __func, ...)
{
    static int vector = 0;
    int ret = 0;
    static int irq_state = 0;

#ifdef TRACE_USB
    {
	int old_console;
	old_console = start_console();  
	printf("%s:%d %d\n", __FUNCTION__, __LINE__, __func);  
	end_console(old_console);
    }
#endif
    
    CYGARC_HAL_SAVE_GP();

    switch (__func) {
#if 0
    case __COMMCTL_IRQ_ENABLE:
        irq_state = 1;
        if (vector == 0) {
            vector = CYGNUM_HAL_INTERRUPT_EINT1;//TODO eth_drv_int_vector();
        }
        HAL_INTERRUPT_UNMASK(vector); 
        break;
    case __COMMCTL_IRQ_DISABLE:
        ret = irq_state;
        irq_state = 0;
	if (vector == 0) {
            vector = CYGNUM_HAL_INTERRUPT_EINT1;//TODO eth_drv_int_vector();
        }
        HAL_INTERRUPT_MASK(vector);
        break;
#endif
    case __COMMCTL_DBG_ISR_VECTOR:
        ret = vector;
        break;
    case __COMMCTL_SET_TIMEOUT:
    {
        va_list ap;

        va_start(ap, __func);

        ret = _timeout;
        _timeout = va_arg(ap, cyg_uint32);

        va_end(ap);
	break;
    }
    case __COMMCTL_FLUSH_OUTPUT:
        usb_io_flush();
	break;
    default:
        break;
    }
    CYGARC_HAL_RESTORE_GP();
    return ret;
}

static int
usb_io_isr(void *__ch_data, int* __ctrlc, 
           CYG_ADDRWORD __vector, CYG_ADDRWORD __data)
{
    char ch;

#ifdef TRACE_USB
    {
	int old_console;
	old_console = start_console();  
	printf("%s:%d\n", __FUNCTION__, __LINE__);  
	end_console(old_console);
    }
#endif
    
    cyg_drv_interrupt_acknowledge(__vector);
    *__ctrlc = 0;
    if (usb_io_getc_nonblock(__ch_data, &ch)) {
        if (ch == 0x03) {
            *__ctrlc = 1;
        }
    }
    return CYG_ISR_HANDLED;
}

// TEMP

int 
start_console(void)
{
    int cur_console;
    cur_console = CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);
    CYGACC_CALL_IF_SET_CONSOLE_COMM(0);
    return cur_console;
}

void
end_console(int old_console)
{
    // Restore original console
    CYGACC_CALL_IF_SET_CONSOLE_COMM(old_console);
}
// TEMP

static void
usb_io_revert_console(void)
{
#ifdef CYGPKG_REDBOOT_ANY_CONSOLE
    console_selected = false;
#endif
    CYGACC_CALL_IF_SET_CONSOLE_COMM(orig_console);
    CYGACC_CALL_IF_SET_DEBUG_COMM(orig_debug);
    console_echo = true;
}

static void
usb_io_assume_console(void)
{
    printf("%s\n", __FUNCTION__);
#ifdef CYGPKG_REDBOOT_ANY_CONSOLE
    console_selected = true;
#endif
    console_echo = false;
    orig_console = CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);
    CYGACC_CALL_IF_SET_CONSOLE_COMM(USB_CHANNEL);
    orig_debug = CYGACC_CALL_IF_SET_DEBUG_COMM(CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);
    CYGACC_CALL_IF_SET_DEBUG_COMM(USB_CHANNEL);
}

static void
usb_io_init(void)
{   
    static int init = 0;
    printf("%s\n", __FUNCTION__);
    if (!init) {
        hal_virtual_comm_table_t* comm;
        int cur = CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);

        // Setup procs in the vector table
        CYGACC_CALL_IF_SET_CONSOLE_COMM(USB_CHANNEL);
        comm = CYGACC_CALL_IF_CONSOLE_PROCS();
        //CYGACC_COMM_IF_CH_DATA_SET(*comm, chan);
        CYGACC_COMM_IF_WRITE_SET(*comm, usb_io_write);
        CYGACC_COMM_IF_READ_SET(*comm, usb_io_read);
        CYGACC_COMM_IF_PUTC_SET(*comm, usb_io_putc);
        CYGACC_COMM_IF_GETC_SET(*comm, usb_io_getc);
        CYGACC_COMM_IF_CONTROL_SET(*comm, usb_io_control);
        CYGACC_COMM_IF_DBG_ISR_SET(*comm, usb_io_isr);
        CYGACC_COMM_IF_GETC_TIMEOUT_SET(*comm, usb_io_getc_timeout);

        // Restore original console
        CYGACC_CALL_IF_SET_CONSOLE_COMM(cur);

        init = 1;
        gdb_active = false;
    }
#ifdef DEBUG_USB
    printf("show usb = %p\n", (void *)&show_usb);
#endif
}

void
usb_state_change_fn(usbs_control_endpoint *ep0, void *data, usbs_state_change change, int old_state)
{
    state = change;
}

// Check for incoming USB debug connection
void
usb_io_test(void)
{
    //if (!have_usb) return;
    __usb_poll();
    if (state != old_state) {
        // Something has changed
        if (state == USBS_STATE_CHANGE_CONFIGURED) {
            // A new connection has arrived
            usb_io_assume_console();
            in_bufp = in_buf;  in_buflen = 1;  *in_bufp = '\r';
            out_bufp = out_buf;  out_buflen = 0;
        }
#if 0
        if (state == _CLOSED) {
            usb_io_init();  // Get ready for another connection
        }
#endif
    }
    old_state = state;
}

// This schedules the 'net_io_test()' function to be run by RedBoot's
// main command loop when idle (i.e. when no input arrives after some
// period of time).
RedBoot_idle(usb_io_test, RedBoot_IDLE_NETIO);

//
// USB initialization
//

// Define USB descriptors
usb_configuration_descriptor usb_configuration = {
    length:             USB_CONFIGURATION_DESCRIPTOR_LENGTH,
    type:               USB_CONFIGURATION_DESCRIPTOR_TYPE,
    total_length_lo:    USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH_LO(1, 2),
    total_length_hi:    USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH_HI(1, 2),
    number_interfaces:  1,
    configuration_id:   1,
    configuration_str:  3,
    attributes:         USB_CONFIGURATION_DESCRIPTOR_ATTR_REQUIRED |
                        USB_CONFIGURATION_DESCRIPTOR_ATTR_SELF_POWERED,
    max_power:          1
};

usb_interface_descriptor usb_interface = {
    length:             USB_INTERFACE_DESCRIPTOR_LENGTH,
    type:               USB_INTERFACE_DESCRIPTOR_TYPE,
    interface_id:       0,
    alternate_setting:  0,
    number_endpoints:   2,
    interface_class:    0x00,
    interface_subclass: 0x00,
    interface_protocol: 0x00,
    interface_str:      0
};

usb_endpoint_descriptor usb_endpoints[] = {
    {
        length:         USB_ENDPOINT_DESCRIPTOR_LENGTH,
        type:           USB_ENDPOINT_DESCRIPTOR_TYPE,
        endpoint:       USB_ENDPOINT_DESCRIPTOR_ENDPOINT_IN | 2,
        attributes:     USB_ENDPOINT_DESCRIPTOR_ATTR_BULK,
        max_packet_lo:  64,
        max_packet_hi:  0,
        interval:       0
    },
    {
        length:         USB_ENDPOINT_DESCRIPTOR_LENGTH,
        type:           USB_ENDPOINT_DESCRIPTOR_TYPE,
        endpoint:       USB_ENDPOINT_DESCRIPTOR_ENDPOINT_OUT | 2,
        attributes:     USB_ENDPOINT_DESCRIPTOR_ATTR_BULK,
        max_packet_lo:  64,
        max_packet_hi:  0,
        interval:       0
    }
};

char language_id_str[] = 
{
    0x04,
    0x03,
    0x09,
    0x04
};

char manufacturer_str[] = 
{
    0x12,
    0x03,
    'i', 0x00,
    'O', 0x00,
    'b', 0x00,
    'j', 0x00,
    'e', 0x00,
    'c', 0x00,
    't', 0x00,
    's', 0x00
};

char product_str[] = 
{
    0x0e,
    0x03,
    'D', 0x00,
    'h', 0x00,
    'a', 0x00,
    'r', 0x00,
    'm', 0x00,
    'a', 0x00
};

char configuration_str[] = 
{
    0x10,
    0x03,
    'R', 0x00,
    'e', 0x00,
    'd', 0x00,
    'B', 0x00,
    'o', 0x00,
    'o', 0x00,
    't', 0x00
};

unsigned char* usb_strings[] = {
    language_id_str,
    manufacturer_str,
    product_str,
    configuration_str
};

usbs_enumeration_data usb_enum_data = {
    device: {
        length:                 USB_DEVICE_DESCRIPTOR_LENGTH,
        type:                   USB_DEVICE_DESCRIPTOR_TYPE,
        usb_spec_lo:            USB_DEVICE_DESCRIPTOR_USB11_LO,
        usb_spec_hi:            USB_DEVICE_DESCRIPTOR_USB11_HI,
        device_class:           USB_DEVICE_DESCRIPTOR_CLASS_INTERFACE,
        device_subclass:        USB_DEVICE_DESCRIPTOR_SUBCLASS_INTERFACE,
        device_protocol:        USB_DEVICE_DESCRIPTOR_PROTOCOL_INTERFACE,
        max_packet_size:        16,
        vendor_lo:              0x47,
        vendor_hi:              0x05,
        product_lo:             0x2b,
        product_hi:             0x10,
        device_lo:              0x00,
        device_hi:              0x01,
        manufacturer_str:       1,
        product_str:            2,
        serial_number_str:      0,
        number_configurations:  1
    },
    configurations:             &usb_configuration,
    total_number_interfaces:    1,
    interfaces:                 &usb_interface,
    total_number_endpoints:     2,
    endpoints:                  usb_endpoints,
    total_number_strings:       4,
    strings:                    usb_strings
};

RedBoot_init(usb_init, RedBoot_INIT_PRIO(10));

void
usb_init(void)
{
    printf("%s\n", __FUNCTION__);
    
    // Set defaults as appropriate
    //have_usb = false;
    usbs_dharma_pdiusbd12_ep0.enumeration_data = &usb_enum_data;
    usbs_dharma_pdiusbd12_ep0.state_change_fn = usb_state_change_fn;
    // Initialize the USB device
    usbs_start(&usbs_dharma_pdiusbd12_ep0);
    // TODO register state change handler, and set this below:
    //have_usb = true;

    //if (have_usb) {
    usb_io_init();
    //}
}
