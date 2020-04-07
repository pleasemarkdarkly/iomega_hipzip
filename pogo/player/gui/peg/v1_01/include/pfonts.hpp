/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pfonts.hpp - Peg font and PegTextThing class definitions.
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
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef _PEGFONTS_
#define _PEGFONTS_

#include <_modules.h>

/*--------------------------------------------------------------------------*/
// PegFont structure definition:

struct PegFont
{
    UCHAR   uType;            // bit-flags defined below
    UCHAR   uAscent;          // Ascent above baseline
    UCHAR   uDescent;         // Descent below baseline
    UCHAR   uHeight;          // total height of character
    WORD    wBytesPerLine;    // total bytes (width) of one scanline
    WORD    wFirstChar;       // first character present in font (page)
    WORD    wLastChar;        // last character present in font (page)
    WORD    *pOffsets;        // bit-offsets for variable-width font
    PegFont *pNext;           // NULL unless multi-page Unicode font
    UCHAR   *pData;           // character bitmap data array
};

/*--------------------------------------------------------------------------*/
// Flags for Font Type:

#define PFT_VARIABLE 0x01   // Variable-Width font (standard)
#define PFT_OUTLINE  0x02   // Outline font
#define PFT_ALIASED  0x04   // Not supported in current release

#define IS_VARWIDTH(a) (a->uType & PFT_VARIABLE)
#define IS_OUTLINE(a)  (a->uType & PFT_OUTLINE)
#define IS_ALIASED(a)  (a->uType & PFT_ALIASED)


/*--------------------------------------------------------------------------*/
// Default fonts:

#ifndef FONTFILE

//extern PegFont SysFont;
//extern PegFont MenuFont;
//extern PegFont TitleFont;     // defaults to SysFont

// extern custom fonts here:
//extern PegFont VerdanaFont16;
#if defined (DDOMOD_SSI_NEO_UI)
extern PegFont Latin_Font_12;
#elif defined (DDOMOD_POGO_UI)
extern PegFont Latin_Font_9;
#elif defined (DDOMOD_DJ_UI)
extern PegFont Latin_Ext_Font_14;
#else
#warning No UI included, defaulting to Latin_Font_9
extern PegFont Latin_Font_9;
#endif

#endif // FONTFILE
/*--------------------------------------------------------------------------*/
// The default font assigned to each object type is defined here. You 
// can modify the default font assignments here to use custom fonts you have
// generated, however this is no longer recommended.
//
// We suggest that instead you use the SetDefaultFont() function during 
// program startup to initialize your default fonts. This way you do not have to modify this header fie To do this, we suggest that you change the following 
// to modify this header file.
//
/*--------------------------------------------------------------------------*/

#define PEG_SPARE_FONT_INDEXES  0   // use this to add new default fonts

enum PEG_DEFAULT_FONT_INDEX {
    PEG_DEFAULT_FONT = 0,           // default = SysFont
    PEG_TITLE_FONT,                 // default = SysFont
    PEG_MENU_FONT,                  // default = MenuFont
    PEG_TBUTTON_FONT,               // default = MenuFont
    PEG_RBUTTON_FONT,               // default = MenuFont
    PEG_CHECKBOX_FONT,              // default = MenuFont
    PEG_PROMPT_FONT,                // default = SysFont
    PEG_STRING_FONT,                // default = SysFont
    PEG_TEXTBOX_FONT,               //
    PEG_GROUP_FONT,
    PEG_ICON_FONT,
    PEG_CELL_FONT,
    PEG_HEADER_FONT,
    PEG_TAB_FONT,
    PEG_MESGWIN_FONT,
    PEG_TREEVIEW_FONT,
    PEG_NUMBER_OF_DEFAULT_FONTS
};
   

//#define DEFAULT_FONT_PTR  &SysFont
//#define SMALL_FONT_PTR    &MenuFont
#if defined (DDOMOD_SSI_NEO_UI)
#define DEFAULT_FONT_PTR  &Latin_Font_12
#define SMALL_FONT_PTR    &Latin_Font_12
#elif defined (DDOMOD_POGO_UI)
#define DEFAULT_FONT_PTR  &Latin_Font_9
#define SMALL_FONT_PTR    &Latin_Font_9
#elif defined (DDOMOD_DJ_UI)
#define DEFAULT_FONT_PTR  &Latin_Ext_Font_14
#define SMALL_FONT_PTR    &Latin_Ext_Font_14
#else
#define DEFAULT_FONT_PTR  &Latin_Font_9
#define SMALL_FONT_PTR    &Latin_Font_9
#endif

#define DEF_TITLE_FONT      DEFAULT_FONT_PTR
#define DEF_MENU_FONT       SMALL_FONT_PTR
#define DEF_TBUTTON_FONT    SMALL_FONT_PTR
#define DEF_RBUTTON_FONT    SMALL_FONT_PTR
#define DEF_CHECKBOX_FONT   SMALL_FONT_PTR
#define DEF_PROMPT_FONT     DEFAULT_FONT_PTR
#define DEF_STRING_FONT     DEFAULT_FONT_PTR
#define DEF_TEXTBOX_FONT    DEFAULT_FONT_PTR
#define DEF_GROUP_FONT      SMALL_FONT_PTR
#define DEF_ICON_FONT       SMALL_FONT_PTR
#define DEF_CELL_FONT       SMALL_FONT_PTR
#define DEF_HEADER_FONT     DEFAULT_FONT_PTR
#define DEF_TAB_FONT        SMALL_FONT_PTR
#define DEF_MESGWIN_FONT    SMALL_FONT_PTR
#define DEF_TREEVIEW_FONT   SMALL_FONT_PTR


/*--------------------------------------------------------------------------*/
class PegTextThing
{
    public:

        PegTextThing(const TCHAR *Text, WORD wCopy = 0, UCHAR uFontIndex = 0);
        PegTextThing(WORD wCopy = 0, UCHAR uFontIndex = 0);
        virtual ~PegTextThing();
        virtual void DataSet(const TCHAR *Text);
        TCHAR *DataGet(void) {return mpText;}
        virtual void SetFont(PegFont *Font) {mpFont = Font;}
        virtual PegFont *GetFont(void) {return mpFont;}
        virtual WORD TextLength(void) {return mwStrLen;}
        void SetCopyMode(void);

        static void SetDefaultFont(const UCHAR uIndex, PegFont *pFont)
        {
            mDefaultFonts[uIndex] = pFont;
        }

        static PegFont *GetDefaultFont (UCHAR uIndex)
        {
            return mDefaultFonts[uIndex];
        }

    protected:

        PegFont *mpFont;
        TCHAR *mpText;
        WORD mwStrLen;
        WORD mwBufferLen;
        UCHAR mbCopy;

    private:

        static PegFont *mDefaultFonts[PEG_NUMBER_OF_DEFAULT_FONTS + 
                                      PEG_SPARE_FONT_INDEXES];

};




#endif

