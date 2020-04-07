/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pegtypes.hpp - Basic data types used by the PEG library.
//
// Author: Kenneth G. Maxwell
//
// Copyright (c) 1997-2000 Swell Software 
//              All Rights Reserved.
//
// Unauthorized redistribution of this source code, in whole or part,
// without the express written permission of Swell Software
// is strictly prohibited.
//
// Notes:
//
// This file contains object types, style flags, and signals used by the
// entire library. Miscelleneous other definitions and macros are also
// contained withing this file.
//
// The basic data types used by PEG are also contained in this file. These 
// include PegPoint, PegRect, PegColor, PegBitmap, PegCapture, etc...
// 
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef _PEGTYPES_
#define _PEGTYPES_

#include <util/tchar/tchar.h>
/*--------------------------------------------------------------------------*/
// Fundamental data types
//
// The data types used by PEG are:
//
//  TCHAR   usually signed character
//  UCHAR     usually unsigned character
//  WORD      usually unsigned 16-bit int
//  SIGNED    usually signed 16-bit int
//  LONG      usually signed 32-bit
//  DWORD     usually unsigned 32-bit
//  COLORVAL  usually 8-bit unsigned, depends on color depth
//
//  The remaining PEG types are defined using these values.
//
//  Since many environments define some or all of the same data types,
//  we only define our basic data types when required and use the environment
//  definitions when available.
/*--------------------------------------------------------------------------*/


#if defined(PEGWIN32)

// all PEG data types defined by windows.h

#else
    
#define BOOL int                // best case for target

#ifndef NULL
#define NULL 0
#endif

#if !defined(PEGSMX)            // smx defines these as members of BOOLEAN enum
#define TRUE 1
#define FALSE 0
#endif

typedef unsigned char UCHAR;    // 8 bit unsigned
typedef unsigned short WORD;    // 16 bit unsigned

#ifndef TX_PORT
typedef long LONG;              // 32 bit signed
typedef unsigned long DWORD;    // 32 bit unsigned
#else
typedef ULONG DWORD;
#endif

#endif                          // if not Win32 if

typedef short int SIGNED;       // 16 bit signed

/*
#ifdef PEG_UNICODE
typedef unsigned short TCHAR; // 16 bit unsigned for PEG_UNICODE
#else
typedef char TCHAR;          // 8 bit unsigned, no PEG_UNICODE
#endif
*/

#ifdef PEG_STRLIB
TCHAR *PegStrCat(TCHAR *s1, const TCHAR *s2);
TCHAR *PegStrnCat(TCHAR *s1, const TCHAR *s2, int iMax);
TCHAR *PegStrCpy(TCHAR *s1, const TCHAR *s2);
TCHAR *PegStrnCpy(TCHAR *s1, const TCHAR *s2, int iMax);
int      PegStrCmp(const TCHAR *s1, const TCHAR *s2);
int      PegStrnCmp(const TCHAR *s1, const TCHAR *s2, int iMax);
int      PegStrLen(const TCHAR *s1);
LONG     PegAtoL(const TCHAR *s1);
int      PegAtoI(const TCHAR *s1);

#ifdef PEG_UNICODE

int      PegStrCmp(const char *s1, const char *s2);
int      PegStrLen(const char *s1);
//int      PegAtoI(const char *s1);
//long     PegAtoL(const char *s1);
TCHAR *PegStrCpy(char *s1, const TCHAR *s2);
TCHAR *PegStrCpy(TCHAR *s1, const char *s2);
char    *PegStrCpy(char *s1, const char *s2);
char    *PegStrCat(char *s1, const char *s2);
char    *PegStrCat(char *s1, const TCHAR *s2);
TCHAR *PegStrCat(TCHAR *s1, const char *s2);
char    *PegStrnCat(char *s1, const TCHAR *s2, int iMax);
void     UnicodeToAscii(char *s1, const TCHAR *s2);
void     AsciiToUnicode(TCHAR *s1, const char *s2);
#endif

/*
#define strcat(a, b) PegStrCat(a, b)
#define strncat(a, b, c) PegStrnCat(a, b, c)
#define strcpy(a, b) PegStrCpy(a, b)
#define strncpy(a, b, c) PegStrnCpy(a, b, c)
#define strcmp(a, b) PegStrCmp(a, b)
#define strncmp(a, b, c) PegStrnCmp(a, b, c)
#define strlen(a) PegStrLen(a)
#define atol(a) PegAtoL(a)
#define atoi(a) PegAtoI(a)
*/

#endif  // PEG_STRLIB

/*--------------------------------------------------------------------------*/
// ROMDATA- data storage qualifier used to put bitmaps, fonts, and string
// literals in ROM. May have to be re-defined for your compiler/linker.
/*--------------------------------------------------------------------------*/
#define ROMDATA  const

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#ifdef USE_PEG_LTOA
TCHAR * _ltoa(long val, TCHAR *s, int rad);
#define ltoa _ltoa
#endif



/*--------------------------------------------------------------------------*/
// PEG_SJIS_CONVERSION- Function to convert SJIS to Unicode
/*--------------------------------------------------------------------------*/

#ifdef PEG_SJIS_CONVERSION
#define SJIS_MAPTABLE_ENTRIES 6880
void SJISToUnicode(TCHAR *s1);
void SJISToUnicode(TCHAR *s1, const TCHAR *s2);
TCHAR SJISToUnicode(TCHAR c1);
void UnicodeToSJIS(TCHAR *s1);
void UnicodeToSJIS(TCHAR *s1, const TCHAR *s2);
TCHAR UnicodeToSJIS(TCHAR c1);
#endif

