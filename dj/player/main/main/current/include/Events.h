// Events.h: how we get events
// danc@iobjects.com 08/08/01
// (c) Interactive Objects

#ifndef __EVENTS_H__
#define __EVENTS_H__

// fdecl
class IUserInterface;

class CEvents
{
public:
    CEvents() {}
    virtual ~CEvents() { }

    virtual void SetUserInterface( IUserInterface* pUI ) = 0;
    virtual void RefreshInterface() = 0;
    virtual int HandleEvent( int key, void* data ) = 0;

protected:
    IUserInterface* m_pUserInterface;
};

#endif // __EVENTS_H__
