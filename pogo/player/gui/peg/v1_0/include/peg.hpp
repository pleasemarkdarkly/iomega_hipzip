/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// peg.hpp- global include file, required by all modules using PEG.
//
// Author: Kenneth G. Maxwell
//
// Copyright (c) 1997-2000 Swell Software 
//              All Rights Reserved.
//
// Unauthorized redistribution of this source code, in whole or part,
// without the express written permission of Swell Software
// is strictly prohibited.
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------

Notes:

  ********************************************************************
  ***                                                              ***
  *** If you are looking for the PEG library configuration flags,  ***  
  *** they have been moved to the new header file                  ***
  *** \peg\include\pconfig.hpp                                     ***
  ***                                                              ***
  ********************************************************************

  This is the only file that needs to be included by the application level
  software in order to use PEG.

----------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef _PEGDEFS_
#define _PEGDEFS_

#include <gui/peg/pconfig.hpp>      // Pull in user configuration settings

/*--------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------

    You should NOT have to make any changes below to build for your 
    target.

----------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

// Some options are dependant, make sure the right options are turned on
// to support dependant options:

#ifdef PEG_HMI_GADGETS
#ifndef PEG_FULL_GRAPHICS
#define PEG_FULL_GRAPHICS
#endif
#endif


#ifdef PEG_AWT_SUPPORT
#ifndef PEG_FULL_GRAPHICS
#define PEG_FULL_GRAPHICS
#endif
#ifndef PEG_FP_GRAPHICS
#define PEG_FP_GRAPHICS
#endif
#ifndef PEG_IMAGE_CONVERT
#define PEG_IMAGE_CONVERT
#endif
#ifndef PEG_IMAGE_SCALING
#define PEG_IMAGE_SCALING
#endif
#endif


#if defined(PEGSMX) && defined(WIN32)   // true for SMX WinBase version
#define PEGWIN32
#endif


/*--------------------------------------------------------------------------*/
// the following disables the warning caused with the Borland compiler
// running in 32-bit mode with PEG's use of 16-bit variables.
/*--------------------------------------------------------------------------*/

#if defined(__BORLANDC__) && defined(__WIN32__)  // (if BC 32-bit compiler)
#pragma warn -sig   // Disables warning "Conversion may lose significant digits"
                    // Occurs often due to use of 16-bit variables in calculations.
#endif

/*--------------------------------------------------------------------------*/
// The following determines the name of the ltoa (or _ltoa??) function
//
// Note that the function _ltoa is provided internally by PEG if the
// definition USE_PEG_LTOA is enabled
/*--------------------------------------------------------------------------*/
#if defined(__BORLANDC__) && !defined(USE_PEG_LTOA)
#define _ltoa(a, b, c)  ltoa(a, b, c)
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// Unique includes and definitions pulled in for each target platform.
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
// Begin system includes for target builds
/*--------------------------------------------------------------------------*/
#if defined(PEGSMX)   // building for SMX?

#if !defined(SMX_H_)
#include "smx.h"
#endif

#if defined(PEGWIN32) // also building for WinBase?

// For WinBase, we want just about everything turned on. These lines
// override the settings in pconfig.hpp, so that building the WinBase
// version of the library is automatic.

#include "windows.h"

#undef  PSCREEN
#define PSCREEN "w32scrn.hpp"  // Win32 screen driver
#define PEG_FULL_GRAPHICS
#undef  PEG_RUNTIME_COLOR_CHECK
#undef  PEG_NUM_COLORS
#define PEG_NUM_COLORS   256
#define PEG_FULL_GRAPHICS
#ifdef  PEG_FP_GRAPHICS
#undef  PEG_FP_GRAPHICS
#endif
#define PEG_IMAGE_CONVERT
#define PEG_VECTOR_FONTS
#define PEG_MULTITHREAD

#endif

#elif defined(PEGX)          // building for ThreadX?

#ifndef TX_API
#include "tx_api.h"
#endif

#define PEG_PRIORITY		28
#define PEG_TIME_SLICE		10
#define PEG_STACK_SIZE    8000

#elif defined(PHARLAP)

#ifndef _WINDOWS_
#include "windows.h"
#endif

#if defined(PEGTNT)
#include "tntapi.h"
#endif

#elif defined(PEGSUPERTASK)

#include <stddef.h>                // fixes errno and size_t problems
extern "C" {
#include "rtoshdrs.h"              // include SuperTask! header files
}

#elif defined(PEGTRONTASK)
#include <stddef.h>                // fixes errno and size_t problems
extern "C" {
#include "tt.h"                    // include TronTask! header files
}

#elif defined(PEGRTXC)

extern "C" {
#include "rtxcapi.h"
}

#elif defined(PEG_OSE)

#if defined(PEGWIN32)				// building for OSE Softkernal?
#ifndef _WINDOWS_
#include "windows.h"
#endif
#endif

#include "ose.h"

