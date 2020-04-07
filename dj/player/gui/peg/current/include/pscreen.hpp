/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pscreen.hpp - Abstract PegScreen class definition.
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
// This class defines all of the methods which must be supported in 
// instantiated screen classes. Users may of course add additional members
// in custom derived classes, but only those members found here will be used
// by PEG.
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef _PEGSCREEN_
#define _PEGSCREEN_

/*--------------------------------------------------------------------------*/
// Pointer Types:

#define PPT_NORMAL 0
#define PPT_VSIZE  1
#define PPT_HSIZE  2
#define PPT_NWSE_SIZE 3
#define PPT_NESW_SIZE 4
#define PPT_IBEAM     5
#define PPT_HAND      6

#define NUM_POINTER_TYPES 7

struct PegPointer
{
    PegBitmap *Bitmap;
    SIGNED xOffset;
    SIGNED yOffset;
};

struct RGB_VAL
{
    UCHAR Red;
    UCHAR Green;
    UCHAR Blue;
    RGB_VAL(UCHAR a, UCHAR b, UCHAR c) {Red = a; Green = b; Blue = c;}
    RGB_VAL() {Red = Green = Blue = 0;}
    RGB_VAL operator += (RGB_VAL r)
    {
        Red += r.Red;
        Green += r.Green;
        Blue += r.Blue;
        return *this;
    }
};

struct VID_MEM_BLOCK {
    DWORD lMagic;
    VID_MEM_BLOCK *pNext;
    VID_MEM_BLOCK *pPrev;
    VID_MEM_BLOCK *pNextFree;
    DWORD lSize;
};

class PegThing;         // forward reference

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// This is the ClipList class definition. Basically, the clip-list is a list
// of areas on the screen that are 'owned' by different windows. Since this
// style of clipping can add significant overhead depending on the number of
// windows which are present on the screen at any one time, this can be
// disabled by turning off PEG_FULL_CLIPPING.
// 
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#ifdef PEG_FULL_CLIPPING

#define VIEWPORT_LIST_INCREMENT 32  // This is how many viewports to add when
                                    // we run out. There is a global pool of
                                    // viewports, allocated to each PSF_VIEWPORT
                                    // OBJECT as needed.

struct Viewport
{
    PegRect  mView;
    Viewport *pNext;
};

#endif


#ifdef PEG_AWT_SUPPORT
#define ClipRect(a, b) ClipRectNoInvalid(a, b)
#endif


/*--------------------------------------------------------------------------*/
// This is the abstract screen class definition that all Peg object are
// coded to. Note that this is an abstract class. An instantiable screen
// class must be defined for a working library.
/*--------------------------------------------------------------------------*/

class PegScreen 
{
    public:
        PegScreen(const PegRect &);
        virtual ~PegScreen() {}

        // Pure Virtual Functions- These functions must be provided in 
        // instantiable screen drivers.

        virtual void BeginDraw(PegThing *) = 0;
        virtual void EndDraw() = 0;
        virtual void BeginDraw(PegThing *, PegBitmap *) = 0;
        virtual void EndDraw(PegBitmap *) = 0;
        virtual void SetPointerType(UCHAR bType) = 0;
        virtual void HidePointer(void) = 0;
        virtual void SetPointer(PegPoint) = 0;
        virtual void Capture(PegCapture *Info, PegRect &Rect) = 0;
        virtual void  SetPalette(SIGNED iFirst, SIGNED iNum, const UCHAR *pGet) = 0;
        virtual UCHAR *GetPalette(DWORD *pPutSize) = 0;
        virtual void ResetPalette(void) = 0;

		#ifdef LINUXPEG
		virtual void PlotPointView(SIGNED, SIGNED, COLORVAL) = 0;
		#endif
		
        #ifdef PEG_IMAGE_SCALING
        virtual COLORVAL GetBitmapPixel(SIGNED x, SIGNED y, PegBitmap *pMap) = 0;
        virtual void PutBitmapPixel(SIGNED x, SIGNED y, PegBitmap *pMap, COLORVAL c) = 0;
        #endif

        #ifdef PEG_BITMAP_WRITER
        UCHAR *DumpWindowsBitmap(DWORD *pPutSize, PegRect &View);
        #endif

        // Virtual Functions- These functions may be overridden in
        // instantiable screen drivers, but they don't have to be.