/*--------------------------------------------------------------------------*/
// COLORVAL- size of this data type depends on number of output colors
/*--------------------------------------------------------------------------*/

#if !defined(PEG_NUM_COLORS)

#ifdef PEGWIN32
typedef UCHAR COLORVAL;
#else
typedef DWORD COLORVAL;
#endif

#elif (PEG_NUM_COLORS > 65535L)
typedef DWORD COLORVAL;
#elif (PEG_NUM_COLORS > 256)
typedef WORD COLORVAL;  // on embedded target, WORD should be defined as 16-bits
#else
typedef UCHAR COLORVAL;
#endif


/*--------------------------------------------------------------------------*/
// Thing types- These values are found in the uType member of all Peg
// GUI objects, and can be queried via the Type() function.
/*--------------------------------------------------------------------------*/

const UCHAR TYPE_THING              = 1;
const UCHAR TYPE_TITLE              = 2;
const UCHAR TYPE_MENU               = 3;
const UCHAR TYPE_BUTTON             = 5;
const UCHAR TYPE_HSCROLL            = 6;
const UCHAR TYPE_VSCROLL            = 7;
const UCHAR TYPE_ICON               = 8;
const UCHAR TYPE_MENU_BAR           = 9;
const UCHAR TYPE_MENU_BUTTON        = 10;
const UCHAR TYPE_TEXTBUTTON         = 11;
const UCHAR TYPE_BMBUTTON           = 12;
const UCHAR TYPE_SPARE              = 13;
const UCHAR TYPE_RADIOBUTTON        = 14;
const UCHAR TYPE_CHECKBOX           = 15;
const UCHAR TYPE_STATUS_BAR         = 16;
const UCHAR TYPE_PROMPT             = 17;
const UCHAR TYPE_VPROMPT            = 18;
const UCHAR TYPE_SPARE3             = 19;
const UCHAR TYPE_SPARE4             = 20;
const UCHAR TYPE_SPARE5             = 21;
const UCHAR TYPE_STRING             = 22;
const UCHAR TYPE_SLIDER             = 23;
const UCHAR TYPE_SPINBUTTON         = 24;
const UCHAR TYPE_GROUP              = 25;
const UCHAR TYPE_MLTEXTBUTTON       = 26;
const UCHAR TYPE_TOOL_BAR_PANEL     = 27;
const UCHAR TYPE_TOOL_BAR           = 28;
const UCHAR TYPE_DECORATEDBUTTON    = 29;


/*--------------------------------------------------------------------------*/
// Chart types- only include if PEG_CHARTING is defined
/*--------------------------------------------------------------------------*/

#ifdef PEG_CHARTING

const UCHAR TYPE_LINE_CHART         = 40;
const UCHAR TYPE_STRIP_CHART        = 41;
const UCHAR TYPE_MULTI_LINE_CHART   = 42;

#endif

/*--------------------------------------------------------------------------*/
// HMI types- only include if PEG_HMI_GADGETS is defined
/*--------------------------------------------------------------------------*/

#ifdef PEG_HMI_GADGETS

const UCHAR TYPE_FDIAL              = 50;
const UCHAR TYPE_FBM_DIAL           = 51;
const UCHAR TYPE_CLR_LIGHT          = 52;
const UCHAR TYPE_BM_LIGHT           = 53;
const UCHAR TYPE_LIN_SCALE          = 54;
const UCHAR TYPE_LIN_BM_SCALE       = 55;

const UCHAR TYPE_CDIAL              = 56;
const UCHAR TYPE_CBM_DIAL           = 57;

#endif

/*--------------------------------------------------------------------------*/
// ... leave twenty additional types for future gadgets ...
//
// Note: It isn't critical if the class types need to be renumbered at
// some point. Only WindowBuilder project files would be affected.
//
/*--------------------------------------------------------------------------*/

const UCHAR FIRST_USER_CONTROL_TYPE = 80;


/*--------------------------------------------------------------------------*/
// Thing types for classes derived from PegWindow. The library performs
// slightly different processing on PegWindow derived classes.
/*--------------------------------------------------------------------------*/

const UCHAR TYPE_WINDOW             = 150;
const UCHAR TYPE_DIALOG             = 151;
const UCHAR TYPE_TABLE              = 152;
const UCHAR TYPE_SPREADSHEET        = 153;
const UCHAR TYPE_TEXTBOX            = 154;
const UCHAR TYPE_MESSAGE            = 155;
const UCHAR TYPE_DECORATED_WIN      = 156;
const UCHAR TYPE_ANIMATION          = 157;
const UCHAR TYPE_NOTEBOOK           = 158;
const UCHAR TYPE_TREEVIEW           = 159;
const UCHAR TYPE_TERMINAL_WIN       = 160;
const UCHAR TYPE_LIST               = 161;
const UCHAR TYPE_VLIST              = 162;
const UCHAR TYPE_HLIST              = 163;
const UCHAR TYPE_COMBO              = 164;
const UCHAR TYPE_EDITBOX            = 165;


const UCHAR FIRST_USER_WINDOW_TYPE  = 200;

/*--------------------------------------------------------------------------*/
// Reserved button IDs. Buttons with these IDs are treated specially by modal
// dialog and modal message window.
/*--------------------------------------------------------------------------*/

const WORD IDB_CLOSE    = 1000;
const WORD IDB_SYSTEM   = 1001;
const WORD IDB_OK       = 1002;
const WORD IDB_CANCEL   = 1003;
const WORD IDB_APPLY    = 1004;
const WORD IDB_ABORT    = 1005;
const WORD IDB_YES      = 1006;
const WORD IDB_NO       = 1007;
const WORD IDB_RETRY    = 1008;


