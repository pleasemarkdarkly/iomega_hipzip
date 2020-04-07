/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pconfig.hpp- PEG configuration flags. 
//
// Author: Kenneth G. Maxwell
//
// Copyright (c) 1997-1998 Swell Software 
//              All Rights Reserved.
//
// Unauthorized redistribution of this source code, in whole or part,
// without the express written permission of Swell Software
// is strictly prohibited.
//
// Notes:
//
// These flags control which features are included in your PEG library.
// When any of these flags are changed, you must rebuilt your working version
// of the library.
//
// These flags also define the target operating system, input devices,
// and to some extent the target video controller.
//
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
// More notes:
/*--------------------------------------------------------------------------*/
/*

  There are a limited number of development environments supported by PEG
  in all distributions. These include:
  
  1) DOS real mode, 640x480x16 color VGA.
  2) DOS real mode, 1024x768x256 color VGA.
  3) Win32 virtual screen driver environment.
  4) X11 virtual screen driver environment.

  Additional environments are provided as part of specific
  RTOS/Compiler/Target packages. Integration packages are included in your
  shipment when PEG is purchased through one of our RTOS partners. Integration
  packages are currently available for following operating systems:
  
  1) SMX          (contact Micro Digital, Inc.)
  2) ETS          (contact PharLap Software, Inc.)
  3) ThreadX      (contact Express Logic, Inc.)
  4) SuperTask! and TronTask! (contact US Software, Inc.)
  5) QNX          (contact Swell Software, Inc.)
  6) Linux        (contact Swell Software, Inc.)
  7) LynxOS       (contact Swell Software, Inc.)
  8) RTXC         (contact Empower, Inc.)
  9) pSOS         (contact Swell Software, Inc.)
 10) Nucleus Plus (contact Swell Software, Inc.)
 11) MQX		  (contact Swell Software, Inc.)
 12) AMX          (contact Kadak Products, Ltd.)
 13) Integrity    (contact Green Hills Software, Inc.)
 14) OSE          (contact Enea OSE Systems)
 15) Embedix      (contact Lineo, Inc.)

  
  There are a few #defines below that must be modified when building
  for DOS, standalone, one of the integrated RTOS kernals, WIN32 or X11.
  These defines must be set properly before building PEG for the desired
  target.
  
  If you are using PEG on a custom OS, you will need to disable all
  of the standard targets, and create a new target specific to your
  environment.
  
  Additional information related to configuring PEG for the development
  environments or for custom targets in contained in the programming and
  reference manual.
  
  Contact Swell Software for assistance configuring PEG for a custom target.

----------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
// to build the library for DOS (or any standalone environment),
// turn on PEGDOS:
/*--------------------------------------------------------------------------*/

#ifdef WIN32
#define PEGWIN32        // turn this on to build library for Win32 app.
#else
#define PEGDOS          // turn this on to run under DOS real mode.
#endif


/*--------------------------------------------------------------------------*/
// to build for SMX, turn off PEGDOS and PEGWIN32, and  
// uncomment the line below:
/*--------------------------------------------------------------------------*/

//#define PEGSMX            // uncomment to build for SMX RTOS.

/*--------------------------------------------------------------------------*/
// to build for ThreadX, turn off all of the above defines, and 
// uncomment the line below:
//
// If you are building for the ThreadX Win32 development environment,
// you should also uncomment PEGWIN32 above.
/*--------------------------------------------------------------------------*/

//#define PEGX              // uncomment to build for ThreadX.


/*--------------------------------------------------------------------------*/
// to build for PharLap, turn off all of the above defines, and 
// uncomment the line below:
/*--------------------------------------------------------------------------*/

//#define PHARLAP            // uncomment to build for PharLap (ETS or TNT)
    

/*--------------------------------------------------------------------------*/
// to build for PharLap TNT realtime DOS extender, turn on PHARLAP (above)
// and also uncomment the line below:
/*--------------------------------------------------------------------------*/

//#define PEGTNT             // uncomment to build for PharLap TNT DOS Extender

/*--------------------------------------------------------------------------*/
// to build for US Software's SuperTask!, turn off all of the above
// and uncomment the line below:
/*--------------------------------------------------------------------------*/