        virtual COLORVAL GetPixel(PegThing *Caller, SIGNED x, SIGNED y);
        virtual void  PutPixel(PegThing *Caller, SIGNED x, SIGNED y, COLORVAL Color);

        virtual DWORD NumColors(void) {return mdNumColors;}
        virtual PegBitmap *CreateBitmap(SIGNED wWidth, SIGNED wHeight);
        virtual void DestroyBitmap(PegBitmap *pMap);

        virtual void Line(PegThing *Caller, SIGNED wXStart, SIGNED wYStart,
            SIGNED wXEnd, SIGNED wYEnd, const PegColor &Color,
            SIGNED wWidth = 1);

        virtual void Rectangle(PegThing *Caller, const PegRect &Rect,
            const PegColor &Color, SIGNED wWidth = 1);

        virtual void Bitmap(PegThing *Caller, PegPoint Where,
            PegBitmap *Getmap, BOOL bOnTop = FALSE);

        virtual void BitmapFill(PegThing *Caller, PegRect Rect, PegBitmap *Getmap);
        virtual void Restore(PegThing *Caller, PegCapture *Info, BOOL bOnTop = FALSE);
        virtual void RectMove(PegThing *Caller, PegRect Get, PegPoint Put);
        virtual void ViewportMove(PegThing *Caller, PegRect MoveRect, PegPoint MoveTo);

        virtual void DrawText(PegThing *Caller, PegPoint Where,
            const TCHAR *Text, PegColor &Color, PegFont *Font, 
            SIGNED Count = -1);

        virtual SIGNED TextHeight(const TCHAR *Text, PegFont *Font);
        virtual SIGNED TextWidth(const TCHAR *Text, PegFont *Font, SIGNED iLen = -1);

       #ifndef PEG_AWT_SUPPORT
        BOOL ClipRect(PegRect &Rect, PegRect &mClip)
        {
            if (mbVirtualDraw)
            {
                Rect &= mVirtualRect;
                if (Rect.wLeft > Rect.wRight ||
                    Rect.wBottom < Rect.wTop)
                {
                    return FALSE;
                }
                return TRUE;
            }
                
            if (!miInvalidCount || !mwDrawNesting)
            {
                return (FALSE);
            }

            Rect &= mInvalid;
            Rect &= mClip;

            if (Rect.wLeft > Rect.wRight ||
                Rect.wBottom < Rect.wTop)
            {
                return FALSE;
            }
            return TRUE;
        }
       #endif

        BOOL ClipRectNoInvalid(PegRect &Rect, PegRect &mClip)
        {
            if (mbVirtualDraw)
            {
                Rect &= mVirtualRect;
                if (Rect.wLeft > Rect.wRight ||
                    Rect.wBottom < Rect.wTop)
                {
                    return FALSE;
                }
                return TRUE;
            }

            Rect &= mClip;
            if (Rect.wLeft > Rect.wRight ||
                Rect.wBottom < Rect.wTop)
            {
                return FALSE;
            }
            return TRUE;
        }
        
        void   Invalidate(const PegRect &Rect);

       #ifdef __BORLANDC__
        BOOL   InvalidOverlap(PegRect &Rect);
       #else
        BOOL   InvalidOverlap(PegRect &Rect)
        {
            if (miInvalidCount)
            {
                if (Rect.Overlap(mInvalid))
                {
                    return TRUE;
                }
            }
            return FALSE;
        }
       #endif

        void RectangleXOR(PegThing *Caller, const PegRect &InRect);
        void InvertRect(PegThing *Caller, PegRect &InRect);

        UCHAR GetPointerType(void);
        SIGNED GetXPointerOffset(void) {return miCurXOffset;}
        SIGNED GetYPointerOffset(void) {return miCurYOffset;}
        PegBitmap *GetPointer(void) {return mpCurPointer;}

        SIGNED GetXRes(void) {return mwHRes;}
        SIGNED GetYRes(void) {return mwVRes;}

       #ifdef PEG_FULL_CLIPPING

       #ifdef PEG_BUILD_PRESC
        virtual void GenerateViewportList(PegThing *pStart);
       #else
        void GenerateViewportList(PegThing *pStart);
       #endif

        void FreeViewports(PegThing *pStart);

       #endif

       #ifdef PEG_VECTOR_FONTS

        PegFont *MakeFont(UCHAR uHeight, BOOL bBold = FALSE,
            BOOL bItalic = FALSE);
        void DeleteFont(PegFont *pFont);

