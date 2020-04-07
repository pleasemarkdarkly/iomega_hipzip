/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// picon.hpp - Definition of PegIcon class.
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

#ifndef _PEGICON_
#define _PEGICON_

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
class PegIcon : public PegThing
{
    public:
        PegIcon(PegThing *, PegBitmap *pbitmap = NULL,
                WORD wId = 0, WORD wStyle = FF_NONE);
        PegIcon(const PegRect &Where, PegBitmap *pbitmap, WORD wId = 0, 
                WORD wStyle = FF_NONE);
        PegIcon(PegBitmap *pbitmap, WORD wId = 0, 
                WORD wStyle = FF_NONE);
        virtual ~PegIcon() {}
        virtual void Draw(void);
        virtual SIGNED Message(const PegMessage &Mesg);
        virtual void SetIcon(PegBitmap *nbm) { mpBitmap = nbm;}
        virtual PegBitmap *GetIcon(void){return mpBitmap;}
        virtual PegThing *GetProxy(void) {return mpProxy;}
        virtual void SetProxy(PegThing *pNew) {mpProxy = pNew;}

    protected:
        PegBitmap *mpBitmap;
        PegThing  *mpProxy;
};


#endif