//#define PEGSUPERTASK

/*--------------------------------------------------------------------------*/
// to build for US Software's TronTask!, turn off all of the above
// and uncomment the line below:
/*--------------------------------------------------------------------------*/

//#define PEGTRONTASK

/*--------------------------------------------------------------------------*/
// to build for Embedded Systems Products RTXC, turn off all of the above
// and uncomment the line below:
/*--------------------------------------------------------------------------*/

//#define PEGRTXC

/*--------------------------------------------------------------------------*/
// to build for Enea OSE, turn off all of the above and uncomment the
// line below.
//
// If building for the OSE SoftkKernal, turn on both PEGWIN32 and PEG_OSE
// 
/*--------------------------------------------------------------------------*/

//#define PEG_OSE

/*--------------------------------------------------------------------------*/
// to build for Accelerated Technology Nucleus Plus, turn off all of the
// above and uncomment the line below:
/*--------------------------------------------------------------------------*/

//#define PEGNUCLEUS

/*--------------------------------------------------------------------------*/
// to build for Precise Software's MQX turn off all of the above and 
// uncomment the line below:
//
// if you are building for MTHREADS PSP and Win32x86 BSP
// Win32 development environment, also uncomment PEGWIN32 (above)
/*--------------------------------------------------------------------------*/

//#define PEGMQX

/*--------------------------------------------------------------------------*/
// to build the library for any POSIX operating system (QNX, Linux, LynxOS)
// turn off all of the above and uncomment
// the line below:
/*--------------------------------------------------------------------------*/
//#define POSIXPEG

#ifdef POSIXPEG

// select which POSIX operating system:
#define LINUXPEG      // turn this on to run with Linux
//#define LYNXPEG       // turn this on to run with LynxOS
//#define QNXPEG        // turn this on to run with QNX

#endif

/*--------------------------------------------------------------------------*/
// to build for the X Window System Version 11 Release 6 development
// environment, uncomment the line below.
/*--------------------------------------------------------------------------*/
//#define PEGX11

#ifdef PEGX11

// Select which OS X is running on
#define X11LINUX		// running on top of Linux
//#define X11LYNX		  // running on top of LynxOS

#endif

/*--------------------------------------------------------------------------*/
// to build for the INTEGRITY operating system, turn off all of the above and
// uncomment the line below.
/*--------------------------------------------------------------------------*/

//#define PEGINTEGRITY

/*--------------------------------------------------------------------------*/
// To build the Peg REmote Screen Server (PRESS), select your RTOS above
// and uncomment the define below. This will build the PRESS (an
// executable, not the PEG library itself) for use on your RTOS. Make sure
// that the PEG_BUILD_PRESC is not defined if you are defining
// PEG_BUILD_PRESS.
/*--------------------------------------------------------------------------*/
//#define PEG_BUILD_PRESS

/*--------------------------------------------------------------------------*/
// To build the Peg REmote Screen Client (PRESC), select your RTOS above
// and uncomment the define below. This will build the library for use
// with remote screen client applications. Make sure that PEG_BUILD_PRESS
// is not defined if you are defining PEG_BUILD_PRESC.
/*--------------------------------------------------------------------------*/
//#define PEG_BUILD_PRESC

/*--------------------------------------------------------------------------*/
// If you are building the client/server model, then you need to
// select a protocol for communications between the client and
// the server. Note: Only 1 protocol may be selected.
/*--------------------------------------------------------------------------*/
#if defined(PEG_BUILD_PRESS) || defined(PEG_BUILD_PRESC)

#define PRESS_TCPIP_INET
//#define PRESS_TCPIP_UNIX
//#define PRESS_NAMED_PIPES
//#define PRESS_INTEGRITY_CONNECTIONS

#endif

/*--------------------------------------------------------------------------*/
// Pick your screen driver.  The screen driver header file is included in
// all files, so that certain functions can be provided as macros for 
// improved performance. The screen driver header file must correspond
// with the screen driver source file you compile and link into your system.
/*--------------------------------------------------------------------------*/

