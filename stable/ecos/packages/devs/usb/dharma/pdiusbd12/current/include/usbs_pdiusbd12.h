#ifndef CYGONCE_USBS_DHARMA_PDIUSBD12_H
# define CYGONCE_USBS_DHARMA_PDIUSBD12_H
//==========================================================================
//
//      include/usbs_dharma_pdiusbd12.h
//
//      The interface exported by the Dharma PDIUSBD12 USB device driver
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
// Purpose:
//
//####DESCRIPTIONEND####
//==========================================================================

#include <cyg/io/usb/usbs.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The Dharma PDIUSBD12 family comes with on-chip USB slave support. This
 * provides three endpoints. Endpoint 0 can only be used for control
 * messages. Endpoints 4 and 5 can only be used for bulk transfers,
 * host->slave for endpoint 4 and slave->host for endpoint 5.
 */
extern usbs_control_endpoint    usbs_dharma_pdiusbd12_ep0;
extern usbs_rx_endpoint         usbs_dharma_pdiusbd12_ep4;
extern usbs_tx_endpoint         usbs_dharma_pdiusbd12_ep5;
    
#ifdef __cplusplus
} /* extern "C" { */
#endif


#endif /* CYGONCE_USBS_DHARMA_PDIUSBD12_H */
