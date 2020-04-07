// IPAddressString.h: A variant of PegString specialized to edit IP Addresses
// danb@iobjects.com 04/26/02
// (c) Interactive Objects

#ifndef IPADDRESSSTRING_H_
#define IPADDRESSSTRING_H_

#include <gui/peg/peg.hpp>

class CIPAddressString : public PegString
{
public:
    CIPAddressString(const PegRect &Rect, const TCHAR *Text = NULL,
        WORD wId = 0, WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT,
        SIGNED iLen = -1);
    
    CIPAddressString(SIGNED iLeft, SIGNED iTop, SIGNED iWidth,
        const TCHAR *Text = NULL, WORD wId = 0, 
        WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT, SIGNED iLen = -1);
    
    CIPAddressString(SIGNED iLeft, SIGNED iTop, const TCHAR *Text = NULL,
        WORD wId = 0, WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT,
        SIGNED iLen = -1);
    
    virtual ~CIPAddressString();
    
    virtual void DataSet(const TCHAR *Text);
    virtual void DataSet(unsigned int uiAddress);
    virtual SIGNED Message(const PegMessage &Mesg);
    
    unsigned int GetIPAddress();
    
    void SetFirstVisible(SIGNED position)
        {miFirstVisibleChar = position;}
    SIGNED GetFirstVisible() const
        {return miFirstVisibleChar;}
    bool ValidateIPAddressString(const TCHAR *Text);
    bool ValidateIPAddressString()
        {return ValidateIPAddressString(mpText);}

private:

    unsigned int m_uiAddress;

};

#endif  // IPADDRESSSTRING_H_