       #endif

       #ifdef PEG_FULL_GRAPHICS

        void Polygon(PegThing *Caller, PegPoint *pPoints,
            SIGNED iNumPoints, const PegColor &Color, SIGNED iWidth = 1);

        void Circle(PegThing *Caller, SIGNED xCenter, SIGNED yCenter, SIGNED r,
            PegColor &Color, SIGNED iWidth = 0);

        void Ellipse(PegThing *Caller, const PegRect &Bound,
            PegColor Color, SIGNED iWidth);

        void PatternLine(PegThing *Caller, SIGNED wXStart, SIGNED wYStart,
            SIGNED wXEnd, SIGNED wYEnd, PegColor &Color, SIGNED wWidth,
            DWORD dPattern);

       #endif

       #ifdef PEG_IMAGE_SCALING

        PegBitmap *ResizeImage(PegBitmap *pSrc, SIGNED iWidth, SIGNED iHeight);

       #endif

       #ifdef PEG_FP_GRAPHICS

        void Arc(PegThing *Caller, SIGNED xc, SIGNED yc, SIGNED radius,
            float start_angle, float end_angle, PegColor Color,
            SIGNED width = 1);

       #endif

       #ifdef PEG_BUILD_PRESS
        friend class PegRemoteScreenServer;
        void GetInvalidRect(PegRect& tRect)
        {
            tRect = mInvalid;
        }
       #endif
        
    protected:

        //---------------------------------------------------------
        // PURE Virtual Functions. These must be provided in instantiable
        // screen drivers. These functions are similar to the public
        // functions of the same name, however they are clipped to a
        // viewport rectangle.
        //---------------------------------------------------------

        virtual void DrawTextView(PegPoint Put, const TCHAR *Text,
            PegColor &Color, PegFont *pFont, SIGNED iLen, PegRect &View) = 0;
        virtual void LineView(SIGNED xStart, SIGNED yStart,
            SIGNED xEnd, SIGNED yEnd,  PegRect &View, PegColor Color,
            SIGNED iWidth) = 0;
        virtual void BitmapView(const PegPoint Where,
            const PegBitmap *pMap, const PegRect &View) = 0;
        virtual void RectMoveView(PegThing *Caller, const PegRect &View,
            const SIGNED xMove, const SIGNED yMove) = 0;

        virtual void HorizontalLine(SIGNED xStart, SIGNED xEnd,
            SIGNED y, COLORVAL cColor, SIGNED iWidth) = 0;
        virtual void VerticalLine(SIGNED yStart, SIGNED yEnd,
            SIGNED x, COLORVAL cColor, SIGNED iWidth) = 0;
        virtual void HorizontalLineXOR(SIGNED xs, SIGNED xe, SIGNED y) = 0;
        virtual void VerticalLineXOR(SIGNED ys, SIGNED ye, SIGNED x) = 0;
        virtual COLORVAL GetPixelView(SIGNED x, SIGNED y) = 0;

        //---------------------------------------------------------
        // End protected pure virtual functions.
        //---------------------------------------------------------

       #ifdef PEG_FULL_CLIPPING

        void SplitView(PegThing *pTarget, PegRect Top, PegRect Bottom);
        void SplitView(PegThing *pTarget, PegThing *Child, PegRect Under);
        void AddViewport(PegThing *pTarget, PegRect NewRect);
        void AllocateViewportBlock();
        Viewport *GetFreeViewport(void);
        Viewport *mpFreeListStart;
        Viewport *mpFreeListEnd;

       #endif

       #ifdef PEG_FULL_GRAPHICS

        virtual void FillPolygon(PegThing *Caller, PegPoint *pPoints,
            SIGNED iNumPoints, const PegColor &Color);

