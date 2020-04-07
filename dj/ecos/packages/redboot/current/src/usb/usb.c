//==========================================================================
//
//      usb.c
//
//      Stand-alone USB support for RedBoot
//
//==========================================================================
//####COPYRIGHTBEGIN####
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    toddm
// Contributors: toddm
// Date:         2001-04-16
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

// TODO This if for Philips only
#include <cyg/io/usb/usbs_pdiusbd12.h>
usbs_control_endpoint * ep = &usbs_dharma_pdiusbd12_ep0;
usbs_rx_endpoint * rx_endpoint = &usbs_dharma_pdiusbd12_ep4;
usbs_tx_endpoint * tx_endpoint = &usbs_dharma_pdiusbd12_ep5;

void
__usb_poll(void)//usbs_control_endpoint *ep)
{
    ep->poll_fn(ep);
}

static int rx_result;
static bool rx_done = false;

void
__usb_rx_completion_fn(void *data, int result)
{
    rx_result = result;
    rx_done = true;
}

int
__usb_read_nonblock(/*usbs_rx_endpoint *rx_endpoint,*/ char *buf, int len)
{
    if (!rx_done) {
	usbs_start_rx_buffer(rx_endpoint, buf, len, __usb_rx_completion_fn, 0);
    }
    
    if (rx_done) {
	rx_done = false;
	return rx_result;
    }
    else {
	return 0;
    }
}

int
__usb_read(/*usbs_rx_endpoint *rx_endpoint,*/ char *buf, int len)
{
    usbs_start_rx_buffer(rx_endpoint, buf, len, __usb_rx_completion_fn, 0);
    while (!rx_done)
	__usb_poll();
    rx_done = false;
    return rx_result;
}

static int tx_result;
static bool tx_done = false;

void
__usb_tx_completion_fn(void *data, int result)
{
    tx_result = result;
    tx_done = true;
}

int
__usb_write(/*usbs_tx_endpoint *tx_endpoint,*/ char *buf, int len)
{
    usbs_start_tx_buffer(tx_endpoint, buf, len, __usb_tx_completion_fn, 0);
    while (!tx_done)
	__usb_poll();
    tx_done = false;
    return tx_result;
}