#ifdef PEGWIN32

#define PSCREEN  "w32scrn.hpp"     // default for Win32
//#define PSCREEN  "monoscrn.hpp"    // substitute this if using monochrome
//#define PSCREEN  "l2scrn.hpp"      // substitute this if using l2scrn.cpp
//#define PSCREEN  "l4scrn.hpp"      // substitute this if using l4scrn.cpp
//#define PSCREEN  "l8scrn.hpp"      // substitute this if using l8scrn.cpp
//#define PSCREEN  "l16scrn.hpp"     // substitute this if using l16scrn.cpp
//#define PSCREEN  "proscrn1.hpp"    // substitute this if using proscrn1.cpp
//#define PSCREEN  "proscrn2.hpp"    // substitute this if using proscrn2.cpp
//#define PSCREEN  "proscrn4.hpp"    // substitute this if using proscrn4.cpp
//#define PSCREEN  "proscrn8.hpp"    // substitute this if using proscrn8.cpp                                   

#elif defined(PEGX11)

#define PSCREEN	 "x11scrn.hpp"     // default for X11

#else

#include <gui/peg/_screendriver.h>   // a dynamic header file that specifies the screen driver

//#define PSCREEN  "vgascrn.hpp"     // default for everything else
//#define PSCREEN  "svgascrn.hpp"    // substitute this if using svgascrn.cpp
//#define PSCREEN  <gui/peg/screendriver/monoscrn.hpp>    // substitute this if using monochrome 
//#define PSCREEN  "l2scrn.hpp"      // substitute this if using l2scrn.cpp
//#define PSCREEN  <gui/peg/screendriver/l4scrn.hpp>      // substitute this if using l4scrn.cpp
//#define PSCREEN  "l8scrn.hpp"      // substitute this if using l8scrn.cpp
//#define PSCREEN  "proscrn1.hpp"    // substitute this if using proscrn1.cpp
//#define PSCREEN  "proscrn2.hpp"    // substitute this if using proscrn2.cpp
//#define PSCREEN  "proscrn4.hpp"    // substitute this if using proscrn4.cpp
//#define PSCREEN  "proscrn8.hpp"    // substitute this if using proscrn8.cpp
//#define PSCREEN  "cl54xpci.hpp"    // substitute this if using Cirrus 54xx PCI
//#define PSCREEN  "lh77mono.hpp"    // monochrome for Sharp LH77790 ARM
//#define PSCREEN  "lh77gray.hpp"    // gray-scale for Sharp LH77790 ARM
//#define PSCREEN  "7211gray.hpp"    // gray-scale for Cirrus Logic 7211 ARM
//#define PSCREEN  "7211mono.hpp"    // monochrome for Cirrus Logic 7211 ARM
//#define PSCREEN  "1330scrn.hpp"    // SED1330 video controller
//#define PSCREEN  "1353scrn.hpp"    // SED1353 eval card, 16-color/gray
//#define PSCREEN  "1354scrn.hpp"    // SED1354 screen driver
//#define PSCREEN  "1355scrn.hpp"    // SED1355 eval card, 8-bpp all res
//#define PSCREEN  "1356scrn.hpp"    // SED1356 eval card, 8-bpp all res
//#define PSCREEN  "1374scr4.hpp"    // SED1374 eval card, 4-bpp all res
//#define PSCREEN  "1375scr4.hpp"    // SED1375 eval card, 4-bpp all res
//#define PSCREEN  "1375scr8.hpp"    // SED1375 eval card, 8-bpp all res
//#define PSCREEN  "1376scr8.hpp"    // SED1376 eval card, 8-bpp all res
//#define PSCREEN  "1386scrn.hpp"    // SED1386 eval card, 8-bpp all res
//#define PSCREEN  "ct545_4.hpp"     // C&T 65545 16 color/gray 
//#define PSCREEN  "ct545_8.hpp"     // C&T 65545 256 color 
//#define PSCREEN  "ct550_8.hpp"     // C&T 65550 256 color 
//#define PSCREEN  "ct690008.hpp"    // C&T 69000 256 color 
//#define PSCREEN  "8106scrn.hpp"    // SED8106 LCD
//#define PSCREEN  "sc400_1.hpp"     // 1-bpp, sc400 controller
//#define PSCREEN  "sc400_2.hpp"     // 1-bpp, sc400 controller
//#define PSCREEN  "mq200_8.hpp"     // MediaQ 200 CRT/LCD Controller at 8bpp
//#define PSCREEN  "mq200_16.hpp"    // MediaQ 200 CRT/LCD Controller at 65k colors
//#define PSCREEN  "ppc823_8.hpp"    // MPC823 onboard controller at 256 colors
//#define PSCREEN  "dragon4.hpp"     // Dragonball- 16 grayscale levels
//#define PSCREEN  "sascrn8.hpp"     // StrongARM SA1100- 256 colors

