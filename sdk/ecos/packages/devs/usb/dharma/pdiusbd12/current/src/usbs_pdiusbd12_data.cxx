//==========================================================================
//
//      usbs_dharma_pdiusbd12.c
//
//      Static data for the Dharma PDIUSBD12 USB device driver
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
// This file contains various objects that should go into extras.o
// rather than libtarget.a, e.g. devtab entries that would normally
// be eliminated by the selective linking.
//
//####DESCRIPTIONEND####
//==========================================================================

#include <pkgconf/devs_usb_dharma_pdiusbd12.h>
#if defined(CYGVAR_DEVS_USB_DHARMA_PDIUSBD12_EP0_DEVTAB_ENTRY) || \
      defined(CYGVAR_DEVS_USB_DHARMA_PDIUSBD12_EP4_DEVTAB_ENTRY) || \
      defined(CYGVAR_DEVS_USB_DHARMA_PDIUSBD12_EP5_DEVTAB_ENTRY)
#include <cyg/io/devtab.h>
#endif
#include <cyg/io/usb/usbs_pdiusbd12.h>

// ----------------------------------------------------------------------------
// Initialization. The goal here is to call usbs_dharma_pdiusbd12_init()
// early on during system startup, to take care of things like
// registering interrupt handlers etc. which are best done
// during system init.
//
// If the endpoint 0 devtab entry is available then its init()
// function can be used to take care of this. However the devtab
// entries are optional so an alternative mechanism must be
// provided. Unfortunately although it is possible to give
// a C function the constructor attribute, it cannot be given
// an initpri attribute. Instead it is necessary to define a
// dummy C++ class.

extern "C" void usbs_dharma_pdiusbd12_init(void);

#ifndef CYGVAR_DEVS_USB_DHARMA_PDIUSBD12_EP0_DEVTAB_ENTRY
class usbs_dharma_pdiusbd12_initialization {
  public:
    usbs_dharma_pdiusbd12_initialization() {
        usbs_dharma_pdiusbd12_init();
    }
};

static usbs_dharma_pdiusbd12_initialization usbs_dharma_pdiusbd12_init_object CYGBLD_ATTRIB_INIT_PRI(CYG_INIT_IO);
#endif

// ----------------------------------------------------------------------------
// The devtab entries. Each of these is optional, many applications
// will want to use the lower-level API rather than go via
// open/read/write/ioctl.

#ifdef CYGVAR_DEVS_USB_DHARMA_PDIUSBD12_EP0_DEVTAB_ENTRY

// For endpoint 0 the only legal operations are get_config() and
// set_config(), and these are provided by the common package.

static bool
usbs_dharma_pdiusbd12_devtab_ep0_init(struct cyg_devtab_entry* tab)
{
    CYG_UNUSED_PARAM(struct cyg_devtab_entry*, tab);
    usbs_dharma_pdiusbd12_init();
    return true;
}

static CHAR_DEVIO_TABLE(usbs_dharma_pdiusbd12_ep0_devtab_functions,
                        &cyg_devio_cwrite,
                        &cyg_devio_cread,
                        &cyg_devio_select,
                        &usbs_devtab_get_config,
                        &usbs_devtab_set_config);

static CHAR_DEVTAB_ENTRY(usbs_dharma_pdiusbd12_ep0_devtab_entry,
                         CYGDAT_DEVS_USB_DHARMA_PDIUSBD12_DEVTAB_BASENAME "0c",
                         0,
                         &usbs_dharma_pdiusbd12_ep0_devtab_functions,
                         &usbs_dharma_pdiusbd12_devtab_ep0_init,
                         0,
                         (void*) &usbs_dharma_pdiusbd12_ep0);
#endif

// ----------------------------------------------------------------------------
// Common routines for ep1 and ep2.
#if defined(CYGVAR_DEVS_USB_DHARMA_PDIUSBD12_EP4_DEVTAB_ENTRY) || defined(CYGVAR_DEVS_USB_DHARMA_PDIUSBD12_EP5_DEVTAB_ENTRY)
static bool
usbs_dharma_pdiusbd12_devtab_dummy_init(struct cyg_devtab_entry* tab)
{
    CYG_UNUSED_PARAM(struct cyg_devtab_entry*, tab);
    return true;
}
#endif

// ----------------------------------------------------------------------------
// ep4 devtab entry. This can only be used for host->slave, so only the
// cread() function makes sense.

#ifdef CYGVAR_DEVS_USB_DHARMA_PDIUSBD12_EP4_DEVTAB_ENTRY

static CHAR_DEVIO_TABLE(usbs_dharma_pdiusbd12_ep4_devtab_functions,
                        &cyg_devio_cwrite,
                        &usbs_devtab_cread,
                        &cyg_devio_select,
                        &usbs_devtab_get_config,
                        &usbs_devtab_set_config);

static CHAR_DEVTAB_ENTRY(usbs_dharma_pdiusbd12_ep4_devtab_entry,
                         CYGDAT_DEVS_USB_DHARMA_PDIUSBD12_DEVTAB_BASENAME "4r",
                         0,
                         &usbs_dharma_pdiusbd12_ep4_devtab_functions,
                         &usbs_dharma_pdiusbd12_devtab_dummy_init,
                         0,
                         (void*) &usbs_dharma_pdiusbd12_ep4);
#endif

// ----------------------------------------------------------------------------
// ep5 devtab entry. This can only be used for slave->host, so only
// the cwrite() function makes sense.

#ifdef CYGVAR_DEVS_USB_DHARMA_PDIUSBD12_EP5_DEVTAB_ENTRY

static CHAR_DEVIO_TABLE(usbs_dharma_pdiusbd12_ep5_devtab_functions,
                        &usbs_devtab_cwrite,
                        &cyg_devio_cread,
                        &cyg_devio_select,
                        &usbs_devtab_get_config,
                        &usbs_devtab_set_config);

static DEVTAB_ENTRY(usbs_dharma_pdiusbd12_ep5_devtab_entry,
                    CYGDAT_DEVS_USB_DHARMA_PDIUSBD12_DEVTAB_BASENAME "5w",
                    0,
                    &usbs_dharma_pdiusbd12_ep5_devtab_functions,
                    &usbs_dharma_pdiusbd12_devtab_dummy_init,
                    0,
                    (void*) &usbs_dharma_pdiusbd12_ep5);

#endif