/*--------------------------------------------------------------------------*/
// The object style flags. These flags change the appearance and operation of
// many PEG object types.
/*--------------------------------------------------------------------------*/

// frame styles:

const WORD FF_NONE     =        0x0001;
const WORD FF_THIN     =        0x0002;

#if (PEG_NUM_COLORS >= 4) || defined(PEG_RUNTIME_COLOR_CHECK)
const WORD FF_RAISED   =        0x0004;
const WORD FF_RECESSED =        0x0008;
#else
const WORD FF_RAISED   =        FF_THIN;
const WORD FF_RECESSED =        FF_THIN;
#endif

const WORD FF_THICK    =        0x0010;
const WORD FF_MASK     =        0x001f;

// Text Justification Style:

const WORD TJ_RIGHT =           0x0020;
const WORD TJ_LEFT =            0x0040;
const WORD TJ_CENTER =          0x0080;
const WORD TJ_MASK =            0x00E0;

// Title Style:

const WORD TF_NONE      =       0x0000;
const WORD TF_SYSBUTTON =       0x0200;
const WORD TF_MINMAXBUTTON =    0x0400;
const WORD TF_CLOSEBUTTON =     0x0800;

// Text Thing Copy Flag

const WORD TT_COPY =            0x2000;

// List Style

const WORD LS_WRAP_SELECT =     0x2000;

// Button Style:

const WORD BF_REPEAT =          0x0002;
const WORD BF_SELECTED =        0x0004;
const WORD BF_DOWNACTION =      0x0008;
const WORD BF_FULLBORDER =      0x0010;

// menu button style

const WORD BF_SEPARATOR =       0x0100;
const WORD BF_CHECKABLE =       0x0200;
const WORD BF_CHECKED =         0x0400;
const WORD BF_DOTABLE =         0x0800;
const WORD BF_DOTTED =          0x1000;

// Decorated Button styles. 

const WORD BF_ORIENT_TR =       0x0100;
const WORD BF_ORIENT_BR =       0x0200;

// Edit Style:

const WORD EF_EDIT =            0x0100;
const WORD EF_PARTIALROW =      0x0200;
const WORD EF_WRAP =            0x0400;
const WORD EF_FULL_SELECT =     0x0800;

// Message Window Style:

const WORD MW_OK =              0x0020;
const WORD MW_YES =             0x0040;
const WORD MW_NO =              0x0080;
const WORD MW_ABORT =           0x0100;
const WORD MW_RETRY =           0x0200;
const WORD MW_CANCEL =          0x0400;

// Table Style:

const WORD TS_SOLID_FILL     =  0x0100;
const WORD TS_PARTIAL_COL    =  0x0200;
const WORD TS_PARTIAL_ROW    =  0x0400;
const WORD TS_DRAW_HORZ_GRID =  0x1000;
const WORD TS_DRAW_VERT_GRID =  0x2000;
const WORD TS_DRAW_GRID      =  0x3000;

// Spreadsheet basic Styles:

const WORD SS_CELL_SELECT =      0x0400;
const WORD SS_PARTIAL_COL =      0x0800;
const WORD SS_MULTI_ROW_SELECT = 0x1000;
const WORD SS_MULTI_COL_SELECT = 0x2000;

// Spreadsheet Column Styles

const WORD SCF_ALLOW_SELECT =    0x0100;
const WORD SCF_SELECTED     =    0x0200;
const WORD SCF_SEPARATOR    =    0x0400;
const WORD SCF_CELL_SELECT  =    0x0800;

// Spreadsheet Row Styles

const WORD SRF_ALLOW_SELECT =    0x0100;
const WORD SRF_SELECTED     =    0x0200;
const WORD SRF_SEPARATOR    =    0x0400;
const WORD SRF_CELL_SELECT  =    0x0800;

// Notebook Styles:

const WORD NS_TOPTABS =         0x0100;        
const WORD NS_BOTTOMTABS =      0x0200;
const WORD NS_TEXTTABS =        0x0400;

// Slider Style:

const WORD SF_SNAP =            0x0100;
const WORD SF_SCALE =           0x0200;

// Progress Bar Style:

const WORD PS_SHOW_VAL =        0x0100;
const WORD PS_RECESSED =        0x0200;
const WORD PS_LED      =        0x0400;
const WORD PS_VERTICAL =        0x0800;
const WORD PS_PERCENT  =        0x1000;

// Spin Button Style:

const WORD SB_VERTICAL =        0x1000;

#ifdef PEG_HMI_GADGETS

// PegDial styles:

const WORD DS_CLOCKWISE =          0x0020;
const WORD DS_TICMARKS =           0x0040;
const WORD DS_THINNEEDLE =         0x0080;
const WORD DS_THICKNEEDLE =        0x0100;
const WORD DS_POLYNEEDLE =         0x0200;
const WORD DS_RECTCOR =            0x0400;
const WORD DS_USERCOR =            0x0800;

const WORD DS_STANDARDSTYLE =      DS_THINNEEDLE|DS_RECTCOR|DS_TICMARKS|
                                   DS_CLOCKWISE;

const WORD DS_THICKNEEDLEWIDTH =  0x03;
const WORD DS_ANCHORWIDTH =       4;
const WORD DS_ANCHORCOLOR =       0;

// PegLight styles:

const WORD LS_RECTANGLE =          0x0100;
const WORD LS_CIRCLE =             0x0200;

// PegScale styles:

const WORD SS_FACELEFT =           0x0020;
const WORD SS_FACETOP =            0x0020;
const WORD SS_BOTTOMTOTOP =        0x0040;
const WORD SS_LEFTTORIGHT =        0x0040;
const WORD SS_ORIENTVERT =         0x0080;
const WORD SS_TICMARKS =           0x0100;
const WORD SS_USERTRACK =          0x0200;
const WORD SS_USERTRAVEL =         0x0400;
const WORD SS_THINNEEDLE =         0x0800;
const WORD SS_THICKNEEDLE =        0x1000;
const WORD SS_POLYNEEDLE =         0x2000;

const WORD SS_STANDARDSTYLE =      SS_FACELEFT|SS_BOTTOMTOTOP|SS_ORIENTVERT|
                                   SS_TICMARKS|SS_POLYNEEDLE;

#endif

// Miscellaneous Appearance Style:

const WORD AF_TRANSPARENT =     0x4000;
const WORD AF_ENABLED =         0x8000;


/*--------------------------------------------------------------------------*/
// Extended style defines for PEG_CHARTING classes. These are NOT PegThing
// Style() flags, these are extended style flags used only with PegChart
// classes.
/*--------------------------------------------------------------------------*/
#ifdef PEG_CHARTING

const WORD CS_DRAWXGRID    = 0x0001;
const WORD CS_DRAWYGRID    = 0x0002;
const WORD CS_DRAWXTICS    = 0x0004;
const WORD CS_DRAWYTICS    = 0x0008;
const WORD CS_AUTOSIZE     = 0x0010;
const WORD CS_SCROLLED     = 0x0020;
const WORD CS_PAGED        = 0x0040;
const WORD CS_DRAWXAXIS    = 0x0080;
const WORD CS_DRAWAGED     = 0x0100;
const WORD CS_DRAWLEADER   = 0x0200;
const WORD CS_DUALYTICS	   = 0x0400;
const WORD CS_DUALYLABELS  = 0x0800;
const WORD CS_XAXISONZEROY = 0x1000;
const WORD CS_DRAWLINEFILL = 0x2000;
const WORD CS_DRAWXLABELS  = 0x4000;
const WORD CS_DRAWYLABELS  = 0x8000;

#endif


/*--------------------------------------------------------------------------*/
// PEG signal definitions. PegBaseSignals are supported by all objects. The
// remaining signals are only supported by the object type indicated in the
// enumeration name.
/*--------------------------------------------------------------------------*/
enum PegBaseSignals {
    PSF_SIZED = 0,          // sent when the object is moved or sized
    PSF_FOCUS_RECEIVED,     // sent when the object receives input focus
    PSF_FOCUS_LOST,         // sent when the object loses input focus
    PSF_KEY_RECEIVED,       // sent when an input key that is not supported is received
    PSF_RIGHTCLICK          // sent when a right-click message is received by the object
};

enum PegTextSignals {
    PSF_TEXT_SELECT    = 8, // sent when the user selects all or a portion of a text object
    PSF_TEXT_EDIT,          // sent each time text object string is modified
    PSF_TEXT_EDITDONE       // sent when a text object modification is complete
};

enum PegButtonSignals {
    PSF_CLICKED = 8,        // default button select notification
    PSF_CHECK_ON,           // sent by check box and  menu button when checked
    PSF_CHECK_OFF,          // sent by check box and menu button when unchecked
    PSF_DOT_ON,             // sent by radio button and menu button when selected
    PSF_DOT_OFF,            // sent by radio button and menu button when unselected
    PSF_LIST_SELECT         // sent by PegList derived objects, including PegComboBox
};

enum PegScrollSignals {
    PSF_SCROLL_CHANGE = 8,  // sent by non-client PegScroll derived objects
    PSF_SLIDER_CHANGE       // sent by PegSlider derived objects
};

enum PegSpinSignals {
    PSF_SPIN_MORE     = 8,  // sent by PegSpinButton on down or right
    PSF_SPIN_LESS           // sent by PegSpinButton on up or left
};

enum PegSpreadsheetSignals {
    PSF_COL_SELECT = 8,     // sent when SpreadSheet column is selected
    PSF_COL_DESELECT,       // send when SpreadSheet column is deselected
    PSF_ROW_SELECT,         // sent when SpreadSheet row is selected
    PSF_ROW_DESELECT,       // sent when SpreadSheet row is deselected
    PSF_CELL_SELECT,        // sent when SpreadSheet cell(s) are selected
    PSF_CELL_RCLICK         // sent when Right-Click on cell
};

enum PegNotebookSignals {
    PSF_PAGE_SELECT = 8     // send whan a new page is selected
};

enum PegTreeSignals {
    PSF_NODE_SELECT = 8,    // sent when a new node is selected
    PSF_NODE_DELETE,        // sent when delete key pressed over node
    PSF_NODE_OPEN,          // sent when node is opened
    PSF_NODE_CLOSE,         // sent when node is closed
    PSF_NODE_RCLICK         // sent on RBUTTON clicked on selected node
};

enum PegTerminalSignals {
    PSF_COMMAND = 8         // sent when user types "Enter" on the
                            // terminal command line.
};


/*--------------------------------------------------------------------------*/
// System Status flags common to all object types
// These flags are maintained internally by PEG, but can be modified (at your
// own risk!) by the application level software.
/*--------------------------------------------------------------------------*/

const WORD PSF_VISIBLE =           0x0001;
const WORD PSF_CURRENT =           0x0002;
const WORD PSF_SELECTABLE =        0x0004;
const WORD PSF_SIZEABLE =          0x0008;
const WORD PSF_MOVEABLE =          0x0010;
const WORD PSF_NONCLIENT =         0x0020;
const WORD PSF_ACCEPTS_FOCUS =     0x0040;
const WORD PSF_KEEPS_CHILD_FOCUS = 0x0080;
const WORD PSF_CONTINUOUS_SCROLL = 0x0100;
const WORD PSF_TAB_STOP =          0x0200;
const WORD PSF_OWNS_POINTER =      0x0400;    
const WORD PSF_ALWAYS_ON_TOP =     0x4000;
const WORD PSF_VIEWPORT =          0x8000;