#endif


/*--------------------------------------------------------------------------*/
// Some systems must determine the output color depth at run time. Examples
// of this type of system include the PegWindowBuilder environment and
// PharLap ETS. In this is your situation, you should turn
// on PEG_RUNTIME_COLOR_CHECK, and the definition PEG_NUM_COLORS should be
// commented out.
//
// Defining PEG_NUM_COLORS produces slightly smaller and faster code than 
// defining PEG_RUNTIME_COLOR_CHECK. If you know your target system's color
// capability, turn off PEG_RUNTIME_COLOR_CHECK and define PEG_NUM_COLORS.
/*--------------------------------------------------------------------------*/

//#define PEG_RUNTIME_COLOR_CHECK   // leave this undefined for most systems.

/*--------------------------------------------------------------------------*/
// The normal mode is to run with PEG_RUNTIME_COLOR_CHECK disabled. In that
// case, you must define the number of colors the screen driver you are using
// supports here:
/*--------------------------------------------------------------------------*/

#ifndef PEG_RUNTIME_COLOR_CHECK

//#define PEG_NUM_COLORS 16777215L   // for 8-8-8 24-bit true-color mode         
//#define PEG_NUM_COLORS 65535
//#define PEG_NUM_COLORS 256
//#define PEG_NUM_COLORS 16
//#define PEG_NUM_COLORS 4
#define PEG_NUM_COLORS 2


/*--------------------------------------------------------------------------*/
// The following flag is used only when PEG_NUM_COLORS is set to 256. This
// flag switches between 256-color palette mode and 3:3:2 packed pixel format.
/*--------------------------------------------------------------------------*/

#if (PEG_NUM_COLORS == 256)

//#define EIGHT_BIT_PACKED_PIXEL      // Default is off, i.e. palette mode
                                    // Turn on for 3:3:2 packed pixel
#endif

#endif

/*--------------------------------------------------------------------------*/
// The following two flags are only used when PEG_NUM_COLORS is set to be 2.
// When you are running with a monochrome screen driver, you can use the
// flags below to fine-tune how things appear on your monochrome screen.
/*--------------------------------------------------------------------------*/

#define MONO_BUTTONSTYLE_3D         // Simulate 3D button appearance, you can
                                    // turn this off if it is too slow on your
                                    // hardware.
#define MONO_INDICATE_TITLE_FOCUS   // Hatch fill active title, you can turn
                                    // this off if it is too slow on your hardware.

/*--------------------------------------------------------------------------*/
// The following group of definitions are used to enable or disable support
// for different input device types. Just turn on what you are
// using, as this will save a small amount of memory and code space if you
// don't have all input types. MOUSE_SUPPORT and TOUCH_SUPPORT are usually
// exclusive, i.e. you want to turn one or the other on but not both.
/*--------------------------------------------------------------------------*/

//#define PEG_MOUSE_SUPPORT       // mouse or joystick message handling

//#define PEG_TOUCH_SUPPORT       // similar to MOUSE_SUPPORT, but does not
                                // require POINTERMOVE messages to work
                                // correctly and removes mouse pointer bitmap.

#define PEG_KEYBOARD_SUPPORT    // support for PM_KEY message handling

/*
  We default PEG_DRAW_FOCUS to ON when PEG_KEYBOARD_SUPPORT is defined,
  however this setting is completely independant and can be enabled or
  disabled as desired.
*/

