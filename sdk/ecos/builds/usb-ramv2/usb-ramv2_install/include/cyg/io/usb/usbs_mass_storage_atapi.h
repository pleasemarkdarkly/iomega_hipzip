#ifndef CYGONCE_USBS_MASS_STORAGE_ATAPI_H_
#define  CYGONCE_USBS_MASS_STORAGE_ATAPI_H_
//==========================================================================
//
//      include/usbs_mass_storage_atapi.h
//
//      Description of the USB slave-side mass storage atapi support
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

#include <cyg/io/usb/usbs_mass_storage.h>

// The idea here is that the protocols can be plugged in via these interface functions.
void usbs_ms_protocol_init(usbs_mass_storage* ms);
int usbs_ms_protocol_check_cbw(cbw_t * cbw);
int usbs_ms_protocol_do_command(usbs_mass_storage * ms, int * bytes_processed);
void usbs_ms_protocol_reset_bus(void);
int usbs_ms_protocol_get_max_lun(void);
    
#ifdef __cplusplus
}; // extern "C"
#endif
    
#endif // CYGONCE_USBS_MASS_STORAGE_ATAPI_H_