/*--------------------------------------------------------------------------*/
// PegWindow Scroll Modes
/*--------------------------------------------------------------------------*/
const UCHAR WSM_VSCROLL =       0x01;
const UCHAR WSM_AUTOVSCROLL =   0x02;
const UCHAR WSM_HSCROLL =       0x04;
const UCHAR WSM_AUTOHSCROLL =   0x08;
const UCHAR WSM_AUTOSCROLL  =   0x0A;
const UCHAR WSM_CONTINUOUS  =   0x80;

/*--------------------------------------------------------------------------*/
// a simple and fast variable swapping algorithm:

#define PEGSWAP(a, b) {a ^= b; b ^= a; a ^= b;}

/*--------------------------------------------------------------------------*/
// MIN and MAX macros

#define PEGMIN(a, b)  (a > b ? b:a)
#define PEGMAX(a, b)  (a > b ? a:b)

/*--------------------------------------------------------------------------*/
// macro for converting signal values to signal masks:

#define SIGMASK(a) (1 << (a))

#define ALL_SIGNALS_MASK 0x0F

/*--------------------------------------------------------------------------*/
// macro for catching signals given object ID and signal value:

#define PEG_SIGNAL(ID, SIG) (FIRST_SIGNAL + (ID << 4) + SIG)

#ifndef PEG_OSE
#define SIGNAL(ID, SIG) PEG_SIGNAL(ID, SIG)
#endif

/*--------------------------------------------------------------------------*/
// PegPoint definition:

struct PegPoint
{
    SIGNED x;
    SIGNED y;
    BOOL operator != (const PegPoint &InPoint) const 
    {
        if (x != InPoint.x || y != InPoint.y)
        {
            return TRUE;
        }
        return FALSE;
    }
    BOOL operator == (const PegPoint &InPoint) const
    {
        if (x == InPoint.x && y == InPoint.y)
        {
            return TRUE;
        }
        return FALSE;
    }
    PegPoint operator +(const PegPoint &Point) const
    {
        PegPoint NewPt;
        NewPt.x = x + Point.x;
        NewPt.y = y + Point.y;
        return NewPt;
    }
};

/*--------------------------------------------------------------------------*/
// PegRect definition

struct PegRect
{
    void Set(SIGNED x1, SIGNED y1, SIGNED x2, SIGNED y2)
    {
        wLeft = x1;
        wTop = y1;
        wRight = x2;
        wBottom = y2;
    }

    void Set(PegPoint ul, PegPoint br)
    {
        wLeft = ul.x;
        wTop = ul.y;
        wRight = br.x;
        wBottom = br.y;
    }
    BOOL Contains(PegPoint Test) const;
    BOOL Contains(SIGNED x, SIGNED y) const;
    BOOL Contains(const PegRect &Rect) const;
    BOOL Overlap(const PegRect &Rect) const;
    void MoveTo(SIGNED x, SIGNED y);
    void Shift(SIGNED xShift, SIGNED yShift);
    PegRect operator &=(const PegRect &Other);
    PegRect operator |= (const PegRect &Other);
    PegRect operator &(const PegRect &Rect) const;
    PegRect operator ^= (const PegRect &Rect);
    PegRect operator +(const PegPoint &Point) const; 
    PegRect operator ++(int);
    PegRect operator += (int x);
    PegRect operator --(int);
    PegRect operator -= (int x);
    BOOL operator != (const PegRect &Rect) const;
    BOOL operator == (const PegRect &Rect) const;
                     
    SIGNED Width(void) const {return (wRight - wLeft + 1);}
    SIGNED Height(void) const { return (wBottom - wTop + 1);}

    SIGNED wLeft;
    SIGNED wTop;
    SIGNED wRight;
    SIGNED wBottom;
};



/*--------------------------------------------------------------------------*/
// Basic Colors- These need to be re-defined based on the type of display.
// Default settings are provided for 2, 4, 16, and 256 color displays.
// Other color depths can be supported by extending this list of color
// definitions.
/*--------------------------------------------------------------------------*/

#ifdef PEG_RUNTIME_COLOR_CHECK

// When running with PharLap ETS, all colors are 32-bit values. The default
// values are shown below, but Hi-Color (i.e. > 256 color) screen drivers
// overwrite these values with the correct colors for the screen driver in
// use.

extern COLORVAL TRANSPARENCY;
extern COLORVAL BLACK;
extern COLORVAL RED;        
extern COLORVAL GREEN;
extern COLORVAL BROWN;        
extern COLORVAL BLUE;        
extern COLORVAL MAGENTA;
extern COLORVAL CYAN;
extern COLORVAL LIGHTGRAY;  
extern COLORVAL DARKGRAY;   
extern COLORVAL LIGHTRED;   
extern COLORVAL LIGHTGREEN; 
extern COLORVAL YELLOW;
extern COLORVAL LIGHTBLUE;  
extern COLORVAL LIGHTMAGENTA;
extern COLORVAL LIGHTCYAN;  
extern COLORVAL WHITE;