#ifdef PEG_KEYBOARD_SUPPORT
//#define PEG_DRAW_FOCUS        // outline control with input focus, optional    
#endif

/*--------------------------------------------------------------------------*/
//
// The following define, FAST_BLIT, instructs the high-level PEG objects to
// use fast bit-blitting to scroll. Turn this on for almost everything. The 
// only time to turn it off is if you are running with a very slow driver,
// such as the generic VGA driver. Even then, FAST_BLIT forces scrolling
// using a RectMove operation, that while slow with generic VGA can improve
// the overall appearance.
//
/*--------------------------------------------------------------------------*/

#define FAST_BLIT           // turn off for generic VGA


/*--------------------------------------------------------------------------*/
// The following define, PEG_FULL_GRAPHICS, brings in additional graphics 
// primitives that are not required internally by PEG. These include Polygon,
// Circle, PatternLine and Arc.
/*--------------------------------------------------------------------------*/

//#define PEG_FULL_GRAPHICS


/*--------------------------------------------------------------------------*/
// The following define, PEG_FP_GRAPHICS, brings in still more graphics
// primitives that are not required internally. The primitives include
// Ellipse and Bezier. These primitives require floating point
// math capability on the target platform, and are not usually included.
/*--------------------------------------------------------------------------*/

//#define PEG_FP_GRAPHICS


/*--------------------------------------------------------------------------*/
// The following define, PEG_CHARTING, brings in the collection of charting
// classes.
/*--------------------------------------------------------------------------*/

//#define PEG_CHARTING      // turn on if you need charting, else turn off 


/*--------------------------------------------------------------------------*/
// The following define, PEG_HMI_GADGETS, brings in the collection of HMI
// controls. This define requires PEG_FULL_GRAPHICS to be turned on.
/*--------------------------------------------------------------------------*/

//#define PEG_HMI_GADGETS 

/*--------------------------------------------------------------------------*/
// The following define will include a PegFileDialog to the library.
// Currently, the PegFileDialog only works on Unix-like operating systems
// to provide portable File Open/Save As operations on different Unix-like
// systems.
/*--------------------------------------------------------------------------*/

#if defined(POSIXPEG) || defined(PEGX11)
#define PEG_FILE_DIALOG
#endif

/*--------------------------------------------------------------------------*/
// The following definitions turn on various run-time image conversion
// facilities. These include run-time decoding of BMP, GIF, JPG, and PNG
// formatted grahics files.
//
// Run-time conversion is NOT used in most applications. Unless you are 
// certain (having read the programming manual) that you need run-time image
// conversion, leave these definitions turned off.
//
// If you turn these definitions on, you should also add the source files
// for each conversion type to your library make file. The supported image
// conversions are:
//
// PEG_BMP_CONVERT: Windows Bitmap, requires pbmpconv.cpp source module
// PEG_GIF_CONVERT: Compuserve GIF, requires pgifconv.cpp source module
// PEG_JPG_CONVERT: JPEG format decoder, requires pjpgconv.cpp source module
// PEG_PNG_DECODER: PNG file decoder, requires ppngconv.cpp source module
// PEG_PNG_ENCODER: PNG file encoder, requires ppngconv.cpp source module
// PEG_QUANT:       Run-time optimal palette producer, requires pquant.cpp
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

//#define PEG_BMP_CONVERT
//#define PEG_GIF_CONVERT
//#define PEG_JPG_CONVERT
//#define PEG_PNG_DECODER
//#define PEG_PNG_ENCODER
//#define PEG_QUANT

/*--------------------------------------------------------------------------*/
// PEG_ZIP/PEG_UNZIP: run-time compress and decompress utilities. These 
// utilities are required for the PNG decoder and encoder (above), but may 
// be pulled in for general use if desired. The compress and decompress
// functions are discrete and you may include only the functionality that
// your application uses.
/*--------------------------------------------------------------------------*/

//#define PEG_ZIP
//#define PEG_UNZIP