        virtual void Circle(const PegRect &LimitRect, SIGNED xCenter,
            SIGNED yCenter, SIGNED Radius, PegColor &Color, SIGNED iWidth);
        virtual void CircleFast(SIGNED xCenter, SIGNED yCenter, SIGNED Radius,
            PegColor &Color, SIGNED iWidth);
        virtual void FillCircle(const PegRect &LimitRect, SIGNED xCenter,
            SIGNED yCenter, SIGNED r, COLORVAL Color);
        virtual void FillCircleFast(SIGNED xCenter, SIGNED yCenter,
            SIGNED r, COLORVAL Color);
        virtual void OutlineCircle(const PegRect &LimitRect, SIGNED xCenter,
            SIGNED yCenter, SIGNED r, COLORVAL Color, SIGNED iWidth);
        virtual void OutlineCircleFast(SIGNED xCenter, SIGNED yCenter, SIGNED r,
            COLORVAL Color, SIGNED iWidth);
        virtual void FillPolygon(const PegRect &LimitRect, PegPoint *pPoints,
            SIGNED iNumPoints, const PegColor &Color);
        virtual void PatternLine(SIGNED wXStart, SIGNED wYStart, SIGNED wXEnd, 
            SIGNED wYEnd, PegRect &Rect, const PegColor &Color, SIGNED wWidth,
            DWORD dPattern);

        void EllipseToView(const PegRect &Bound, PegColor &color, SIGNED width,
            PegRect rView);
        void EllipseFast(const PegRect &Bound, PegColor &color, SIGNED width);
        void PlotEllipsePoints(PegRect &Bound);

       #endif

       #ifdef PEG_FP_GRAPHICS

        void ArcToView(SIGNED xc, SIGNED yc, SIGNED radius, float start_angle,
	        float end_angle, PegColor Color, SIGNED width, PegRect &View);
        void CheckArcPoint(SIGNED x, SIGNED y, SIGNED test_var,
        SIGNED type, SIGNED start, SIGNED end, PegRect &View);
        void ArcLine(SIGNED xs, SIGNED ys, SIGNED xe, SIGNED ye, PegRect &View);
        void ArcPlot(SIGNED x, SIGNED y, SIGNED test_var, SIGNED type, COLORVAL color,
	        SIGNED start_test, SIGNED end_test, PegRect &View);
        void ArcFill(SIGNED xc, SIGNED yc, SIGNED radius, float start_angle,
	        float end_angle, PegColor color, PegRect &mView);

       #endif

       #ifdef PEG_VECTOR_FONTS
        void FontLine(UCHAR *pData, WORD wBitOffset,
            PegPoint Start, PegPoint End, WORD wBytesPerRow);
       #endif

        void InitVidMemManager(UCHAR *pStart, UCHAR *pEnd);
        UCHAR *AllocBitmap(DWORD lSize);
        void FreeBitmap(UCHAR *pData);

        virtual BOOL WideLine(PegThing *Caller, SIGNED xStart, SIGNED yStart,
            SIGNED xEnd, SIGNED yEnd, const PegColor &Color, SIGNED iWidth);
        virtual void Rectangle(const PegRect &InRect, PegRect &View, const PegColor &Color,
            SIGNED wWidth);

        PegRect mInvalid;
        PegRect mVirtualRect;
        SIGNED mwHRes;
        SIGNED mwVRes;
        DWORD  mdNumColors;
        WORD mwTotalViewports;
        WORD mwDrawNesting;
        SIGNED miInvalidCount;     // has any area of screen been invalidated?
        BOOL mbVirtualDraw;     // drawing to off-screen bitmap?
        PegPointer mpPointers[NUM_POINTER_TYPES];
        PegBitmap *mpCurPointer;
        COLORVAL PEGFAR **mpScanPointers;
        COLORVAL PEGFAR **mpSaveScanPointers;
        SIGNED    miCurXOffset;
        SIGNED    miCurYOffset;

    private:

        VID_MEM_BLOCK *mpFreeVidMem;

       #ifdef PEG_IMAGE_SCALING

        void RectStretch(SIGNED xs1,SIGNED ys1,SIGNED xs2,SIGNED ys2,
            SIGNED xd1,SIGNED yd1, SIGNED xd2,SIGNED yd2,
            PegBitmap *pSrc, PegBitmap *pDest);

        void Stretch(SIGNED x1,SIGNED x2,SIGNED y1,SIGNED y2,
            SIGNED yr,SIGNED yw, PegBitmap *pSrc, PegBitmap *pDest);

       #endif

       #if defined(PEG_FULL_GRAPHICS) || defined(PEG_FP_GRAPHICS)
        SIGNED *mpCoord;
       #endif

};


/*--------------------------------------------------------------------------*/
// Prototype for CreatePegScreen()-
//
// The actual implementation is in each individual derived PegScreen class.
/*--------------------------------------------------------------------------*/
PegScreen *CreatePegScreen(void);



#endif

// End of file