#define PCLR_HIGHLIGHT       WHITE
#define PCLR_LOWLIGHT        DARKGRAY
#define PCLR_SHADOW          BLACK
#define PCLR_ACTIVE_TITLE    BLUE
#define PCLR_INACTIVE_TITLE  DARKGRAY
#define PCLR_NORMAL_TEXT     BLACK
#define PCLR_HIGH_TEXT       WHITE
#define PCLR_NORM_TEXT_BACK  WHITE
#define PCLR_HIGH_TEXT_BACK  BLUE
#define PCLR_CLIENT          WHITE
#define PCLR_DIALOG          LIGHTGRAY
#define PCLR_BORDER          LIGHTGRAY
#define PCLR_BUTTON_FACE     LIGHTGRAY
#define PCLR_CURSOR          LIGHTBLUE
#define PCLR_DESKTOP         BLACK
#define PCLR_FOCUS_INDICATOR DARKGRAY

#else

// For all other platforms, we determine the color depth (and resulting
// color values) at compile time.

const COLORVAL TRANSPARENCY   = (COLORVAL) 0xff; // transparent color

#if (PEG_NUM_COLORS > 256) && !defined(_PEG_BITMAP_MODULE_)

// We define the first 16 colors when running in a HICOLOR or TRUECOLOR
// mode. For 16-bit operation, the most common configuration is 5-6-5
// RGB. This is the first set of values below. A less common operation is
// 5-5-5 RGB, which the second set of values. Note: 5-5-5 operation is 
// the mode to use when running the 16-bit driver l16scrn.cpp under 
// Win32. If you are using 16-bit color, set the definitions RGB_USE_555
// and RGB_USE_565 to match your system (only one should be turned on).

#if (PEG_NUM_C0LORS == 65535)   // 16-bit color mode?

#ifdef PEGWIN32
#define RGB_USE_555          // use 5-5-5 mode for Win32    
#else
#define RGB_USE_565         // otherwise default to use 5-6-5 mode
#endif

#ifdef RGB_USE_565          // Use this version for 5-6-5 RGB operation,
                            // the most common 16-bit color mode.

const COLORVAL BLACK           = 0x0000;
const COLORVAL RED             = 0xb800;        
const COLORVAL GREEN           = 0x05e0;
const COLORVAL BROWN           = 0xbde0;        
const COLORVAL BLUE            = 0x0017;        
const COLORVAL MAGENTA         = 0xb817;
const COLORVAL CYAN            = 0x05f7;
const COLORVAL LIGHTGRAY       = 0xc618;  
const COLORVAL DARKGRAY        = 0x8410;   
const COLORVAL LIGHTRED        = 0xf800;   
const COLORVAL LIGHTGREEN      = 0x07e0; 
const COLORVAL YELLOW          = 0xffe0;
const COLORVAL LIGHTBLUE       = 0x001f;  
const COLORVAL LIGHTMAGENTA    = 0xf81f;
const COLORVAL LIGHTCYAN       = 0x07ff;  
const COLORVAL WHITE           = 0xffff;

#else   // here for 5-5-5 operation, a few controllers and Win32 use this mode

const COLORVAL BLACK           = 0x0000;
const COLORVAL RED             = 0x5c00;        
const COLORVAL GREEN           = 0x02e0;
const COLORVAL BROWN           = 0x5ee0;        
const COLORVAL BLUE            = 0x0017;        
const COLORVAL MAGENTA         = 0x5c17;
const COLORVAL CYAN            = 0x02f7;
const COLORVAL LIGHTGRAY       = 0x6318;  
const COLORVAL DARKGRAY        = 0x4210;   
const COLORVAL LIGHTRED        = 0x7c00;   
const COLORVAL LIGHTGREEN      = 0x03e0; 
const COLORVAL YELLOW          = 0x7fe0;
const COLORVAL LIGHTBLUE       = 0x001f;  
const COLORVAL LIGHTMAGENTA    = 0x7c1f;
const COLORVAL LIGHTCYAN       = 0x03ff;  
const COLORVAL WHITE           = 0x7fff;

#endif

#else

// here for 24-bpp color:
const COLORVAL BLACK           = 0x000000;
const COLORVAL RED             = 0xbf0000;        
const COLORVAL GREEN           = 0x00bf00;
const COLORVAL BROWN           = 0xbfbf00;        
const COLORVAL BLUE            = 0x0000bf;        
const COLORVAL MAGENTA         = 0xbf00bf;
const COLORVAL CYAN            = 0x00bfbf;
const COLORVAL LIGHTGRAY       = 0xc0c0c0;  
const COLORVAL DARKGRAY        = 0x808080;   
const COLORVAL LIGHTRED        = 0xff0000;   
const COLORVAL LIGHTGREEN      = 0x00ff00; 
const COLORVAL YELLOW          = 0xffff00;
const COLORVAL LIGHTBLUE       = 0x0000ff;  
const COLORVAL LIGHTMAGENTA    = 0xff00ff;
const COLORVAL LIGHTCYAN       = 0x00ffff;  
const COLORVAL WHITE           = 0xffffff;
#endif

#define PCLR_HIGHLIGHT       WHITE
#define PCLR_LOWLIGHT        DARKGRAY
#define PCLR_SHADOW          BLACK
#define PCLR_ACTIVE_TITLE    BLUE
#define PCLR_INACTIVE_TITLE  DARKGRAY
#define PCLR_NORMAL_TEXT     BLACK
#define PCLR_HIGH_TEXT       WHITE
#define PCLR_NORM_TEXT_BACK  WHITE
#define PCLR_HIGH_TEXT_BACK  BLUE
#define PCLR_CLIENT          WHITE
#define PCLR_DIALOG          LIGHTGRAY
#define PCLR_BORDER          LIGHTGRAY
#define PCLR_BUTTON_FACE     LIGHTGRAY
#define PCLR_CURSOR          LIGHTBLUE
#define PCLR_DESKTOP         BLACK
#define PCLR_FOCUS_INDICATOR DARKGRAY