#elif defined(PEGNUCLEUS)           // building for Nucleus Plus?

#include "nucleus.h"                // include Nucleus Plus header files.

#if defined(PEGWIN32)               // also building for NT prototype system?
#ifndef _WINDOWS_
#include "windows.h"
#endif
#endif

#elif defined(PEGMQX)				// building for Precise/MQX

#include "mqx.h"
#include "mutex.h"
#include "message.h"
#include "timer.h"

#if defined(PEGWIN32)				// building for Precise/MQX Win32x86
#ifndef _WINDOWS_
#include "windows.h"
#endif
#endif

#elif defined(PEGWIN32)             // building for Win32 standalone

#ifndef _WINDOWS_
#include "windows.h"
#endif

#elif defined(PEGX11)			    // building for X11

#define _REENTRANT
#include <pthread.h>
#include <X11/Xlib.h>

#ifdef PEG_BUILD_PRESS
#include <sys/types.h>              // for fd_set types
#endif

#elif defined(LINUXPEG)             // building for Linux

#define _REENTRANT
#include <pthread.h>
#include <sys/io.h>

#ifdef PEG_BUILD_PRESS
#include <sys/types.h>              // for fd_set types
#endif

#elif defined(LYNXPEG)              // building for LynxOS

#define _REENTRANT
#include <pthread.h>
#include <mqueue.h>

#elif defined(PEGINTEGRITY)         // Building for GHS INTEGRITY

#include <INTEGRITY.h>

#endif
/*--------------------------------------------------------------------------*/
// End system includes for target builds
/*--------------------------------------------------------------------------*/

#define NUM_PEG_FREE_MSGS   40     // how many messages are available

/*--------------------------------------------------------------------------*/
// Begin PEG integration includes for target builds
/*--------------------------------------------------------------------------*/
#if defined(PEGSMX)                // building for SMX?

#if defined(PEGWIN32)              // building for WinBase?
#include "smxpegwb.hpp"
#elif defined(PEG_MULTITHREAD)
#include "smxpegmt.hpp"            // SMX MULTITHREAD header
#else
#include "smxpeg.hpp"              // SMX GRAPHICS_SERVER header
#endif

#elif defined(PEGX)                // building for ThreadX?

#if defined(PEGWIN32)
#include "thxpegw.hpp"			   // building for the Win32 development platform
#else
#include "thxpeg.hpp"              // bring in ThreadX integration header
#endif

#elif defined(PHARLAP)             // building for PharLap ETS?

#if defined(PEGTNT)
#include "tntapi.h"                // bring in the TNT integration header
#include "tntpeg.hpp"
#else
#include "etspeg.hpp"              // bring in ETS integration header
#endif

#elif defined(PEGSUPERTASK) || defined(PEGTRONTASK)
#define PEG_PRIORITY 100           // default to 'mid priority'
#include "superpeg.hpp"            // include integration header file

#elif defined(PEGQNX)              // building for QNX?

#include "qnxpeg.hpp"              // bring in QNX integration header

#elif defined(LYNXPEG)              // building for LynxOS
#include "lynxpeg.hpp"

#elif defined(LINUXPEG)				// building for Linux
#include "linuxpeg.hpp"

#elif defined(PEGRTXC)             // building for RTXC??

#include "rtxcpeg.hpp"             // bring in RTXC integration header

#elif defined(PEG_OSE)

#include "osepeg.hpp"              // bring in OSE integration header

#elif defined(PEGNUCLEUS)          // building for Nucleus Plus?

#if defined(PEGWIN32)
#include "nucpegnt.hpp"            // building for NT prototype system?
#else
#include "nuclpeg.hpp"              // default: building for embedded target
#endif

#elif defined(PEGMQX)			   // building for Precise/MQX

#if defined(PEGWIN32)
#include "mqxpegmt.hpp"			   // building for Win32x86 BSP
#else
#include "mqxpeg.hpp"			   // building for target
#endif

#elif defined(PEGX11)			   // building for X11

#include "x11peg.hpp"

#elif defined(PEGINTEGRITY)         // Building for GHS INTEGRITY

#include "intgypeg.hpp"

#else                              // building standalone...

#define LOCK_PEG        
#define UNLOCK_PEG
#define LOCK_MESSAGE_QUEUE
#define UNLOCK_MESSAGE_QUEUE
#define LOCK_TIMER_LIST 
#define UNLOCK_TIMER_LIST
#define ENQUEUE_TASK_MESSAGE(a, b)

#endif
/*--------------------------------------------------------------------------*/
// End PEG integration includes for target builds
/*--------------------------------------------------------------------------*/

#include "pegtypes.hpp"

#include "pegkeys.hpp"
#include "pfonts.hpp"
#include "pmessage.hpp"
#include "pscreen.hpp"

#ifndef PEG_BUILD_PRESC
#include PSCREEN                    // must be chosen above
#endif