/*--------------------------------------------------------------------------*/
//
// The following define, PEG_IMAGE_SCALING, pulls in functions necessary for
// run-time re-sizing of images. This capability is usually NOT required
// for embedded systems, but is provided when needed (for example, when 
// building for JAVA-AWT support this is required).
//
/*--------------------------------------------------------------------------*/

//#define PEG_IMAGE_SCALING

/*--------------------------------------------------------------------------*/
//
// The following define, PEG_BITMAP_WRITER, pulls in functions necessary for
// capturing a portion or all of the screen and writing it out as a Windows
// bitmap file. The user must provide the actual data output function, PEG
// makes a Windows compatible bitmap in memory and returns the address and
// size. 
//
// PEG_BITMAP_WRITER is usually only used for debugging or making screen
// shots for promotional literature, it isn't generally used for a production
// run-time build.
/*--------------------------------------------------------------------------*/

#define PEG_BITMAP_WRITER

/*--------------------------------------------------------------------------*/
//
// The following define, PEG_VECTOR_FONTS, pulls in functions necessary for
// vector (scalable) font support. The default is to use only bitmapped fonts.
//
/*--------------------------------------------------------------------------*/

//#define PEG_VECTOR_FONTS


/*--------------------------------------------------------------------------*/
//
// The following definition, PEG_FULL_CLIPPING, should be turned on in almost
// all cases. If your application needs to have the absolute minimum code
// size, and you are willing to design your application such that objects
// are never off the screen or overlapping, you can turn off this definition
// to save several KBytes of code space.
//
/*--------------------------------------------------------------------------*/

#define PEG_FULL_CLIPPING    // Always leave this on initially. This 
                             // determines the type of window clipping 
                             // algorithm used internally by PEG.


/*--------------------------------------------------------------------------*/
// PEG_MULTITHREAD- This definitions instructs PEG to invoke the resource
// protection and message queue management macros needed to support having
// multiple tasks directly interact with the GUI. This definition is turned
// on by default for the in-house integrations, and turned off otherwise. If
// you have ported to a new RTOS and you support the resource protection
// macros, you can turn this on if you want to support the multithread model.
/*--------------------------------------------------------------------------*/
#if defined(PHARLAP) || defined(PEGX) || defined(PEGSMX)

#define PEG_MULTITHREAD      // This is used when building PEG with one
                             // of the multitasking RTOS kernals. Refer
                             // to the user's manual for additional
                             // information. This define has no effect unless
                             // you are running PEG with an RTOS that has
                             // been fully integrated, and should be disabled
                             // for stand-alone operation.

#elif defined(PEGRTXC) || defined(PEGSUPERTASK) || defined(PEGTRONTASK)

#define PEG_MULTITHREAD

#elif defined(PEG_OSE) || defined(PEGNUCLEUS) || defined(PEGMQX)

#define PEG_MULTITHREAD

#elif defined(PEGX11) || defined(LINUXPEG) || defined(LYNXPEG)

#define PEG_MULTITHREAD

#else
//#define PEG_MULTITHREAD    // usually not defined unless custom integration
#endif


/*--------------------------------------------------------------------------*/
// The following definition, PEG_AWT_SUPPORT, pulls in various functions
// required for supporting the JAVA AWT model. This definition also disables
// internal invalid region checking in the PegScreen drivers. This definition
// should only be turned on when you are building PEG specifically to support
// the JAVA AWT.
/*--------------------------------------------------------------------------*/

//#define PEG_AWT_SUPPORT


/*--------------------------------------------------------------------------*/
// The following definition, PEG_UNICODE, turns on 16-bit character encoding.
// If you choose to define PEG_UNICODE, you might also need to turn on
// PEG_STRLIB, which pulls in 16-bit versions of the  common string functions.
/*--------------------------------------------------------------------------*/

#define PEG_UNICODE         // use 16-bit character encoding


/*--------------------------------------------------------------------------*/
// PEG_SJIS_CONVERSION- Include table and functions for converting between
// Japanese Shift-JIS and Unicode character encoding?
/*--------------------------------------------------------------------------*/

//#define PEG_SJIS_CONVERSION // include Shift-JIS conversion functions