#elif (PEG_NUM_COLORS == 256) && defined(EIGHT_BIT_PACKED_PIXEL)

const COLORVAL BLACK           = 0x00;
const COLORVAL RED             = 0xa0;
const COLORVAL GREEN           = 0x14;
const COLORVAL BROWN           = 0xb4;
const COLORVAL BLUE            = 0x02;
const COLORVAL MAGENTA         = 0xa2;
const COLORVAL CYAN            = 0x16;
const COLORVAL LIGHTGRAY       = 0xb6;
const COLORVAL DARKGRAY        = 0x92;
const COLORVAL LIGHTRED        = 0xe0;
const COLORVAL LIGHTGREEN      = 0x1c;
const COLORVAL YELLOW          = 0xfc;
const COLORVAL LIGHTBLUE       = 0x03;
const COLORVAL LIGHTMAGENTA    = 0xe3;
const COLORVAL LIGHTCYAN       = 0x1f;
const COLORVAL WHITE           = 0xff;

#define PCLR_HIGHLIGHT       WHITE
#define PCLR_LOWLIGHT        DARKGRAY
#define PCLR_SHADOW          BLACK
#define PCLR_ACTIVE_TITLE    BLUE
#define PCLR_INACTIVE_TITLE  DARKGRAY
#define PCLR_NORMAL_TEXT     BLACK
#define PCLR_HIGH_TEXT       WHITE
#define PCLR_NORM_TEXT_BACK  WHITE
#define PCLR_HIGH_TEXT_BACK  BLUE
#define PCLR_CLIENT          WHITE
#define PCLR_DIALOG          LIGHTGRAY
#define PCLR_BORDER          LIGHTGRAY
#define PCLR_BUTTON_FACE     LIGHTGRAY
#define PCLR_CURSOR          LIGHTBLUE
#define PCLR_DESKTOP         BLACK
#define PCLR_FOCUS_INDICATOR DARKGRAY

#elif (PEG_NUM_COLORS >= 16)

const COLORVAL BLACK           = 0;
const COLORVAL RED             = 1;        
const COLORVAL GREEN           = 2;
const COLORVAL BROWN           = 3;        
const COLORVAL BLUE            = 4;        
const COLORVAL MAGENTA         = 5;
const COLORVAL CYAN            = 6;
const COLORVAL LIGHTGRAY       = 7;  
const COLORVAL DARKGRAY        = 8;   
const COLORVAL LIGHTRED        = 9;   
const COLORVAL LIGHTGREEN      = 10; 
const COLORVAL YELLOW          = 11;
const COLORVAL LIGHTBLUE       = 12;  
const COLORVAL LIGHTMAGENTA    = 13;
const COLORVAL LIGHTCYAN       = 14;  
const COLORVAL WHITE           = 15;

#define PCLR_HIGHLIGHT       WHITE
#define PCLR_LOWLIGHT        DARKGRAY
#define PCLR_SHADOW          BLACK
#define PCLR_ACTIVE_TITLE    BLUE
#define PCLR_INACTIVE_TITLE  DARKGRAY
#define PCLR_NORMAL_TEXT     BLACK
#define PCLR_HIGH_TEXT       WHITE
#define PCLR_NORM_TEXT_BACK  WHITE
#define PCLR_HIGH_TEXT_BACK  BLUE
#define PCLR_CLIENT          WHITE
#define PCLR_DIALOG          LIGHTGRAY
#define PCLR_BORDER          LIGHTGRAY
#define PCLR_BUTTON_FACE     LIGHTGRAY
#define PCLR_CURSOR          LIGHTBLUE
#define PCLR_DESKTOP         BLACK
#define PCLR_FOCUS_INDICATOR DARKGRAY

#elif (PEG_NUM_COLORS == 4)

    // four color grayscale color definitions

#if 1
// most common color definitions
const COLORVAL BLACK           = 0;
const COLORVAL DARKGRAY        = 1;   
const COLORVAL LIGHTGRAY       = 2;  
const COLORVAL WHITE           = 3;

#else
// for some video/LCD we need to invert the gray values
const COLORVAL BLACK           = 3;
const COLORVAL DARKGRAY        = 2;   
const COLORVAL LIGHTGRAY       = 1;  
const COLORVAL WHITE           = 0;

#endif

#define PCLR_HIGHLIGHT       WHITE
#define PCLR_LOWLIGHT        DARKGRAY
#define PCLR_SHADOW          BLACK
#define PCLR_ACTIVE_TITLE    DARKGRAY
#define PCLR_INACTIVE_TITLE  DARKGRAY
#define PCLR_NORMAL_TEXT     BLACK
#define PCLR_HIGH_TEXT       WHITE
#define PCLR_NORM_TEXT_BACK  WHITE
#define PCLR_HIGH_TEXT_BACK  DARKGRAY
#define PCLR_CLIENT          WHITE
#define PCLR_DIALOG          LIGHTGRAY
#define PCLR_BORDER          LIGHTGRAY
#define PCLR_BUTTON_FACE     LIGHTGRAY
#define PCLR_CURSOR          DARKGRAY
#define PCLR_DESKTOP         WHITE
#define PCLR_FOCUS_INDICATOR DARKGRAY

#else

    // here for Monochrome color definitions:

const COLORVAL BLACK           = 0;
const COLORVAL WHITE           = 1;