#include <gui/peg/pthing.hpp>
#include <gui/peg/picon.hpp>
//#include <gui/peg/pmenu.hpp>
//#include <gui/peg/pbutton.hpp>
//#include <gui/peg/pmltbtn.hpp>
//#include <gui/peg/pdecbtn.hpp>
//#include <gui/peg/ptoolbar.hpp>
//#include <gui/peg/pgroup.hpp>
//#include <gui/peg/pslider.hpp>
//#include <gui/peg/pscroll.hpp>
//#include <gui/peg/pspin.hpp>
//#include <gui/peg/pprogbar.hpp>
//#include <gui/peg/ptitle.hpp>
//#include <gui/peg/pprompt.hpp>
//#include <gui/peg/pvprompt.hpp>
//#include <gui/peg/pstatbar.hpp>
#include <gui/peg/pwindow.hpp>
//#include <gui/peg/ptable.hpp>
//#include <gui/peg/pspread.hpp>
//#include <gui/peg/panimwin.hpp>
//#include <gui/peg/pnotebk.hpp>
//#include <gui/peg/ptree.hpp>
#include <gui/peg/ppresent.hpp>
#include <gui/peg/pstring.hpp>
//#include <gui/peg/plist.hpp>
//#include <gui/peg/pcombo.hpp>
#include <gui/peg/ptextbox.hpp>
//#include <gui/peg/peditbox.hpp>
#include <gui/peg/pdecwin.hpp>
//#include <gui/peg/pdialog.hpp>
//#include <gui/peg/pmesgwin.hpp>
//#include <gui/peg/pprogwin.hpp>
//#include <gui/peg/ptermwin.hpp>
//#include <gui/peg/p2dpoly.hpp>

#ifdef PEG_CHARTING

//#include "pchart.hpp"
//#include "plnchart.hpp"
//#include "pmlchart.hpp"
//#include "pstchart.hpp"

#endif

#ifdef PEG_HMI_GADGETS

//#include "psincos.hpp"
//#include "pdial.hpp"
//#include "pfdial.hpp"
//#include "pcdial.hpp"
//#include "pfbdial.hpp"
//#include "pcbdial.hpp"
//#include "plight.hpp"
//#include "pclight.hpp"
//#include "pblight.hpp"
//#include "pscale.hpp"
//#include "plscale.hpp"
//#include "plbscale.hpp"

#endif

#ifdef PEG_FILE_DIALOG
#include "pfdialog.hpp"
#endif

/*--------------------------------------------------------------------------*/
// Check for needing PEG_ZIP and/or PEG_UNZIP for PNG encoding or decoding
/*--------------------------------------------------------------------------*/
#if !defined(PEG_ZIP)
#if defined(PEG_PNG_ENCODER)
#define PEG_ZIP
#endif
#endif

#if !defined(PEG_UNZIP)
#if defined(PEG_PNG_DECODER)
#define PEG_UNZIP
#endif
#endif

#if defined(PEG_ZIP) || defined(PEG_UNZIP)
#include "pzip.hpp"
#endif

#if defined(PEG_BMP_CONVERT) || defined(PEG_GIF_CONVERT) || defined(PEG_JPG_CONVERT)
#define PEG_IMAGE_CONVERT
#elif defined(PEG_PNG_DECODER) || defined(PEG_PNG_ENCODER)
#define PEG_IMAGE_CONVERT
#endif

#if defined(PEG_IMAGE_CONVERT)
#if defined(PEG_QUANT)
#include "pquant.hpp"
#endif
#include "pimgconv.hpp"

#if defined(PEG_GIF_CONVERT)
#include "pgifconv.hpp"
#endif

#if defined(PEG_BMP_CONVERT)
#include "pbmpconv.hpp"
#endif

#if defined(PEG_JPG_CONVERT)
#include "pjpgconv.hpp"
#endif

#if defined(PEG_PNG_ENCODER) || defined(PEG_PNG_DECODER)
#include "ppngconv.hpp"
#endif

#endif      // PEG_IMAGE_CONVERT if

/*--------------------------------------------------------------------------*/
// Building the Peg REmote Screen Server
/*--------------------------------------------------------------------------*/
#ifdef PEG_BUILD_PRESS

#include "pressdef.hpp"
#include "pressmsg.hpp"
#include "press.hpp"

#endif

/*--------------------------------------------------------------------------*/
// Building a Peg REmote Screen Client
/*--------------------------------------------------------------------------*/
#ifdef PEG_BUILD_PRESC

#include "pressdef.hpp"
#include "pressmsg.hpp"
#include "prescpm.hpp"
#include "prescscr.hpp"

#endif

#if defined(PHARLAP)

#if defined(PEGTNT)
PegPresentationManager *TntGUIPresentation(void);
#else
PegPresentationManager *EtsGUIPresentation(void);
#endif


#endif

/*--------------------------------------------------------------------------*/


#endif