/*--------------------------------------------------------------------------*/
// The following definition, PEG_STRLIB, pulls in the PEG string library
// functions. This can be used if your compiler run-time library causes
// problems. For example, if you building a 32-bit standalone application
// using the MS 32-bit run-time library, several WINAPI functions are
// referenced by that library. Some platforms, including SMX and ETS, take
// care of this problem. If you are building standalone, you have the option
// of using your compiler run-time library or using the PEG implementation
// of the common string functions. If you are building for PEG_UNICODE, 
// the PEG versions support both 8 and 16-bit character encoding. Many run-time
// implementations support only 8-bit encoding, in which case you want to
// use the PEG replacements if you are using Unicode.
/*--------------------------------------------------------------------------*/

#ifdef PEG_UNICODE
#define PEG_STRLIB          // default to PEG string functions if PEG_UNICODE
#else
//#define PEG_STRLIB        // otherwise default to compiler run-time library
#endif


/*--------------------------------------------------------------------------*/
// The following definition, USE_PEG_LTOA, instructs PEG to provide the
// non-ANSI function _ltoa. This function is used by PegSpinButton and the
// demo applications instead of sprintf. If your compiler library does not
// provide the function _ltoa, turn on the define below and add the file
// \peg\source\ltoa.cpp to your PEG library make file.
/*--------------------------------------------------------------------------*/

#ifdef PEG_UNICODE
#define USE_PEG_LTOA        // default to PEG version if PEG_UNICODE
#else
#define USE_PEG_LTOA      // otherwise, default to compiler version
#endif

/*--------------------------------------------------------------------------*/
// The definition ONE_SECOND must be the number of times the
// PegMessageQueue::TimerTick function is called in one second. If your 
// hi-level PegTimers are not running at the right speed, it means that the
// TimerTick function is being called faster or slower than the define below.
/*--------------------------------------------------------------------------*/
#define ONE_SECOND   18     // How many PegMessageQueue::TimerTicks in one second?


/*--------------------------------------------------------------------------*/
// The definition PEGFAR should be 'far' if building for x86 real mode.
// FOR ALL OTHER TARGETS PEGFAR be defined as nothing, null, nada.
/*--------------------------------------------------------------------------*/

#if defined(PEGWIN32)

#define PEGFAR

#else

//#define PEGFAR far          // use this for x86 real mode targets
#define PEGFAR           // use this for everything else

#endif


/*--------------------------------------------------------------------------*/
// Many of the provided drivers are designed for use on the x86 series.  
// These map our PINB/POUTB names to whatever the compiler designed dreamed
// up.  You may need to enhance this list to support other versions of
// these compilers and/or add another compiler.  In some instances, an embedded
// system will map a 'normal' peripheral into memory and it can be accessed
// by some creative macro definitions.
/*--------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#define POUTB(a, b) _outp(a, b)
#define POUTW(a, b) _outpw(a, b)
#define PINB(a) _inp(a)

#elif defined(__BORLANDC__) || defined(_WATCOMC_)
#define POUTB(a, b) outp(a, b)
#define POUTW(a, b) outpw(a, b)
#define PINB(a) inp(a)

#elif defined(CADUL)
#define POUTB(a, b) outbyte(a, b)
#define POUTW(a, b) outhword(a, b)
#define PINB(a) inbyte(a)

#elif defined(__HIGHC__)
#define POUTB(a, b) _outb(a, b)
#define POUTW(a, b) _outpw(a, b)
#define PINB(a) _inb(a)

#elif defined(LYNXPEG)
extern "C" {
void __outb(unsigned short, unsigned char);
unsigned char __inb(unsigned short);
}
#define POUTB(port, b) __outb(port, b)
#define PINB(port) __inb(port)

#elif defined(LINUXPEG)
#include <unistd.h>
#include <sys/io.h>
#define POUTB(port, a) outb((unsigned char) a, port)
#define PINB(port) inb(port)

#else

// PUT YOUR COMPILER DEFINITION HERE
#define POUTB(a, b) outp(a, b)
#define PINB(a) inp(a)       

#endif


/*--------------------------------------------------------------------------*/
// End of configuration settings.
/*--------------------------------------------------------------------------*/