#define PCLR_HIGHLIGHT       WHITE
#define PCLR_LOWLIGHT        BLACK
#define PCLR_SHADOW          BLACK
#define PCLR_ACTIVE_TITLE    WHITE
#define PCLR_INACTIVE_TITLE  WHITE
#define PCLR_NORMAL_TEXT     BLACK
#define PCLR_HIGH_TEXT       WHITE
#define PCLR_NORM_TEXT_BACK  WHITE
#define PCLR_HIGH_TEXT_BACK  BLACK
#define PCLR_CLIENT          WHITE
#define PCLR_DIALOG          WHITE
#define PCLR_BORDER          BLACK
#define PCLR_BUTTON_FACE     WHITE
#define PCLR_CURSOR          BLACK
#define PCLR_DESKTOP         WHITE
#define PCLR_FOCUS_INDICATOR BLACK

#endif

#endif      // end of PharLap ETS if

/*--------------------------------------------------------------------------*/
// Color Flags:
/*--------------------------------------------------------------------------*/

const UCHAR CF_NONE =    0x00;
const UCHAR CF_FILL =    0x01;
const UCHAR CF_DASHED =  0x02;
const UCHAR CF_XOR =     0x04;
const UCHAR CF_ALPHA =   0x08;

/*--------------------------------------------------------------------------*/
// The Peg Color Indices, passed when calls are made to 'SetColor'

const UCHAR PCI_NORMAL          = 0;
const UCHAR PCI_SELECTED        = 1;
const UCHAR PCI_NTEXT           = 2;
const UCHAR PCI_STEXT           = 3;

// Extended SpreadSheet color indexes:

const UCHAR PCI_SS_COLHEADBACK = 4;
const UCHAR PCI_SS_COLHEADTEXT = 5;
const UCHAR PCI_SS_ROWHEADBACK = 6;
const UCHAR PCI_SS_ROWHEADTEXT = 7;
const UCHAR PCI_SS_DIVIDER     = 8;
const UCHAR PCI_SS_BACKGROUND  = 9;

#define THING_COLOR_LIST_SIZE 4

/*--------------------------------------------------------------------------*/
// PegColor definition:

struct PegColor
{
    PegColor (COLORVAL fore, COLORVAL back = PCLR_DIALOG, UCHAR Flags = CF_NONE)
    {
        uForeground = fore;
        uBackground = back;
        uFlags = Flags;
    }

    PegColor()
    {
        uForeground = uBackground = BLACK;
        uFlags = CF_NONE;
    }

    void Set(COLORVAL fore, COLORVAL back = BLACK, UCHAR Flags = CF_NONE)
    {
        uForeground = fore;
        uBackground = back;
        uFlags = Flags;
    }

    COLORVAL uForeground;
    COLORVAL uBackground;
    UCHAR uFlags;
};

/*--------------------------------------------------------------------------*/
// PegBitmap structure and flags definition.
//
// PegBitmap is a position-independant bitmap header that contains 
// type and size information used by PegScreen.
/*--------------------------------------------------------------------------*/

#define   BMF_RAW          0x00
#define   BMF_RLE          0x01
#define   BMF_NATIVE       0x02
#define   BMF_ROTATED      0x04
#define   BMF_HAS_TRANS    0x10     //
#define   BMF_SPRITE       0x20     // bitmap resides in video memory

struct PegBitmap
{
    UCHAR uFlags;          // combination of flags above
    UCHAR uBitsPix;        // 1, 2, 4, or 8
    WORD  wWidth;          // in pixels
    WORD  wHeight;         // in pixels
    DWORD dTransColor;     // transparent color for > 8bpp bitmaps
    UCHAR PEGFAR *pStart;  // bitmap data pointer
};

#define IS_RLE(a) (a->uFlags & BMF_RLE)
#define HAS_TRANS(a) (a->uFlags & BMF_HAS_TRANS)
#define IS_NATIVE(a) (a->uFlags & BMF_NATIVE)
#define IS_ROTATED(a) (a->uFlags & BMF_ROTATED)
#define IS_SPRITE(a) (a->uFlags & BMF_SPRITE)

/*--------------------------------------------------------------------------*/
// PegCapture data type definition
//
// PegCapture is a PegBitmap with extra position and status information.
/*--------------------------------------------------------------------------*/
class PegCapture
{
    public:
        PegCapture(void)
        {
            mRect.Set(0, 0, 0, 0);
            mBitmap.pStart = 0;
            mbValid = FALSE;
            mlDataSize = 0;
        }

        ~PegCapture()
        {
            if (mBitmap.pStart)
            {
                delete mBitmap.pStart;
            }
        }

        PegRect &Pos(void) {return mRect;}
        PegPoint Point(void);
        LONG     DataSize(void) {return mlDataSize;}

        void SetPos(PegRect &Rect)
        {
            mRect = Rect;
            mBitmap.wWidth = Rect.Width();
            mBitmap.wHeight = Rect.Height();
        }

        BOOL IsValid(void) const {return mbValid;}
        void SetValid(BOOL bValid) {mbValid = bValid;}
        void Realloc(LONG lSize);
        void Reset(void);
        void MoveTo(SIGNED iLeft, SIGNED iTop);
        void Shift(SIGNED xShift, SIGNED yShift) {mRect.Shift(xShift, yShift);}
        PegBitmap *Bitmap(void) {return &mBitmap;}

    private:

        PegRect mRect;
        PegBitmap mBitmap;
        LONG    mlDataSize;
        BOOL    mbValid;
};


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// The definition BF_SEPARATOR was mis-spelled in earlier releases. To avoid
// compile problems with existing application code, we are temporarily
// definining the old spelling->new spelling. The old spelling will eventually
// be removed entirely.
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#define BF_SEPERATOR BF_SEPARATOR

#endif

// End of file


